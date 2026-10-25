#include "UI/GridItemTooltipWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/Widget.h"
#include "UI/GridInventorySlotWidget.h"
#include "UI/GridItemTooltipWeaponLineWidget.h"
#include "UI/GridItemTooltipEquipmentComparisonWidget.h"
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

bool UGridItemTooltipWidget::ShouldOmitLegacyArmor(
    const FGridItemTooltipView& View,
    const FGridItemTooltipStatLine& Stat)
{
    if (Stat.StatId != TEXT("Armor") ||
        !View.WeaponProfile.bHasProfile || !View.WeaponProfile.bPassive)
    {
        return false;
    }
    return View.WeaponProfile.Facts.ContainsByPredicate(
        [](const FGridWeaponTooltipFact& Fact)
        {
            return Fact.StatId == TEXT("WeaponPhysicalArmor");
        });
}

bool UGridItemTooltipWidget::PresentTooltipView(const FGridItemTooltipView& View)
{
    // Fail closed before changing Designer properties if a required authored
    // section/class is missing. Never silently lose the old bonus rows.
    const bool bLegacy = View.StatLines.ContainsByPredicate(
        [&View](const FGridItemTooltipStatLine& Stat)
        {
            return !ShouldOmitLegacyArmor(View, Stat);
        });
    const bool bWeapons = View.WeaponProfile.bHasProfile &&
        !View.WeaponProfile.Facts.IsEmpty();
    const bool bComparisons = !View.EquipmentComparisons.IsEmpty();
    if (!View.bValid || !Text_ItemName || !Text_ItemType || !Text_Description ||
        !Text_Quantity || !Text_Weight || !Text_Equipment || !Image_ItemIcon ||
        (bLegacy && (!VerticalBox_StatLines || !WeaponLineWidgetClass)) ||
        (bWeapons && (!VerticalBox_WeaponSection ||
            !VerticalBox_WeaponFacts || !WeaponLineWidgetClass)) ||
        (bComparisons && (!VerticalBox_Comparisons ||
            !ComparisonRowWidgetClass || !WeaponLineWidgetClass)))
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
    if (!PopulateLegacyStats(View) ||
        !PopulateWeaponFacts(View) || !PopulateEquipmentComparisons(View))
    {
        return false;
    }
    return true;
}

bool UGridItemTooltipWidget::PopulateLegacyStats(const FGridItemTooltipView& View)
{
    const bool bShow = View.StatLines.ContainsByPredicate(
        [&View](const FGridItemTooltipStatLine& Stat)
        {
            return !ShouldOmitLegacyArmor(View, Stat);
        });
    SetSection(VerticalBox_StatsSection, bShow);
    if (!VerticalBox_StatLines)
    {
        return !bShow;
    }
    VerticalBox_StatLines->ClearChildren();
    if (!bShow) return true;

    for (const FGridItemTooltipStatLine& Stat : View.StatLines)
    {
        if (ShouldOmitLegacyArmor(View, Stat))
        {
            continue;
        }
        UGridItemTooltipWeaponLineWidget* Line =
            CreateWidget<UGridItemTooltipWeaponLineWidget>(this, WeaponLineWidgetClass);
        if (!Line) return false;
        VerticalBox_StatLines->AddChildToVerticalBox(Line);
        if (!Line->PresentLegacyStat(Stat)) return false;
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

bool UGridItemTooltipWidget::PopulateEquipmentComparisons(
    const FGridItemTooltipView& View)
{
    const bool bShow = !View.EquipmentComparisons.IsEmpty();
    SetSection(VerticalBox_ComparisonSection, bShow);
    // 04.2B replaces the detached weapon-comparison root with one group per
    // equipped slot. Never present the same profile twice.
    SetSection(VerticalBox_WeaponComparisonSection, false);
    if (VerticalBox_WeaponComparisons)
    {
        VerticalBox_WeaponComparisons->ClearChildren();
    }
    if (!VerticalBox_Comparisons)
    {
        return !bShow;
    }
    VerticalBox_Comparisons->ClearChildren();
    if (!bShow) return true;

    for (const FGridItemTooltipEquipmentComparison& Comparison : View.EquipmentComparisons)
    {
        UGridItemTooltipEquipmentComparisonWidget* Group =
            CreateWidget<UGridItemTooltipEquipmentComparisonWidget>(
                this, ComparisonRowWidgetClass);
        if (!Group) return false;
        VerticalBox_Comparisons->AddChildToVerticalBox(Group);
        if (!Group->PresentComparison(Comparison, WeaponLineWidgetClass)) return false;
    }
    return true;
}
