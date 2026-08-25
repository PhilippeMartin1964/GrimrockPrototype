#include "Runtime/Monsters/GridMonsterMovementComponent.h"

#include "Core/GridDirectionUtils.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterOccupancySubsystem.h"

DEFINE_LOG_CATEGORY(LogGridMonsterMovement);

UGridMonsterMovementComponent::UGridMonsterMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UGridMonsterMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	AGridMonsterActor* Monster = GetMonsterOwner();
	if (bAutoInitialize && Monster && Monster->IsRuntimeLevelActive() && Monster->bMonsterEnabled && !Monster->IsDead())
	{
		InitializeMovement(nullptr);
	}
}

void UGridMonsterMovementComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ReleaseOccupancy();
	Super::EndPlay(EndPlayReason);
}

void UGridMonsterMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AGridMonsterActor* Monster = GetMonsterOwner();
	if (!IsValid(Monster) || ActiveMotion == EGridMonsterMotionType::None)
	{
		SetComponentTickEnabled(false);
		return;
	}

	MotionElapsed += FMath::Max(0.0f, DeltaTime);
	const float LinearAlpha = MotionDuration <= KINDA_SMALL_NUMBER ? 1.0f : FMath::Clamp(MotionElapsed / MotionDuration, 0.0f, 1.0f);
	const float VisualAlpha = bUseEaseInOut ? FMath::InterpEaseInOut(0.0f, 1.0f, LinearAlpha, 2.0f) : LinearAlpha;

	if (ActiveMotion == EGridMonsterMotionType::Move)
	{
		Monster->SetActorLocation(FMath::Lerp(MotionStartLocation, MotionTargetLocation, VisualAlpha));
		// MoveAlpha mirrors the actual spatial interpolation used by the actor so
		// in-place locomotion can evaluate its phase against travelled cell distance.
		Monster->SetMovementAnimationState(true, VisualAlpha);

		if (LinearAlpha >= 1.0f)
		{
			CompleteMove();
		}
	}
	else if (ActiveMotion == EGridMonsterMotionType::Turn)
	{
		Monster->SetActorRotation(FQuat::Slerp(MotionStartRotation, MotionTargetRotation, VisualAlpha));

		if (LinearAlpha >= 1.0f)
		{
			CompleteTurn();
		}
	}
}

bool UGridMonsterMovementComponent::InitializeMovement(AGridLevelRuntimeActor* InRuntimeActor)
{
	AGridMonsterActor* Monster = GetMonsterOwner();
	AGridLevelRuntimeActor* CandidateRuntime = IsValid(InRuntimeActor) ? InRuntimeActor : FindRuntimeActor();
	bool bInferredCell = false;

	if (IsValid(Monster) && IsValid(CandidateRuntime) && bInferCellFromActorLocation)
	{
		const FVector WorldLocation = Monster->GetActorLocation();
		int32 ResolvedX = INDEX_NONE;
		int32 ResolvedY = INDEX_NONE;
		FVector LocalOffset = FVector::ZeroVector;
		if (CandidateRuntime->TryResolveWorldCellFromImpactPoint(WorldLocation, ResolvedX, ResolvedY, LocalOffset))
		{
			Monster->CurrentCell = FIntPoint(ResolvedX, ResolvedY);
			bInferredCell = true;
			UE_LOG(LogGridMonsterMovement, Verbose, TEXT("[GridMonsterMovement] InferCell Monster=%s WorldLocation=%s Cell=(%d,%d) LocalOffset=%s"),
				*GetNameSafe(Monster), *WorldLocation.ToCompactString(), ResolvedX, ResolvedY, *LocalOffset.ToCompactString());
		}
		else
		{
			UE_LOG(LogGridMonsterMovement, Error, TEXT("[GridMonsterMovement] InferCellFailed Monster=%s WorldLocation=%s CurrentCell=(%d,%d) Runtime=%s"),
				*GetNameSafe(Monster), *WorldLocation.ToCompactString(), Monster->CurrentCell.X, Monster->CurrentCell.Y, *GetNameSafe(CandidateRuntime));
			return false;
		}
	}

	if (!ValidateInitialization(Monster, CandidateRuntime))
	{
		return false;
	}

	if (bInitialized)
	{
		ReleaseOccupancy();
	}

	RuntimeActor = CandidateRuntime;
	OccupancySubsystem = GetWorld() ? GetWorld()->GetSubsystem<UGridMonsterOccupancySubsystem>() : nullptr;
	if (!OccupancySubsystem)
	{
		UE_LOG(LogGridMonsterMovement, Warning, TEXT("[GridMonsterMovement] Missing occupancy subsystem for %s."), *GetNameSafe(Monster));
		RuntimeActor = nullptr;
		return false;
	}

	if (RuntimeActor->IsPartyOnCell(Monster->CurrentCell.X, Monster->CurrentCell.Y))
	{
		UE_LOG(LogGridMonsterMovement, Error, TEXT("[GridMonsterMovement] %s cannot initialize on party cell (%d,%d)."), *GetNameSafe(Monster),
			Monster->CurrentCell.X, Monster->CurrentCell.Y);
		RuntimeActor = nullptr;
		OccupancySubsystem = nullptr;
		return false;
	}

	if (!OccupancySubsystem->RegisterMonster(Monster, Monster->CurrentCell))
	{
		UE_LOG(LogGridMonsterMovement, Warning, TEXT("[GridMonsterMovement] Cell (%d,%d) is already occupied or reserved for %s."), Monster->CurrentCell.X,
			Monster->CurrentCell.Y, *GetNameSafe(Monster));
		RuntimeActor = nullptr;
		OccupancySubsystem = nullptr;
		return false;
	}

	bInitialized = true;
	ReservedCell = Monster->CurrentCell;

	if (bSnapToCellOnInitialize)
	{
		Monster->SetActorLocation(RuntimeActor->GetCellCenterWorld(Monster->CurrentCell.X, Monster->CurrentCell.Y));
	}
	Monster->ApplyFacingRotation();
	UE_LOG(LogGridMonsterMovement, Verbose, TEXT("[GridMonsterMovement] Initialize Monster=%s Cell=(%d,%d) Inferred=%s Registered=true"), *GetNameSafe(Monster),
		Monster->CurrentCell.X, Monster->CurrentCell.Y, bInferredCell ? TEXT("true") : TEXT("false"));
	return true;
}

bool UGridMonsterMovementComponent::TryMove(EGridEdge Direction)
{
	AGridMonsterActor* Monster = GetMonsterOwner();
	if (!bInitialized || !IsValid(Monster) || !RuntimeActor || !OccupancySubsystem || Monster->IsDead() || IsBusy() || !IsCardinalDirection(Direction))
	{
		return false;
	}

	int32 TargetX = INDEX_NONE;
	int32 TargetY = INDEX_NONE;
	if (!RuntimeActor->CanMove(Monster->CurrentCell.X, Monster->CurrentCell.Y, Direction) ||
		!RuntimeActor->TryGetNeighborCell(Monster->CurrentCell.X, Monster->CurrentCell.Y, Direction, TargetX, TargetY))
	{
		return false;
	}

	const FIntPoint TargetCell(TargetX, TargetY);
	if (!RuntimeActor->IsValidCell(TargetX, TargetY) || !RuntimeActor->IsWalkableCell(TargetX, TargetY) || RuntimeActor->IsPartyOnCell(TargetX, TargetY) ||
		OccupancySubsystem->IsCellBlocked(TargetCell, Monster))
	{
		return false;
	}

	if (!OccupancySubsystem->TryReserveCell(Monster, TargetCell))
	{
		return false;
	}

	MotionStartCell = Monster->CurrentCell;
	MotionTargetCell = TargetCell;
	ReservedCell = TargetCell;
	MotionStartLocation = RuntimeActor->GetCellCenterWorld(MotionStartCell.X, MotionStartCell.Y);
	MotionTargetLocation = RuntimeActor->GetCellCenterWorld(MotionTargetCell.X, MotionTargetCell.Y);
	MotionElapsed = 0.0f;
	MotionDuration = GetMoveDuration();
	ActiveMotion = EGridMonsterMotionType::Move;

	Monster->SetActorLocation(MotionStartLocation);
	Monster->SetMovementAnimationState(true, 0.0f);
	SetComponentTickEnabled(true);
	return true;
}

bool UGridMonsterMovementComponent::TryMoveForward()
{
	const AGridMonsterActor* Monster = GetMonsterOwner();
	return Monster ? TryMove(Monster->Facing) : false;
}

bool UGridMonsterMovementComponent::TryTurnLeft()
{
	const AGridMonsterActor* Monster = GetMonsterOwner();
	return Monster ? StartTurn(GridDirectionUtils::RotateLeft(Monster->Facing), -1) : false;
}

bool UGridMonsterMovementComponent::TryTurnRight()
{
	const AGridMonsterActor* Monster = GetMonsterOwner();
	return Monster ? StartTurn(GridDirectionUtils::RotateRight(Monster->Facing), 1) : false;
}

bool UGridMonsterMovementComponent::TeleportToGridPose(FIntPoint Cell, EGridEdge Facing)
{
	AGridMonsterActor* Monster = GetMonsterOwner();
	if (!bInitialized || !IsValid(Monster) || !RuntimeActor || !OccupancySubsystem || IsBusy() || !RuntimeActor->IsValidCell(Cell.X, Cell.Y) ||
		!RuntimeActor->IsWalkableCell(Cell.X, Cell.Y) || RuntimeActor->IsPartyOnCell(Cell.X, Cell.Y) || OccupancySubsystem->IsCellBlocked(Cell, Monster))
	{
		return false;
	}

	const FIntPoint PreviousCell = Monster->CurrentCell;
	OccupancySubsystem->UnregisterMonster(Monster);
	if (!OccupancySubsystem->RegisterMonster(Monster, Cell))
	{
		OccupancySubsystem->RegisterMonster(Monster, PreviousCell);
		return false;
	}

	Monster->CurrentCell = Cell;
	Monster->Facing = IsCardinalDirection(Facing) ? Facing : EGridEdge::North;
	ReservedCell = Cell;
	Monster->SetActorLocation(RuntimeActor->GetCellCenterWorld(Cell.X, Cell.Y));
	Monster->ApplyFacingRotation();
	return true;
}

void UGridMonsterMovementComponent::CancelCurrentAction()
{
	AGridMonsterActor* Monster = GetMonsterOwner();
	if (!IsValid(Monster) || ActiveMotion == EGridMonsterMotionType::None)
	{
		ResetMotionState();
		return;
	}

	if (ActiveMotion == EGridMonsterMotionType::Move)
	{
		if (OccupancySubsystem)
		{
			OccupancySubsystem->CancelReservation(Monster);
		}
		Monster->SetActorLocation(MotionStartLocation);
		Monster->SetMovementAnimationState(false, 0.0f);
		ReservedCell = Monster->CurrentCell;
	}
	else if (ActiveMotion == EGridMonsterMotionType::Turn)
	{
		Monster->SetActorRotation(MotionStartRotation);
		Monster->SetTurnAnimationState(0);
	}

	ResetMotionState();
}

void UGridMonsterMovementComponent::ReleaseOccupancy()
{
	AGridMonsterActor* Monster = GetMonsterOwner();
	CancelCurrentAction();

	if (OccupancySubsystem && IsValid(Monster))
	{
		OccupancySubsystem->UnregisterMonster(Monster);
	}

	bInitialized = false;
	RuntimeActor = nullptr;
	OccupancySubsystem = nullptr;
}

void UGridMonsterMovementComponent::HandleOwnerDeath()
{
	ReleaseOccupancy();
}

AGridMonsterActor* UGridMonsterMovementComponent::GetMonsterOwner() const
{
	return Cast<AGridMonsterActor>(GetOwner());
}

AGridLevelRuntimeActor* UGridMonsterMovementComponent::FindRuntimeActor() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	for (TActorIterator<AGridLevelRuntimeActor> It(World); It; ++It)
	{
		return *It;
	}
	return nullptr;
}

bool UGridMonsterMovementComponent::ValidateInitialization(AGridMonsterActor* Monster, AGridLevelRuntimeActor* CandidateRuntime) const
{
	if (!IsValid(Monster) || !IsValid(CandidateRuntime) || !IsValid(Monster->MonsterDefinition))
	{
		return false;
	}

	if (Monster->MonsterDefinition->GridFootprint != FIntPoint(1, 1))
	{
		UE_LOG(LogGridMonsterMovement, Error, TEXT("[GridMonsterMovement] MON3 supports only a 1x1 footprint. Monster=%s Footprint=(%d,%d)"),
			*GetNameSafe(Monster), Monster->MonsterDefinition->GridFootprint.X, Monster->MonsterDefinition->GridFootprint.Y);
		return false;
	}

	return CandidateRuntime->IsValidCell(Monster->CurrentCell.X, Monster->CurrentCell.Y) &&
		CandidateRuntime->IsWalkableCell(Monster->CurrentCell.X, Monster->CurrentCell.Y);
}

bool UGridMonsterMovementComponent::StartTurn(EGridEdge TargetFacing, int32 DirectionSign)
{
	AGridMonsterActor* Monster = GetMonsterOwner();
	if (!bInitialized || !IsValid(Monster) || Monster->IsDead() || IsBusy() || !IsCardinalDirection(TargetFacing))
	{
		return false;
	}

	MotionStartFacing = Monster->Facing;
	MotionTargetFacing = TargetFacing;
	MotionStartRotation = Monster->GetActorQuat();
	MotionTargetRotation = FRotator(0.0f, GridDirectionUtils::ToYaw(TargetFacing), 0.0f).Quaternion();
	MotionElapsed = 0.0f;
	MotionDuration = GetTurnDuration();
	ActiveMotion = EGridMonsterMotionType::Turn;

	Monster->SetTurnAnimationState(DirectionSign);
	SetComponentTickEnabled(true);
	return true;
}

void UGridMonsterMovementComponent::CompleteMove()
{
	AGridMonsterActor* Monster = GetMonsterOwner();
	if (!IsValid(Monster) || !OccupancySubsystem || !OccupancySubsystem->CommitMove(Monster, MotionStartCell, MotionTargetCell))
	{
		UE_LOG(LogGridMonsterMovement, Error, TEXT("[GridMonsterMovement] Failed to commit move for %s from (%d,%d) to (%d,%d)."), *GetNameSafe(Monster),
			MotionStartCell.X, MotionStartCell.Y, MotionTargetCell.X, MotionTargetCell.Y);
		CancelCurrentAction();
		return;
	}

	const FIntPoint CompletedFrom = MotionStartCell;
	const FIntPoint CompletedTo = MotionTargetCell;
	Monster->CurrentCell = CompletedTo;
	Monster->SetActorLocation(MotionTargetLocation);
	Monster->SetMovementAnimationState(false, 1.0f);
	ReservedCell = CompletedTo;
	ResetMotionState();
	OnMoveCompleted.Broadcast(CompletedFrom, CompletedTo);
}

void UGridMonsterMovementComponent::CompleteTurn()
{
	AGridMonsterActor* Monster = GetMonsterOwner();
	if (!IsValid(Monster))
	{
		ResetMotionState();
		return;
	}

	const EGridEdge CompletedFrom = MotionStartFacing;
	const EGridEdge CompletedTo = MotionTargetFacing;
	Monster->Facing = CompletedTo;
	Monster->SetActorRotation(MotionTargetRotation);
	Monster->SetTurnAnimationState(0);
	ResetMotionState();
	OnTurnCompleted.Broadcast(CompletedFrom, CompletedTo);
}

void UGridMonsterMovementComponent::ResetMotionState()
{
	ActiveMotion = EGridMonsterMotionType::None;
	MotionElapsed = 0.0f;
	MotionDuration = 0.0f;
	SetComponentTickEnabled(false);
}

float UGridMonsterMovementComponent::GetMoveDuration() const
{
	const AGridMonsterActor* Monster = GetMonsterOwner();
	return FMath::Max(KINDA_SMALL_NUMBER, Monster && Monster->MonsterDefinition ? Monster->MonsterDefinition->MoveDuration : 0.36f);
}

float UGridMonsterMovementComponent::GetTurnDuration() const
{
	const AGridMonsterActor* Monster = GetMonsterOwner();
	return FMath::Max(KINDA_SMALL_NUMBER, Monster && Monster->MonsterDefinition ? Monster->MonsterDefinition->TurnDuration : 0.12f);
}

bool UGridMonsterMovementComponent::IsCardinalDirection(EGridEdge Direction)
{
	return Direction == EGridEdge::North || Direction == EGridEdge::East || Direction == EGridEdge::South || Direction == EGridEdge::West;
}
