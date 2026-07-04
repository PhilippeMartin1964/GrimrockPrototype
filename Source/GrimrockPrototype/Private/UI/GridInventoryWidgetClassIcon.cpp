#include "UI/GridInventoryWidget.h"

#include "Components/Border.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "RPG/RPGClassVisualAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"

void UGridInventoryWidget::NativeTick (const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick (MyGeometry, InDeltaTime);
    RefreshSelectedCharacterClassIcon ();
    BuildPaperDollEquipmentPanel ();
}

const URPGClassVisualAsset* UGridInventoryWidget::FindClassVisualForClass (FName ClassId) const
{
    if (ClassId.IsNone ())
    {
        return nullptr;
    }

    for (const URPGClassVisualAsset* ClassVisual : AvailableClassVisuals)
    {
        if (ClassVisual && ClassVisual->IsValidForClass (ClassId))
        {
            return ClassVisual;
        }
    }

    return nullptr;
}

void UGridInventoryWidget::RefreshSelectedCharacterClassIcon ()
{
    if (!Image_CharacterClassIcon && !Border_CharacterClassAccent)
    {
        return;
    }

    if (!InventoryComponent)
    {
        if (Image_CharacterClassIcon)
        {
            Image_CharacterClassIcon->SetVisibility (ESlateVisibility::Collapsed);
        }
        if (Border_CharacterClassAccent)
        {
            Border_CharacterClassAccent->SetVisibility (ESlateVisibility::Collapsed);
        }
        return;
    }

    FRPGCharacterVisualSelection VisualSelection;
    if (!InventoryComponent->GetCharacterVisualSelection (GetSelectedCharacterIndex (), VisualSelection))
    {
        if (Image_CharacterClassIcon)
        {
            Image_CharacterClassIcon->SetVisibility (ESlateVisibility::Collapsed);
        }
        if (Border_CharacterClassAccent)
        {
            Border_CharacterClassAccent->SetVisibility (ESlateVisibility::Collapsed);
        }
        return;
    }

    const URPGClassVisualAsset* ClassVisual = FindClassVisualForClass (VisualSelection.ClassId);
    const TSoftObjectPtr<UTexture2D> ClassIcon = ClassVisual && !ClassVisual->ClassIcon.IsNull ()
        ? ClassVisual->ClassIcon
        : VisualSelection.ClassIcon;

    if (Image_CharacterClassIcon)
    {
        if (ClassIcon.IsNull ())
        {
            Image_CharacterClassIcon->SetVisibility (ESlateVisibility::Collapsed);
        }
        else
        {
            Image_CharacterClassIcon->SetBrushFromSoftTexture (ClassIcon, false);
            Image_CharacterClassIcon->SetVisibility (ESlateVisibility::HitTestInvisible);
        }
    }

    if (Border_CharacterClassAccent)
    {
        if (!ClassVisual)
        {
            Border_CharacterClassAccent->SetVisibility (ESlateVisibility::Collapsed);
        }
        else
        {
            Border_CharacterClassAccent->SetBrushColor (ClassVisual->AccentColor);
            Border_CharacterClassAccent->SetVisibility (ESlateVisibility::HitTestInvisible);
        }
    }
}
