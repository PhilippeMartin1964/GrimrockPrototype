#include "UI/GridInventoryWidget.h"

#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "Runtime/GridPartyInventoryComponent.h"

void UGridInventoryWidget::NativeTick (const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick (MyGeometry, InDeltaTime);
    RefreshSelectedCharacterClassIcon ();
}

void UGridInventoryWidget::RefreshSelectedCharacterClassIcon ()
{
    if (!Image_CharacterClassIcon)
    {
        return;
    }

    if (!InventoryComponent)
    {
        Image_CharacterClassIcon->SetVisibility (ESlateVisibility::Collapsed);
        return;
    }

    FRPGCharacterVisualSelection VisualSelection;
    if (!InventoryComponent->GetCharacterVisualSelection (GetSelectedCharacterIndex (), VisualSelection) ||
        VisualSelection.ClassIcon.IsNull ())
    {
        Image_CharacterClassIcon->SetVisibility (ESlateVisibility::Collapsed);
        return;
    }

    Image_CharacterClassIcon->SetBrushFromSoftTexture (VisualSelection.ClassIcon, false);
    Image_CharacterClassIcon->SetVisibility (ESlateVisibility::HitTestInvisible);
}
