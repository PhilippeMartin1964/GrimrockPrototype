#pragma once

#include "CoreMinimal.h"
#include "GridStatusEffectTypes.generated.h"

class UGridStatusEffectDefinitionAsset;

UENUM (BlueprintType)
enum class EGridStatusEffectDisposition : uint8
{
    Neutral UMETA (DisplayName = "Neutral"),
    Buff    UMETA (DisplayName = "Buff"),
    Debuff  UMETA (DisplayName = "Debuff")
};

UENUM (BlueprintType)
enum class EGridStatusEffectDurationUnit : uint8
{
    Turns     UMETA (DisplayName = "Turns"),
    Rounds    UMETA (DisplayName = "Rounds"),
    Permanent UMETA (DisplayName = "Permanent")
};

UENUM (BlueprintType)
enum class EGridStatusEffectStackPolicy : uint8
{
    NoStack           UMETA (DisplayName = "No Stack"),
    RefreshDuration   UMETA (DisplayName = "Refresh Duration"),
    AddStacks         UMETA (DisplayName = "Add Stacks"),
    ReplaceIfStronger UMETA (DisplayName = "Replace If Stronger")
};

/** One runtime application of a data-driven status effect. */
USTRUCT (BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridStatusEffectRuntimeState
{
    GENERATED_BODY ()

    /** Stable identity of the static effect definition. */
    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects")
    FName EffectId = NAME_None;

    /**
     * Stable gameplay identity of the source when one exists.
     * An invalid Guid means an anonymous/system/environment source.
     */
    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects")
    FGuid SourceId;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects")
    int32 StackCount = 1;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects")
    EGridStatusEffectDurationUnit DurationUnit =
        EGridStatusEffectDurationUnit::Rounds;

    /** Zero is reserved for Permanent effects. */
    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects")
    int32 RemainingDuration = 0;

    bool IsValid () const
    {
        if (EffectId.IsNone () || StackCount < 1)
        {
            return false;
        }

        switch (DurationUnit)
        {
        case EGridStatusEffectDurationUnit::Turns:
        case EGridStatusEffectDurationUnit::Rounds:
            return RemainingDuration > 0;

        case EGridStatusEffectDurationUnit::Permanent:
            return RemainingDuration == 0;

        default:
            return false;
        }
    }
};

/**
 * Common ordered runtime container used by party characters and monsters.
 * MON16.1 only accepts one runtime entry per EffectId; stacking resolution is
 * intentionally deferred to MON16.2.
 */
USTRUCT (BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridStatusEffectCollection
{
    GENERATED_BODY ()

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects")
    TArray<FGridStatusEffectRuntimeState> ActiveEffects;

    int32 Num () const
    {
        return ActiveEffects.Num ();
    }

    bool IsEmpty () const
    {
        return ActiveEffects.IsEmpty ();
    }

    void Reset ()
    {
        ActiveEffects.Reset ();
    }

    const FGridStatusEffectRuntimeState* FindByEffectId (FName EffectId) const;

    bool Contains (FName EffectId) const
    {
        return FindByEffectId (EffectId) != nullptr;
    }

    /**
     * Atomically creates and appends one runtime state from Definition.
     * DurationOverride == INDEX_NONE uses the definition default.
     */
    bool TryAdd (
        const UGridStatusEffectDefinitionAsset& Definition,
        const FGuid& SourceId,
        int32 InitialStackCount,
        int32 DurationOverride,
        FString& OutError);

    bool TryAdd (
        const UGridStatusEffectDefinitionAsset& Definition,
        const FGuid& SourceId,
        FString& OutError)
    {
        return TryAdd (
            Definition,
            SourceId,
            1,
            INDEX_NONE,
            OutError);
    }

private:
    void SortDeterministically ();
};
