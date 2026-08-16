#pragma once

#include "CoreMinimal.h"
#include "RPG/StatusEffects/GridStatusEffectTypes.h"

/** Pure projection from active status-effect state to the existing initiative modifier. */
class GRIMROCKPROTOTYPE_API FGridStatusEffectInitiativeResolver
{
public:
    /**
     * Returns the additive runtime initiative modifier contributed by all
     * active status effects. Each definition contribution is multiplied by
     * StackCount and the final value is saturated to int32.
     */
    static int32 ComputeModifier (
        const FGridStatusEffectCollection& StatusEffects);

    /** Returns one active effect's saturated initiative contribution. */
    static int32 ComputeContribution (
        const FGridStatusEffectRuntimeState& StatusEffect);
};
