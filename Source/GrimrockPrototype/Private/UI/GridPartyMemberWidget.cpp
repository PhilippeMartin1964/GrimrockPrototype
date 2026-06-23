#include "UI/GridPartyMemberWidget.h"

#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "RPG/RPGClassVisualAsset.h"

void UGridPartyMemberWidget::InitializePartyMember (int32 InCharacterIndex)
{
    CharacterIndex = InCharacterIndex;
}

void UGridPartyMemberWidget::SetCharacterSummary (const FGridInventoryCharacterSummary& InSummary)
{
    CachedSummary = InSummary;
    CharacterIndex = InSummary.CharacterIndex;
    RefreshBoundMemberFields ();
    RefreshBoundMemberVisuals ();
    RefreshMemberVisual ();
}

void UGridPartyMemberWidget::SetAvailableClassVisuals (
    const TArray<URPGClassVisualAsset*>& InAvailableClassVisuals)
{
    AvailableClassVisuals.Reset ();
    for (URPGClassVisualAsset* ClassVisual : InAvailableClassVisuals)
    {
        if (ClassVisual)
        {
            AvailableClassVisuals.Add (ClassVisual);
        }
    }
    RefreshBoundMemberVisuals ();
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

const URPGClassVisualAsset* UGridPartyMemberWidget::FindClassVisualForCachedClass () const
{
    if (CachedSummary.ClassId.IsNone ())
    {
        return nullptr;
    }

    for (const URPGClassVisualAsset* ClassVisual : AvailableClassVisuals)
    {
        if (ClassVisual && ClassVisual->IsValidForClass (CachedSummary.ClassId))
        {
            return ClassVisual;
        }
    }

    return nullptr;
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

void UGridPartyMemberWidget::RefreshBoundMemberVisuals ()
{
    const URPGClassVisualAsset* ClassVisual = FindClassVisualForCachedClass ();
    const TSoftObjectPtr<UTexture2D> ClassIcon = ClassVisual && !ClassVisual->ClassIcon.IsNull ()
        ? ClassVisual->ClassIcon
        : CachedSummary.ClassIcon;

    if (Image_ClassIcon)
    {
        if (ClassIcon.IsNull ())
        {
            Image_ClassIcon->SetVisibility (ESlateVisibility::Collapsed);
        }
        else
        {
            Image_ClassIcon->SetBrushFromSoftTexture (ClassIcon, false);
            Image_ClassIcon->SetVisibility (ESlateVisibility::HitTestInvisible);
        }
    }

    if (Border_ClassAccent)
    {
        if (!ClassVisual)
        {
            Border_ClassAccent->SetVisibility (ESlateVisibility::Collapsed);
        }
        else
        {
            Border_ClassAccent->SetBrushColor (ClassVisual->AccentColor);
            Border_ClassAccent->SetVisibility (ESlateVisibility::HitTestInvisible);
        }
    }
}
