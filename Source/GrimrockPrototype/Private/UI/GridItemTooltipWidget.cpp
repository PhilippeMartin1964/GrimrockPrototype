#include "UI/GridItemTooltipWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/Widget.h"
#include "UI/GridInventorySlotWidget.h"
#include "UI/GridItemTooltipWeaponLineWidget.h"
#include "UI/GridItemTooltipEquipmentComparisonWidget.h"
#include "UI/UILocalizationText.h"

DEFINE_LOG_CATEGORY_STATIC(LogGridItemTooltip, Log, All);

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
    if (!IsValid(InInventorySlot))
    {
        UE_LOG(LogGridItemTooltip, Warning,
            TEXT("PresentFromInventorySlot failed: inventory slot is null/invalid (widget=%s)."),
            *GetNameSafe(this));
        return false;
    }
    return PresentTooltipView(InInventorySlot->GetTooltipView());
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
    // A missing optional UMG binding used to cause a completely empty
    // frame with no actionable error after reparenting WBP_ItemTooltip.
    if (!View.bValid)
    {
        UE_LOG(LogGridItemTooltip, Warning,
            TEXT("Tooltip presentation refused: invalid item view (WBP class=%s)."),
            *GetNameSafe(GetClass()));
        return false;
    }
    const bool bLegacy = View.StatLines.ContainsByPredicate(
        [&View](const FGridItemTooltipStatLine& Stat)
        {
            return !ShouldOmitLegacyArmor(View, Stat);
        });
    const bool bWeapons = View.WeaponProfile.bHasProfile &&
        !View.WeaponProfile.Facts.IsEmpty();
    const bool bComparisons = !View.EquipmentComparisons.IsEmpty();

    TArray<FString> Missing;
    if (!Text_ItemName) Missing.Add(TEXT("Text_ItemName"));
    if (!Text_ItemType) Missing.Add(TEXT("Text_ItemType"));
    if (!Text_Description) Missing.Add(TEXT("Text_Description"));
    if (!Text_Quantity) Missing.Add(TEXT("Text_Quantity"));
    if (!Text_Weight) Missing.Add(TEXT("Text_Weight"));
    if (!Text_Equipment) Missing.Add(TEXT("Text_Equipment"));
    if (!Image_ItemIcon) Missing.Add(TEXT("Image_ItemIcon"));
    if (bLegacy && !VerticalBox_StatLines)
        Missing.Add(TEXT("VerticalBox_StatLines"));
    if (bWeapons && !VerticalBox_WeaponSection)
        Missing.Add(TEXT("VerticalBox_WeaponSection"));
    if (bWeapons && !VerticalBox_WeaponFacts)
        Missing.Add(TEXT("VerticalBox_WeaponFacts"));
    if (bComparisons && !VerticalBox_Comparisons)
        Missing.Add(TEXT("VerticalBox_Comparisons"));
    if ((bLegacy || bWeapons || bComparisons) && !WeaponLineWidgetClass)
        Missing.Add(TEXT("Class Defaults > WeaponLineWidgetClass"));
    if (bComparisons && !ComparisonRowWidgetClass)
        Missing.Add(TEXT("Class Defaults > ComparisonRowWidgetClass"));
    if (!Missing.IsEmpty())
    {
        UE_LOG(LogGridItemTooltip, Error,
            TEXT("Tooltip presenter refused for %s: missing [%s]. View: legacy=%d, weapon=%d, comparisons=%d. Check Is Variable, widget names, and Class Defaults."),
            *GetNameSafe(GetClass()), *FString::Join(Missing, TEXT(", ")),
            bLegacy ? 1 : 0, bWeapons ? 1 : 0, bComparisons ? 1 : 0);
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
    if (!PopulateLegacyStats(View))
    {
        UE_LOG(LogGridItemTooltip, Error,
            TEXT("Tooltip row creation failed at PopulateLegacyStats (WBP=%s). Check Text_Label/Text_Value/Delta TextBlocks and Is Variable on WBP_ItemTooltipNativeLine."),
            *GetNameSafe(GetClass()));
        return false;
    }
    if (!PopulateWeaponFacts(View))
    {
        UE_LOG(LogGridItemTooltip, Error,
            TEXT("Tooltip row creation failed at PopulateWeaponFacts (WBP=%s). Check WeaponLineWidgetClass and the native line widget bindings."),
            *GetNameSafe(GetClass()));
        return false;
    }
    if (!PopulateEquipmentComparisons(View))
    {
        UE_LOG(LogGridItemTooltip, Error,
            TEXT("Tooltip row creation failed at PopulateEquipmentComparisons (WBP=%s). Check WBP_ItemTooltipNativeComparison bindings/parent and nested line widget."),
            *GetNameSafe(GetClass()));
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
