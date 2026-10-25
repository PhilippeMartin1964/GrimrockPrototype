#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/GridInventoryUiTypes.h"
#include "GridItemTooltipEquipmentComparisonWidget.generated.h"

class UGridItemTooltipWeaponLineWidget;
class UTextBlock;
class UVerticalBox;
class UWidget;

/**
 * Designer-owned equipment comparison group: one equipment slot, including
 * existing bonus/resistance deltas AND catalogued weapon facts.
 * No nested Blueprint loops, no visual styling changes from C++.
 */
UCLASS(BlueprintType, Blueprintable)
class GRIMROCKPROTOTYPE_API UGridItemTooltipEquipmentComparisonWidget
    : public UUserWidget
{
    GENERATED_BODY()

public:
    bool PresentComparison(
        const FGridItemTooltipEquipmentComparison& Data,
        TSubclassOf<UGridItemTooltipWeaponLineWidget> LineClass);

    /** Shield profile armor is authoritative, not definition-level ArmorBonus. */
    static bool ShouldOmitLegacyArmor(
        const FGridItemTooltipEquipmentComparison& Data,
        const FGridItemTooltipStatLine& Stat);

protected:
    UPROPERTY(meta=(BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_EquipmentSlot;

    UPROPERTY(meta=(BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_EquippedItem;

    UPROPERTY(meta=(BindWidgetOptional))
    TObjectPtr<UVerticalBox> VerticalBox_ComparisonStats;

    UPROPERTY(meta=(BindWidgetOptional))
    TObjectPtr<UWidget> VerticalBox_ComparisonWeaponSection;

    UPROPERTY(meta=(BindWidgetOptional))
    TObjectPtr<UVerticalBox> VerticalBox_ComparisonWeaponFacts;
};
