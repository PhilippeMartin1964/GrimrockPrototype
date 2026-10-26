#include "UI/GridItemTooltipWeaponLineWidget.h"

#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Engine/Texture2D.h"

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

FName UGridItemTooltipWeaponLineWidget::GetFallbackIconStatId(FName StatId)
{
    // The global preview reports effective remaining armor rather than the
    // historical nominal Armor stat. Both use the same physical-armor icon.
    if (StatId == TEXT("CurrentPhysicalArmor") ||
        StatId == TEXT("WeaponPhysicalArmor"))
    {
        return TEXT("Armor");
    }
    return NAME_None;
}

void UGridItemTooltipWeaponLineWidget::PresentIcon(FName StatId)
{
    const TObjectPtr<UTexture2D>* Texture = StatIconTextures.Find(StatId);
    if (!Texture || !IsValid(Texture->Get()))
    {
        const FName FallbackId = GetFallbackIconStatId(StatId);
        if (!FallbackId.IsNone())
        {
            Texture = StatIconTextures.Find(FallbackId);
        }
    }
    const bool bHasIcon = Image_StatIcon && Texture && IsValid(Texture->Get());
    // Collapse the enclosing 48x48 slot, not just the image; otherwise
    // text-only weapon facts stretch the tooltip to many screen heights.
    if (SizeBox_StatIcon)
    {
        SizeBox_StatIcon->SetVisibility(bHasIcon
            ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    }
    if (!Image_StatIcon)
    {
        return;
    }
    if (bHasIcon)
    {
        Image_StatIcon->SetBrushFromTexture(Texture->Get(), false);
        Image_StatIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
    }
    else
    {
        Image_StatIcon->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void UGridItemTooltipWeaponLineWidget::PresentDelta(
    bool bHasDelta, EGridItemTooltipDeltaState State, const FText& Text)
{
    SetOptional(Text_DeltaPositive, FText::GetEmpty());
    SetOptional(Text_DeltaNegative, FText::GetEmpty());
    SetOptional(Text_DeltaNeutral, FText::GetEmpty());
    if (!bHasDelta)
    {
        return;
    }
    switch (State)
    {
        case EGridItemTooltipDeltaState::Positive:
            SetOptional(Text_DeltaPositive, Text);
            break;
        case EGridItemTooltipDeltaState::Negative:
            SetOptional(Text_DeltaNegative, Text);
            break;
        default:
            SetOptional(Text_DeltaNeutral, Text);
            break;
    }
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
    PresentIcon(Fact.StatId);
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
    HideComparisons();
    SetOptional(Text_Label, Fact.Label);
    SetOptional(Text_Value, Fact.CandidateText);
    SetOptional(Text_Equipped, Fact.EquippedText);
    SetOptional(Text_Slot, SlotLabel);
    PresentIcon(Fact.StatId);
    PresentDelta(Fact.bHasDelta, VisibleDeltaState(Fact), Fact.DeltaText);
    return true;
}

bool UGridItemTooltipWeaponLineWidget::PresentLegacyStat(
    const FGridItemTooltipStatLine& Stat, bool bShowBeforeValue)
{
    if (!Text_Label || !Text_Value ||
        !Text_DeltaPositive || !Text_DeltaNegative || !Text_DeltaNeutral)
    {
        return false;
    }
    SetOptional(Text_Label, Stat.Label);
    HideComparisons();
    SetOptional(Text_Value, Stat.ValueText);
    // Only global before/after scenarios use the Equipped column for the
    // character's BEFORE value. Legacy single-item rows remain unchanged.
    if (bShowBeforeValue && Stat.bHasComparison)
    {
        SetOptional(Text_Equipped, FText::AsNumber(Stat.EquippedValue));
    }
    PresentIcon(Stat.StatId);
    PresentDelta(Stat.bHasComparison, Stat.DeltaState, Stat.DeltaDisplayText);
    return true;
}
