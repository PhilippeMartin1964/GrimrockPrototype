#include "UI/GridItemTooltipEquipmentComparisonWidget.h"

#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/Widget.h"
#include "UI/GridItemTooltipWeaponLineWidget.h"

bool UGridItemTooltipEquipmentComparisonWidget::ShouldOmitLegacyArmor(
    const FGridItemTooltipEquipmentComparison& Data,
    const FGridItemTooltipStatLine& Stat)
{
    if (Stat.StatId != TEXT("Armor"))
    {
        return false;
    }
    return Data.WeaponFacts.ContainsByPredicate(
        [](const FGridWeaponTooltipComparisonFact& Fact)
        {
            return Fact.StatId == TEXT("WeaponPhysicalArmor");
        });
}

bool UGridItemTooltipEquipmentComparisonWidget::PresentComparison(
    const FGridItemTooltipEquipmentComparison& Data,
    TSubclassOf<UGridItemTooltipWeaponLineWidget> LineClass)
{
    const bool bShowLegacy = Data.StatLines.ContainsByPredicate(
        [&Data](const FGridItemTooltipStatLine& Stat)
        {
            return !ShouldOmitLegacyArmor(Data, Stat);
        });
    const bool bShowWeapon = !Data.WeaponFacts.IsEmpty();
    if (!Text_EquipmentSlot || !Text_EquippedItem ||
        (bShowLegacy && !VerticalBox_ComparisonStats) ||
        (bShowWeapon && !VerticalBox_ComparisonWeaponFacts) ||
        ((bShowLegacy || bShowWeapon) && !LineClass))
    {
        return false;
    }
    Text_EquipmentSlot->SetText(Data.SlotLabel);
    Text_EquippedItem->SetText(Data.EquippedItemDisplayText);
    Text_EquippedItem->SetVisibility(Data.EquippedItemDisplayText.IsEmpty()
        ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);

    if (VerticalBox_ComparisonStats)
    {
        VerticalBox_ComparisonStats->ClearChildren();
    }
    if (VerticalBox_ComparisonWeaponFacts)
    {
        VerticalBox_ComparisonWeaponFacts->ClearChildren();
    }
    if (VerticalBox_ComparisonWeaponSection)
    {
        VerticalBox_ComparisonWeaponSection->SetVisibility(bShowWeapon
            ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    }
    if (bShowLegacy)
    {
        for (const FGridItemTooltipStatLine& Stat : Data.StatLines)
        {
            if (ShouldOmitLegacyArmor(Data, Stat))
            {
                continue;
            }
            UGridItemTooltipWeaponLineWidget* Line =
                CreateWidget<UGridItemTooltipWeaponLineWidget>(this, LineClass);
            if (!Line) return false;
            VerticalBox_ComparisonStats->AddChildToVerticalBox(Line);
            if (!Line->PresentLegacyStat(Stat, Data.bGlobalEquipmentScenario)) return false;
        }
    }
    if (bShowWeapon)
    {
        for (const FGridWeaponTooltipComparisonFact& Fact : Data.WeaponFacts)
        {
            UGridItemTooltipWeaponLineWidget* Line =
                CreateWidget<UGridItemTooltipWeaponLineWidget>(this, LineClass);
            if (!Line) return false;
            VerticalBox_ComparisonWeaponFacts->AddChildToVerticalBox(Line);
            if (!Line->PresentComparison(Fact, FText::GetEmpty())) return false;
        }
    }
    return true;
}
