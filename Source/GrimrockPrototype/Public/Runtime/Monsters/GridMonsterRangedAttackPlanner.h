#pragma once

#include "CoreMinimal.h"
#include "Core/GridTypes.h"
#include "Runtime/Combat/GridCombatTypes.h"
#include "Runtime/Monsters/GridMonsterTypes.h"

/**
 * Pure MON17.3 planner for an already-positioned ranged monster.
 *
 * It may rotate toward an axial party cell and append one RangedAttack, but it
 * never moves the monster to a different cell. Range seeking and kiting remain
 * owned by MON17.4.
 */
class GRIMROCKPROTOTYPE_API FGridMonsterRangedAttackPlanner
{
public:
	/** Returns the cardinal direction from SourceCell to an axial TargetCell. */
	static EGridEdge GetAxialDirection(const FIntPoint& SourceCell, const FIntPoint& TargetCell);

	/**
     * Builds zero to two free Turn actions followed by one paid RangedAttack.
     * Returns false and leaves OutActions empty when the current cell is not a
     * legal firing position for Attack.
     */
	static bool BuildStationaryRangedTurn(const FGuid& SourceActorId, const FIntPoint& StartCell, EGridEdge StartFacing, const FIntPoint& PartyCell,
		int32 AvailableActionPoints, const FGridMonsterAttackDefinition& Attack, bool bLineOfSightSatisfied, TArray<FGridCombatAction>& OutActions);
};
