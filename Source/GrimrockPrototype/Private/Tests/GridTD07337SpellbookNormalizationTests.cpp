#if WITH_DEV_AUTOMATION_TESTS

#include "Magic/GridPartySpellbookComponent.h"
#include "Magic/GridProductionSpellLibrary.h"
#include "Magic/GridSpellbookPersistence.h"
#include "Misc/AutomationTest.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Save/GrimrockPartySaveGame.h"
#include "UObject/UnrealType.h"

namespace GridTD07337Normalization
{
	FGridCharacterInventoryState MakeTD07337NCharacter(uint32 Suffix)
	{
		FGridCharacterInventoryState Character;
		Character.CharacterId = FGuid(7, 3, 37, Suffix);
		Character.Experience = 0;
		Character.Level = 1;
		return Character;
	}

	UGridPartySpellbookComponent* MakeTD07337NFacade(FGridPartyInventoryState Party, UGridPartyInventoryComponent*& OutInventory)
	{
		OutInventory = NewObject<UGridPartyInventoryComponent>();
		OutInventory->PartyInventoryState = MoveTemp(Party);
		UGridPartySpellbookComponent* Spellbook = NewObject<UGridPartySpellbookComponent>();
		Spellbook->InitializeSpellbookComponent(OutInventory);
		return Spellbook;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07337SchemaAuthorityTest,
	"Grimrock.TechnicalDebt.TD07_3_3_7.Normalization.SchemaAuthority",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07337SchemaAuthorityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FProperty* KnownSpellsProperty = FindFProperty<FProperty>(FGridCharacterInventoryState::StaticStruct(), TEXT("KnownSpellIds"));
	TestNotNull(TEXT("KnownSpellIds exists on durable character state"), KnownSpellsProperty);
	TestTrue(TEXT("KnownSpellIds is durable and non-transient"), KnownSpellsProperty && !KnownSpellsProperty->HasAnyPropertyFlags(CPF_Transient));
	TestNull(TEXT("Component-owned SpellbookState is removed"),
		FindFProperty<FProperty>(UGridPartySpellbookComponent::StaticClass(), TEXT("SpellbookState")));
	TestNull(TEXT("Separate CharacterSpellbookStates SaveGame mirror is removed"),
		FindFProperty<FProperty>(UGrimrockPartySaveGame::StaticClass(), TEXT("CharacterSpellbookStates")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07337DirectMutationTest,
	"Grimrock.TechnicalDebt.TD07_3_3_7.Normalization.DirectMutation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07337DirectMutationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD07337Normalization;
	FGridPartyInventoryState Party;
	const FGridCharacterInventoryState Character = MakeTD07337NCharacter(2);
	Party.ActiveCharacters.Add(Character);
	UGridPartyInventoryComponent* Inventory = nullptr;
	UGridPartySpellbookComponent* Spellbook = MakeTD07337NFacade(Party, Inventory);
	const FGuid CharacterId = Character.CharacterId;

	TestEqual(TEXT("Haste learns"), Spellbook->LearnSpell(CharacterId, FGridProductionSpellLibrary::HasteId()), EGridSpellbookMutationResult::Success);
	TestEqual(TEXT("Arcane Bolt learns"), Spellbook->LearnSpell(CharacterId, FGridProductionSpellLibrary::ArcaneBoltId()),
		EGridSpellbookMutationResult::Success);
	TestEqual(TEXT("Unknown SpellId is rejected"), Spellbook->LearnSpell(CharacterId, TEXT("Spell_RemovedContent")),
		EGridSpellbookMutationResult::InvalidSpell);
	const TArray<FName>& Known = Inventory->PartyInventoryState.ActiveCharacters[0].KnownSpellIds;
	TestEqual(TEXT("Two canonical durable entries"), Known.Num(), 2);
	if (Known.Num() == 2)
	{
		TestEqual(TEXT("Durable order is deterministic"), Known[0], FGridProductionSpellLibrary::ArcaneBoltId());
		TestEqual(TEXT("Haste follows"), Known[1], FGridProductionSpellLibrary::HasteId());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07337ActivePoolDurabilityTest,
	"Grimrock.TechnicalDebt.TD07_3_3_7.Normalization.ActivePoolDurability",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07337ActivePoolDurabilityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD07337Normalization;
	FGridPartyInventoryState Party;
	FGridCharacterInventoryState Reserve = MakeTD07337NCharacter(3);
	Reserve.KnownSpellIds.Add(FGridProductionSpellLibrary::LesserHealId());
	Party.CharacterPool.Add(Reserve);

	FGridCharacterInventoryState Moved = Party.CharacterPool[0];
	Party.CharacterPool.Reset();
	Party.ActiveCharacters.Add(Moved);
	TestTrue(TEXT("Moving Pool -> Active carries Spellbook knowledge naturally"),
		Party.ActiveCharacters[0].KnownSpellIds.Contains(FGridProductionSpellLibrary::LesserHealId()));
	FString Error;
	TestTrue(TEXT("Moved durable state validates without reconciliation"), FGridSpellbookPersistence::ValidatePartySpellbooks(Party, Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07337SaveSchemaVersionTest,
	"Grimrock.TechnicalDebt.TD07_3_3_7.Normalization.SaveSchemaVersion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07337SaveSchemaVersionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	TestEqual(TEXT("TD07.3.3.7 opens SaveGame v17"), UGrimrockPartySaveGame::CurrentSaveVersion, 17);
	UGrimrockPartySaveGame* Current = NewObject<UGrimrockPartySaveGame>();
	TestEqual(TEXT("New SaveGame starts on v17"), Current->SaveVersion, 17);
	TestTrue(TEXT("Current v17 is compatible"), Current->IsCompatible());

	UGrimrockPartySaveGame* Previous = NewObject<UGrimrockPartySaveGame>();
	Previous->SaveVersion = 16;
	FText Error;
	TestFalse(TEXT("Previous v16 is rejected without migration"), Previous->ValidateCurrentState(Error));
	TestFalse(TEXT("Previous v16 is incompatible"), Previous->IsCompatible());
	TestEqual(TEXT("Validation does not rewrite v16"), Previous->SaveVersion, 16);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
