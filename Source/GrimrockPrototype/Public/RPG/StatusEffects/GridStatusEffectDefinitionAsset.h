#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RPG/StatusEffects/GridStatusEffectTypes.h"
#include "GridStatusEffectDefinitionAsset.generated.h"

/** Static data authority for one generic status effect. */
UCLASS (BlueprintType)
class GRIMROCKPROTOTYPE_API UGridStatusEffectDefinitionAsset : public UPrimaryDataAsset
{
    GENERATED_BODY ()

public:
    virtual FPrimaryAssetId GetPrimaryAssetId () const override;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects|Identity")
    FName EffectId = NAME_None;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects|Identity")
    FText DisplayName;

    UPROPERTY (
        EditAnywhere,
        BlueprintReadOnly,
        Category = "RPG|Status Effects|Identity",
        meta = (MultiLine = "true"))
    FText Description;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects|Rules")
    EGridStatusEffectDisposition Disposition =
        EGridStatusEffectDisposition::Neutral;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects|Rules")
    EGridStatusEffectDurationUnit DurationUnit =
        EGridStatusEffectDurationUnit::Rounds;

    /** Timed effects require a positive value; Permanent requires zero. */
    UPROPERTY (
        EditAnywhere,
        BlueprintReadOnly,
        Category = "RPG|Status Effects|Rules",
        meta = (ClampMin = "0"))
    int32 DefaultDuration = 1;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects|Stacking")
    EGridStatusEffectStackPolicy StackPolicy =
        EGridStatusEffectStackPolicy::NoStack;

    UPROPERTY (
        EditAnywhere,
        BlueprintReadOnly,
        Category = "RPG|Status Effects|Stacking",
        meta = (ClampMin = "1"))
    int32 MaxStacks = 1;

    /**
     * Declarative contribution reserved for MON16.4. MON16.1 never applies it
     * to FGridCombatantInitiativeEntry::InitiativeModifier.
     */
    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects|Combat")
    int32 InitiativeModifier = 0;

    UFUNCTION (BlueprintPure, Category = "RPG|Status Effects|Validation")
    bool IsValidDefinition () const;

    UFUNCTION (BlueprintCallable, Category = "RPG|Status Effects|Validation")
    bool ValidateDefinition (UPARAM (ref) FString& OutError) const;

    /**
     * Pure factory for one runtime application. OutState is unchanged on
     * failure. DurationOverride == INDEX_NONE uses DefaultDuration.
     */
    bool BuildRuntimeState (
        const FGuid& SourceId,
        int32 InitialStackCount,
        int32 DurationOverride,
        FGridStatusEffectRuntimeState& OutState,
        FString& OutError) const;
};
