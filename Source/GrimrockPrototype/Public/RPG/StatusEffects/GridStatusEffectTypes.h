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

UENUM (BlueprintType)
enum class EGridStatusEffectApplyOutcome : uint8
{
    None                UMETA (DisplayName = "None"),
    Added               UMETA (DisplayName = "Added"),
    RefreshedDuration   UMETA (DisplayName = "Refreshed Duration"),
    AddedStacks         UMETA (DisplayName = "Added Stacks"),
    ReplacedStronger    UMETA (DisplayName = "Replaced Stronger"),
    RejectedNoStack     UMETA (DisplayName = "Rejected No Stack"),
    RejectedNotStronger UMETA (DisplayName = "Rejected Not Stronger")
};

USTRUCT (BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridStatusEffectRuntimeState
{
    GENERATED_BODY ()

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects")
    FName EffectId = NAME_None;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects")
    FGuid SourceId;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects")
    int32 StackCount = 1;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects")
    EGridStatusEffectDurationUnit DurationUnit = EGridStatusEffectDurationUnit::Rounds;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects")
    int32 RemainingDuration = 0;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects")
    int32 Potency = 0;

    /**
     * Transient static-data authority used by MON16.3 periodic resolution.
     * EffectId remains the stable identity; this pointer is never a source id
     * and is not part of the MON16.7 persistence contract.
     */
    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Transient, Category = "RPG|Status Effects")
    TObjectPtr<UGridStatusEffectDefinitionAsset> DefinitionAsset = nullptr;

    bool IsValid () const
    {
        if (EffectId.IsNone () || StackCount < 1 || Potency < 0)
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

USTRUCT (BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridStatusEffectApplyResult
{
    GENERATED_BODY ()

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects")
    EGridStatusEffectApplyOutcome Outcome = EGridStatusEffectApplyOutcome::None;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects")
    FName EffectId = NAME_None;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects")
    int32 PreviousStackCount = 0;
    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects")
    int32 CurrentStackCount = 0;
    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects")
    int32 PreviousRemainingDuration = 0;
    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects")
    int32 CurrentRemainingDuration = 0;
    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects")
    int32 PreviousPotency = 0;
    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects")
    int32 CurrentPotency = 0;

    bool DidMutate () const
    {
        return Outcome == EGridStatusEffectApplyOutcome::Added ||
            Outcome == EGridStatusEffectApplyOutcome::RefreshedDuration ||
            Outcome == EGridStatusEffectApplyOutcome::AddedStacks ||
            Outcome == EGridStatusEffectApplyOutcome::ReplacedStronger;
    }

    void Reset () { *this = FGridStatusEffectApplyResult (); }
};

USTRUCT (BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridStatusEffectAdvanceResult
{
    GENERATED_BODY ()

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects")
    EGridStatusEffectDurationUnit DurationUnit = EGridStatusEffectDurationUnit::Rounds;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects")
    TArray<FName> AdvancedEffectIds;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects")
    TArray<FName> ExpiredEffectIds;

    bool HasChanges () const { return !AdvancedEffectIds.IsEmpty (); }

    void Reset (EGridStatusEffectDurationUnit InDurationUnit)
    {
        DurationUnit = InDurationUnit;
        AdvancedEffectIds.Reset ();
        ExpiredEffectIds.Reset ();
    }
};

USTRUCT (BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridStatusEffectCollection
{
    GENERATED_BODY ()

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "RPG|Status Effects")
    TArray<FGridStatusEffectRuntimeState> ActiveEffects;

    int32 Num () const { return ActiveEffects.Num (); }
    bool IsEmpty () const { return ActiveEffects.IsEmpty (); }
    void Reset () { ActiveEffects.Reset (); }

    const FGridStatusEffectRuntimeState* FindByEffectId (FName EffectId) const;
    bool Contains (FName EffectId) const { return FindByEffectId (EffectId) != nullptr; }

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
        return TryAdd (Definition, SourceId, 1, INDEX_NONE, OutError);
    }

    bool TryApply (
        const UGridStatusEffectDefinitionAsset& Definition,
        const FGuid& SourceId,
        int32 InitialStackCount,
        int32 DurationOverride,
        int32 PotencyOverride,
        FGridStatusEffectApplyResult& OutResult,
        FString& OutError);

    bool TryApply (
        const UGridStatusEffectDefinitionAsset& Definition,
        const FGuid& SourceId,
        FGridStatusEffectApplyResult& OutResult,
        FString& OutError)
    {
        return TryApply (
            Definition,
            SourceId,
            1,
            INDEX_NONE,
            INDEX_NONE,
            OutResult,
            OutError);
    }

    void AdvanceDuration (
        EGridStatusEffectDurationUnit DurationUnit,
        FGridStatusEffectAdvanceResult& OutResult);

private:
    void SortDeterministically ();
};
