#include "Runtime/GrimrockPartyPawn.h"

#include "Core/GridDirectionUtils.h"
#include "Core/GridObjectBehavior.h"
#include "InputActionValue.h"
#include "Kismet/GameplayStatics.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Sound/SoundBase.h"

namespace
{
	FVector GetBlockedMoveWorldDirection(EGridEdge Direction)
	{
		switch (Direction)
		{
			case EGridEdge::North:
				return FVector(0.f, 1.f, 0.f);
			case EGridEdge::East:
				return FVector(1.f, 0.f, 0.f);
			case EGridEdge::South:
				return FVector(0.f, -1.f, 0.f);
			case EGridEdge::West:
				return FVector(-1.f, 0.f, 0.f);
			default:
				return FVector::ZeroVector;
		}
	}

	bool IsSpatialPartyMovementReject(EGridPartyMovementRejectReason RejectReason)
	{
		return RejectReason == EGridPartyMovementRejectReason::TargetCellUnavailable ||
			   RejectReason == EGridPartyMovementRejectReason::PassageBlocked ||
			   RejectReason == EGridPartyMovementRejectReason::TargetCellOccupied;
	}

	constexpr uint32 PartyMovementAudioPitchSalt = 0x504D4155u;
}

void AGrimrockPartyPawn::SetGridStart(AGridLevelRuntimeActor* InLevelRuntimeActor, int32 StartX, int32 StartY, EGridEdge StartFacing)
{
	LevelRuntimeActor = InLevelRuntimeActor;
	CurrentCellX = StartX;
	CurrentCellY = StartY;
	Facing = StartFacing;
	bIsPitFalling = false;
	PitFallElapsed = 0.f;
	PitFallTargetLevelId = NAME_None;
	PitFallTargetCellX = INDEX_NONE;
	PitFallTargetCellY = INDEX_NONE;
	PitFallTargetFacing = EGridEdge::None;
	bPitFallLandingCameraImpactActive = false;
	PitFallLandingCameraImpactElapsed = 0.0f;
	CurrentPitFallLandingCameraOffset = FVector::ZeroVector;

	SnapToCurrentCell();
}

void AGrimrockPartyPawn::SnapToCurrentCell()
{
	if (!HasLevelRuntimeActor())
	{
		return;
	}

	const FVector WorldPos = GetCellCenterOnLevel(CurrentCellX, CurrentCellY, EyeHeight);
	SetActorLocation(WorldPos);

	FRotator Rot = GetActorRotation();
	Rot.Yaw = GridDirectionUtils::ToYaw(Facing);
	SetActorRotation(Rot);
}

void AGrimrockPartyPawn::HandleMoveForward(const FInputActionValue& Value)
{
	(void)Value;

	if (bCharacterCreationModalActive)
	{
		return;
	}
	DismissReadableMessageIfVisible();

	const EGridEdge Direction = GridDirectionUtils::GetForward(Facing);

	if (IsBusy())
	{
		BufferMoveCommand(Direction);
		return;
	}

	TryStartMove(Direction);
}

void AGrimrockPartyPawn::HandleMoveBackward(const FInputActionValue& Value)
{
	(void)Value;

	if (bCharacterCreationModalActive)
	{
		return;
	}
	DismissReadableMessageIfVisible();

	const EGridEdge Direction = GridDirectionUtils::GetBackward(Facing);

	if (IsBusy())
	{
		BufferMoveCommand(Direction);
		return;
	}

	TryStartMove(Direction);
}

void AGrimrockPartyPawn::HandleTurnLeft(const FInputActionValue& Value)
{
	(void)Value;

	if (bCharacterCreationModalActive)
	{
		return;
	}
	DismissReadableMessageIfVisible();

	if (IsBusy())
	{
		BufferTurnCommand(false);
		return;
	}

	TryStartTurn(false);
}

void AGrimrockPartyPawn::HandleTurnRight(const FInputActionValue& Value)
{
	(void)Value;

	if (bCharacterCreationModalActive)
	{
		return;
	}
	DismissReadableMessageIfVisible();

	if (IsBusy())
	{
		BufferTurnCommand(true);
		return;
	}

	TryStartTurn(true);
}

void AGrimrockPartyPawn::HandleStrafeLeft(const FInputActionValue& Value)
{
	(void)Value;

	if (bCharacterCreationModalActive)
	{
		return;
	}
	DismissReadableMessageIfVisible();

	const EGridEdge Direction = GridDirectionUtils::GetLeft(Facing);

	if (IsBusy())
	{
		BufferMoveCommand(Direction);
		return;
	}

	TryStartMove(Direction);
}

void AGrimrockPartyPawn::HandleStrafeRight(const FInputActionValue& Value)
{
	(void)Value;

	if (bCharacterCreationModalActive)
	{
		return;
	}
	DismissReadableMessageIfVisible();

	const EGridEdge Direction = GridDirectionUtils::GetRight(Facing);

	if (IsBusy())
	{
		BufferMoveCommand(Direction);
		return;
	}

	TryStartMove(Direction);
}

bool AGrimrockPartyPawn::TryStartMove(EGridEdge MoveDirection)
{
	if (bCharacterCreationModalActive || bIsMoving || bIsTurning || bIsBlockedMoveFeedbackActive || bIsPitFalling || !HasLevelRuntimeActor())
	{
		return false;
	}

	int32 NextX = CurrentCellX;
	int32 NextY = CurrentCellY;

	UGridTurnManagerComponent* TurnManager = FindTurnManager();
	if (IsValid(TurnManager) && TurnManager->bCombatActive)
	{
		FIntPoint TargetCell;
		EGridPartyMovementRejectReason RejectReason = EGridPartyMovementRejectReason::None;
		if (!TurnManager->RequestPartyTranslation(MoveDirection, TargetCell, RejectReason))
		{
			if (IsSpatialPartyMovementReject(RejectReason))
			{
				TryStartBlockedMoveFeedback(MoveDirection);
			}
			return false;
		}
		NextX = TargetCell.X;
		NextY = TargetCell.Y;
	}
	else
	{
		if (!CanMoveOnLevel(CurrentCellX, CurrentCellY, MoveDirection))
		{
			TryStartBlockedMoveFeedback(MoveDirection);
			return false;
		}
		if (!TryGetNeighborOnLevel(CurrentCellX, CurrentCellY, MoveDirection, NextX, NextY))
		{
			TryStartBlockedMoveFeedback(MoveDirection);
			return false;
		}
	}

	MoveStartLocation = GetActorLocation();
	MoveTargetLocation = GetCellCenterOnLevel(NextX, NextY, EyeHeight);
	MoveElapsed = 0.f;
	bIsMoving = true;

	MoveStartCellX = CurrentCellX;
	MoveStartCellY = CurrentCellY;

	CurrentCellX = NextX;
	CurrentCellY = NextY;
	ActiveMoveDirection = MoveDirection;

	PlayFootstepSound();
	return true;
}

bool AGrimrockPartyPawn::TryStartBlockedMoveFeedback(EGridEdge MoveDirection)
{
	if (!bEnableBlockedMoveFeedback || bIsMoving || bIsTurning || bIsBlockedMoveFeedbackActive || bIsPitFalling || !HasLevelRuntimeActor() ||
		BlockedMoveDistance <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	const FVector DirectionWorld = GetBlockedMoveWorldDirection(MoveDirection);
	if (DirectionWorld.IsNearlyZero())
	{
		return false;
	}

	ClearBufferedCommand();
	BlockedMoveOriginLocation = GetCellCenterOnLevel(CurrentCellX, CurrentCellY, EyeHeight);
	BlockedMoveDirectionWorld = DirectionWorld;
	BlockedMoveElapsed = 0.f;
	bBlockedMoveImpactSoundPlayed = false;
	bIsBlockedMoveFeedbackActive = true;

	// The feedback is visual only. Re-anchor before the nudge so repeated
	// impacts can never accumulate positional drift away from the logical cell.
	SetActorLocation(BlockedMoveOriginLocation);
	return true;
}

void AGrimrockPartyPawn::UpdateBlockedMoveFeedback(float DeltaSeconds)
{
	if (!bIsBlockedMoveFeedbackActive)
	{
		return;
	}

	const float SafeForwardDuration = FMath::Max(0.01f, BlockedMoveForwardDuration);
	const float SafeReturnDuration = FMath::Max(0.01f, BlockedMoveReturnDuration);
	const float TotalDuration = SafeForwardDuration + SafeReturnDuration;

	BlockedMoveElapsed += FMath::Max(0.f, DeltaSeconds);

	if (!bBlockedMoveImpactSoundPlayed && BlockedMoveElapsed >= SafeForwardDuration)
	{
		bBlockedMoveImpactSoundPlayed = true;
		PlayBlockedMoveSound();
	}

	if (BlockedMoveElapsed >= TotalDuration)
	{
		SetActorLocation(BlockedMoveOriginLocation);
		BlockedMoveElapsed = 0.f;
		BlockedMoveDirectionWorld = FVector::ZeroVector;
		bBlockedMoveImpactSoundPlayed = false;
		bIsBlockedMoveFeedbackActive = false;
		return;
	}

	float OffsetAlpha = 0.f;
	if (BlockedMoveElapsed <= SafeForwardDuration)
	{
		const float PhaseAlpha = FMath::Clamp(BlockedMoveElapsed / SafeForwardDuration, 0.f, 1.f);
		OffsetAlpha = 1.f - FMath::Square(1.f - PhaseAlpha);
	}
	else
	{
		const float PhaseAlpha = FMath::Clamp((BlockedMoveElapsed - SafeForwardDuration) / SafeReturnDuration, 0.f, 1.f);
		OffsetAlpha = FMath::Square(1.f - PhaseAlpha);
	}

	SetActorLocation(BlockedMoveOriginLocation + (BlockedMoveDirectionWorld * BlockedMoveDistance * OffsetAlpha));
}

bool AGrimrockPartyPawn::PlayFootstepSound()
{
	return PlayMovementSound(FootstepSounds, FootstepVolume, FootstepAudioOccurrence, FootstepAudioPlaybackRequestCount);
}

bool AGrimrockPartyPawn::PlayBlockedMoveSound()
{
	return PlayMovementSound(BlockedMoveSounds, BlockedMoveVolume, BlockedMoveAudioOccurrence, BlockedMoveAudioPlaybackRequestCount);
}

bool AGrimrockPartyPawn::PlayMovementSound(
	const TArray<TObjectPtr<USoundBase>>& Sounds, float VolumeMultiplier, int32& OccurrenceCounter, int32& PlaybackRequestCounter)
{
	if (!bMovementAudioEnabled || Sounds.IsEmpty())
	{
		return false;
	}

	USoundBase* SelectedSound = nullptr;
	const int32 StartIndex = OccurrenceCounter % Sounds.Num();
	for (int32 Offset = 0; Offset < Sounds.Num(); ++Offset)
	{
		const int32 CandidateIndex = (StartIndex + Offset) % Sounds.Num();
		if (USoundBase* Candidate = Sounds[CandidateIndex].Get())
		{
			SelectedSound = Candidate;
			break;
		}
	}
	if (!IsValid(SelectedSound))
	{
		return false;
	}

	const int32 OccurrenceNumber = ++OccurrenceCounter;
	++PlaybackRequestCounter;
	const float PitchMultiplier = SelectMovementAudioPitch(OccurrenceNumber);

	if (bNativeMovementAudioPlaybackEnabled && GetWorld())
	{
		UGameplayStatics::PlaySound2D(this, SelectedSound, FMath::Max(0.f, VolumeMultiplier), PitchMultiplier);
	}
	return true;
}

float AGrimrockPartyPawn::SelectMovementAudioPitch(int32 OccurrenceNumber) const
{
	const float Variation = FMath::Clamp(MovementAudioPitchVariation, 0.f, 0.25f);
	if (Variation <= KINDA_SMALL_NUMBER)
	{
		return 1.f;
	}

	uint32 Seed = PartyMovementAudioPitchSalt;
	Seed = HashCombine(Seed, GetTypeHash(FMath::Max(1, OccurrenceNumber)));
	FRandomStream PitchStream(static_cast<int32>(Seed));
	return PitchStream.FRandRange(1.f - Variation, 1.f + Variation);
}

bool AGrimrockPartyPawn::PlayPitFallScream()
{
	return PlayMovementSound(PitFallScreamSounds, PitFallScreamVolume, PitFallScreamAudioOccurrence, PitFallScreamPlaybackRequestCount);
}

bool AGrimrockPartyPawn::PlayPitFallLandingSound()
{
	return PlayMovementSound(
		PitFallLandingSounds, PitFallLandingVolume, PitFallLandingAudioOccurrence, PitFallLandingPlaybackRequestCount);
}

bool AGrimrockPartyPawn::BeginPitFall(const FGridObjectTransitionParams& Transition)
{
	if (bCharacterCreationModalActive || bIsMoving || bIsTurning || bIsBlockedMoveFeedbackActive || bIsPitFalling || !HasLevelRuntimeActor() ||
		Transition.TargetLevelId.IsNone() || Transition.TargetFacing == EGridEdge::None)
	{
		return false;
	}

	ClearBufferedCommand();
	bIsFreeLooking = false;
	FreeLookYaw = 0.f;
	FreeLookPitch = 0.f;
	ApplyFreeLookRotation();

	PitFallStartLocation = GetActorLocation();
	PitFallElapsed = 0.f;
	PitFallTargetLevelId = Transition.TargetLevelId;
	PitFallTargetCellX = Transition.TargetCellX;
	PitFallTargetCellY = Transition.TargetCellY;
	PitFallTargetFacing = Transition.TargetFacing;
	bIsPitFalling = true;
	PlayPitFallScream();
	return true;
}

void AGrimrockPartyPawn::UpdatePitFall(float DeltaSeconds)
{
	if (!bIsPitFalling)
	{
		return;
	}

	if (!LevelRuntimeActor)
	{
		bIsPitFalling = false;
		SetActorLocation(PitFallStartLocation);
		return;
	}

	const float SafeDuration = FMath::Max(0.05f, PitFallDuration);
	PitFallElapsed += FMath::Max(0.f, DeltaSeconds);
	const float Alpha = FMath::Clamp(PitFallElapsed / SafeDuration, 0.f, 1.f);
	const float FallAlpha = Alpha * Alpha;
	SetActorLocation(PitFallStartLocation - FVector::UpVector * FMath::Max(0.f, PitFallDistance) * FallAlpha);

	if (Alpha < 1.f)
	{
		return;
	}

	const FName TargetLevelId = PitFallTargetLevelId;
	const int32 TargetCellX = PitFallTargetCellX;
	const int32 TargetCellY = PitFallTargetCellY;
	const EGridEdge TargetFacing = PitFallTargetFacing;

	bIsPitFalling = false;
	PitFallElapsed = 0.f;
	PitFallTargetLevelId = NAME_None;
	PitFallTargetCellX = INDEX_NONE;
	PitFallTargetCellY = INDEX_NONE;
	PitFallTargetFacing = EGridEdge::None;
	ClearBufferedCommand();

	if (!LevelRuntimeActor->TravelToDungeonLevel(TargetLevelId, TargetCellX, TargetCellY, TargetFacing, this))
	{
		SnapToCurrentCell();
		return;
	}

	// Landing feedback belongs to the destination level. Trigger it only after
	// the inter-level travel has succeeded and the party has been snapped there.
	PlayPitFallLandingSound();
	StartPitFallLandingCameraImpact();
}

void AGrimrockPartyPawn::StartPitFallLandingCameraImpact()
{
	PitFallLandingCameraImpactElapsed = 0.0f;
	CurrentPitFallLandingCameraOffset = FVector::ZeroVector;
	bPitFallLandingCameraImpactActive =
		bEnablePitFallLandingCameraImpact && PitFallLandingCameraImpactDistance > KINDA_SMALL_NUMBER;
}

void AGrimrockPartyPawn::UpdatePitFallLandingCameraImpact(float DeltaSeconds)
{
	if (!bPitFallLandingCameraImpactActive)
	{
		CurrentPitFallLandingCameraOffset = FVector::ZeroVector;
		return;
	}

	const float ImpactDuration = FMath::Max(0.01f, PitFallLandingCameraImpactDuration);
	const float RecoveryDuration = FMath::Max(0.01f, PitFallLandingCameraRecoveryDuration);
	const float TotalDuration = ImpactDuration + RecoveryDuration;

	PitFallLandingCameraImpactElapsed += FMath::Max(0.0f, DeltaSeconds);
	if (PitFallLandingCameraImpactElapsed >= TotalDuration)
	{
		PitFallLandingCameraImpactElapsed = 0.0f;
		CurrentPitFallLandingCameraOffset = FVector::ZeroVector;
		bPitFallLandingCameraImpactActive = false;
		return;
	}

	float CompressionAlpha = 0.0f;
	if (PitFallLandingCameraImpactElapsed <= ImpactDuration)
	{
		const float PhaseAlpha = FMath::Clamp(PitFallLandingCameraImpactElapsed / ImpactDuration, 0.0f, 1.0f);
		CompressionAlpha = 1.0f - FMath::Square(1.0f - PhaseAlpha);
	}
	else
	{
		const float PhaseAlpha =
			FMath::Clamp((PitFallLandingCameraImpactElapsed - ImpactDuration) / RecoveryDuration, 0.0f, 1.0f);
		CompressionAlpha = FMath::Square(1.0f - PhaseAlpha);
	}

	CurrentPitFallLandingCameraOffset =
		-FVector::UpVector * FMath::Max(0.0f, PitFallLandingCameraImpactDistance) * CompressionAlpha;
}

bool AGrimrockPartyPawn::TryStartTurn(bool bTurnRight)
{
	if (bCharacterCreationModalActive || bIsMoving || bIsTurning || bIsBlockedMoveFeedbackActive || bIsPitFalling)
	{
		return false;
	}

	const EGridEdge TargetFacing = bTurnRight ? GridDirectionUtils::RotateRight(Facing) : GridDirectionUtils::RotateLeft(Facing);
	UGridTurnManagerComponent* TurnManager = FindTurnManager();
	if (IsValid(TurnManager) && TurnManager->bCombatActive)
	{
		EGridPartyMovementRejectReason RejectReason = EGridPartyMovementRejectReason::None;
		if (!TurnManager->RequestPartyRotation(TargetFacing, RejectReason))
		{
			return false;
		}
	}

	TurnStartYaw = GetActorRotation().Yaw;

	Facing = TargetFacing;
	TurnTargetYaw = GridDirectionUtils::ToYaw(Facing);

	TurnDeltaYaw = FMath::FindDeltaAngleDegrees(TurnStartYaw, TurnTargetYaw);

	TurnElapsed = 0.f;
	bIsTurning = true;
	return true;
}

void AGrimrockPartyPawn::UpdateMove(float DeltaSeconds)
{
	const float SafeDuration = FMath::Max(0.01f, MoveDuration);

	MoveElapsed += DeltaSeconds;
	const float Alpha = FMath::Clamp(MoveElapsed / SafeDuration, 0.f, 1.f);

	const FVector NewLocation = FMath::Lerp(MoveStartLocation, MoveTargetLocation, Alpha);
	SetActorLocation(NewLocation);

	if (Alpha >= 1.f)
	{
		SetActorLocation(MoveTargetLocation);
		bIsMoving = false;
		MoveElapsed = 0.f;
		ActiveMoveDirection = EGridEdge::None;
		if (LevelRuntimeActor)
		{
			// A physical open Pit has absolute priority on cell entry. Do this before
			// triggers, pressure plates, combat turn completion or generic transitions
			// so no secondary cell event can suppress the fall.
			if (LevelRuntimeActor->TryBeginPitFallAtCell(CurrentCellX, CurrentCellY, this))
			{
				ClearBufferedCommand();
				return;
			}

			LevelRuntimeActor->HandlePartyCellChanged(MoveStartCellX, MoveStartCellY, CurrentCellX, CurrentCellY);
			if (UGridTurnManagerComponent* TurnManager = FindTurnManager())
			{
				FGridCombatantInitiativeEntry ActiveBefore;
				const int32 CharacterBefore =
					TurnManager->GetActiveCombatant(ActiveBefore) && ActiveBefore.Side == EGridCombatantSide::Party ? ActiveBefore.CharacterIndex : INDEX_NONE;
				if (TurnManager->NotifyPartyTranslationCompleted())
				{
					FGridCombatantInitiativeEntry ActiveAfter;
					const int32 CharacterAfter =
						TurnManager->GetActiveCombatant(ActiveAfter) && ActiveAfter.Side == EGridCombatantSide::Party ? ActiveAfter.CharacterIndex : INDEX_NONE;
					if (CharacterBefore != CharacterAfter)
					{
						ClearBufferedCommand();
					}
				}
			}
			LevelRuntimeActor->TryExecuteTransitionAtCell(CurrentCellX, CurrentCellY, this, false);
		}
	}
}

void AGrimrockPartyPawn::UpdateTurn(float DeltaSeconds)
{
	const float SafeDuration = FMath::Max(0.01f, TurnDuration);

	TurnElapsed += DeltaSeconds;
	const float Alpha = FMath::Clamp(TurnElapsed / SafeDuration, 0.f, 1.f);

	const float NewYaw = TurnStartYaw + (TurnDeltaYaw * Alpha);

	FRotator Rot = GetActorRotation();
	Rot.Yaw = NewYaw;
	SetActorRotation(Rot);

	if (Alpha >= 1.f)
	{
		Rot.Yaw = TurnTargetYaw;
		SetActorRotation(Rot);

		bIsTurning = false;
		TurnElapsed = 0.f;
		TurnDeltaYaw = 0.f;
		if (UGridTurnManagerComponent* TurnManager = FindTurnManager())
		{
			TurnManager->NotifyPartyRotationCompleted();
		}
	}
}
