#if WITH_DEV_AUTOMATION_TESTS

#include "Magic/GridProductionSpellLibrary.h"
#include "Magic/GridSpellbookUI.h"
#include "Misc/AutomationTest.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "UI/GridCombatHotbarDragDropOperation.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	UGridPartyInventoryComponent* CreateMON187Inventory()
	{
		UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
		Inventory->InitializeDefaultPartyIfNeeded();
		return Inventory;
	}

	FGridCharacterSpellbookState MakeMON187Spellbook(const UGridPartyInventoryComponent& Inventory)
	{
		FGridCharacterSpellbookState Spellbook;
		Spellbook.CharacterId = Inventory.PartyInventoryState.ActiveCharacters[0].CharacterId;
		return Spellbook;
	}

	TArray<FGridCombatHotbarBinding> ReadMON187Hotbar(UGridPartyInventoryComponent& Inventory)
	{
		TArray<FGridCombatHotbarBinding> Bindings;
		Bindings.SetNum(FGridCombatHotbarBinding::SlotCount);
		for (int32 SlotIndex = 0; SlotIndex < Bindings.Num(); ++SlotIndex)
		{
			Bindings[SlotIndex].Reset(SlotIndex);
			Inventory.GetCharacterCombatHotbarBinding(0, SlotIndex, Bindings[SlotIndex]);
		}
		return Bindings;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON187ProductionEntriesTest, "Grimrock.Magic.MON18.7a.ProductionEntriesReflectKnowledge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON187ProductionEntriesTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UGridPartyInventoryComponent* Inventory = CreateMON187Inventory();
	FGridCharacterSpellbookState Spellbook = MakeMON187Spellbook(*Inventory);
	Spellbook.LearnSpell(FGridProductionSpellLibrary::ArcaneBoltId());
	Spellbook.LearnSpell(FGridProductionSpellLibrary::HasteId());

	TArray<FGridSpellbookEntryView> Entries;
	UGridSpellbookUILibrary::BuildProductionSpellbookEntries(Spellbook, ReadMON187Hotbar(*Inventory), Entries);

	TestEqual(TEXT("Only known spells are projected"), Entries.Num(), 2);
	TestEqual(TEXT("Knowledge order is preserved"), Entries[0].SpellId, FGridProductionSpellLibrary::ArcaneBoltId());
	TestTrue(TEXT("Arcane Bolt definition resolves"), Entries[0].bDefinitionResolved);
	TestTrue(TEXT("Arcane Bolt can be assigned"), Entries[0].bCanAssignToHotbar);
	TestTrue(TEXT("UI action is a Spell source"), Entries[0].CombatActionDefinition.SourcePolicy == EGridCombatActionSourcePolicy::Spell);
	TestEqual(TEXT("Mana cost is projected"), Entries[0].ManaCost, 3);
	TestEqual(TEXT("AP cost is projected"), Entries[0].ActionPointCost, 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON187MissingDefinitionTest, "Grimrock.Magic.MON18.7a.UnknownDefinitionRemainsVisible",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON187MissingDefinitionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UGridPartyInventoryComponent* Inventory = CreateMON187Inventory();
	FGridCharacterSpellbookState Spellbook = MakeMON187Spellbook(*Inventory);
	Spellbook.LearnSpell(TEXT("Spell_NotRegistered"));

	TArray<FGridSpellbookEntryView> Entries;
	UGridSpellbookUILibrary::BuildProductionSpellbookEntries(Spellbook, ReadMON187Hotbar(*Inventory), Entries);

	TestEqual(TEXT("Known unresolved spell remains visible"), Entries.Num(), 1);
	TestFalse(TEXT("Definition is explicitly unresolved"), Entries[0].bDefinitionResolved);
	TestFalse(TEXT("Unresolved spell cannot be assigned"), Entries[0].bCanAssignToHotbar);
	TestEqual(TEXT("Stable SpellId is preserved"), Entries[0].SpellId, FName(TEXT("Spell_NotRegistered")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMON187BindingContractTest, "Grimrock.Magic.MON18.7a.SpellBindingContract", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON187BindingContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const FName SpellId = FGridProductionSpellLibrary::LesserHealId();
	const FGridCombatHotbarBinding Binding = UGridSpellbookUILibrary::MakeSpellHotbarBinding(SpellId, 4);
	TestTrue(TEXT("Spell binding is structurally valid"), Binding.IsValid());
	TestEqual(TEXT("Action identity reuses SpellId"), Binding.ActionId, SpellId);
	TestTrue(TEXT("SourcePolicy is Spell"), Binding.SourcePolicy == EGridCombatActionSourcePolicy::Spell);
	TestEqual(TEXT("SourceDefinitionId reuses SpellId"), Binding.SourceDefinitionId, SpellId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMON187AssignKnownSpellTest, "Grimrock.Magic.MON18.7a.AssignKnownSpell", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON187AssignKnownSpellTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UGridPartyInventoryComponent* Inventory = CreateMON187Inventory();
	FGridCharacterSpellbookState Spellbook = MakeMON187Spellbook(*Inventory);
	const FName SpellId = FGridProductionSpellLibrary::ArcaneBoltId();
	Spellbook.LearnSpell(SpellId);

	TestTrue(TEXT("Known spell assignment succeeds"),
		UGridSpellbookUILibrary::AssignKnownSpellToHotbar(Inventory, 0, Spellbook, SpellId, 3) == EGridSpellHotbarAssignmentResult::Success);

	FGridCombatHotbarBinding Binding;
	TestTrue(TEXT("Assigned slot can be read"), Inventory->GetCharacterCombatHotbarBinding(0, 3, Binding));
	TestTrue(TEXT("Assigned slot contains the spell identity"), UGridSpellbookUILibrary::IsSpellHotbarBinding(Binding, SpellId));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON187DuplicateMoveSwapTest, "Grimrock.Magic.MON18.7a.DuplicateAssignmentMovesOrSwaps",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON187DuplicateMoveSwapTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UGridPartyInventoryComponent* Inventory = CreateMON187Inventory();
	FGridCharacterSpellbookState Spellbook = MakeMON187Spellbook(*Inventory);
	const FName ArcaneBolt = FGridProductionSpellLibrary::ArcaneBoltId();
	const FName Haste = FGridProductionSpellLibrary::HasteId();
	Spellbook.LearnSpell(ArcaneBolt);
	Spellbook.LearnSpell(Haste);

	UGridSpellbookUILibrary::AssignKnownSpellToHotbar(Inventory, 0, Spellbook, ArcaneBolt, 1);
	UGridSpellbookUILibrary::AssignKnownSpellToHotbar(Inventory, 0, Spellbook, Haste, 2);

	TestTrue(TEXT("Reassigning an existing spell uses move/swap"),
		UGridSpellbookUILibrary::AssignKnownSpellToHotbar(Inventory, 0, Spellbook, ArcaneBolt, 2) == EGridSpellHotbarAssignmentResult::Success);

	FGridCombatHotbarBinding SlotOne;
	FGridCombatHotbarBinding SlotTwo;
	Inventory->GetCharacterCombatHotbarBinding(0, 1, SlotOne);
	Inventory->GetCharacterCombatHotbarBinding(0, 2, SlotTwo);
	TestTrue(TEXT("Haste is swapped into the old slot"), UGridSpellbookUILibrary::IsSpellHotbarBinding(SlotOne, Haste));
	TestTrue(TEXT("Arcane Bolt occupies the requested slot"), UGridSpellbookUILibrary::IsSpellHotbarBinding(SlotTwo, ArcaneBolt));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON187RejectUnknownNoMutationTest, "Grimrock.Magic.MON18.7a.UnknownSpellNoMutation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON187RejectUnknownNoMutationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UGridPartyInventoryComponent* Inventory = CreateMON187Inventory();
	FGridCharacterSpellbookState Spellbook = MakeMON187Spellbook(*Inventory);

	TestTrue(TEXT("Unknown spell is rejected"),
		UGridSpellbookUILibrary::AssignKnownSpellToHotbar(Inventory, 0, Spellbook, FGridProductionSpellLibrary::CurePoisonId(), 5) ==
			EGridSpellHotbarAssignmentResult::UnknownSpell);

	FGridCombatHotbarBinding Binding;
	Inventory->GetCharacterCombatHotbarBinding(0, 5, Binding);
	TestTrue(TEXT("Rejected assignment leaves target slot empty"), Binding.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMON187DragDropPayloadTest, "Grimrock.Magic.MON18.7a.DragDropPayload", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON187DragDropPayloadTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UGridPartyInventoryComponent* Inventory = CreateMON187Inventory();
	FGridCharacterSpellbookState Spellbook = MakeMON187Spellbook(*Inventory);
	const FName SpellId = FGridProductionSpellLibrary::LesserHealId();
	Spellbook.LearnSpell(SpellId);

	TArray<FGridSpellbookEntryView> Entries;
	UGridSpellbookUILibrary::BuildProductionSpellbookEntries(Spellbook, ReadMON187Hotbar(*Inventory), Entries);
	if (!TestEqual(TEXT("One entry is available"), Entries.Num(), 1))
	{
		return false;
	}

	UGridCombatHotbarDragDropOperation* Operation = NewObject<UGridCombatHotbarDragDropOperation>();
	Operation->InitializeFromSpellbookEntry(0, Entries[0]);
	TestTrue(TEXT("Payload is marked as Spellbook source"), Operation->bFromSpellbook);
	TestFalse(TEXT("Payload is not an action-palette source"), Operation->bFromActionPalette);
	TestEqual(TEXT("Payload keeps SpellId"), Operation->Binding.SourceDefinitionId, SpellId);

	TestTrue(TEXT("Blueprint drop hook commits through MON12 hotbar"),
		Operation->CommitSpellbookDrop(Inventory, Spellbook, 7) == EGridSpellHotbarAssignmentResult::Success);
	FGridCombatHotbarBinding Binding;
	Inventory->GetCharacterCombatHotbarBinding(0, 7, Binding);
	TestTrue(TEXT("Drop target contains the spell"), UGridSpellbookUILibrary::IsSpellHotbarBinding(Binding, SpellId));
	return true;
}

#endif
