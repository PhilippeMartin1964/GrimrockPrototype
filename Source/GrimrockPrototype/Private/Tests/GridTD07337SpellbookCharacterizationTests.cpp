#if WITH_DEV_AUTOMATION_TESTS

#include "Magic/GridPartySpellbookComponent.h"
#include "Magic/GridProductionSpellLibrary.h"
#include "Magic/GridSpellbookPersistence.h"
#include "Misc/AutomationTest.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "UObject/UnrealType.h"

namespace GridTD07337Characterization
{
	FGuid MakeTD07337Id(uint32 Suffix)
	{
		return FGuid(7, 3, 37, Suffix);
	}

	FGridCharacterInventoryState MakeTD07337Character(const FGuid& CharacterId)
	{
		FGridCharacterInventoryState Character;
		Character.CharacterId = CharacterId;
		Character.Experience = 0;
		Character.Level = 1;
		Character.CombatHotbarSlots.SetNum(FGridCombatHotbarBinding::SlotCount);
		for (int32 SlotIndex = 0; SlotIndex < FGridCombatHotbarBinding::SlotCount; ++SlotIndex)
		{
			Character.CombatHotbarSlots[SlotIndex].Reset(SlotIndex);
		}
		return Character;
	}

	UGridPartySpellbookComponent* MakeTD07337Facade(FGridPartyInventoryState Party, UGridPartyInventoryComponent*& OutInventory)
	{
		OutInventory = NewObject<UGridPartyInventoryComponent>();
		OutInventory->PartyInventoryState = MoveTemp(Party);
		UGridPartySpellbookComponent* Spellbook = NewObject<UGridPartySpellbookComponent>();
		Spellbook->InitializeSpellbookComponent(OutInventory);
		return Spellbook;
	}

	FGridCombatHotbarBinding MakeTD07337SpellBinding(FName SpellId, int32 SlotIndex)
	{
		FGridCombatHotbarBinding Binding;
		Binding.Reset(SlotIndex);
		Binding.ActionId = SpellId;
		Binding.SourcePolicy = EGridCombatActionSourcePolicy::Spell;
		Binding.SourceDefinitionId = SpellId;
		return Binding;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07337RuntimeAuthorityBoundaryTest, "Grimrock.TechnicalDebt.TD07_3_3_7.Characterization.RuntimeAuthorityBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07337RuntimeAuthorityBoundaryTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD07337Characterization;
	const FGuid CharacterId = MakeTD07337Id(1);
	FGridPartyInventoryState Party;
	Party.ActiveCharacters.Add(MakeTD07337Character(CharacterId));
	UGridPartyInventoryComponent* Inventory = nullptr;
	UGridPartySpellbookComponent* Spellbook = MakeTD07337Facade(Party, Inventory);

	TestEqual(TEXT("Facade learns into durable character state"), Spellbook->LearnSpell(CharacterId, FGridProductionSpellLibrary::ArcaneBoltId()),
		EGridSpellbookMutationResult::Success);
	TestTrue(TEXT("Durable character owns learned SpellId"),
		Inventory->PartyInventoryState.ActiveCharacters[0].KnownSpellIds.Contains(FGridProductionSpellLibrary::ArcaneBoltId()));
	TestTrue(TEXT("Facade immediately reads durable authority"), Spellbook->KnowsSpell(CharacterId, FGridProductionSpellLibrary::ArcaneBoltId()));
	TestNull(
		TEXT("Parallel component SpellbookState is removed"), FindFProperty<FProperty>(UGridPartySpellbookComponent::StaticClass(), TEXT("SpellbookState")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07337SparsePersistenceMirrorTest, "Grimrock.TechnicalDebt.TD07_3_3_7.Characterization.SparsePersistenceMirror",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07337SparsePersistenceMirrorTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD07337Characterization;
	const FGuid MageId = MakeTD07337Id(2);
	const FGuid ReserveId = MakeTD07337Id(3);
	FGridPartyInventoryState Party;
	Party.ActiveCharacters.Add(MakeTD07337Character(MageId));
	Party.CharacterPool.Add(MakeTD07337Character(ReserveId));
	UGridPartyInventoryComponent* Inventory = nullptr;
	UGridPartySpellbookComponent* Spellbook = MakeTD07337Facade(Party, Inventory);

	Spellbook->LearnSpell(MageId, FGridProductionSpellLibrary::HasteId());
	Spellbook->LearnSpell(MageId, FGridProductionSpellLibrary::ArcaneBoltId());
	TestEqual(TEXT("Durable state stores two canonical identities"), Inventory->PartyInventoryState.ActiveCharacters[0].KnownSpellIds.Num(), 2);
	TestTrue(TEXT("Empty reserve needs no sparse mirror"), Inventory->PartyInventoryState.CharacterPool[0].KnownSpellIds.IsEmpty());
	const FGridPartyInventoryState Copy = Inventory->PartyInventoryState;
	TestTrue(
		TEXT("Ordinary party-state copy carries Spellbook knowledge"), Copy.ActiveCharacters[0].KnownSpellIds.Contains(FGridProductionSpellLibrary::HasteId()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07337RestoreReplacementBoundaryTest, "Grimrock.TechnicalDebt.TD07_3_3_7.Characterization.RestoreReplacementBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07337RestoreReplacementBoundaryTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD07337Characterization;
	FGridPartyInventoryState Source;
	FGridCharacterInventoryState Mage = MakeTD07337Character(MakeTD07337Id(5));
	Mage.KnownSpellIds.Add(FGridProductionSpellLibrary::ArcaneBoltId());
	Source.ActiveCharacters.Add(Mage);
	Source.ActiveCharacters.Add(MakeTD07337Character(MakeTD07337Id(6)));

	FGridPartyInventoryState Runtime = Source;
	Runtime.ActiveCharacters[0].KnownSpellIds = { FGridProductionSpellLibrary::HasteId() };
	Runtime = Source;
	TestTrue(TEXT("Whole-party restore replaces stale Spellbook state"),
		Runtime.ActiveCharacters[0].KnownSpellIds.Contains(FGridProductionSpellLibrary::ArcaneBoltId()));
	TestFalse(TEXT("Stale runtime-only spell disappears"), Runtime.ActiveCharacters[0].KnownSpellIds.Contains(FGridProductionSpellLibrary::HasteId()));
	TestTrue(TEXT("Absent durable knowledge remains empty"), Runtime.ActiveCharacters[1].KnownSpellIds.IsEmpty());

	FGridPartyInventoryState InvalidCandidate = Source;
	InvalidCandidate.ActiveCharacters[0].KnownSpellIds = { TEXT("Spell_RemovedContent") };
	FString Error;
	TestFalse(TEXT("Invalid exact-match candidate is rejected before commit"), FGridSpellbookPersistence::ValidatePartySpellbooks(InvalidCandidate, Error));
	TestTrue(TEXT("Original state remains unchanged after candidate rejection"),
		Source.ActiveCharacters[0].KnownSpellIds.Contains(FGridProductionSpellLibrary::ArcaneBoltId()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07337CanonicalDefinitionAndHotbarIndependenceTest,
	"Grimrock.TechnicalDebt.TD07_3_3_7.Characterization.CanonicalDefinitionAndHotbarIndependence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07337CanonicalDefinitionAndHotbarIndependenceTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD07337Characterization;
	const FGuid CharacterId = MakeTD07337Id(8);
	FGridPartyInventoryState Party;
	Party.ActiveCharacters.Add(MakeTD07337Character(CharacterId));
	Party.ActiveCharacters[0].CombatHotbarSlots[0] = MakeTD07337SpellBinding(FGridProductionSpellLibrary::ArcaneBoltId(), 0);
	UGridPartyInventoryComponent* Inventory = nullptr;
	UGridPartySpellbookComponent* Spellbook = MakeTD07337Facade(Party, Inventory);

	TestFalse(TEXT("Hotbar reference does not teach its SpellId"), Spellbook->KnowsSpell(CharacterId, FGridProductionSpellLibrary::ArcaneBoltId()));
	TestTrue(TEXT("Spell hotbar binding remains structurally valid"), Inventory->PartyInventoryState.ActiveCharacters[0].CombatHotbarSlots[0].IsValid());
	TestEqual(TEXT("Unknown legacy SpellId is rejected by exact-match mutation"), Spellbook->LearnSpell(CharacterId, TEXT("Spell_RemovedContent")),
		EGridSpellbookMutationResult::InvalidSpell);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
