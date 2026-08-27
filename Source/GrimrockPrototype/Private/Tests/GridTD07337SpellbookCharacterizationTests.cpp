#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Magic/GridPartySpellbookComponent.h"
#include "Magic/GridProductionSpellLibrary.h"
#include "Magic/GridSpellbookPersistence.h"
#include "Runtime/GridInventoryTypes.h"
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07337RuntimeAuthorityBoundaryTest,
	"Grimrock.TechnicalDebt.TD07_3_3_7.Characterization.RuntimeAuthorityBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07337RuntimeAuthorityBoundaryTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD07337Characterization;

	UGridPartySpellbookComponent* Spellbook = NewObject<UGridPartySpellbookComponent>();
	const FGuid CharacterId = MakeTD07337Id(1);
	TestTrue(TEXT("Runtime component registers a character spellbook"), Spellbook->EnsureCharacterSpellbook(CharacterId));
	TestEqual(TEXT("Runtime component learns a spell"), Spellbook->LearnSpell(CharacterId, FGridProductionSpellLibrary::ArcaneBoltId()),
		EGridSpellbookMutationResult::Success);
	TestTrue(TEXT("Runtime component is the immediate read authority"), Spellbook->KnowsSpell(CharacterId, FGridProductionSpellLibrary::ArcaneBoltId()));

	const FProperty* StateProperty =
		FindFProperty<FProperty>(UGridPartySpellbookComponent::StaticClass(), GET_MEMBER_NAME_CHECKED(UGridPartySpellbookComponent, SpellbookState));
	TestNotNull(TEXT("SpellbookState remains reflected"), StateProperty);
	TestTrue(TEXT("SpellbookState is transient runtime authority"), StateProperty && StateProperty->HasAnyPropertyFlags(CPF_Transient));
	TestNull(TEXT("Character state does not yet own KnownSpellIds"),
		FindFProperty<FProperty>(FGridCharacterInventoryState::StaticStruct(), TEXT("KnownSpellIds")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07337SparsePersistenceMirrorTest,
	"Grimrock.TechnicalDebt.TD07_3_3_7.Characterization.SparsePersistenceMirror",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07337SparsePersistenceMirrorTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD07337Characterization;

	const FGuid MageId = MakeTD07337Id(2);
	const FGuid EmptyActiveId = MakeTD07337Id(3);
	const FGuid ReserveId = MakeTD07337Id(4);

	FGridPartyInventoryState Party;
	Party.ActiveCharacters.Add(MakeTD07337Character(MageId));
	Party.ActiveCharacters.Add(MakeTD07337Character(EmptyActiveId));
	Party.CharacterPool.Add(MakeTD07337Character(ReserveId));

	FGridPartySpellbookState Runtime;
	TestTrue(TEXT("Mage runtime container created"), Runtime.EnsureCharacter(MageId));
	TestTrue(TEXT("Empty active runtime container created"), Runtime.EnsureCharacter(EmptyActiveId));
	TestTrue(TEXT("Reserve runtime container created"), Runtime.EnsureCharacter(ReserveId));
	TestEqual(TEXT("Mage learns Haste first"), Runtime.LearnSpell(MageId, FGridProductionSpellLibrary::HasteId()), EGridSpellbookMutationResult::Success);
	TestEqual(TEXT("Mage learns Arcane Bolt second"), Runtime.LearnSpell(MageId, FGridProductionSpellLibrary::ArcaneBoltId()),
		EGridSpellbookMutationResult::Success);
	TestEqual(TEXT("Reserve learns Lesser Heal"), Runtime.LearnSpell(ReserveId, FGridProductionSpellLibrary::LesserHealId()),
		EGridSpellbookMutationResult::Success);

	TArray<FGridCharacterSpellbookSaveState> Saved;
	FString Error;
	TestTrue(TEXT("Runtime spellbooks capture into a separate Save mirror"),
		FGridSpellbookPersistence::CapturePartySpellbooks(Party, Runtime, Saved, Error));
	TestEqual(TEXT("Empty character is omitted from sparse Save mirror"), Saved.Num(), 2);
	if (Saved.Num() == 2)
	{
		TestTrue(TEXT("Save mirror is deterministically ordered by CharacterId"), Saved[0].CharacterId == MageId && Saved[1].CharacterId == ReserveId);
		TestEqual(TEXT("Mage snapshot contains two known spells"), Saved[0].KnownSpellIds.Num(), 2);
		if (Saved[0].KnownSpellIds.Num() == 2)
		{
			TestEqual(TEXT("Save mirror sorts SpellIds independently of runtime insertion order"), Saved[0].KnownSpellIds[0],
				FGridProductionSpellLibrary::ArcaneBoltId());
			TestEqual(TEXT("Second deterministic SpellId is Haste"), Saved[0].KnownSpellIds[1], FGridProductionSpellLibrary::HasteId());
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07337RestoreReplacementBoundaryTest,
	"Grimrock.TechnicalDebt.TD07_3_3_7.Characterization.RestoreReplacementBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07337RestoreReplacementBoundaryTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD07337Characterization;

	const FGuid MageId = MakeTD07337Id(5);
	const FGuid EmptyActiveId = MakeTD07337Id(6);
	const FGuid ReserveId = MakeTD07337Id(7);

	FGridPartyInventoryState Party;
	Party.ActiveCharacters.Add(MakeTD07337Character(MageId));
	Party.ActiveCharacters.Add(MakeTD07337Character(EmptyActiveId));
	Party.CharacterPool.Add(MakeTD07337Character(ReserveId));

	FGridPartySpellbookState Runtime;
	Runtime.EnsureCharacter(MageId);
	Runtime.LearnSpell(MageId, FGridProductionSpellLibrary::HasteId());

	FGridCharacterSpellbookSaveState SavedMage;
	SavedMage.CharacterId = MageId;
	SavedMage.KnownSpellIds.Add(FGridProductionSpellLibrary::ArcaneBoltId());
	TArray<FGridCharacterSpellbookSaveState> Saved;
	Saved.Add(SavedMage);

	FString Error;
	TestTrue(TEXT("Restore replaces the prior runtime spellbook"),
		FGridSpellbookPersistence::RestorePartySpellbooks(Party, Saved, Runtime, Error));
	TestEqual(TEXT("Restore creates containers for every Active and pooled character"), Runtime.CharacterSpellbooks.Num(), 3);
	TestTrue(TEXT("Persisted spell is restored"), Runtime.KnowsSpell(MageId, FGridProductionSpellLibrary::ArcaneBoltId()));
	TestFalse(TEXT("Stale runtime-only spell is removed by replacement"), Runtime.KnowsSpell(MageId, FGridProductionSpellLibrary::HasteId()));
	TestTrue(TEXT("Absent active snapshot restores empty"),
		Runtime.FindSpellbook(EmptyActiveId) && Runtime.FindSpellbook(EmptyActiveId)->KnownSpellIds.IsEmpty());
	TestTrue(TEXT("Absent pooled snapshot restores empty"),
		Runtime.FindSpellbook(ReserveId) && Runtime.FindSpellbook(ReserveId)->KnownSpellIds.IsEmpty());

	const FGridPartySpellbookState BeforeInvalidRestore = Runtime;
	FGridCharacterSpellbookSaveState InvalidSaved;
	InvalidSaved.CharacterId = MageId;
	InvalidSaved.KnownSpellIds.Add(NAME_None);
	TArray<FGridCharacterSpellbookSaveState> InvalidStates;
	InvalidStates.Add(InvalidSaved);
	TestFalse(TEXT("Invalid replacement snapshot is rejected atomically"),
		FGridSpellbookPersistence::RestorePartySpellbooks(Party, InvalidStates, Runtime, Error));
	TestTrue(TEXT("Failed restore preserves previous spell knowledge"), Runtime.KnowsSpell(MageId, FGridProductionSpellLibrary::ArcaneBoltId()));
	TestEqual(TEXT("Failed restore preserves runtime container count"), Runtime.CharacterSpellbooks.Num(), BeforeInvalidRestore.CharacterSpellbooks.Num());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07337LegacyToleranceAndHotbarIndependenceTest,
	"Grimrock.TechnicalDebt.TD07_3_3_7.Characterization.LegacyToleranceAndHotbarIndependence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07337LegacyToleranceAndHotbarIndependenceTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD07337Characterization;

	const FGuid CharacterId = MakeTD07337Id(8);
	FGridPartyInventoryState Party;
	Party.ActiveCharacters.Add(MakeTD07337Character(CharacterId));
	Party.ActiveCharacters[0].CombatHotbarSlots[0] = MakeTD07337SpellBinding(FGridProductionSpellLibrary::ArcaneBoltId(), 0);

	FGridPartySpellbookState EmptyRestored;
	TArray<FGridCharacterSpellbookSaveState> EmptySaved;
	FString Error;
	TestTrue(TEXT("Empty Spellbook state restores beside a configured Spell hotbar"),
		FGridSpellbookPersistence::RestorePartySpellbooks(Party, EmptySaved, EmptyRestored, Error));
	TestFalse(TEXT("Hotbar reference does not teach its SpellId"),
		EmptyRestored.KnowsSpell(CharacterId, FGridProductionSpellLibrary::ArcaneBoltId()));
	TestTrue(TEXT("Spell hotbar binding remains structurally valid"), Party.ActiveCharacters[0].CombatHotbarSlots[0].IsValid());

	FGridCharacterSpellbookSaveState UnknownSaved;
	UnknownSaved.CharacterId = CharacterId;
	UnknownSaved.KnownSpellIds.Add(TEXT("Spell_RemovedContent"));
	TArray<FGridCharacterSpellbookSaveState> UnknownStates;
	UnknownStates.Add(UnknownSaved);
	FGridPartySpellbookState UnknownRestored;
	TestTrue(TEXT("Current MON18.8 persistence accepts an unknown non-empty SpellId"),
		FGridSpellbookPersistence::RestorePartySpellbooks(Party, UnknownStates, UnknownRestored, Error));
	TestTrue(TEXT("Unknown SpellId is preserved as known legacy content"),
		UnknownRestored.KnowsSpell(CharacterId, TEXT("Spell_RemovedContent")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
