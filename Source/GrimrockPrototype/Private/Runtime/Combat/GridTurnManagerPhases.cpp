#include "Runtime/Combat/GridTurnManagerComponent.h"

#include "Core/GridDirectionUtils.h"
#include "HAL/PlatformTime.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterBehaviorComponent.h"
#include "Runtime/Monsters/GridMonsterCombatComponent.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterFastHarasserPlanner.h"
#include "Runtime/Monsters/GridMonsterMovementComponent.h"
#include "Runtime/Monsters/GridMonsterOccupancySubsystem.h"
#include "Runtime/Monsters/GridMonsterPathfinder.h"
#include "Runtime/Monsters/GridMonsterRangedAttackPlanner.h"
#include "Runtime/Monsters/GridMonsterRangedKeeperPlanner.h"

namespace
{
	class FScopedTurnPlanningMetrics
	{
	public:
		FScopedTurnPlanningMetrics(bool bInEnabled, FGridCombatRuntimeMetrics& InMetrics, const TArray<FGridCombatAction>& InActions)
			: bEnabled(bInEnabled)
			, Metrics(InMetrics)
			, Actions(InActions)
			, StartSeconds(bEnabled ? FPlatformTime::Seconds() : 0.0)
		{
		}

		~FScopedTurnPlanningMetrics()
		{
			if (!bEnabled)
			{
				return;
			}

			const float ElapsedMilliseconds = static_cast<float>((FPlatformTime::Seconds() - StartSeconds) * 1000.0);
			Metrics.LastTurnPlanningMilliseconds = ElapsedMilliseconds;
			Metrics.MaximumTurnPlanningMilliseconds = FMath::Max(Metrics.MaximumTurnPlanningMilliseconds, ElapsedMilliseconds);
			Metrics.PeakPendingActionCount = FMath::Max(Metrics.PeakPendingActionCount, Actions.Num());
		}

	private:
		bool bEnabled;
		FGridCombatRuntimeMetrics& Metrics;
		const TArray<FGridCombatAction>& Actions;
		double StartSeconds;
	};

	int32 GetMonsterInitiative(const AGridMonsterActor* Monster)
	{
		return Monster && Monster->MonsterDefinition ? Monster->MonsterDefinition->Initiative : MIN_int32;
	}

	bool GetPostAttackPlanState(const TArray<FGridCombatAction>& Actions, const FIntPoint& StartCell, EGridEdge StartFacing, int32 AvailableActionPoints,
		FIntPoint& OutAttackCell, EGridEdge& OutAttackFacing, int32& OutRemainingActionPoints)
	{
		OutAttackCell = StartCell;
		OutAttackFacing = StartFacing == EGridEdge::None ? EGridEdge::North : StartFacing;
		OutRemainingActionPoints = FMath::Max(0, AvailableActionPoints);

		for (const FGridCombatAction& Action : Actions)
		{
			if (Action.Type == EGridCombatActionType::Turn)
			{
				const EGridEdge TargetFacing = FGridMonsterPathfinder::GetDirectionBetweenAdjacentCells(OutAttackCell, Action.TargetCell);
				if (TargetFacing != EGridEdge::None)
				{
					OutAttackFacing = TargetFacing;
				}
			}
			else if (Action.Type == EGridCombatActionType::Move)
			{
				const EGridEdge MoveFacing = FGridMonsterPathfinder::GetDirectionBetweenAdjacentCells(OutAttackCell, Action.TargetCell);
				if (MoveFacing != EGridEdge::None)
				{
					OutAttackFacing = MoveFacing;
				}
				OutAttackCell = Action.TargetCell;
			}

			OutRemainingActionPoints -= Action.ActionPointCost;
			if (Action.Type == EGridCombatActionType::MeleeAttack)
			{
				return true;
			}
		}

		return false;
	}

	bool IsRelevantRetreatExit(const AGridLevelRuntimeActor* RuntimeActor, const UGridMonsterOccupancySubsystem* OccupancySubsystem,
		const AGridMonsterActor* Monster, const FIntPoint& FromCell, EGridEdge Direction, const FIntPoint& PartyCell, FIntPoint& OutCell)
	{
		OutCell = FGridMonsterPathfinder::GetNeighborCell(FromCell, Direction);
		return RuntimeActor && OccupancySubsystem && OutCell != PartyCell && RuntimeActor->IsValidCell(OutCell.X, OutCell.Y) &&
			RuntimeActor->IsWalkableCell(OutCell.X, OutCell.Y) && RuntimeActor->CanMove(FromCell.X, FromCell.Y, Direction) &&
			!OccupancySubsystem->IsCellBlocked(OutCell, Monster);
	}

	void BuildRetreatCandidates(const AGridLevelRuntimeActor* RuntimeActor, const UGridMonsterOccupancySubsystem* OccupancySubsystem,
		const AGridMonsterActor* Monster, const FIntPoint& AttackCell, const FIntPoint& PartyCell, TArray<FGridRetreatCandidate>& OutCandidates)
	{
		OutCandidates.Reset();
		if (!RuntimeActor || !OccupancySubsystem || !IsValid(Monster))
		{
			return;
		}

		const TArray<EGridEdge>& OrderedDirections = FGridMonsterPathfinder::GetOrderedDirections();
		for (int32 DirectionIndex = 0; DirectionIndex < OrderedDirections.Num(); ++DirectionIndex)
		{
			const EGridEdge Direction = OrderedDirections[DirectionIndex];
			FIntPoint CandidateCell;
			if (!IsRelevantRetreatExit(RuntimeActor, OccupancySubsystem, Monster, AttackCell, Direction, PartyCell, CandidateCell) ||
				!OccupancySubsystem->CanReserveCell(Monster, CandidateCell))
			{
				continue;
			}

			FGridRetreatCandidate Candidate;
			Candidate.Cell = CandidateCell;
			Candidate.Direction = Direction;
			Candidate.DistanceToParty = FGridMonsterPathfinder::ManhattanDistance(CandidateCell, PartyCell);
			Candidate.StableDirectionOrder = DirectionIndex;

			for (const EGridEdge ExitDirection : OrderedDirections)
			{
				FIntPoint ExitCell;
				if (!IsRelevantRetreatExit(RuntimeActor, OccupancySubsystem, Monster, CandidateCell, ExitDirection, PartyCell, ExitCell))
				{
					continue;
				}

				++Candidate.ExitCount;
				if (FGridMonsterPathfinder::ManhattanDistance(ExitCell, PartyCell) < Candidate.DistanceToParty)
				{
					Candidate.bCanContinuePursuit = true;
				}
			}

			Candidate.bIsCulDeSac = Candidate.ExitCount <= 1;
			OutCandidates.Add(Candidate);
		}
	}

	bool MON174SelectRangedKeeperAttack(const UGridMonsterDefinitionAsset* Definition, const UGridMonsterCombatComponent* CombatComponent,
		FGridMonsterAttackDefinition& OutAttack, bool& OutMayAttackThisTurn)
	{
		OutAttack = FGridMonsterAttackDefinition();
		OutMayAttackThisTurn = false;
		if (!IsValid(Definition) || !IsValid(CombatComponent))
		{
			return false;
		}

		const FGridMonsterAttackDefinition* BestAvailable = nullptr;
		const FGridMonsterAttackDefinition* BestAuthored = nullptr;
		for (const FGridMonsterAttackDefinition& Attack : Definition->Attacks)
		{
			if (!Attack.IsValidDefinition() || !Attack.IsRangedAttack())
			{
				continue;
			}

			if (!BestAuthored || Attack.Priority > BestAuthored->Priority)
			{
				BestAuthored = &Attack;
			}

			if (!CombatComponent->IsAttackOnCooldown(Attack.AttackId) && (!BestAvailable || Attack.Priority > BestAvailable->Priority))
			{
				BestAvailable = &Attack;
			}
		}

		const FGridMonsterAttackDefinition* Selected = BestAvailable ? BestAvailable : BestAuthored;
		if (!Selected)
		{
			return false;
		}

		OutAttack = *Selected;
		OutMayAttackThisTurn = BestAvailable != nullptr;
		return true;
	}

	bool MON174HasLegalRangedShot(
		const AGridLevelRuntimeActor* RuntimeActor, const FIntPoint& FromCell, const FIntPoint& PartyCell, const FGridMonsterAttackDefinition& Attack)
	{
		const int32 Distance = FGridMonsterPathfinder::ManhattanDistance(FromCell, PartyCell);
		if (!Attack.SupportsDistance(Distance) || FGridMonsterRangedAttackPlanner::GetAxialDirection(FromCell, PartyCell) == EGridEdge::None)
		{
			return false;
		}

		if (!Attack.bRequiresLineOfSight)
		{
			return true;
		}

		return IsValid(RuntimeActor) &&
			FGridMonsterPerception::HasStraightLineOfSight(FromCell, PartyCell, Attack.RangeCells,
				[RuntimeActor](const FIntPoint& From, const FIntPoint& To)
				{
					const EGridEdge Direction = FGridMonsterPathfinder::GetDirectionBetweenAdjacentCells(From, To);
					return IsValid(RuntimeActor) && Direction != EGridEdge::None && RuntimeActor->CanMove(From.X, From.Y, Direction);
				});
	}

	void MON174BuildValidFiringGoals(const AGridLevelRuntimeActor* RuntimeActor, const UGridMonsterOccupancySubsystem* OccupancySubsystem,
		const AGridMonsterActor* Monster, const FIntPoint& PartyCell, const FGridMonsterAttackDefinition& Attack, int32 MinDistance, int32 MaxDistance,
		TArray<FIntPoint>& OutGoals)
	{
		FGridMonsterRangedKeeperPlanner::BuildAxialFiringCandidates(PartyCell, MinDistance, MaxDistance, OutGoals);

		OutGoals.RemoveAll(
			[RuntimeActor, OccupancySubsystem, Monster, PartyCell, &Attack](const FIntPoint& Candidate)
			{
				return !IsValid(RuntimeActor) || !IsValid(OccupancySubsystem) || !IsValid(Monster) || Candidate == PartyCell ||
					!RuntimeActor->IsValidCell(Candidate.X, Candidate.Y) || !RuntimeActor->IsWalkableCell(Candidate.X, Candidate.Y) ||
					OccupancySubsystem->IsCellBlocked(Candidate, Monster) || !MON174HasLegalRangedShot(RuntimeActor, Candidate, PartyCell, Attack);
			});
	}

	bool MON174FindPathToFiringGoals(const AGridLevelRuntimeActor* RuntimeActor, const UGridMonsterOccupancySubsystem* OccupancySubsystem,
		const AGridMonsterActor* Monster, const FIntPoint& PartyCell, const TArray<FIntPoint>& Goals, FGridMonsterPathResult& OutResult)
	{
		OutResult.Reset();
		if (!IsValid(RuntimeActor) || !IsValid(OccupancySubsystem) || !IsValid(Monster) || Goals.IsEmpty())
		{
			return false;
		}

		FGridMonsterPathQuery Query;
		Query.Start = Monster->CurrentCell;
		Query.Goals = Goals;
		Query.MaxVisitedCells = 1024;
		Query.bAllowBlockedGoal = false;

		FGridMonsterPathContext Context;
		Context.IsValidCell = [RuntimeActor](const FIntPoint& Cell)
		{
			return IsValid(RuntimeActor) && RuntimeActor->IsValidCell(Cell.X, Cell.Y);
		};
		Context.IsWalkableCell = [RuntimeActor](const FIntPoint& Cell)
		{
			return IsValid(RuntimeActor) && RuntimeActor->IsWalkableCell(Cell.X, Cell.Y);
		};
		Context.CanTraverse = [RuntimeActor](const FIntPoint& From, const FIntPoint& To)
		{
			const EGridEdge Direction = FGridMonsterPathfinder::GetDirectionBetweenAdjacentCells(From, To);
			return IsValid(RuntimeActor) && Direction != EGridEdge::None && RuntimeActor->CanMove(From.X, From.Y, Direction);
		};
		Context.IsCellBlocked = [OccupancySubsystem, Monster, PartyCell](const FIntPoint& Cell)
		{
			return Cell == PartyCell || !IsValid(OccupancySubsystem) || OccupancySubsystem->IsCellBlocked(Cell, Monster);
		};

		return FGridMonsterPathfinder::FindPath(Query, Context, OutResult);
	}

	bool MON174TryPlanRangedKeeper(const AGridLevelRuntimeActor* RuntimeActor, const UGridMonsterMovementComponent* MovementComponent,
		AGridMonsterActor* Monster, UGridMonsterCombatComponent* CombatComponent, const FIntPoint& PartyCell, int32 AvailableActionPoints,
		TArray<FGridCombatAction>& OutActions)
	{
		OutActions.Reset();
		if (!IsValid(Monster) || !IsValid(Monster->MonsterDefinition) || !IsValid(CombatComponent))
		{
			return false;
		}

		FGridMonsterAttackDefinition RangedAttack;
		bool bMayAttackThisTurn = false;
		if (!MON174SelectRangedKeeperAttack(Monster->MonsterDefinition, CombatComponent, RangedAttack, bMayAttackThisTurn))
		{
			return false;
		}

		const int32 CurrentDistance = FGridMonsterPathfinder::ManhattanDistance(Monster->CurrentCell, PartyCell);
		const bool bCurrentShotLegal = MON174HasLegalRangedShot(RuntimeActor, Monster->CurrentCell, PartyCell, RangedAttack);

		const int32 AuthoredPreferredMin = FMath::Max(0, Monster->MonsterDefinition->PreferredMinDistance);
		const int32 AuthoredPreferredMax = FMath::Max(AuthoredPreferredMin, Monster->MonsterDefinition->PreferredMaxDistance);
		const int32 PreferredMin = FMath::Max(RangedAttack.MinRangeCells, AuthoredPreferredMin);
		const int32 PreferredMax = FMath::Min(RangedAttack.RangeCells, AuthoredPreferredMax);
		const bool bHasPreferredBand = PreferredMin <= PreferredMax;

		if (bMayAttackThisTurn && bCurrentShotLegal && bHasPreferredBand &&
			FGridMonsterRangedKeeperPlanner::IsPreferredDistance(CurrentDistance, PreferredMin, PreferredMax) &&
			FGridMonsterRangedAttackPlanner::BuildStationaryRangedTurn(
				Monster->ResolvePersistenceId(), Monster->CurrentCell, Monster->Facing, PartyCell, AvailableActionPoints, RangedAttack, true, OutActions))
		{
			UE_LOG(LogGridMonsterAI, Verbose, TEXT("[GridRangedKeeper] Plan Monster=%s Mode=PreferredShot Distance=%d Preferred=%d..%d Attack=%s"),
				*GetNameSafe(Monster), CurrentDistance, PreferredMin, PreferredMax, *RangedAttack.AttackId.ToString());
			return true;
		}

		UGridMonsterOccupancySubsystem* OccupancySubsystem = IsValid(MovementComponent) ? MovementComponent->GetOccupancySubsystem() : nullptr;

		if (bHasPreferredBand && IsValid(RuntimeActor) && IsValid(OccupancySubsystem))
		{
			TArray<FIntPoint> PreferredGoals;
			MON174BuildValidFiringGoals(RuntimeActor, OccupancySubsystem, Monster, PartyCell, RangedAttack, PreferredMin, PreferredMax, PreferredGoals);

			FGridMonsterPathResult PreferredPath;
			if (MON174FindPathToFiringGoals(RuntimeActor, OccupancySubsystem, Monster, PartyCell, PreferredGoals, PreferredPath) &&
				FGridMonsterRangedKeeperPlanner::BuildRepositionTurn(Monster->ResolvePersistenceId(), Monster->CurrentCell, Monster->Facing, PartyCell,
					PreferredPath.Cells, AvailableActionPoints, RangedAttack, true, bMayAttackThisTurn, OutActions))
			{
				UE_LOG(LogGridMonsterAI, Verbose,
					TEXT(
						"[GridRangedKeeper] Plan Monster=%s Mode=PreferredReposition From=(%d,%d) Goal=(%d,%d) PathSteps=%d Distance=%d Preferred=%d..%d Attack=%s Available=%s"),
					*GetNameSafe(Monster), Monster->CurrentCell.X, Monster->CurrentCell.Y, PreferredPath.ReachedGoal.X, PreferredPath.ReachedGoal.Y,
					PreferredPath.Cells.Num(), CurrentDistance, PreferredMin, PreferredMax, *RangedAttack.AttackId.ToString(),
					bMayAttackThisTurn ? TEXT("true") : TEXT("false"));
				return true;
			}
		}

		if (bMayAttackThisTurn && bCurrentShotLegal &&
			FGridMonsterRangedAttackPlanner::BuildStationaryRangedTurn(
				Monster->ResolvePersistenceId(), Monster->CurrentCell, Monster->Facing, PartyCell, AvailableActionPoints, RangedAttack, true, OutActions))
		{
			UE_LOG(LogGridMonsterAI, Verbose, TEXT("[GridRangedKeeper] Plan Monster=%s Mode=LegalShotFallback Distance=%d Attack=%s"), *GetNameSafe(Monster),
				CurrentDistance, *RangedAttack.AttackId.ToString());
			return true;
		}

		if (!bCurrentShotLegal && IsValid(RuntimeActor) && IsValid(OccupancySubsystem))
		{
			TArray<FIntPoint> LegalGoals;
			MON174BuildValidFiringGoals(
				RuntimeActor, OccupancySubsystem, Monster, PartyCell, RangedAttack, RangedAttack.MinRangeCells, RangedAttack.RangeCells, LegalGoals);

			FGridMonsterPathResult LegalPath;
			if (MON174FindPathToFiringGoals(RuntimeActor, OccupancySubsystem, Monster, PartyCell, LegalGoals, LegalPath) &&
				FGridMonsterRangedKeeperPlanner::BuildRepositionTurn(Monster->ResolvePersistenceId(), Monster->CurrentCell, Monster->Facing, PartyCell,
					LegalPath.Cells, AvailableActionPoints, RangedAttack, true, bMayAttackThisTurn, OutActions))
			{
				UE_LOG(LogGridMonsterAI, Verbose,
					TEXT("[GridRangedKeeper] Plan Monster=%s Mode=LegalReposition From=(%d,%d) Goal=(%d,%d) PathSteps=%d Attack=%s Available=%s"),
					*GetNameSafe(Monster), Monster->CurrentCell.X, Monster->CurrentCell.Y, LegalPath.ReachedGoal.X, LegalPath.ReachedGoal.Y,
					LegalPath.Cells.Num(), *RangedAttack.AttackId.ToString(), bMayAttackThisTurn ? TEXT("true") : TEXT("false"));
				return true;
			}
		}

		return false;
	}
}

void UGridTurnManagerComponent::BeginRound()
{
	if (!bCombatActive || !PhaseState.BeginRound())
	{
		return;
	}

	RoundNumber = PhaseState.GetRoundNumber();
	if (bCollectRuntimeMetrics)
	{
		++RuntimeMetrics.RoundsStarted;
	}
	if (InitiativeOrder.IsEmpty())
	{
		ResetPartyMobilityForRound();
		BeginPlayerCharacterPhase();
		SetPhase(PhaseState.GetPhase());
		SetPartyInputLocked(false);
	}
	else
	{
		ResetInitiativeRound();
		SetPartyInputLocked(true);
	}

	FGridCombatLogEntry RoundEntry;
	RoundEntry.RoundNumber = RoundNumber;
	RoundEntry.Phase = CurrentPhase;
	RoundEntry.Type = EGridCombatLogEntryType::RoundStarted;
	RoundEntry.Message = FGridCombatLogFormatter::FormatRoundStarted(RoundNumber);
	AppendCombatLogEntry(RoundEntry);
	OnRoundStarted.Broadcast(RoundNumber);

	if (!InitiativeOrder.IsEmpty())
	{
		BeginNextCombatantTurn();
	}
}

void UGridTurnManagerComponent::BeginEnemyPhase()
{
	if (!HasLivingPartyCharacter())
	{
		FinishCombat(EGridCombatPhase::Defeat);
		return;
	}

	if (!HasLivingCombatMonster())
	{
		FinishCombat(EGridCombatPhase::Victory);
		return;
	}

	BuildEnemyTurnOrder();
	CurrentEnemyIndex = INDEX_NONE;
	BeginNextMonsterTurn();
}

void UGridTurnManagerComponent::BuildEnemyTurnOrder()
{
	EnemyTurnOrder.Reset();
	for (AGridMonsterActor* Monster : CombatMonsters)
	{
		if (IsValid(Monster) && !Monster->IsDead())
		{
			EnemyTurnOrder.Add(Monster);
		}
	}

	EnemyTurnOrder.Sort(
		[](const TObjectPtr<AGridMonsterActor>& Left, const TObjectPtr<AGridMonsterActor>& Right)
		{
			const int32 LeftInitiative = GetMonsterInitiative(Left.Get());
			const int32 RightInitiative = GetMonsterInitiative(Right.Get());
			if (LeftInitiative != RightInitiative)
			{
				return LeftInitiative > RightInitiative;
			}

			const FGuid LeftPersistenceId = Left ? Left->ResolvePersistenceId() : FGuid();
			const FGuid RightPersistenceId = Right ? Right->ResolvePersistenceId() : FGuid();
			const FString LeftId = LeftPersistenceId.IsValid() ? LeftPersistenceId.ToString(EGuidFormats::Digits) : GetNameSafe(Left.Get());
			const FString RightId = RightPersistenceId.IsValid() ? RightPersistenceId.ToString(EGuidFormats::Digits) : GetNameSafe(Right.Get());
			return LeftId < RightId;
		});
}

void UGridTurnManagerComponent::BeginNextMonsterTurn()
{
	UnbindCurrentMovement();
	UnbindCurrentCombat();
	CurrentMonster = nullptr;
	PendingActions.Reset();
	bHasActiveAction = false;
	ActiveAction = FGridCombatAction();
	ActiveActionTimeoutRemaining = 0.0f;
	ResetActiveAttackState();

	while (++CurrentEnemyIndex < EnemyTurnOrder.Num())
	{
		AGridMonsterActor* Candidate = EnemyTurnOrder[CurrentEnemyIndex];
		if (!IsValid(Candidate) || Candidate->IsDead() || !PrepareMonsterForCombat(Candidate))
		{
			continue;
		}

		CurrentMonster = Candidate;
		CurrentMonsterMaximumActionPoints = FMath::Max(0, Candidate->MonsterDefinition->ActionPointsPerTurn);
		ActionPointBudget.Reset(CurrentMonsterMaximumActionPoints);
		CurrentMonsterRemainingActionPoints = ActionPointBudget.GetRemainingPoints();
		if (bCollectRuntimeMetrics)
		{
			++RuntimeMetrics.MonsterTurnsStarted;
		}

		BindCurrentMovement(Candidate->FindComponentByClass<UGridMonsterMovementComponent>());
		BindCurrentCombat(Candidate->FindComponentByClass<UGridMonsterCombatComponent>());

		FGridCombatLogEntry TurnEntry;
		TurnEntry.RoundNumber = RoundNumber;
		TurnEntry.Phase = CurrentPhase;
		TurnEntry.Type = EGridCombatLogEntryType::MonsterTurnStarted;
		TurnEntry.SourceId = ResolveMonsterLogId(Candidate);
		TurnEntry.SourceDisplayName = ResolveMonsterDisplayName(Candidate);
		TurnEntry.Message = FGridCombatLogFormatter::FormatMonsterTurnStarted(TurnEntry.SourceDisplayName);
		AppendCombatLogEntry(TurnEntry);
		OnMonsterTurnStarted.Broadcast(Candidate);
		PrepareCurrentMonsterActions();
		ExecuteNextAction();
		return;
	}

	FinishEnemyPhase();
}

void UGridTurnManagerComponent::PrepareCurrentMonsterActions()
{
	PendingActions.Reset();
	FScopedTurnPlanningMetrics PlanningMetrics(bCollectRuntimeMetrics, RuntimeMetrics, PendingActions);

	if (!IsValid(CurrentMonster) || CurrentMonster->IsDead())
	{
		return;
	}

	UGridMonsterBehaviorComponent* Behavior = CurrentMonster->FindComponentByClass<UGridMonsterBehaviorComponent>();
	if (!Behavior)
	{
		FGridMonsterTurnPlanner::BuildMovementTurn(CurrentMonster->ResolvePersistenceId(), CurrentMonster->CurrentCell, CurrentMonster->Facing,
			TArray<FIntPoint>(), CurrentMonsterRemainingActionPoints, PendingActions);
		return;
	}

	Behavior->RefreshPerception();
	const bool bHasPartyPerception = Behavior->HasPartyPerception();
	const FIntPoint PartyCell = IsValid(PartyPawn) ? FIntPoint(PartyPawn->CurrentCellX, PartyPawn->CurrentCellY) : FIntPoint::ZeroValue;
	const bool bKnowsCurrentPartyCell =
		IsValid(PartyPawn) && (bHasPartyPerception || (Behavior->bHasLastKnownPartyCell && Behavior->LastKnownPartyCell == PartyCell));
	const bool bRangedKeeper =
		IsValid(CurrentMonster->MonsterDefinition) && CurrentMonster->MonsterDefinition->HasAIProfile(EGridMonsterAIProfile::RangedKeeper);

	if (bRangedKeeper)
	{
		if (bKnowsCurrentPartyCell && IsValid(CurrentCombatComponent) &&
			MON174TryPlanRangedKeeper(
				RuntimeActor, CurrentMovementComponent, CurrentMonster, CurrentCombatComponent, PartyCell, CurrentMonsterRemainingActionPoints, PendingActions))
		{
			return;
		}

		UE_LOG(LogGridMonsterAI, Verbose, TEXT("[GridRangedKeeper] Plan Monster=%s Mode=Wait Reason=NoValidFiringPlan"), *GetNameSafe(CurrentMonster));
		FGridMonsterTurnPlanner::BuildMovementTurn(CurrentMonster->ResolvePersistenceId(), CurrentMonster->CurrentCell, CurrentMonster->Facing,
			TArray<FIntPoint>(), CurrentMonsterRemainingActionPoints, PendingActions);
		return;
	}

	if (bKnowsCurrentPartyCell && IsValid(CurrentCombatComponent))
	{
		const int32 DistanceToParty = FGridMonsterPathfinder::ManhattanDistance(CurrentMonster->CurrentCell, PartyCell);
		FGridMonsterAttackDefinition RangedAttack;
		if (CurrentCombatComponent->GetPreferredAttackForRange(DistanceToParty, RangedAttack) && RangedAttack.IsRangedAttack())
		{
			bool bLineOfSightSatisfied = true;
			if (RangedAttack.bRequiresLineOfSight)
			{
				bLineOfSightSatisfied = IsValid(RuntimeActor) &&
					FGridMonsterPerception::HasStraightLineOfSight(CurrentMonster->CurrentCell, PartyCell, RangedAttack.RangeCells,
						[this](const FIntPoint& From, const FIntPoint& To)
						{
							const EGridEdge Direction = FGridMonsterPathfinder::GetDirectionBetweenAdjacentCells(From, To);
							return IsValid(RuntimeActor) && Direction != EGridEdge::None && RuntimeActor->CanMove(From.X, From.Y, Direction);
						});
			}

			if (FGridMonsterRangedAttackPlanner::BuildStationaryRangedTurn(CurrentMonster->ResolvePersistenceId(), CurrentMonster->CurrentCell,
					CurrentMonster->Facing, PartyCell, CurrentMonsterRemainingActionPoints, RangedAttack, bLineOfSightSatisfied, PendingActions))
			{
				return;
			}
		}
	}

	bool bFoundPath = false;
	if (bHasPartyPerception)
	{
		bFoundPath = Behavior->FindPursuitPath();
	}
	else if (Behavior->bHasLastKnownPartyCell)
	{
		bFoundPath = Behavior->FindPathToLastKnownPartyCell();
	}

	const TArray<FIntPoint> EmptyPath;
	const TArray<FIntPoint>& PlannedPath = bFoundPath ? Behavior->LastPath : EmptyPath;

	FGridMonsterAttackDefinition MeleeAttack;
	const bool bCanPlanMelee =
		bHasPartyPerception && IsValid(CurrentCombatComponent) && CurrentCombatComponent->GetPreferredMeleeAttack(MeleeAttack) && IsValid(PartyPawn);

	if (bCanPlanMelee)
	{
		FGridMonsterTurnPlanner::BuildDirectMeleeTurn(CurrentMonster->ResolvePersistenceId(), CurrentMonster->CurrentCell, CurrentMonster->Facing, PartyCell,
			PlannedPath, CurrentMonsterRemainingActionPoints, MeleeAttack.AttackId, MeleeAttack.ActionPointCost, PendingActions);

		if (!CurrentMonster->MonsterDefinition->HasAIProfile(EGridMonsterAIProfile::FastHarasser))
		{
			return;
		}

		FIntPoint AttackCell;
		EGridEdge AttackFacing = EGridEdge::North;
		int32 RemainingAfterAttack = 0;
		if (!GetPostAttackPlanState(PendingActions, CurrentMonster->CurrentCell, CurrentMonster->Facing, CurrentMonsterRemainingActionPoints, AttackCell,
				AttackFacing, RemainingAfterAttack))
		{
			return;
		}

		if (RemainingAfterAttack < 1)
		{
			UE_LOG(LogGridMonsterAI, Verbose, TEXT("[GridFastHarasser] NoRetreat Monster=%s Reason=NoActionPoints"), *GetNameSafe(CurrentMonster));
			return;
		}

		UGridMonsterOccupancySubsystem* OccupancySubsystem = CurrentMovementComponent ? CurrentMovementComponent->GetOccupancySubsystem() : nullptr;
		TArray<FGridRetreatCandidate> Candidates;
		BuildRetreatCandidates(RuntimeActor, OccupancySubsystem, CurrentMonster, AttackCell, PartyCell, Candidates);

		FGridRetreatDecision RetreatDecision;
		if (!FGridFastHarasserPlanner::SelectBestRetreatCell(Candidates, FGridMonsterPathfinder::ManhattanDistance(AttackCell, PartyCell), RetreatDecision))
		{
			UE_LOG(LogGridMonsterAI, Verbose, TEXT("[GridFastHarasser] NoRetreat Monster=%s Reason=NoValidCell"), *GetNameSafe(CurrentMonster));
			return;
		}

		float Roll = 0.0f;
		const float RetreatChance = FMath::Clamp(CurrentMonster->MonsterDefinition->RetreatChance, 0.0f, 1.0f);
		const bool bShouldRetreat = FGridFastHarasserPlanner::ShouldRetreat(RetreatChance, CombatRandomStream, &Roll);

		UE_LOG(LogGridMonsterAI, Verbose,
			TEXT("[GridFastHarasser] Decision Monster=%s Chance=%.2f Roll=%.3f CandidateCount=%d Retreat=%s Cell=(%d,%d) Score=%d"),
			*GetNameSafe(CurrentMonster), RetreatChance, Roll, Candidates.Num(), bShouldRetreat ? TEXT("true") : TEXT("false"), RetreatDecision.RetreatCell.X,
			RetreatDecision.RetreatCell.Y, RetreatDecision.Score);

		FGridMonsterTurnPlanner::BuildFastHarasserTurn(CurrentMonster->ResolvePersistenceId(), CurrentMonster->CurrentCell, CurrentMonster->Facing, PartyCell,
			PlannedPath, CurrentMonsterRemainingActionPoints, MeleeAttack.AttackId, MeleeAttack.ActionPointCost, RetreatDecision, bShouldRetreat,
			PendingActions);
		return;
	}

	FGridMonsterTurnPlanner::BuildMovementTurn(CurrentMonster->ResolvePersistenceId(), CurrentMonster->CurrentCell, CurrentMonster->Facing, PlannedPath,
		CurrentMonsterRemainingActionPoints, PendingActions);
}

void UGridTurnManagerComponent::FinishCurrentMonsterTurn()
{
	AGridMonsterActor* CompletedMonster = CurrentMonster;
	FGridCombatantInitiativeEntry* CompletedEntry =
		IsValid(CompletedMonster) ? FindInitiativeEntry(EGridCombatantSide::Monster, CompletedMonster->ResolvePersistenceId()) : nullptr;
	UnbindCurrentMovement();
	UnbindCurrentCombat();
	PendingActions.Reset();
	bHasActiveAction = false;
	ActiveAction = FGridCombatAction();
	ActiveActionTimeoutRemaining = 0.0f;
	ResetActiveAttackState();
	CurrentMonsterMaximumActionPoints = 0;
	CurrentMonsterRemainingActionPoints = 0;
	ActionPointBudget.Reset(0);
	CurrentMonster = nullptr;

	if (!InitiativeOrder.IsEmpty() && CompletedEntry && CompletedEntry->State != EGridCombatantTurnState::Defeated)
	{
		SetInitiativeEntryState(*CompletedEntry, EGridCombatantTurnState::Completed);
	}

	if (IsValid(CompletedMonster))
	{
		OnMonsterTurnEnded.Broadcast(CompletedMonster);
	}

	if (!bCombatActive)
	{
		return;
	}

	if (!InitiativeOrder.IsEmpty())
	{
		BeginNextCombatantTurn();
	}
	else
	{
		BeginNextMonsterTurn();
	}
}

void UGridTurnManagerComponent::FinishEnemyPhase()
{
	if (!InitiativeOrder.IsEmpty())
	{
		FinishInitiativeRound();
		return;
	}

	if (!bCombatActive || !PhaseState.CompleteEnemyPhase())
	{
		return;
	}

	SetPhase(PhaseState.GetPhase());

	if (!HasLivingPartyCharacter())
	{
		FinishCombat(EGridCombatPhase::Defeat);
		return;
	}

	if (!HasLivingCombatMonster())
	{
		FinishCombat(EGridCombatPhase::Victory);
		return;
	}

	if (!PhaseState.BeginNextRound())
	{
		AbortCombat();
		return;
	}

	RoundNumber = PhaseState.GetRoundNumber();
	ResetPartyMobilityForRound();
	BeginPlayerCharacterPhase();
	if (bCollectRuntimeMetrics)
	{
		++RuntimeMetrics.RoundsStarted;
	}
	SetPhase(PhaseState.GetPhase());
	SetPartyInputLocked(false);

	FGridCombatLogEntry RoundEntry;
	RoundEntry.RoundNumber = RoundNumber;
	RoundEntry.Phase = CurrentPhase;
	RoundEntry.Type = EGridCombatLogEntryType::RoundStarted;
	RoundEntry.Message = FGridCombatLogFormatter::FormatRoundStarted(RoundNumber);
	AppendCombatLogEntry(RoundEntry);
	OnRoundStarted.Broadcast(RoundNumber);
}

void UGridTurnManagerComponent::FinishCombat(EGridCombatPhase ResultPhase)
{
	if (!bCombatActive || !PhaseState.FinishCombat(ResultPhase))
	{
		return;
	}

	ClearPlayerCharacterTurnStates();
	ResetCombatActionCooldowns();
	ClearPartyMobilityState(true);
	ClearInitiativeState(true);
	bPlayerAttackResolutionInProgress = false;
	bPendingVictoryAfterPlayerAttack = false;

	if (CurrentMovementComponent && CurrentMovementComponent->IsBusy())
	{
		CurrentMovementComponent->CancelCurrentAction();
	}
	if (CurrentCombatComponent)
	{
		CurrentCombatComponent->CancelAttackPresentation();
	}

	UnbindCurrentMovement();
	UnbindCurrentCombat();
	UnbindCombatMonsterDeaths();
	bWaitingForCombatStart = false;
	CombatStartDelayRemaining = 0.0f;
	ActiveActionTimeoutRemaining = 0.0f;
	bHasActiveAction = false;
	ActiveAction = FGridCombatAction();
	PendingActions.Reset();
	EnemyTurnOrder.Reset();
	CurrentMonster = nullptr;
	CurrentEnemyIndex = INDEX_NONE;
	CurrentMonsterMaximumActionPoints = 0;
	CurrentMonsterRemainingActionPoints = 0;
	ActionPointBudget.Reset(0);
	ResetActiveAttackState();
	bCombatActive = false;
	CombatMonsters.Reset();

	FGridCombatLogEntry ResultEntry;
	ResultEntry.RoundNumber = RoundNumber;
	ResultEntry.Phase = ResultPhase;
	ResultEntry.Type = ResultPhase == EGridCombatPhase::Victory ? EGridCombatLogEntryType::Victory : EGridCombatLogEntryType::Defeat;
	ResultEntry.Message = FGridCombatLogFormatter::FormatCombatEnded(ResultPhase);
	AppendCombatLogEntry(ResultEntry);

	SetPhase(PhaseState.GetPhase());
	SetPartyInputLocked(false);
	OnCombatEnded.Broadcast(ResultPhase);
	RefreshTickEnabled();
}

bool UGridTurnManagerComponent::IsPartyAtRest() const
{
	if (!IsValid(PartyPawn) || !IsValid(RuntimeActor) || PendingPartyMotionType != EGridPendingPartyMotionType::None)
	{
		return false;
	}

	const FVector ExpectedLocation = RuntimeActor->GetCellCenterWorld(PartyPawn->CurrentCellX, PartyPawn->CurrentCellY, PartyPawn->EyeHeight);
	const float LocationErrorSquared = FVector::DistSquared(PartyPawn->GetActorLocation(), ExpectedLocation);
	const float ExpectedYaw = GridDirectionUtils::ToYaw(PartyPawn->Facing);
	const float YawError = FMath::Abs(FMath::FindDeltaAngleDegrees(PartyPawn->GetActorRotation().Yaw, ExpectedYaw));

	return LocationErrorSquared <= FMath::Square(2.0f) && YawError <= 1.0f;
}

bool UGridTurnManagerComponent::HasLivingCombatMonster() const
{
	for (const AGridMonsterActor* Monster : CombatMonsters)
	{
		if (IsValid(Monster) && !Monster->IsDead())
		{
			return true;
		}
	}
	return false;
}

bool UGridTurnManagerComponent::HasLivingPartyCharacter() const
{
	if (!IsValid(PartyPawn) || !IsValid(PartyPawn->PartyInventoryComponent))
	{
		return false;
	}

	for (const FGridCharacterInventoryState& Character : PartyPawn->PartyInventoryComponent->PartyInventoryState.ActiveCharacters)
	{
		if (Character.DerivedStats.CurrentHealth > 0)
		{
			return true;
		}
	}
	return false;
}
