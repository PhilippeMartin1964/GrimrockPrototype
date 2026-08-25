#include "Runtime/Monsters/GridMonsterRangedAttackPlanner.h"

#include "Core/GridDirectionUtils.h"
#include "Runtime/Monsters/GridMonsterPathfinder.h"

namespace
{
	FGridCombatAction MakeTurnAction(const FGuid& SourceActorId, const FIntPoint& CurrentCell, EGridEdge TargetFacing)
	{
		FGridCombatAction Action;
		Action.ActionId = FGuid::NewGuid();
		Action.Type = EGridCombatActionType::Turn;
		Action.SourceActorId = SourceActorId;
		Action.TargetCell = FGridMonsterPathfinder::GetNeighborCell(CurrentCell, TargetFacing);
		Action.ActionPointCost = 0;
		return Action;
	}

	FGridCombatAction MakeRangedAttackAction(const FGuid& SourceActorId, const FIntPoint& PartyCell, FName AttackId, int32 ActionPointCost)
	{
		FGridCombatAction Action;
		Action.ActionId = FGuid::NewGuid();
		Action.Type = EGridCombatActionType::RangedAttack;
		Action.SourceActorId = SourceActorId;
		Action.TargetCell = PartyCell;
		Action.AttackId = AttackId;
		Action.ActionPointCost = FMath::Max(0, ActionPointCost);
		return Action;
	}

	bool AppendTurnsToward(
		const FGuid& SourceActorId, const FIntPoint& CurrentCell, EGridEdge DesiredFacing, EGridEdge& InOutFacing, TArray<FGridCombatAction>& OutActions)
	{
		if (DesiredFacing == EGridEdge::None)
		{
			return false;
		}

		if (InOutFacing == EGridEdge::None)
		{
			InOutFacing = EGridEdge::North;
		}

		if (InOutFacing == DesiredFacing)
		{
			return true;
		}

		if (GridDirectionUtils::RotateLeft(InOutFacing) == DesiredFacing)
		{
			InOutFacing = GridDirectionUtils::RotateLeft(InOutFacing);
			OutActions.Add(MakeTurnAction(SourceActorId, CurrentCell, InOutFacing));
			return true;
		}

		if (GridDirectionUtils::RotateRight(InOutFacing) == DesiredFacing)
		{
			InOutFacing = GridDirectionUtils::RotateRight(InOutFacing);
			OutActions.Add(MakeTurnAction(SourceActorId, CurrentCell, InOutFacing));
			return true;
		}

		InOutFacing = GridDirectionUtils::RotateLeft(InOutFacing);
		OutActions.Add(MakeTurnAction(SourceActorId, CurrentCell, InOutFacing));
		InOutFacing = GridDirectionUtils::RotateLeft(InOutFacing);
		OutActions.Add(MakeTurnAction(SourceActorId, CurrentCell, InOutFacing));
		return InOutFacing == DesiredFacing;
	}
}

EGridEdge FGridMonsterRangedAttackPlanner::GetAxialDirection(const FIntPoint& SourceCell, const FIntPoint& TargetCell)
{
	if (SourceCell == TargetCell)
	{
		return EGridEdge::None;
	}

	if (SourceCell.X == TargetCell.X)
	{
		return TargetCell.Y > SourceCell.Y ? EGridEdge::North : EGridEdge::South;
	}

	if (SourceCell.Y == TargetCell.Y)
	{
		return TargetCell.X > SourceCell.X ? EGridEdge::East : EGridEdge::West;
	}

	return EGridEdge::None;
}

bool FGridMonsterRangedAttackPlanner::BuildStationaryRangedTurn(const FGuid& SourceActorId, const FIntPoint& StartCell, EGridEdge StartFacing,
	const FIntPoint& PartyCell, int32 AvailableActionPoints, const FGridMonsterAttackDefinition& Attack, bool bLineOfSightSatisfied,
	TArray<FGridCombatAction>& OutActions)
{
	OutActions.Reset();

	if (!Attack.IsValidDefinition() || !Attack.IsRangedAttack() || Attack.AttackId.IsNone() || AvailableActionPoints < Attack.ActionPointCost ||
		(Attack.bRequiresLineOfSight && !bLineOfSightSatisfied))
	{
		return false;
	}

	const int32 Distance = FGridMonsterPathfinder::ManhattanDistance(StartCell, PartyCell);
	const EGridEdge AttackFacing = GetAxialDirection(StartCell, PartyCell);
	if (AttackFacing == EGridEdge::None || !Attack.SupportsDistance(Distance))
	{
		return false;
	}

	EGridEdge SimulatedFacing = StartFacing == EGridEdge::None ? EGridEdge::North : StartFacing;
	if (!AppendTurnsToward(SourceActorId, StartCell, AttackFacing, SimulatedFacing, OutActions))
	{
		OutActions.Reset();
		return false;
	}

	OutActions.Add(MakeRangedAttackAction(SourceActorId, PartyCell, Attack.AttackId, Attack.ActionPointCost));
	return true;
}
