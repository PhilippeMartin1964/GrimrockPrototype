#if WITH_DEV_AUTOMATION_TESTS

#include "Kismet/GameplayStatics.h"
#include "Magic/GridPartySpellbookComponent.h"
#include "Magic/GridProductionSpellLibrary.h"
#include "Magic/GridSpellbookPersistence.h"
#include "Misc/AutomationTest.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Save/GrimrockPartySaveGame.h"

namespace GridMON188Tests
{
	struct FMON188DiskSlot
	{
		FString SlotName = FString::Printf(TEXT("MON188_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits));
		int32 UserIndex = 0;
		~FMON188DiskSlot()
		{
			UGameplayStatics::DeleteGameInSlot(SlotName, UserIndex);
		}
	};

	FGridCharacterInventoryState MakeMON188Character(const FGuid& CharacterId)
	{
		FGridCharacterInventoryState Character;
		Character.CharacterId = CharacterId;
		Character.Experience = 0;
		Character.Level = 1;
		Character.CombatHotbarSlots.SetNum(FGridCombatHotbarBinding::SlotCount);
		for (int32 SlotIndex = 0; SlotIndex < Character.CombatHotbarSlots.Num(); ++SlotIndex)
		{
			Character.CombatHotbarSlots[SlotIndex].Reset(SlotIndex);
		}
		return Character;
	}

	FGridPartyInventoryState MakeMON188Party(const TArray<FGuid>& ActiveIds, const TArray<FGuid>& PoolIds = {})
	{
		FGridPartyInventoryState Party;
		for (const FGuid& CharacterId : ActiveIds)
		{
			Party.ActiveCharacters.Add(MakeMON188Character(CharacterId));
			Party.ActiveEquipment.AddDefaulted();
		}
		for (const FGuid& CharacterId : PoolIds)
		{
			Party.CharacterPool.Add(MakeMON188Character(CharacterId));
		}
		return Party;
	}

	FGridCombatHotbarBinding MakeMON188SpellBinding(FName SpellId, int32 SlotIndex)
	{
		FGridCombatHotbarBinding Binding;
		Binding.Reset(SlotIndex);
		Binding.ActionId = SpellId;
		Binding.SourcePolicy = EGridCombatActionSourcePolicy::Spell;
		Binding.SourceDefinitionId = SpellId;
		return Binding;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMagicMON188SingleCharacterRoundTripTest, "Grimrock.Magic.MON18.8.SingleCharacterRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMagicMON188SingleCharacterRoundTripTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMON188Tests;
	FGridPartyInventoryState Party = MakeMON188Party({ FGuid(18, 8, 1, 1) });
	Party.ActiveCharacters[0].KnownSpellIds = { FGridProductionSpellLibrary::ArcaneBoltId(), FGridProductionSpellLibrary::LesserHealId() };
	FString Error;
	TestTrue(TEXT("Direct durable Spellbook validates"), FGridSpellbookPersistence::ValidatePartySpellbooks(Party, Error));
	const FGridPartyInventoryState Restored = Party;
	TestTrue(TEXT("Arcane Bolt survives ordinary party-state copy"),
		Restored.ActiveCharacters[0].KnownSpellIds.Contains(FGridProductionSpellLibrary::ArcaneBoltId()));
	TestTrue(TEXT("Lesser Heal survives ordinary party-state copy"),
		Restored.ActiveCharacters[0].KnownSpellIds.Contains(FGridProductionSpellLibrary::LesserHealId()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMagicMON188MultipleCharactersRoundTripTest, "Grimrock.Magic.MON18.8.MultipleCharactersRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMagicMON188MultipleCharactersRoundTripTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMON188Tests;
	const FGuid MageId(18, 8, 2, 1);
	const FGuid PriestId(18, 8, 2, 2);
	const FGuid ReserveId(18, 8, 2, 3);
	FGridPartyInventoryState Party = MakeMON188Party({ MageId, PriestId }, { ReserveId });
	Party.ActiveCharacters[0].KnownSpellIds = { FGridProductionSpellLibrary::ArcaneBoltId(), FGridProductionSpellLibrary::HasteId() };
	Party.ActiveCharacters[1].KnownSpellIds = { FGridProductionSpellLibrary::LesserHealId() };

	FString Error;
	TestTrue(TEXT("Active and pooled durable Spellbooks validate"), FGridSpellbookPersistence::ValidatePartySpellbooks(Party, Error));
	const FGridPartyInventoryState Restored = Party;
	TestEqual(TEXT("Every character carries its own Spellbook state"), Restored.ActiveCharacters.Num() + Restored.CharacterPool.Num(), 3);
	TestTrue(TEXT("Reserve remains empty"), Restored.CharacterPool[0].KnownSpellIds.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMagicMON188UnknownDefinitionRejectedTest, "Grimrock.Magic.MON18.8.UnknownDefinitionRejected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMagicMON188UnknownDefinitionRejectedTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMON188Tests;
	FGridPartyInventoryState Party = MakeMON188Party({ FGuid(18, 8, 4, 1) });
	Party.ActiveCharacters[0].KnownSpellIds.Add(TEXT("Spell_RemovedContent"));
	FString Error;
	TestFalse(TEXT("Exact-match schema rejects unknown SpellId"), FGridSpellbookPersistence::ValidatePartySpellbooks(Party, Error));
	TestTrue(TEXT("Unknown definition rejection reports a reason"), !Error.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMagicMON188InvalidSpellIdRejectedTest, "Grimrock.Magic.MON18.8.InvalidSpellIdRejected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMagicMON188InvalidSpellIdRejectedTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMON188Tests;
	FGridPartyInventoryState Party = MakeMON188Party({ FGuid(18, 8, 5, 1) });
	Party.ActiveCharacters[0].KnownSpellIds.Add(NAME_None);
	FString Error;
	TestFalse(TEXT("NAME_None SpellId is rejected"), FGridSpellbookPersistence::ValidatePartySpellbooks(Party, Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMagicMON188DuplicateSpellRejectedTest, "Grimrock.Magic.MON18.8.DuplicateSpellRejected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMagicMON188DuplicateSpellRejectedTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMON188Tests;
	FGridPartyInventoryState Party = MakeMON188Party({ FGuid(18, 8, 6, 1) });
	Party.ActiveCharacters[0].KnownSpellIds = { FGridProductionSpellLibrary::ArcaneBoltId(), FGridProductionSpellLibrary::ArcaneBoltId() };
	FString Error;
	TestFalse(TEXT("Duplicate durable SpellId is rejected"), FGridSpellbookPersistence::ValidatePartySpellbooks(Party, Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMagicMON188DuplicateCharacterRejectedTest, "Grimrock.Magic.MON18.8.DuplicateCharacterRejected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMagicMON188DuplicateCharacterRejectedTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMON188Tests;
	const FGuid CharacterId(18, 8, 7, 1);
	FGridPartyInventoryState Party = MakeMON188Party({ CharacterId }, { CharacterId });
	FString Error;
	TestFalse(TEXT("Duplicate CharacterId across Active and Pool is rejected"), FGridSpellbookPersistence::ValidatePartySpellbooks(Party, Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMagicMON188DeterministicMutationTest, "Grimrock.Magic.MON18.8.DeterministicMutation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMagicMON188DeterministicMutationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMON188Tests;
	const FGuid CharacterId(18, 8, 8, 1);
	UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
	Inventory->PartyInventoryState = MakeMON188Party({ CharacterId });
	UGridPartySpellbookComponent* Spellbook = NewObject<UGridPartySpellbookComponent>();
	Spellbook->InitializeSpellbookComponent(Inventory);
	TestEqual(TEXT("Haste learns"), Spellbook->LearnSpell(CharacterId, FGridProductionSpellLibrary::HasteId()), EGridSpellbookMutationResult::Success);
	TestEqual(
		TEXT("Arcane Bolt learns"), Spellbook->LearnSpell(CharacterId, FGridProductionSpellLibrary::ArcaneBoltId()), EGridSpellbookMutationResult::Success);
	const TArray<FName>& Known = Inventory->PartyInventoryState.ActiveCharacters[0].KnownSpellIds;
	TestEqual(TEXT("Two durable spells"), Known.Num(), 2);
	if (Known.Num() == 2)
	{
		TestEqual(TEXT("Deterministic first SpellId"), Known[0], FGridProductionSpellLibrary::ArcaneBoltId());
		TestEqual(TEXT("Deterministic second SpellId"), Known[1], FGridProductionSpellLibrary::HasteId());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMagicMON188HotbarSpellRoundTripTest, "Grimrock.Magic.MON18.8.HotbarSpellRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMagicMON188HotbarSpellRoundTripTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMON188Tests;
	FGridPartyInventoryState Party = MakeMON188Party({ FGuid(18, 8, 9, 1) });
	Party.ActiveCharacters[0].KnownSpellIds.Add(FGridProductionSpellLibrary::ArcaneBoltId());
	Party.ActiveCharacters[0].CombatHotbarSlots[2] = MakeMON188SpellBinding(FGridProductionSpellLibrary::ArcaneBoltId(), 2);
	const FGridPartyInventoryState Restored = Party;
	TestTrue(TEXT("Spell remains known after direct durable restore"),
		Restored.ActiveCharacters[0].KnownSpellIds.Contains(FGridProductionSpellLibrary::ArcaneBoltId()));
	TestTrue(TEXT("Hotbar binding remains independent Spell source"),
		Restored.ActiveCharacters[0].CombatHotbarSlots[2].SourcePolicy == EGridCombatActionSourcePolicy::Spell);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMagicMON188HotbarWithoutKnowledgePreservedTest, "Grimrock.Magic.MON18.8.HotbarWithoutKnowledgePreserved",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMagicMON188HotbarWithoutKnowledgePreservedTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMON188Tests;
	FGridPartyInventoryState Party = MakeMON188Party({ FGuid(18, 8, 10, 1) });
	Party.ActiveCharacters[0].CombatHotbarSlots[0] = MakeMON188SpellBinding(FGridProductionSpellLibrary::ArcaneBoltId(), 0);
	TestFalse(
		TEXT("Hotbar does not teach the referenced spell"), Party.ActiveCharacters[0].KnownSpellIds.Contains(FGridProductionSpellLibrary::ArcaneBoltId()));
	TestTrue(TEXT("Spell hotbar binding remains structurally valid"), Party.ActiveCharacters[0].CombatHotbarSlots[0].IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMagicMON188SpellBindingIsNotItemDefinitionTest, "Grimrock.Magic.MON18.8.SpellBindingIsNotItemDefinition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMagicMON188SpellBindingIsNotItemDefinitionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMON188Tests;
	UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
	Inventory->PartyInventoryState = MakeMON188Party({ FGuid(18, 8, 11, 1) });
	Inventory->PartyInventoryState.ActiveCharacters[0].CombatHotbarSlots[4] = MakeMON188SpellBinding(FGridProductionSpellLibrary::ArcaneBoltId(), 4);

	int32 ResolverCallCount = 0;
	FName MissingDefinitionId = NAME_None;
	TestTrue(TEXT("Rehydration accepts Spell binding without item lookup"),
		Inventory->RehydrateOwnedItemDefinitions(
			[&ResolverCallCount](FName) -> UGridItemDefinitionAsset*
			{
				++ResolverCallCount;
				return nullptr;
			},
			MissingDefinitionId));
	TestEqual(TEXT("Spell SourceDefinitionId is never sent to item resolver"), ResolverCallCount, 0);
	TestTrue(TEXT("No missing ItemDefinition is reported for a Spell"), MissingDefinitionId.IsNone());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMagicMON188DiskRoundTripTest, "Grimrock.Magic.MON18.8.DiskRoundTrip", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMagicMON188DiskRoundTripTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMON188Tests;
	FMON188DiskSlot DiskSlot;
	UGrimrockPartySaveGame* Source = NewObject<UGrimrockPartySaveGame>();
	Source->PartyInventoryState = MakeMON188Party({ FGuid(18, 8, 12, 1) });
	Source->PartyInventoryState.ActiveCharacters[0].KnownSpellIds = { FGridProductionSpellLibrary::ArcaneBoltId(), FGridProductionSpellLibrary::HasteId(),
		FGridProductionSpellLibrary::LesserHealId() };
	Source->PartyInventoryState.ActiveCharacters[0].CombatHotbarSlots[1] = MakeMON188SpellBinding(FGridProductionSpellLibrary::LesserHealId(), 1);

	TestTrue(TEXT("Current-schema durable Spellbook writes to disk"), UGameplayStatics::SaveGameToSlot(Source, DiskSlot.SlotName, DiskSlot.UserIndex));
	UGrimrockPartySaveGame* Loaded = Cast<UGrimrockPartySaveGame>(UGameplayStatics::LoadGameFromSlot(DiskSlot.SlotName, DiskSlot.UserIndex));
	TestNotNull(TEXT("Temporary disk save reloads"), Loaded);
	if (!Loaded)
	{
		return false;
	}
	TestEqual(TEXT("Disk round trip uses current SaveVersion"), Loaded->SaveVersion, UGrimrockPartySaveGame::CurrentSaveVersion);
	TestTrue(TEXT("Loaded save is compatible"), Loaded->IsCompatible());
	TestEqual(TEXT("Three known spells survive in character state"), Loaded->PartyInventoryState.ActiveCharacters[0].KnownSpellIds.Num(), 3);
	TestTrue(TEXT("Spell hotbar survives beside durable knowledge"),
		Loaded->PartyInventoryState.ActiveCharacters[0].CombatHotbarSlots[1].SourcePolicy == EGridCombatActionSourcePolicy::Spell);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
