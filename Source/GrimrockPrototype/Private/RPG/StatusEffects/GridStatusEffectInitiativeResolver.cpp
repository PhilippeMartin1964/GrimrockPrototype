#include "RPG/StatusEffects/GridStatusEffectInitiativeResolver.h"

#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"

int32 FGridStatusEffectInitiativeResolver::ComputeModifier (
    const FGridStatusEffectCollection& StatusEffects)
{
    int64 Total = 0;
    for (const FGridStatusEffectRuntimeState& StatusEffect :
        StatusEffects.ActiveEffects)
    {
        if (!StatusEffect.IsValid () ||
            !IsValid (StatusEffect.DefinitionAsset))
        {
            continue;
        }

        const int64 Contribution =
            static_cast<int64> (
                StatusEffect.DefinitionAsset->InitiativeModifier) *
            static_cast<int64> (StatusEffect.StackCount);
        Total = FMath::Clamp<int64> (
            Total + Contribution,
            static_cast<int64> (MIN_int32),
            static_cast<int64> (MAX_int32));
    }

    return static_cast<int32> (Total);
}

int32 FGridStatusEffectInitiativeResolver::ComputeContribution (
    const FGridStatusEffectRuntimeState& StatusEffect)
{
    if (!StatusEffect.IsValid () ||
        !IsValid (StatusEffect.DefinitionAsset))
    {
        return 0;
    }

    const int64 Contribution =
        static_cast<int64> (
            StatusEffect.DefinitionAsset->InitiativeModifier) *
        static_cast<int64> (StatusEffect.StackCount);
    return static_cast<int32> (FMath::Clamp<int64> (
        Contribution,
        static_cast<int64> (MIN_int32),
        static_cast<int64> (MAX_int32)));
}
