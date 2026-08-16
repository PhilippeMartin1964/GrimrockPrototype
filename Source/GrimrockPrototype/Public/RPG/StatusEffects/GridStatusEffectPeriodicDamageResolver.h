#pragma once

#include "CoreMinimal.h"
#include "RPG/StatusEffects/GridStatusEffectTypes.h"
#include "Runtime/Combat/GridCombatTypes.h"

/** Pure snapshot of one periodic status damage resolution. */
struct GRIMROCKPROTOTYPE_API FGridStatusEffectPeriodicDamageResolution
{
    FName EffectId = NAME_None;
    FGuid SourceId;
    EGridDamageType DamageType = EGridDamageType::Physical;
    int32 StackCount = 0;
    int32 RawDamage = 0;
    FGridAttackResult DamageResult;
};

/**
 * Pure MON16.3 adapter between a generic runtime status and the existing
 * combat damage pipeline. It owns no actors, inventory or lifecycle state.
 */
class GRIMROCKPROTOTYPE_API FGridStatusEffectPeriodicDamageResolver
{
public:
    static bool Resolve (
        const FGridStatusEffectRuntimeState& State,
        const FGridAttackTargetStats& Target,
        FGridStatusEffectPeriodicDamageResolution& OutResolution,
        FString& OutError);
};
