#pragma once

#include "CoreMinimal.h"
#include "RPG/StatusEffects/GridStatusEffectTypes.h"
#include "GridStatusEffectPresentation.generated.h"

class UTexture2D;

/** Read-only MON16.6 projection of one authoritative runtime status. */
USTRUCT (BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridStatusEffectPresentationView
{
    GENERATED_BODY ()

    UPROPERTY (BlueprintReadOnly, Category = "RPG|Status Effects|Presentation")
    FName EffectId = NAME_None;

    UPROPERTY (BlueprintReadOnly, Category = "RPG|Status Effects|Presentation")
    FText DisplayName;

    UPROPERTY (BlueprintReadOnly, Category = "RPG|Status Effects|Presentation")
    FText Description;

    UPROPERTY (BlueprintReadOnly, Category = "RPG|Status Effects|Presentation")
    TSoftObjectPtr<UTexture2D> Icon;

    UPROPERTY (BlueprintReadOnly, Category = "RPG|Status Effects|Presentation")
    EGridStatusEffectDisposition Disposition = EGridStatusEffectDisposition::Neutral;

    UPROPERTY (BlueprintReadOnly, Category = "RPG|Status Effects|Presentation")
    EGridStatusEffectDurationUnit DurationUnit = EGridStatusEffectDurationUnit::Rounds;

    UPROPERTY (BlueprintReadOnly, Category = "RPG|Status Effects|Presentation")
    int32 RemainingDuration = 0;

    UPROPERTY (BlueprintReadOnly, Category = "RPG|Status Effects|Presentation")
    int32 StackCount = 1;

    UPROPERTY (BlueprintReadOnly, Category = "RPG|Status Effects|Presentation")
    int32 Potency = 0;

    UPROPERTY (BlueprintReadOnly, Category = "RPG|Status Effects|Presentation")
    int32 InitiativeContribution = 0;

    UPROPERTY (BlueprintReadOnly, Category = "RPG|Status Effects|Presentation")
    bool bPeriodicDamage = false;

    UPROPERTY (BlueprintReadOnly, Category = "RPG|Status Effects|Presentation")
    bool bSkipActivation = false;

    UPROPERTY (BlueprintReadOnly, Category = "RPG|Status Effects|Presentation")
    bool bBlockSpellActions = false;

    UPROPERTY (BlueprintReadOnly, Category = "RPG|Status Effects|Presentation")
    bool bBlockTranslation = false;

    UPROPERTY (BlueprintReadOnly, Category = "RPG|Status Effects|Presentation")
    FText CompactDurationText;

    UPROPERTY (BlueprintReadOnly, Category = "RPG|Status Effects|Presentation")
    FText DurationText;

    UPROPERTY (BlueprintReadOnly, Category = "RPG|Status Effects|Presentation")
    FText LabelText;

    UPROPERTY (BlueprintReadOnly, Category = "RPG|Status Effects|Presentation")
    FText ToolTipText;
};

/** Pure formatting/projection helpers. No widget, actor or gameplay mutation. */
class GRIMROCKPROTOTYPE_API FGridStatusEffectPresentationBuilder
{
public:
    static bool BuildOne (
        const FGridStatusEffectRuntimeState& State,
        FGridStatusEffectPresentationView& OutView);

    static void Build (
        const FGridStatusEffectCollection& StatusEffects,
        TArray<FGridStatusEffectPresentationView>& OutViews);

    static FText FormatCompactDuration (
        EGridStatusEffectDurationUnit DurationUnit,
        int32 RemainingDuration);

    static FText FormatDuration (
        EGridStatusEffectDurationUnit DurationUnit,
        int32 RemainingDuration);

    static FText BuildSummary (
        const TArray<FGridStatusEffectPresentationView>& Views);
};
