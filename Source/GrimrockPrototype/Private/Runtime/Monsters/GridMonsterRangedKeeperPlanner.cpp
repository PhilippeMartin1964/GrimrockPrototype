#include "Runtime/Monsters/GridMonsterRangedKeeperPlanner.h"

#include "Runtime/Combat/GridCombatTurnPlan.h"
#include "Runtime/Monsters/GridMonsterPathfinder.h"
#include "Runtime/Monsters/GridMonsterRangedAttackPlanner.h"

namespace
{
	FGridCombatAction MON174MakeTurnAction(const FGuid& SourceActorId, const FIntPoint& CurrentCell, EGridEdge TargetFacing)
	{
		FGridCombatAction Action;
		Action.ActionId = FGuid::NewGuid();
		Action.Type = EGridCombatActionType::Turn;
		Action.SourceActorId = SourceActorId;
		Action.TargetCell = FGridMonsterPathfinder::GetNeighborCell(CurrentCell, TargetFacing);
		Action.ActionPointCost = 0;
		Action.bIsRepositioningAction = true;
		return Action;
	}

	FGridCombatAction MON174MakeMoveAction(const FGuid& SourceActorId, const FIntPoint& TargetCell)
	{
		FGridCombatAction Action;
		Action.ActionId = FGuid::NewGuid();
		Action.Type = EGridCombatActionType::Move;
		Action.SourceActorId = SourceActorId;
		Action.TargetCell = TargetCell;
		Action.ActionPointCost = 1;
		Action.bIsRepositioningAction = true;
		return Action;
	}
}

void FGridMonsterRangedKeeperPlanner::BuildAxialFiringCandidates(
	const FIntPoint& PartyCell, int32 MinDistanceCells, int32 MaxDistanceCells, TArray<FIntPoint>& OutCandidates)
{
	OutCandidates.Reset();

	const int32 SafeMinDistance = FMath::Max(1, MinDistanceCells);
	const int32 SafeMaxDistance = FMath::Max(SafeMinDistance, MaxDistanceCells);

	for (int32 Distance = SafeMinDistance; Distance <= SafeMaxDistance; ++Distance)
	{
		for (const EGridEdge Direction : FGridMonsterPathfinder::GetOrderedDirections())
		{
			FIntPoint Candidate = PartyCell;
			for (int32 StepIndex = 0; StepIndex < Distance; ++StepIndex)
			{
				Candidate = FGridMonsterPathfinder::GetNeighborCell(Candidate, Direction);
			}
			OutCandidates.Add(Candidate);
		}
	}
}

bool FGridMonsterRangedKeeperPlanner::IsPreferredDistance(int32 DistanceCells, int32 PreferredMinDistance, int32 PreferredMaxDistance)
{
	const int32 SafeMin = FMath::Max(0, PreferredMinDistance);
	const int32 SafeMax = FMath::Max(SafeMin, PreferredMaxDistance);
	return DistanceCells >= SafeMin && DistanceCells <= SafeMax;
}

bool FGridMonsterRangedKeeperPlanner::BuildRepositionTurn(const FGuid& SourceActorId, const FIntPoint& StartCell, EGridEdge StartFacing,
	const FIntPoint& PartyCell, const TArray<FIntPoint>& PathToFiringCell, int32 AvailableActionPoints, const FGridMonsterAttackDefinition& Attack,
	bool bGoalLineOfSightSatisfied, bool bMayAttackThisTurn, TArray<FGridCombatAction>& OutActions)
{
	OutActions.Reset();

	if (!SourceActorId.IsValid() || !Attack.IsValidDefinition() || !Attack.IsRangedAttack())
	{
		return false;
	}

	FIntPoint SimulatedCell = StartCell;
	EGridEdge SimulatedFacing = StartFacing == EGridEdge::None ? EGridEdge::North : StartFacing;
	int32 RemainingActionPoints = FMath::Max(0, AvailableActionPoints);

	for (const FIntPoint& Step : PathToFiringCell)
	{
		if (RemainingActionPoints < 1)
		{
			break;
		}

		const EGridEdge DesiredFacing = FGridMonsterPathfinder::GetDirectionBetweenAdjacentCells(SimulatedCell, Step);
		if (!GridCombatTurnPlan::AppendTurnsToward(SourceActorId, SimulatedCell, DesiredFacing, SimulatedFacing, OutActions, MON174MakeTurnAction))
		{
			break;
		}

		OutActions.Add(MON174MakeMoveAction(SourceActorId, Step));
		SimulatedCell = Step;
		--RemainingActionPoints;
	}

	const bool bReachedFiringCell = PathToFiringCell.IsEmpty() || SimulatedCell == PathToFiringCell.Last();
	if (bReachedFiringCell && bMayAttackThisTurn)
	{
		TArray<FGridCombatAction> AttackActions;
		if (FGridMonsterRangedAttackPlanner::BuildStationaryRangedTurn(
				SourceActorId, SimulatedCell, SimulatedFacing, PartyCell, RemainingActionPoints, Attack, bGoalLineOfSightSatisfied, AttackActions))
		{
			OutActions.Append(AttackActions);
		}
	}

	return !OutActions.IsEmpty();
}
