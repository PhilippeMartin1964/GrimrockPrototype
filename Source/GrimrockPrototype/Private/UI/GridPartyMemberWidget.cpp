#include "UI/GridPartyMemberWidget.h"

#include "Components/TextBlock.h"

void UGridPartyMemberWidget::InitializePartyMember (int32 InCharacterIndex)
{
    CharacterIndex = InCharacterIndex;
}

void UGridPartyMemberWidget::SetCharacterSummary (const FGridInventoryCharacterSummary& InSummary)
{
    CachedSummary = InSummary;
    CharacterIndex = InSummary.CharacterIndex;
    RefreshBoundMemberFields ();
    RefreshMemberVisual ();
}

FString UGridPartyMemberWidget::GetDisplayNameText () const
{
    const FString NameText = CachedSummary.DisplayName.IsEmpty ()
        ? FString::Printf (TEXT ("Hero_%02d"), CharacterIndex + 1)
        : CachedSummary.DisplayName.ToString ();
    return NameText;
}

FString UGridPartyMemberWidget::GetClassLevelText () const
{
    const FString ClassText = CachedSummary.ClassDisplayName.IsEmpty ()
        ? (CachedSummary.ClassId.IsNone () ? FString (TEXT ("Classe inconnue")) : CachedSummary.ClassId.ToString ())
        : CachedSummary.ClassDisplayName.ToString ();
    return FString::Printf (TEXT ("%s - Niv. %d"), *ClassText, CachedSummary.Level);
}

FString UGridPartyMemberWidget::GetWeightText () const
{
    return FString::Printf (
        TEXT ("Charge %.1f / %.1f"),
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

void UGridPartyMemberWidget::RefreshBoundMemberFields ()
{
    if (Text_Name)
    {
        Text_Name->SetText (FText::FromString (GetDisplayNameText ()));
    }
    if (Text_ClassLevel)
    {
        Text_ClassLevel->SetText (FText::FromString (GetClassLevelText ()));
    }
    if (Text_Weight)
    {
        Text_Weight->SetText (FText::FromString (GetWeightText ()));
    }
}
