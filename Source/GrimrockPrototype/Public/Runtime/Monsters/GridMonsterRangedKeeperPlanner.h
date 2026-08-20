#pragma once

#include "CoreMinimal.h"
#include "Core/GridTypes.h"
#include "Runtime/Combat/GridCombatTypes.h"
#include "Runtime/Monsters/GridMonsterTypes.h"

/**
 * Pure MON17.4 planner helpers for RangedKeeper positioning.
 *
 * World queries remain outside this class. Gameplay supplies a path that ends
 * on a validated axial firing cell; this planner only converts that path into
 * deterministic Turn / Move / RangedAttack actions.
 */
class GRIMROCKPROTOTYPE_API FGridMonsterRangedKeeperPlanner
{
public:
    /** Builds cardinal firing cells around PartyCell for the inclusive band. */
    static void BuildAxialFiringCandidates (
        const FIntPoint& PartyCell,
        int32 MinDistanceCells,
        int32 MaxDistanceCells,
        TArray<FIntPoint>& OutCandidates);

    /** Inclusive preferred-distance predicate with defensive range normalization. */
    static bool IsPreferredDistance (
        int32 DistanceCells,
        int32 PreferredMinDistance,
        int32 PreferredMaxDistance);

    /**
     * Converts a path toward a validated firing cell into this turn's actions.
     * Movement costs one AP per cell. If the goal is reached and enough AP
     * remain, the existing stationary ranged planner appends the attack.
     *
     * bMayAttackThisTurn lets a monster reposition while its ranged attack is
     * cooling down without bypassing the cooldown authority.
     */
    static bool BuildRepositionTurn (
        const FGuid& SourceActorId,
        const FIntPoint& StartCell,
        EGridEdge StartFacing,
        const FIntPoint& PartyCell,
        const TArray<FIntPoint>& PathToFiringCell,
        int32 AvailableActionPoints,
        const FGridMonsterAttackDefinition& Attack,
        bool bGoalLineOfSightSatisfied,
        bool bMayAttackThisTurn,
        TArray<FGridCombatAction>& OutActions);
};
