#include "UI/GridCombatHotbarDragDropOperation.h"

#include "Runtime/GridPartyInventoryComponent.h"

void UGridCombatHotbarDragDropOperation::InitializeFromHotbarSlot(int32 InCharacterIndex, int32 InSourceSlotIndex, const FGridCombatHotbarBinding& InBinding)
{
	CharacterIndex = InCharacterIndex;
	SourceSlotIndex = InSourceSlotIndex;
	Binding = InBinding;
	bFromActionPalette = false;
	bFromSpellbook = false;
}

void UGridCombatHotbarDragDropOperation::InitializeFromActionPalette(int32 InCharacterIndex, const FGridAvailableCombatAction& InAction)
{
	CharacterIndex = InCharacterIndex;
	SourceSlotIndex = INDEX_NONE;
	bFromActionPalette = true;
	bFromSpellbook = false;
	Binding = FGridCombatHotbarBinding();
	Binding.ActionId = InAction.Definition.ActionId;
	Binding.SourcePolicy = InAction.Definition.SourcePolicy;
	Binding.SourceDefinitionId = InAction.SourceDefinitionId;
}

void UGridCombatHotbarDragDropOperation::InitializeFromSpellbookEntry(int32 InCharacterIndex, const FGridSpellbookEntryView& InEntry)
{
	CharacterIndex = InCharacterIndex;
	SourceSlotIndex = INDEX_NONE;
	bFromActionPalette = false;
	bFromSpellbook = InEntry.bCanAssignToHotbar && !InEntry.SpellId.IsNone();
	Binding = FGridCombatHotbarBinding();
	if (bFromSpellbook)
	{
		Binding.ActionId = InEntry.SpellId;
		Binding.SourcePolicy = EGridCombatActionSourcePolicy::Spell;
		Binding.SourceDefinitionId = InEntry.SpellId;
	}
}

EGridSpellHotbarAssignmentResult UGridCombatHotbarDragDropOperation::CommitSpellbookDrop(
	UGridPartyInventoryComponent* InventoryComponent, const FGridCharacterSpellbookState& Spellbook, int32 TargetSlotIndex) const
{
	if (!bFromSpellbook || CharacterIndex == INDEX_NONE || Binding.SourcePolicy != EGridCombatActionSourcePolicy::Spell)
	{
		return EGridSpellHotbarAssignmentResult::UnknownSpell;
	}

	return UGridSpellbookUILibrary::AssignKnownSpellToHotbar(InventoryComponent, CharacterIndex, Spellbook, Binding.SourceDefinitionId, TargetSlotIndex);
}
