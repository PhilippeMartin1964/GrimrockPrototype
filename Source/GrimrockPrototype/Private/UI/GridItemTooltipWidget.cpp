#include "UI/GridItemTooltipWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/Widget.h"
#include "UI/GridInventorySlotWidget.h"
#include "UI/GridItemTooltipWeaponLineWidget.h"
#include "UI/UILocalizationText.h"

namespace
{
    void SetDynamicText(UTextBlock* TextBlock, const FText& Text)
    {
        if (TextBlock)
        {
            TextBlock->SetText(Text);
            TextBlock->SetVisibility(Text.IsEmpty()
                ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
        }
    }

    void SetSection(UWidget* Section, bool bVisible)
    {
        if (Section)
        {
            Section->SetVisibility(bVisible
                ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
        }
    }
}

FText UGridItemTooltipWidget::FormatItemLevel(int32 Level)
{
    if (Level < 1 || Level > 20)
    {
        return FText::GetEmpty();
    }
    FFormatNamedArguments Args;
    Args.Add(TEXT("Level"), FText::AsNumber(Level));
    return FText::Format(
        FUILocalizationText::Get(TEXT("Inventory.Tooltip.ItemLevelFormat")), Args);
}

bool UGridItemTooltipWidget::PresentFromInventorySlot(UGridInventorySlotWidget* InInventorySlot)
{
    return IsValid(InInventorySlot) && PresentTooltipView(InInventorySlot->GetTooltipView());
}

bool UGridItemTooltipWidget::HasWeaponComparisons(const FGridItemTooltipView& View)
{
    for (const FGridItemTooltipEquipmentComparison& Comp : View.EquipmentComparisons)
    {
        if (!Comp.WeaponFacts.IsEmpty())
        {
            return true;
        }
    }
    return false;
}

bool UGridItemTooltipWidget::PresentTooltipView(const FGridItemTooltipView& View)
{
    // Do not partially draw a UI which cannot show its catalogue values.
    if (!View.bValid || !Text_ItemName || !Text_ItemType || !Text_Description ||
        !Text_Quantity || !Text_Weight || !Text_Equipment || !Image_ItemIcon ||
        (View.WeaponProfile.bHasProfile && !View.WeaponProfile.Facts.IsEmpty() &&
            (!VerticalBox_WeaponSection || !VerticalBox_WeaponFacts || !WeaponLineWidgetClass)) ||
        (HasWeaponComparisons(View) &&
            (!VerticalBox_WeaponComparisonSection || !VerticalBox_WeaponComparisons ||
                !WeaponLineWidgetClass)))
    {
        return false;
    }
    SetDynamicText(Text_ItemName, View.DisplayName);
    SetDynamicText(Text_ItemType, View.ItemType);
    SetDynamicText(Text_Description, View.Description);
    SetDynamicText(Text_Quantity, View.QuantityText);
    SetDynamicText(Text_Weight, View.WeightText);
    SetDynamicText(Text_Equipment, View.CompatibleSlotsText);
    SetDynamicText(Text_Light, View.LightStateText);
    SetDynamicText(Text_ItemLevel, View.bHasItemLevel
        ? FormatItemLevel(View.ItemLevel) : FText::GetEmpty());
    if (View.Icon)
    {
        Image_ItemIcon->SetBrushFromTexture(View.Icon, false);
        Image_ItemIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
    }
    else
    {
        Image_ItemIcon->SetVisibility(ESlateVisibility::Collapsed);
    }
    // The current legacy StatLines/Comparison BP presentation is NOT removed in 04.2A.
    // 04.2B will remove those BP loops only after the native replacements work in PIE.
    if (!PopulateWeaponFacts(View) || !PopulateWeaponComparisons(View))
    {
        return false;
    }
    return true;
}

bool UGridItemTooltipWidget::PopulateWeaponFacts(const FGridItemTooltipView& View)
{
    const bool bShow = View.WeaponProfile.bHasProfile &&
        !View.WeaponProfile.Facts.IsEmpty();
    SetSection(VerticalBox_WeaponSection, bShow);
    if (!VerticalBox_WeaponFacts)
    {
        return !bShow;
    }
    VerticalBox_WeaponFacts->ClearChildren();
    if (!bShow) return true;

    for (const FGridWeaponTooltipFact& Fact : View.WeaponProfile.Facts)
    {
        UGridItemTooltipWeaponLineWidget* Line =
            CreateWidget<UGridItemTooltipWeaponLineWidget>(this, WeaponLineWidgetClass);
        if (!Line) return false;
        VerticalBox_WeaponFacts->AddChildToVerticalBox(Line);
        // Initialize AFTER AddChild, so designer/BP Construct cannot overwrite it.
        if (!Line->PresentFact(Fact)) return false;
    }
    return true;
}

bool UGridItemTooltipWidget::PopulateWeaponComparisons(const FGridItemTooltipView& View)
{
    const bool bShow = HasWeaponComparisons(View);
    SetSection(VerticalBox_WeaponComparisonSection, bShow);
    if (!VerticalBox_WeaponComparisons)
    {
        return !bShow;
    }
    VerticalBox_WeaponComparisons->ClearChildren();
    if (!bShow) return true;

    for (const FGridItemTooltipEquipmentComparison& Comparison : View.EquipmentComparisons)
    {
        for (const FGridWeaponTooltipComparisonFact& Fact : Comparison.WeaponFacts)
        {
            UGridItemTooltipWeaponLineWidget* Line =
                CreateWidget<UGridItemTooltipWeaponLineWidget>(this, WeaponLineWidgetClass);
            if (!Line) return false;
            VerticalBox_WeaponComparisons->AddChildToVerticalBox(Line);
            if (!Line->PresentComparison(Fact, Comparison.SlotLabel)) return false;
        }
    }
    return true;
}
