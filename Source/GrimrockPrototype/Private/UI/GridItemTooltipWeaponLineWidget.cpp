#include "UI/GridItemTooltipWeaponLineWidget.h"

#include "Components/TextBlock.h"

namespace
{
    void SetOptional(UTextBlock* Widget, const FText& Value)
    {
        if (Widget)
        {
            Widget->SetText(Value);
            Widget->SetVisibility(Value.IsEmpty()
                ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
        }
    }
}

EGridItemTooltipDeltaState UGridItemTooltipWeaponLineWidget::VisibleDeltaState(
    const FGridWeaponTooltipComparisonFact& Fact)
{
    return Fact.bHasDelta ? Fact.DeltaState : EGridItemTooltipDeltaState::Neutral;
}

void UGridItemTooltipWeaponLineWidget::HideComparisons()
{
    SetOptional(Text_Equipped, FText::GetEmpty());
    SetOptional(Text_Slot, FText::GetEmpty());
    SetOptional(Text_DeltaPositive, FText::GetEmpty());
    SetOptional(Text_DeltaNegative, FText::GetEmpty());
    SetOptional(Text_DeltaNeutral, FText::GetEmpty());
}

bool UGridItemTooltipWeaponLineWidget::PresentFact(const FGridWeaponTooltipFact& Fact)
{
    if (!Text_Label || !Text_Value)
    {
        return false;
    }
    SetOptional(Text_Label, Fact.Label);
    SetOptional(Text_Value, Fact.ValueText);
    HideComparisons();
    return true;
}

bool UGridItemTooltipWeaponLineWidget::PresentComparison(
    const FGridWeaponTooltipComparisonFact& Fact, const FText& SlotLabel)
{
    if (!Text_Label || !Text_Value || !Text_Equipped ||
        !Text_DeltaPositive || !Text_DeltaNegative || !Text_DeltaNeutral)
    {
        return false;
    }
    SetOptional(Text_Label, Fact.Label);
    SetOptional(Text_Value, Fact.CandidateText);
    SetOptional(Text_Equipped, Fact.EquippedText);
    SetOptional(Text_Slot, SlotLabel);
    HideComparisons();
    if (!Fact.bHasDelta)
    {
        return true; // NdM+K, damage type or scaling: never an invented color.
    }
    switch (VisibleDeltaState(Fact))
    {
        case EGridItemTooltipDeltaState::Positive:
            SetOptional(Text_DeltaPositive, Fact.DeltaText);
            break;
        case EGridItemTooltipDeltaState::Negative:
            SetOptional(Text_DeltaNegative, Fact.DeltaText);
            break;
        default:
            SetOptional(Text_DeltaNeutral, Fact.DeltaText);
            break;
    }
    return true;
}
