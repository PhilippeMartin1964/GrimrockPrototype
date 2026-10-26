#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/GridInventoryUiTypes.h"
#include "GridItemTooltipWidget.generated.h"

class UGridInventorySlotWidget;
class UGridItemTooltipWeaponLineWidget;
class UGridItemTooltipEquipmentComparisonWidget;
class UImage;
class UTextBlock;
class UVerticalBox;
class UWidget;

/**
 * Native inventory-tooltip presenter: same separation as CombatTooltip.
 * The existing WBP_ItemTooltip remains untouched until the supervised 04.2B reparent.
 * This class never creates individual TextBlocks, colors, icons or layout.
 */
UCLASS(BlueprintType, Blueprintable)
class GRIMROCKPROTOTYPE_API UGridItemTooltipWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** Different name from legacy BP InitFromInventorySlot; safe during transition. */
    UFUNCTION(BlueprintCallable, Category="Inventory|Tooltip")
    bool PresentFromInventorySlot(UGridInventorySlotWidget* InInventorySlot);

    UFUNCTION(BlueprintCallable, Category="Inventory|Tooltip")
    bool PresentTooltipView(const FGridItemTooltipView& View);

    /** Stable localization format; the source of truth is UI.csv -> ST_UI. */
    static FText FormatItemLevel(int32 Level);

    /** A catalogued shield's true level armor wins over authored generic ArmorBonus. */
    static bool ShouldOmitLegacyArmor(
        const FGridItemTooltipView& View,
        const FGridItemTooltipStatLine& Stat);

protected:
    /** Assign a Designer-authored child WBP class after the reparent in 04.2B. */
    UPROPERTY(EditDefaultsOnly, Category="Inventory|Tooltip|Designer")
    TSubclassOf<UGridItemTooltipWeaponLineWidget> WeaponLineWidgetClass;

    /** Designer-authored per-equipment-slot comparison group. */
    UPROPERTY(EditDefaultsOnly, Category="Inventory|Tooltip|Designer")
    TSubclassOf<UGridItemTooltipEquipmentComparisonWidget> ComparisonRowWidgetClass;

    UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly, Category="Inventory|Tooltip|Designer")
    TObjectPtr<UTextBlock> Text_ItemName;
    UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly, Category="Inventory|Tooltip|Designer")
    TObjectPtr<UTextBlock> Text_ItemType;
    UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly, Category="Inventory|Tooltip|Designer")
    TObjectPtr<UTextBlock> Text_Description;
    UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly, Category="Inventory|Tooltip|Designer")
    TObjectPtr<UTextBlock> Text_Quantity;
    UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly, Category="Inventory|Tooltip|Designer")
    TObjectPtr<UTextBlock> Text_Weight;
    UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly, Category="Inventory|Tooltip|Designer")
    TObjectPtr<UTextBlock> Text_Equipment;
    UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly, Category="Inventory|Tooltip|Designer")
    TObjectPtr<UTextBlock> Text_Light;
    UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly, Category="Inventory|Tooltip|Designer")
    TObjectPtr<UTextBlock> Text_ItemLevel;
    UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly, Category="Inventory|Tooltip|Designer")
    TObjectPtr<UImage> Image_ItemIcon;

    /** Text_WeaponTitle remains STATIC and references ST_UI directly in Designer. */
    /** Structured stat and per-slot comparison containers still used by native rows. */
    UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly, Category="Inventory|Tooltip|Designer")
    TObjectPtr<UVerticalBox> VerticalBox_StatLines;

    UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly, Category="Inventory|Tooltip|Designer")
    TObjectPtr<UVerticalBox> VerticalBox_Comparisons;

    UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly, Category="Inventory|Tooltip|Designer")
    TObjectPtr<UWidget> VerticalBox_StatsSection;

    UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly, Category="Inventory|Tooltip|Designer")
    TObjectPtr<UWidget> VerticalBox_ComparisonSection;

    UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly, Category="Inventory|Tooltip|Designer")
    TObjectPtr<UWidget> VerticalBox_WeaponSection;
    UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly, Category="Inventory|Tooltip|Designer")
    TObjectPtr<UVerticalBox> VerticalBox_WeaponFacts;

private:
    bool PopulateLegacyStats(const FGridItemTooltipView& View);
    bool PopulateWeaponFacts(const FGridItemTooltipView& View);
    bool PopulateEquipmentComparisons(const FGridItemTooltipView& View);
};
