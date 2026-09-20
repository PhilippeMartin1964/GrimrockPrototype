#include "UI/GridInventoryWidget.h"

#include "Components/Border.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "RPG/RPGAuthoringIdentityResolver.h"
#include "RPG/RPGClassVisualAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"

void UGridInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RegisterBoundPartyMemberWidgets();

	// UI-SPLIT03: the right-side inventory bag deliberately has no paper doll.
	// Registration itself must therefore be role-aware, not only validation,
	// otherwise a correct WBP_InventoryBag emits 18 false SlotMissing warnings.
	if (HasPaperDollPresentation())
	{
		RegisterPaperDollEquipmentSlotWidgets();
		ValidatePaperDollEquipmentRegistration();
	}

	RefreshRegisteredSlotWidgets();
}

bool UGridInventoryWidget::HasPaperDollPresentation() const
{
	return Border_EquipmentPanel || SlotWidget_Head || SlotWidget_Face || SlotWidget_Amulet || SlotWidget_Shoulders || SlotWidget_Shirt ||
		SlotWidget_Chest || SlotWidget_Cloak || SlotWidget_Bracers || SlotWidget_Gloves || SlotWidget_Belt || SlotWidget_Legs || SlotWidget_Feet ||
		SlotWidget_Ring1 || SlotWidget_Ring2 || SlotWidget_Earring1 || SlotWidget_Earring2 || SlotWidget_MainHand || SlotWidget_OffHand;
}

void UGridInventoryWidget::NativeDestruct()
{
	if (InventoryComponent)
	{
		InventoryComponent->OnPartyInventoryChanged.RemoveDynamic(this, &UGridInventoryWidget::HandlePartyInventoryChanged);
	}

	Super::NativeDestruct();
}

void UGridInventoryWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	RefreshSelectedCharacterClassIcon();
}

const URPGClassVisualAsset* UGridInventoryWidget::FindClassVisualForClass(FName ClassId) const
{
	if (ClassId.IsNone())
	{
		return nullptr;
	}

	for (const URPGClassVisualAsset* ClassVisual : AvailableClassVisuals)
	{
		if (ClassVisual && ClassVisual->IsValidForClass(ClassId))
		{
			FRPGAuthoringIdentityResolver::RememberClassVisual(const_cast<URPGClassVisualAsset*>(ClassVisual));
			return ClassVisual;
		}
	}

	return FRPGAuthoringIdentityResolver::ResolveClassVisualByClassId(ClassId);
}

void UGridInventoryWidget::RefreshSelectedCharacterClassIcon()
{
	if (!Image_CharacterClassIcon && !Border_CharacterClassAccent)
	{
		return;
	}

	if (!InventoryComponent)
	{
		if (Image_CharacterClassIcon)
		{
			Image_CharacterClassIcon->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (Border_CharacterClassAccent)
		{
			Border_CharacterClassAccent->SetVisibility(ESlateVisibility::Collapsed);
		}
		return;
	}

	FRPGCharacterVisualSelection VisualSelection;
	if (!InventoryComponent->GetCharacterVisualSelection(GetSelectedCharacterIndex(), VisualSelection))
	{
		if (Image_CharacterClassIcon)
		{
			Image_CharacterClassIcon->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (Border_CharacterClassAccent)
		{
			Border_CharacterClassAccent->SetVisibility(ESlateVisibility::Collapsed);
		}
		return;
	}

	const URPGClassVisualAsset* ClassVisual = FindClassVisualForClass(VisualSelection.ClassId);
	const TSoftObjectPtr<UTexture2D> ClassIcon =
		ClassVisual && !ClassVisual->ClassIcon.IsNull() ? ClassVisual->ClassIcon : FRPGAuthoringIdentityResolver::ResolveClassIcon(VisualSelection.ClassId);

	if (Image_CharacterClassIcon)
	{
		if (ClassIcon.IsNull())
		{
			Image_CharacterClassIcon->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			Image_CharacterClassIcon->SetBrushFromSoftTexture(ClassIcon, false);
			Image_CharacterClassIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
	}

	if (Border_CharacterClassAccent)
	{
		if (!ClassVisual)
		{
			Border_CharacterClassAccent->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			Border_CharacterClassAccent->SetBrushColor(ClassVisual->AccentColor);
			Border_CharacterClassAccent->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
	}
}
