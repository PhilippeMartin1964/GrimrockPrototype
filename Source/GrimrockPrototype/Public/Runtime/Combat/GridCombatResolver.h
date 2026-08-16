#pragma once

#include "CoreMinimal.h"
#include "Runtime/Combat/GridCombatTypes.h"
#include "Runtime/GridInventoryTypes.h"

/**
 * Pure deterministic combat rules shared by monsters, characters, traps and projectiles.
 * The resolver never mutates an actor or inventory state.
 */
class GRIMROCKPROTOTYPE_API FGridCombatResolver
{
public:
    static FGridAttackResult ResolveAttack (
        const FGridAttackSourceStats& Source,
        const FGridAttackTargetStats& Target,
        const FGridAttackDefinition& Attack,
        FRandomStream& RandomStream);

    /** Explicit-roll overload used by deterministic automation tests. */
    static FGridAttackResult ResolveAttackFromRolls (
        const FGridAttackSourceStats& Source,
        const FGridAttackTargetStats& Target,
        const FGridAttackDefinition& Attack,
        int32 NaturalAttackRoll,
        int32 DamageRoll);

    /**
     * Guaranteed direct damage path used by hazards and status effects.
     * It reuses the standard multiplier, resistance and armor-pool rules but
     * performs no attack roll, evasion check or critical hit.
     */
    static FGridAttackResult ResolveDirectDamage (
        const FGridAttackTargetStats& Target,
        EGridDamageType DamageType,
        int32 RawDamage,
        EGridPhysicalDamageSubtype PhysicalSubtype =
            EGridPhysicalDamageSubtype::None);

    /** Shared mapping from the canonical resistance set to one damage type. */
    static int32 GetResistancePercent (
        const FGridDamageResistanceSet& Resistances,
        EGridDamageType DamageType);
};

/** Deterministic party target selection for grid monsters. */
class GRIMROCKPROTOTYPE_API FGridPartyTargetSelector
{
public:
    /**
     * Slots 0..FrontLineSlotCount-1 form the front line. Remaining slots form
     * the second line. Only living characters are eligible.
     */
    static int32 SelectTarget (
        const FGridPartyInventoryState& PartyState,
        FRandomStream& RandomStream,
        int32 FrontLineSlotCount = 3);
};
