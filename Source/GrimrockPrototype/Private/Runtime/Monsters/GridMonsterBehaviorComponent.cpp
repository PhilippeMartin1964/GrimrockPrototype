#include "Runtime/Monsters/GridMonsterBehaviorComponent.h"

#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Monsters/GridAutomaticPerceptionEngagementSubsystem.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterOccupancySubsystem.h"
#include "Runtime/Monsters/GridMonsterPatrolSubsystem.h"

DEFINE_LOG_CATEGORY(LogGridMonsterAI);

UGridMonsterBehaviorComponent::UGridMonsterBehaviorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UGridMonsterBehaviorComponent::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoInitialize && InitializeBehavior(nullptr, nullptr))
	{
		if (bRefreshPerceptionOnBeginPlay)
		{
			RefreshPerception();
		}

		// Spawn/restore may occur inside an atomic encounter transaction. The
		// subsystem deliberately defers the actual evaluation until next tick.
		GridAutomaticPerceptionEngagement::Request(RuntimeActor, TEXT("MonsterBeginPlay"));
	}
}

bool UGridMonsterBehaviorComponent::InitializeBehavior(AGridLevelRuntimeActor* InRuntimeActor, AGrimrockPartyPawn* InPartyPawn)
{
	AGridMonsterActor* Monster = GetMonsterOwner();
	AGridLevelRuntimeActor* CandidateRuntime = IsValid(InRuntimeActor) ? InRuntimeActor : FindRuntimeActor();
	AGrimrockPartyPawn* CandidateParty = IsValid(InPartyPawn) ? InPartyPawn : FindPartyPawn();

	if (!IsValid(Monster) || !IsValid(Monster->MonsterDefinition) || !IsValid(CandidateRuntime) || !IsValid(CandidateParty))
	{
		UE_LOG(LogGridMonsterAI, Warning, TEXT("[GridMonsterBehavior] Initialization failed. Monster=%s Definition=%s Runtime=%s Party=%s"),
			*GetNameSafe(Monster), *GetNameSafe(Monster ? Monster->MonsterDefinition : nullptr), *GetNameSafe(CandidateRuntime), *GetNameSafe(CandidateParty));
		return false;
	}

	UGridMonsterOccupancySubsystem* CandidateOccupancy = GetWorld() ? GetWorld()->GetSubsystem<UGridMonsterOccupancySubsystem>() : nullptr;
	if (!CandidateOccupancy)
	{
		UE_LOG(LogGridMonsterAI, Warning, TEXT("[GridMonsterBehavior] Missing occupancy subsystem for %s."), *GetNameSafe(Monster));
		return false;
	}

	RuntimeActor = CandidateRuntime;
	PartyPawn = CandidateParty;
	OccupancySubsystem = CandidateOccupancy;
	bInitialized = true;
	ClearPathResult();
	return true;
}

bool UGridMonsterBehaviorComponent::RefreshPerception()
{
	AGridMonsterActor* Monster = GetMonsterOwner();
	if (!bInitialized || !IsValid(Monster) || !IsValid(Monster->MonsterDefinition) || !IsValid(RuntimeActor) || !IsValid(PartyPawn))
	{
		return false;
	}

	const bool bPreviousCanSee = bCanSeeParty;
	const bool bPreviousCanHear = bCanHearParty;
	const FIntPoint PartyCell = GetPartyCell();

	// MON14.2 keeps MON4's axial/grid LOS geometry but constrains vision to the
	// cardinal ray emitted by the monster's authoritative Facing.
	bCanSeeParty =
		FGridMonsterPerception::HasDirectionalLineOfSight(Monster->CurrentCell, Monster->Facing, PartyCell, Monster->MonsterDefinition->SightRangeCells,
			[this](const FIntPoint& From, const FIntPoint& To)
			{
				return CanTraverseCells(From, To);
			});

	bCanHearParty = FGridMonsterPerception::CanHearThroughGrid(
		Monster->CurrentCell, PartyCell, Monster->MonsterDefinition->HearingRangeCells,
		[this](const FIntPoint& From, const FIntPoint& To)
		{
			if (!RuntimeActor)
			{
				return false;
			}
			const EGridEdge Direction = FGridMonsterPathfinder::GetDirectionBetweenAdjacentCells(From, To);
			return Direction != EGridEdge::None && RuntimeActor->CanSoundTraverse(From.X, From.Y, Direction);
		});

	const bool bHasPartyPerception = HasPartyPerception();
	if (bHasPartyPerception)
	{
		bHasLastKnownPartyCell = true;
		LastKnownPartyCell = PartyCell;
	}

	UpdateOwnerStateFromPerception();

	// MON14.4: reuse the already validated MON7 group/range contract as a
	// local exploration alarm. This never starts combat by itself; it only
	// wakes eligible allies and gives them the source's last known party cell.
	if (bHasPartyPerception)
	{
		if (UWorld* World = GetWorld())
		{
			if (UGridMonsterPatrolSubsystem* PatrolSubsystem = World->GetSubsystem<UGridMonsterPatrolSubsystem>())
			{
				PatrolSubsystem->HandleExplorationAlert(Monster, PartyCell, bCanSeeParty ? TEXT("PerceptionVision") : TEXT("PerceptionHearing"));
			}
		}
	}

	if (bPreviousCanSee != bCanSeeParty || bPreviousCanHear != bCanHearParty)
	{
		OnPerceptionChanged.Broadcast(bCanSeeParty, bCanHearParty);
	}

	// Historical callers (including the F5 diagnostic path) keep the MON4
	// sight-or-hearing contract. Only the scoped MON14.1 automatic collector
	// requires sight for a monster to become a direct combat source.
	return GridAutomaticPerceptionEngagement::IsVisualSourceRequired() ? bCanSeeParty : bHasPartyPerception;
}

void UGridMonsterBehaviorComponent::ClearLastKnownPartyCell()
{
	bHasLastKnownPartyCell = false;
	LastKnownPartyCell = FIntPoint::ZeroValue;
	ClearPathResult();

	if (AGridMonsterActor* Monster = GetMonsterOwner())
	{
		if (!Monster->IsDead() && !HasPartyPerception())
		{
			Monster->SetMonsterState(EGridMonsterState::Idle);
		}
	}
}

bool UGridMonsterBehaviorComponent::FindPursuitPath()
{
	if (!bInitialized || !IsValid(PartyPawn) || !HasPartyPerception())
	{
		ClearPathResult();
		return false;
	}

	TArray<FIntPoint> AttackGoals;
	if (!BuildAttackGoals(GetPartyCell(), AttackGoals))
	{
		ClearPathResult();
		return false;
	}

	FGridMonsterPathQuery Query;
	Query.Start = GetMonsterOwner()->CurrentCell;
	Query.Goals = MoveTemp(AttackGoals);
	Query.MaxVisitedCells = 1024;
	Query.bAllowBlockedGoal = false;

	const bool bFound = ExecutePathQuery(Query);
	if (bFound)
	{
		if (AGridMonsterActor* Monster = GetMonsterOwner())
		{
			if (!Monster->IsDead() && !LastPath.IsEmpty())
			{
				Monster->SetMonsterState(EGridMonsterState::Pursuing);
			}
		}
	}
	return bFound;
}

bool UGridMonsterBehaviorComponent::FindPathToLastKnownPartyCell()
{
	if (!bInitialized || !bHasLastKnownPartyCell)
	{
		ClearPathResult();
		return false;
	}

	if (IsValid(RuntimeActor) && RuntimeActor->IsPartyOnCell(LastKnownPartyCell.X, LastKnownPartyCell.Y))
	{
		AGridMonsterActor* Monster = GetMonsterOwner();
		TArray<FIntPoint> AttackGoals;
		if (!IsValid(Monster) || !BuildAttackGoals(LastKnownPartyCell, AttackGoals))
		{
			ClearPathResult();
			return false;
		}

		FGridMonsterPathQuery Query;
		Query.Start = Monster->CurrentCell;
		Query.Goals = MoveTemp(AttackGoals);
		Query.MaxVisitedCells = 1024;
		Query.bAllowBlockedGoal = false;
		return ExecutePathQuery(Query);
	}
	return FindPathToCell(LastKnownPartyCell, false);
}

bool UGridMonsterBehaviorComponent::FindPathToCell(FIntPoint TargetCell, bool bAllowBlockedGoal)
{
	AGridMonsterActor* Monster = GetMonsterOwner();
	if (!bInitialized || !IsValid(Monster))
	{
		ClearPathResult();
		return false;
	}

	FGridMonsterPathQuery Query;
	Query.Start = Monster->CurrentCell;
	Query.Goals.Add(TargetCell);
	Query.MaxVisitedCells = 1024;
	Query.bAllowBlockedGoal = bAllowBlockedGoal;
	return ExecutePathQuery(Query);
}

bool UGridMonsterBehaviorComponent::GetNextPathCell(FIntPoint& OutCell) const
{
	if (LastPath.IsEmpty())
	{
		return false;
	}
	OutCell = LastPath[0];
	return true;
}

bool UGridMonsterBehaviorComponent::GetNextPathDirection(EGridEdge& OutDirection) const
{
	const AGridMonsterActor* Monster = GetMonsterOwner();
	if (!Monster || LastPath.IsEmpty())
	{
		OutDirection = EGridEdge::None;
		return false;
	}

	OutDirection = FGridMonsterPathfinder::GetDirectionBetweenAdjacentCells(Monster->CurrentCell, LastPath[0]);
	return OutDirection != EGridEdge::None;
}

void UGridMonsterBehaviorComponent::DrawDebugPath(float Duration) const
{
	const AGridMonsterActor* Monster = GetMonsterOwner();
	if (!IsValid(Monster) || !IsValid(RuntimeActor) || !GetWorld())
	{
		return;
	}

	FVector Previous = RuntimeActor->GetCellCenterWorld(Monster->CurrentCell.X, Monster->CurrentCell.Y, 20.0f);

	for (int32 Index = 0; Index < LastPath.Num(); ++Index)
	{
		const FIntPoint Cell = LastPath[Index];
		const FVector Current = RuntimeActor->GetCellCenterWorld(Cell.X, Cell.Y, 20.0f);
		DrawDebugLine(GetWorld(), Previous, Current, FColor::Cyan, false, Duration, 0, 6.0f);
		DrawDebugBox(GetWorld(), Current, FVector(20.0f, 20.0f, 8.0f), Index == LastPath.Num() - 1 ? FColor::Green : FColor::Cyan, false, Duration, 0, 3.0f);
		Previous = Current;
	}
}

void UGridMonsterBehaviorComponent::LogDebugState() const
{
	const AGridMonsterActor* Monster = GetMonsterOwner();
	UE_LOG(LogGridMonsterAI, Log,
		TEXT(
			"[GridMonsterBehavior] Monster=%s Initialized=%s Cell=(%d,%d) Facing=%s See=%s Hear=%s LastKnown=%s(%d,%d) PathFound=%s Goal=(%d,%d) Steps=%d Visited=%d"),
		*GetNameSafe(Monster), bInitialized ? TEXT("true") : TEXT("false"), Monster ? Monster->CurrentCell.X : INDEX_NONE,
		Monster ? Monster->CurrentCell.Y : INDEX_NONE, Monster ? *UEnum::GetValueAsString(Monster->Facing) : TEXT("None"),
		bCanSeeParty ? TEXT("true") : TEXT("false"), bCanHearParty ? TEXT("true") : TEXT("false"), bHasLastKnownPartyCell ? TEXT("true") : TEXT("false"),
		LastKnownPartyCell.X, LastKnownPartyCell.Y, bLastPathFound ? TEXT("true") : TEXT("false"), LastPathGoal.X, LastPathGoal.Y, LastPath.Num(),
		LastVisitedCellCount);
}

AGridMonsterActor* UGridMonsterBehaviorComponent::GetMonsterOwner() const
{
	return Cast<AGridMonsterActor>(GetOwner());
}

AGridLevelRuntimeActor* UGridMonsterBehaviorComponent::FindRuntimeActor() const
{
	if (const AGridMonsterActor* Monster = GetMonsterOwner())
	{
		if (AGridLevelRuntimeActor* OwnerRuntime = Cast<AGridLevelRuntimeActor>(Monster->GetOwner()))
		{
			return OwnerRuntime;
		}
	}

	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AGridLevelRuntimeActor> It(World); It; ++It)
		{
			return *It;
		}
	}
	return nullptr;
}

AGrimrockPartyPawn* UGridMonsterBehaviorComponent::FindPartyPawn() const
{
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AGrimrockPartyPawn> It(World); It; ++It)
		{
			return *It;
		}
	}
	return nullptr;
}

FIntPoint UGridMonsterBehaviorComponent::GetPartyCell() const
{
	return IsValid(PartyPawn) ? FIntPoint(PartyPawn->CurrentCellX, PartyPawn->CurrentCellY) : FIntPoint::ZeroValue;
}

bool UGridMonsterBehaviorComponent::BuildAttackGoals(const FIntPoint& TargetPartyCell, TArray<FIntPoint>& OutGoals) const
{
	OutGoals.Reset();
	const AGridMonsterActor* Monster = GetMonsterOwner();
	if (!IsValid(Monster) || !IsValid(RuntimeActor) || !OccupancySubsystem || !IsValid(PartyPawn))
	{
		return false;
	}

	for (const EGridEdge FromPartyDirection : FGridMonsterPathfinder::GetOrderedDirections())
	{
		const FIntPoint Candidate = FGridMonsterPathfinder::GetNeighborCell(TargetPartyCell, FromPartyDirection);
		const EGridEdge DirectionToParty = FGridMonsterPathfinder::GetDirectionBetweenAdjacentCells(Candidate, TargetPartyCell);

		if (!RuntimeActor->IsValidCell(Candidate.X, Candidate.Y) || !RuntimeActor->IsWalkableCell(Candidate.X, Candidate.Y) ||
			DirectionToParty == EGridEdge::None || !RuntimeActor->CanMove(Candidate.X, Candidate.Y, DirectionToParty) || IsCellBlockedForOwner(Candidate))
		{
			continue;
		}
		OutGoals.Add(Candidate);
	}
	return !OutGoals.IsEmpty();
}

bool UGridMonsterBehaviorComponent::ExecutePathQuery(const FGridMonsterPathQuery& Query)
{
	AGridMonsterActor* Monster = GetMonsterOwner();
	if (!bInitialized || !IsValid(Monster) || !IsValid(RuntimeActor) || !OccupancySubsystem)
	{
		ClearPathResult();
		return false;
	}

	FGridMonsterPathContext Context;
	Context.IsValidCell = [this](const FIntPoint& Cell)
	{
		return RuntimeActor && RuntimeActor->IsValidCell(Cell.X, Cell.Y);
	};
	Context.IsWalkableCell = [this](const FIntPoint& Cell)
	{
		return RuntimeActor && RuntimeActor->IsWalkableCell(Cell.X, Cell.Y);
	};
	Context.CanTraverse = [this](const FIntPoint& From, const FIntPoint& To)
	{
		return CanTraverseCells(From, To);
	};
	Context.IsCellBlocked = [this](const FIntPoint& Cell)
	{
		return IsCellBlockedForOwner(Cell);
	};

	FGridMonsterPathResult Result;
	const bool bFound = FGridMonsterPathfinder::FindPath(Query, Context, Result);
	StorePathResult(Result);

	if (bFound && bDrawPathAfterQuery)
	{
		DrawDebugPath(5.0f);
	}
	return bFound;
}

bool UGridMonsterBehaviorComponent::CanTraverseCells(const FIntPoint& From, const FIntPoint& To) const
{
	if (!RuntimeActor)
	{
		return false;
	}

	const EGridEdge Direction = FGridMonsterPathfinder::GetDirectionBetweenAdjacentCells(From, To);
	return Direction != EGridEdge::None && RuntimeActor->CanMove(From.X, From.Y, Direction);
}

bool UGridMonsterBehaviorComponent::IsCellBlockedForOwner(const FIntPoint& Cell) const
{
	const AGridMonsterActor* Monster = GetMonsterOwner();
	if (!RuntimeActor || !OccupancySubsystem)
	{
		return true;
	}

	if (RuntimeActor->IsPartyOnCell(Cell.X, Cell.Y))
	{
		return true;
	}
	return OccupancySubsystem->IsCellBlocked(Cell, Monster);
}

void UGridMonsterBehaviorComponent::StorePathResult(const FGridMonsterPathResult& Result)
{
	bLastPathFound = Result.bFound;
	LastPath = Result.Cells;
	LastPathGoal = Result.ReachedGoal;
	LastVisitedCellCount = Result.VisitedCellCount;
}

void UGridMonsterBehaviorComponent::ClearPathResult()
{
	bLastPathFound = false;
	LastPath.Reset();
	LastPathGoal = FIntPoint::ZeroValue;
	LastVisitedCellCount = 0;
}

void UGridMonsterBehaviorComponent::UpdateOwnerStateFromPerception()
{
	AGridMonsterActor* Monster = GetMonsterOwner();
	if (!IsValid(Monster) || Monster->IsDead())
	{
		return;
	}

	if (HasPartyPerception() && (Monster->MonsterState == EGridMonsterState::Dormant || Monster->MonsterState == EGridMonsterState::Idle))
	{
		Monster->SetMonsterState(EGridMonsterState::Alert);
	}
}
