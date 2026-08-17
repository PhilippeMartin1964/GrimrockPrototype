#pragma once

#include "CoreMinimal.h"
#include "RPG/StatusEffects/GridStatusEffectTypes.h"

class UGridStatusEffectDefinitionAsset;

/**
 * MON16.7 conversion boundary between transient runtime status state and the
 * stable SaveGame snapshot. DefinitionAsset is deliberately never persisted.
 */
struct GRIMROCKPROTOTYPE_API FGridStatusEffectPersistence
{
    static bool ValidateSavedCollection (
        const TArray<FGridStatusEffectSaveState>& SavedStates,
        FString& OutError);

    static bool CaptureCollection (
        const FGridStatusEffectCollection& RuntimeCollection,
        TArray<FGridStatusEffectSaveState>& OutSavedStates,
        FString& OutError);

    static bool RestoreCollection (
        const TArray<FGridStatusEffectSaveState>& SavedStates,
        TFunctionRef<UGridStatusEffectDefinitionAsset* (FName)> DefinitionResolver,
        FGridStatusEffectCollection& OutRuntimeCollection,
        FString& OutError);

    static bool RestoreCollection (
        const TArray<FGridStatusEffectSaveState>& SavedStates,
        FGridStatusEffectCollection& OutRuntimeCollection,
        FString& OutError);

    /** Resolves the canonical GridStatusEffect:EffectId primary asset. */
    static UGridStatusEffectDefinitionAsset* ResolveDefinitionByEffectId (
        FName EffectId);
};
