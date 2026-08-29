#include "Runtime/GrimrockPartyPawn.h"

#include "Core/GridDirectionUtils.h"
#include "InputActionValue.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridLevelRuntimeActor.h"

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
}

void AGrimrockPartyPawn::SetGridStart(AGridLevelRuntimeActor* InLevelRuntimeActor, int32 StartX, int32 StartY, EGridEdge StartFacing)
{
	LevelRuntimeActor = InLevelRuntimeActor;
	CurrentCellX = StartX;
	CurrentCellY = StartY;
	Facing = StartFacing;

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
	if (bCharacterCreationModalActive || bIsMoving || bIsTurning || bIsBlockedMoveFeedbackActive || !HasLevelRuntimeActor())
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

	return true;
}

bool AGrimrockPartyPawn::TryStartBlockedMoveFeedback(EGridEdge MoveDirection)
{
	if (!bEnableBlockedMoveFeedback || bIsMoving || bIsTurning || bIsBlockedMoveFeedbackActive || !HasLevelRuntimeActor() ||
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

	if (BlockedMoveElapsed >= TotalDuration)
	{
		SetActorLocation(BlockedMoveOriginLocation);
		BlockedMoveElapsed = 0.f;
		BlockedMoveDirectionWorld = FVector::ZeroVector;
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

bool AGrimrockPartyPawn::TryStartTurn(bool bTurnRight)
{
	if (bCharacterCreationModalActive || bIsMoving || bIsTurning || bIsBlockedMoveFeedbackActive)
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
