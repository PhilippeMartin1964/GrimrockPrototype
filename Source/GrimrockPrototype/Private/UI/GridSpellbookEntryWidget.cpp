#include "UI/GridSpellbookEntryWidget.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/TextBlock.h"
#include "InputCoreTypes.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "UI/GridCombatHotbarDragDropOperation.h"

void UGridSpellbookEntryWidget::InitializeSpellEntry(const FGridSpellbookEntryView& InEntry)
{
	Entry = InEntry;
	RefreshEntryVisual();
}

void UGridSpellbookEntryWidget::RefreshEntryVisual()
{
	if (Text_SpellName)
	{
		Text_SpellName->SetText(GetSpellNameText());
	}
	if (Text_School)
	{
		Text_School->SetText(GetSchoolText());
	}
	if (Text_Cost)
	{
		Text_Cost->SetText(GetCostText());
	}
	if (Text_Range)
	{
		Text_Range->SetText(GetRangeText());
	}
	if (Text_Hotbar)
	{
		Text_Hotbar->SetText(GetHotbarText());
	}
	if (Text_Description)
	{
		Text_Description->SetText(Entry.Description);
	}

	SetToolTipText(Entry.bCanAssignToHotbar ? NSLOCTEXT("GridSpellbook", "SpellDragToHotbar", "Glissez ce sort vers un raccourci de la barre d'actions.")
											: NSLOCTEXT("GridSpellbook", "SpellCannotDragToHotbar", "Ce sort ne peut pas être assigné à la barre d'actions."));
}

FReply UGridSpellbookEntryWidget::NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && Entry.bCanAssignToHotbar && !Entry.SpellId.IsNone())
	{
		return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;
	}

	return Super::NativeOnPreviewMouseButtonDown(InGeometry, InMouseEvent);
}

void UGridSpellbookEntryWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);

	if (!Entry.bCanAssignToHotbar || Entry.SpellId.IsNone())
	{
		return;
	}

	AGrimrockPartyPawn* PartyPawn = Cast<AGrimrockPartyPawn>(GetOwningPlayerPawn());
	UGridPartyInventoryComponent* InventoryComponent = PartyPawn ? PartyPawn->PartyInventoryComponent.Get() : nullptr;
	if (!InventoryComponent)
	{
		return;
	}

	const int32 CharacterIndex = InventoryComponent->GetSelectedCharacterIndex();
	if (!InventoryComponent->IsValidCharacterIndex(CharacterIndex))
	{
		return;
	}

	UGridCombatHotbarDragDropOperation* Operation = NewObject<UGridCombatHotbarDragDropOperation>(this);
	if (!Operation)
	{
		return;
	}

	Operation->InitializeFromSpellbookEntry(CharacterIndex, Entry);
	Operation->DefaultDragVisual = nullptr;
	Operation->Pivot = EDragPivot::MouseDown;
	OutOperation = Operation;
}

FText UGridSpellbookEntryWidget::GetSpellNameText() const
{
	if (!Entry.DisplayName.IsEmpty())
	{
		return Entry.DisplayName;
	}
	return Entry.SpellId.IsNone() ? NSLOCTEXT("GridSpellbook", "UnknownSpell", "Sort inconnu") : FText::FromName(Entry.SpellId);
}

FText UGridSpellbookEntryWidget::GetSchoolText() const
{
	if (const UEnum* SchoolEnum = StaticEnum<EGridSpellSchool>())
	{
		return SchoolEnum->GetDisplayNameTextByValue(static_cast<int64>(Entry.School));
	}
	return FText::GetEmpty();
}

FText UGridSpellbookEntryWidget::GetCostText() const
{
	if (!Entry.bDefinitionResolved)
	{
		return NSLOCTEXT("GridSpellbook", "UnresolvedDefinition", "Définition introuvable");
	}

	return FText::Format(
		NSLOCTEXT("GridSpellbook", "SpellCostFormat", "Mana {0}  |  PA {1}"), FText::AsNumber(Entry.ManaCost), FText::AsNumber(Entry.ActionPointCost));
}

FText UGridSpellbookEntryWidget::GetRangeText() const
{
	if (!Entry.bDefinitionResolved)
	{
		return FText::GetEmpty();
	}

	if (Entry.MinRangeCells == Entry.MaxRangeCells)
	{
		return FText::Format(NSLOCTEXT("GridSpellbook", "SpellRangeSingle", "Portée {0}"), FText::AsNumber(Entry.MaxRangeCells));
	}

	return FText::Format(
		NSLOCTEXT("GridSpellbook", "SpellRangeInterval", "Portée {0}-{1}"), FText::AsNumber(Entry.MinRangeCells), FText::AsNumber(Entry.MaxRangeCells));
}

FText UGridSpellbookEntryWidget::GetHotbarText() const
{
	if (!Entry.bAssignedToHotbar || Entry.AssignedHotbarSlotIndex == INDEX_NONE)
	{
		return NSLOCTEXT("GridSpellbook", "SpellNotAssigned", "Non assigné");
	}

	const int32 DisplayKey = Entry.AssignedHotbarSlotIndex == 9 ? 0 : Entry.AssignedHotbarSlotIndex + 1;
	return FText::Format(NSLOCTEXT("GridSpellbook", "SpellAssignedFormat", "Raccourci {0}"), FText::AsNumber(DisplayKey));
}
