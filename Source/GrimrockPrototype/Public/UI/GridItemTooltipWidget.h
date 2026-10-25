#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/GridInventoryUiTypes.h"
#include "GridItemTooltipWidget.generated.h"

class UGridInventorySlotWidget;
class UGridItemTooltipWeaponLineWidget;
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

protected:
    /** Assign a Designer-authored child WBP class after the reparent in 04.2B. */
    UPROPERTY(EditDefaultsOnly, Category="Inventory|Tooltip|Designer")
    TSubclassOf<UGridItemTooltipWeaponLineWidget> WeaponLineWidgetClass;

    UPROPERTY(meta=(BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_ItemName;
    UPROPERTY(meta=(BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_ItemType;
    UPROPERTY(meta=(BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_Description;
    UPROPERTY(meta=(BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_Quantity;
    UPROPERTY(meta=(BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_Weight;
    UPROPERTY(meta=(BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_Equipment;
    UPROPERTY(meta=(BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_Light;
    UPROPERTY(meta=(BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_ItemLevel;
    UPROPERTY(meta=(BindWidgetOptional))
    TObjectPtr<UImage> Image_ItemIcon;

    /** Text_WeaponTitle remains STATIC and references ST_UI directly in Designer. */
    UPROPERTY(meta=(BindWidgetOptional))
    TObjectPtr<UWidget> VerticalBox_WeaponSection;
    UPROPERTY(meta=(BindWidgetOptional))
    TObjectPtr<UVerticalBox> VerticalBox_WeaponFacts;
    UPROPERTY(meta=(BindWidgetOptional))
    TObjectPtr<UWidget> VerticalBox_WeaponComparisonSection;
    UPROPERTY(meta=(BindWidgetOptional))
    TObjectPtr<UVerticalBox> VerticalBox_WeaponComparisons;

private:
    static bool HasWeaponComparisons(const FGridItemTooltipView& View);
    bool PopulateWeaponFacts(const FGridItemTooltipView& View);
    bool PopulateWeaponComparisons(const FGridItemTooltipView& View);
};
