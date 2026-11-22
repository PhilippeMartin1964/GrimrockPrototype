#pragma once

#include "Core/GridDirectionUtils.h"
#include "Runtime/Combat/GridCombatTypes.h"

namespace GridCombatTurnPlan
{
	// Left wins the 180-degree tie. Action creation and planner policy stay at the call site.
	template <typename TMakeTurnAction>
	bool AppendTurnsToward(const FGuid& SourceActorId, const FIntPoint& CurrentCell, EGridEdge DesiredFacing, EGridEdge& InOutFacing,
		TArray<FGridCombatAction>& OutActions, TMakeTurnAction MakeTurnAction)
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
