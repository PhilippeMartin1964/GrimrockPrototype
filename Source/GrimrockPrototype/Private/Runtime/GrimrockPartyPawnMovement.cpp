#include "Runtime/GrimrockPartyPawn.h"

#include "Core/GridDirectionUtils.h"
#include "InputActionValue.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridLevelRuntimeActor.h"

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
	if (bCharacterCreationModalActive || bIsMoving || bIsTurning || !HasLevelRuntimeActor())
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
			return false;
		}
		NextX = TargetCell.X;
		NextY = TargetCell.Y;
	}
	else if (!CanMoveOnLevel(CurrentCellX, CurrentCellY, MoveDirection) || !TryGetNeighborOnLevel(CurrentCellX, CurrentCellY, MoveDirection, NextX, NextY))
	{
		return false;
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

bool AGrimrockPartyPawn::TryStartTurn(bool bTurnRight)
{
	if (bCharacterCreationModalActive || bIsMoving || bIsTurning)
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
