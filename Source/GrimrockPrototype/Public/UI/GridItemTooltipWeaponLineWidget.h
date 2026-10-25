#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/GridInventoryUiTypes.h"
#include "GridItemTooltipWeaponLineWidget.generated.h"

class UTextBlock;
class UImage;
class UTexture2D;
class USizeBox;

/**
 * One Designer-authored weapon fact/comparison line.
 * C++ owns data and visibility; TextBlock style, color, font and layout stay in UMG.
 */
UCLASS(BlueprintType, Blueprintable)
class GRIMROCKPROTOTYPE_API UGridItemTooltipWeaponLineWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    bool PresentFact(const FGridWeaponTooltipFact& Fact);
    /** Legacy stat/resistance with semantic delta and Designer-owned icon. */
    bool PresentLegacyStat(const FGridItemTooltipStatLine& Stat);
    bool PresentComparison(const FGridWeaponTooltipComparisonFact& Fact, const FText& SlotLabel);

    /** Native, purely semantic decision: no runtime tint overrides. */
    static EGridItemTooltipDeltaState VisibleDeltaState(const FGridWeaponTooltipComparisonFact& Fact);

protected:
    /** Stable StatId -> texture mapping, edited in the line WBP's Class Defaults. */
    UPROPERTY(EditDefaultsOnly, Category="Inventory|Tooltip|Designer")
    TMap<FName, TObjectPtr<UTexture2D>> StatIconTextures;

    /** Optional 48x48 icon, dimensions controlled only by the Designer. */
    /** The 48x48 slot is collapsed when no icon exists, keeping text-only rows compact. */
    UPROPERTY(meta=(BindWidgetOptional))
    TObjectPtr<USizeBox> SizeBox_StatIcon;

    UPROPERTY(meta=(BindWidgetOptional))
    TObjectPtr<UImage> Image_StatIcon;

    UPROPERTY(meta=(BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_Label;

    UPROPERTY(meta=(BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_Value;

    UPROPERTY(meta=(BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_Equipped;

    UPROPERTY(meta=(BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_Slot;

    /** Three independently styled Designer TextBlocks; exactly one can be shown. */
    UPROPERTY(meta=(BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_DeltaPositive;

    UPROPERTY(meta=(BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_DeltaNegative;

    UPROPERTY(meta=(BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_DeltaNeutral;

private:
    void HideComparisons();
    void PresentIcon(FName StatId);
    void PresentDelta(bool bHasDelta, EGridItemTooltipDeltaState State, const FText& Text);
};
