#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RPG/StatusEffects/GridStatusEffectTypes.h"
#include "Runtime/GridInventoryTypes.h"
#include "GridStatusEffectDefinitionAsset.generated.h"

/** Optional deterministic periodic damage payload for MON16.3. */
USTRUCT (BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridStatusEffectPeriodicDamageProfile
{
    GENERATED_BODY ()

    /** Canonical combat damage type; no parallel resistance vocabulary. */
    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects|Periodic Damage")
    EGridDamageType DamageType = EGridDamageType::Physical;

    /** Raw damage dealt by each active stack on the effect's duration boundary. */
    UPROPERTY (
        EditAnywhere,
        BlueprintReadOnly,
        Category = "RPG|Status Effects|Periodic Damage",
        meta = (ClampMin = "0"))
    int32 DamagePerStack = 0;

    bool IsEnabled () const
    {
        return DamagePerStack > 0;
    }

    bool IsValid () const
    {
        return DamagePerStack >= 0;
    }
};

/**
 * Generic MON16.5 combat restrictions. Names such as Stun, Silence and
 * Immobilize remain data-only conventions; production code consumes these
 * boolean capabilities and never compares EffectId.
 */
USTRUCT (BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridStatusEffectControlProfile
{
    GENERATED_BODY ()

    /** Consume the target's next matching initiative activation without acting. */
    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects|Control")
    bool bSkipActivation = false;

    /** Block combat actions whose canonical SourcePolicy is Spell. */
    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects|Control")
    bool bBlockSpellActions = false;

    /** Block cell translation while still allowing turning and attacks. */
    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects|Control")
    bool bBlockTranslation = false;

    bool HasAnyRestriction () const
    {
        return bSkipActivation || bBlockSpellActions || bBlockTranslation;
    }

    void Merge (const FGridStatusEffectControlProfile& Other)
    {
        bSkipActivation = bSkipActivation || Other.bSkipActivation;
        bBlockSpellActions = bBlockSpellActions || Other.bBlockSpellActions;
        bBlockTranslation = bBlockTranslation || Other.bBlockTranslation;
    }
};

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

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects|Identity", meta = (MultiLine = "true"))
    FText Description;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects|Rules")
    EGridStatusEffectDisposition Disposition = EGridStatusEffectDisposition::Neutral;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects|Rules")
    EGridStatusEffectDurationUnit DurationUnit = EGridStatusEffectDurationUnit::Rounds;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects|Rules", meta = (ClampMin = "0"))
    int32 DefaultDuration = 1;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects|Rules", meta = (ClampMin = "0"))
    int32 DefaultPotency = 0;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects|Stacking")
    EGridStatusEffectStackPolicy StackPolicy = EGridStatusEffectStackPolicy::NoStack;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects|Stacking", meta = (ClampMin = "1"))
    int32 MaxStacks = 1;

    /**
     * Periodic damage executes immediately before the matching Turns/Rounds
     * duration decrement. A zero DamagePerStack means no periodic damage.
     */
    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects|Periodic Damage")
    FGridStatusEffectPeriodicDamageProfile PeriodicDamage;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects|Combat")
    int32 InitiativeModifier = 0;

    /** Optional action/movement restrictions aggregated by MON16.5. */
    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects|Control")
    FGridStatusEffectControlProfile Control;

    UFUNCTION (BlueprintPure, Category = "RPG|Status Effects|Validation")
    bool IsValidDefinition () const;

    UFUNCTION (BlueprintCallable, Category = "RPG|Status Effects|Validation")
    bool ValidateDefinition (UPARAM (ref) FString& OutError) const;

    bool BuildRuntimeState (
        const FGuid& SourceId,
        int32 InitialStackCount,
        int32 DurationOverride,
        FGridStatusEffectRuntimeState& OutState,
        FString& OutError) const
    {
        return BuildRuntimeState (
            SourceId,
            InitialStackCount,
            DurationOverride,
            INDEX_NONE,
            OutState,
            OutError);
    }

    bool BuildRuntimeState (
        const FGuid& SourceId,
        int32 InitialStackCount,
        int32 DurationOverride,
        int32 PotencyOverride,
        FGridStatusEffectRuntimeState& OutState,
        FString& OutError) const;
};