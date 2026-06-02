#include "UI/GridPartyMemberWidget.h"

void UGridPartyMemberWidget::InitializePartyMember (int32 InCharacterIndex)
{
    CharacterIndex = InCharacterIndex;
}

void UGridPartyMemberWidget::SetCharacterSummary (const FGridInventoryCharacterSummary& InSummary)
{
    CachedSummary = InSummary;
    CharacterIndex = InSummary.CharacterIndex;
    RefreshMemberVisual ();
}

FString UGridPartyMemberWidget::GetDisplayNameText () const
{
    const FString NameText = CachedSummary.DisplayName.IsEmpty ()
        ? FString::Printf (TEXT ("Hero_%02d"), CharacterIndex + 1)
        : CachedSummary.DisplayName.ToString ();
    return FString::Printf (TEXT ("%d %s"), CharacterIndex, *NameText);
}

FString UGridPartyMemberWidget::GetClassLevelText () const
{
    const FString ClassText = CachedSummary.ClassId.IsNone ()
        ? FString (TEXT ("Unknown"))
        : CachedSummary.ClassId.ToString ();
    return FString::Printf (TEXT ("%s Lv%d"), *ClassText, CachedSummary.Level);
}

FString UGridPartyMemberWidget::GetWeightText () const
{
    return FString::Printf (
        TEXT ("%.1f / %.1f"),
        CachedSummary.CurrentWeight,
        CachedSummary.MaxWeight);
}

bool UGridPartyMemberWidget::IsSelected () const
{
    return CachedSummary.bIsSelected;
}

void UGridPartyMemberWidget::HandleClicked ()
{
    OnPartyMemberClicked.Broadcast (CharacterIndex);
}

void UGridPartyMemberWidget::RefreshMemberVisual_Implementation ()
{
}
