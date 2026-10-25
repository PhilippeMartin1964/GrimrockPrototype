#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/GridInventoryUiTypes.h"
#include "GridItemTooltipWeaponLineWidget.generated.h"

class UTextBlock;

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
    bool PresentComparison(const FGridWeaponTooltipComparisonFact& Fact, const FText& SlotLabel);

    /** Native, purely semantic decision: no runtime tint overrides. */
    static EGridItemTooltipDeltaState VisibleDeltaState(const FGridWeaponTooltipComparisonFact& Fact);

protected:
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
};
