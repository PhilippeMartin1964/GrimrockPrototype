#include "UI/GridCombatHotbarDragDropOperation.h"

#include "Runtime/GridPartyInventoryComponent.h"

void UGridCombatHotbarDragDropOperation::InitializeFromHotbarSlot(int32 InCharacterIndex, int32 InSourceSlotIndex, const FGridCombatHotbarBinding& InBinding)
{
	CharacterIndex = InCharacterIndex;
	SourceSlotIndex = InSourceSlotIndex;
	Binding = InBinding;
	bFromSpellbook = false;
}

void UGridCombatHotbarDragDropOperation::InitializeFromSpellbookEntry(int32 InCharacterIndex, const FGridSpellbookEntryView& InEntry)
{
	CharacterIndex = InCharacterIndex;
	SourceSlotIndex = INDEX_NONE;
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
