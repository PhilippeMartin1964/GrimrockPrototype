#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "RPG/RPGCharacterRulesLibrary.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGExperienceRewardService.h"
#include "RPG/RPGLevelUpService.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Save/GrimrockPartySaveGame.h"

namespace
{
	URPGClassAsset* MakeMON153Class(UObject* Outer, FName ClassId = TEXT("MON153_Fighter"))
	{
		URPGClassAsset* ClassDefinition = NewObject<URPGClassAsset>(Outer);
		ClassDefinition->ClassId = ClassId;
		ClassDefinition->DisplayName = FText::FromString(TEXT("MON15.3 Fighter"));
		ClassDefinition->BaseAttributes = FRPGAttributes{ 12, 14, 12, 10, 10, 10 };
		ClassDefinition->HealthAtLevelOne = 20;
		ClassDefinition->HealthPerLevel = 5;
		ClassDefinition->ManaAtLevelOne = 10;
		ClassDefinition->ManaPerLevel = 3;
		ClassDefinition->BasePhysicalArmor = 2;
		ClassDefinition->BaseMagicalArmor = 1;
		return ClassDefinition;
	}

	FGridCharacterInventoryState MakeMON153Character(URPGClassAsset* ClassDefinition, int32 Level, int32 Experience)
	{
		FGridCharacterInventoryState Character;
		Character.CharacterId = FGuid::NewGuid();
		Character.DisplayName = FText::FromString(TEXT("MON15.3 Hero"));
		Character.ClassId = ClassDefinition ? ClassDefinition->ClassId : NAME_None;
		Character.ClassDisplayName = ClassDefinition ? ClassDefinition->DisplayName : FText::GetEmpty();
		Character.ClassDefinition = ClassDefinition;
		Character.Level = Level;
		Character.Experience = Experience;
		Character.Attributes = ClassDefinition ? ClassDefinition->BaseAttributes : FRPGAttributes{};
		Character.DerivedStats = URPGCharacterRulesLibrary::CalculateDerivedStats(Character.Attributes, ClassDefinition, Level);
		Character.Resources = URPGCharacterRulesLibrary::InitializeCharacterResources(Character.DerivedStats, ClassDefinition);
		Character.InventorySlots.SetNum(1);
		Character.CombatHotbarSlots.SetNum(FGridCombatHotbarBinding::SlotCount);
		for (int32 SlotIndex = 0; SlotIndex < Character.CombatHotbarSlots.Num(); ++SlotIndex)
		{
			Character.CombatHotbarSlots[SlotIndex].Reset(SlotIndex);
		}
		return Character;
	}

	UGridPartyInventoryComponent* MakeMON153Party(URPGClassAsset*& OutClassDefinition, int32 Level, int32 Experience)
	{
		UGridPartyInventoryComponent* Component = NewObject<UGridPartyInventoryComponent>();
		OutClassDefinition = MakeMON153Class(Component);
		Component->PartyInventoryState.ActiveCharacters.Add(MakeMON153Character(OutClassDefinition, Level, Experience));
		Component->PartyInventoryState.ActiveEquipment.SetNum(1);
		Component->PartyInventoryState.SelectedCharacterIndex = 0;
		Component->PartyInventoryState.MaxActiveCharacters = 6;
		Component->PartyInventoryState.bInitialCharacterCreationCompleted = true;
		return Component;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON153SingleLevelResourcePolicyTest, "Grimrock.RPG.MON15.3.SingleLevelResourcePolicy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON153SingleLevelResourcePolicyTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeMON153Party(ClassDefinition, 1, 1000);
	FGridCharacterInventoryState& Character = Component->PartyInventoryState.ActiveCharacters[0];

	TestEqual(TEXT("Fixture level-one max health"), Character.DerivedStats.MaxHealth, 21);
	TestEqual(TEXT("Fixture level-one max mana"), Character.DerivedStats.MaxMana, 10);

	Character.Resources.CurrentHealth = 15; // six damage taken.
	Character.Resources.CurrentMana = 4;    // six mana spent.

	TestTrue(TEXT("The pending level is applied"), FRPGLevelUpService::ApplyPendingLevelUp(Component, 0));
	TestEqual(TEXT("Stored level becomes two"), Character.Level, 2);
	TestEqual(TEXT("XP is never consumed by level-up"), Character.Experience, 1000);
	TestEqual(TEXT("HealthPerLevel and Constitution are applied"), Character.DerivedStats.MaxHealth, 27);
	TestEqual(TEXT("Absolute damage deficit is preserved"), Character.Resources.CurrentHealth, 21);
	TestEqual(TEXT("ManaPerLevel is applied"), Character.DerivedStats.MaxMana, 13);
	TestEqual(TEXT("Absolute mana deficit is preserved"), Character.Resources.CurrentMana, 7);
	TestEqual(TEXT("Physical armor is recalculated from the class"), Character.Resources.CurrentPhysicalArmor, 2);
	TestEqual(TEXT("Magical armor is recalculated from the class"), Character.Resources.CurrentMagicalArmor, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGMON153MultiLevelTransactionTest, "Grimrock.RPG.MON15.3.MultiLevelTransaction", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON153MultiLevelTransactionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeMON153Party(ClassDefinition, 1, 6000);
	FGridCharacterInventoryState& Character = Component->PartyInventoryState.ActiveCharacters[0];

	int32 EventCount = 0;
	int32 EventPreviousLevel = INDEX_NONE;
	int32 EventNewLevel = INDEX_NONE;
	int32 EventLevelsGained = 0;
	const FDelegateHandle EventHandle = FRPGLevelUpService::OnCharacterLevelUpApplied().AddLambda(
		[&EventCount, &EventPreviousLevel, &EventNewLevel, &EventLevelsGained](int32 CharacterIndex, int32 PreviousLevel, int32 NewLevel, int32 LevelsGained)
		{
			check(CharacterIndex == 0);
			++EventCount;
			EventPreviousLevel = PreviousLevel;
			EventNewLevel = NewLevel;
			EventLevelsGained = LevelsGained;
		});

	TestTrue(TEXT("One transaction applies all pending levels"), FRPGLevelUpService::ApplyPendingLevelUp(Component, 0));
	FRPGLevelUpService::OnCharacterLevelUpApplied().Remove(EventHandle);

	TestEqual(TEXT("6000 cumulative XP resolves to level four"), Character.Level, 4);
	TestEqual(TEXT("Three levels are gained in one transaction"), EventLevelsGained, 3);
	TestEqual(TEXT("Exactly one level-up event is emitted"), EventCount, 1);
	TestEqual(TEXT("Event previous level"), EventPreviousLevel, 1);
	TestEqual(TEXT("Event new level"), EventNewLevel, 4);
	TestEqual(TEXT("Level-four max health is calculated directly"), Character.DerivedStats.MaxHealth, 39);
	TestEqual(TEXT("Level-four max mana is calculated directly"), Character.DerivedStats.MaxMana, 19);
	TestEqual(TEXT("A full-health character stays full after growth"), Character.Resources.CurrentHealth, 39);
	TestEqual(TEXT("A full-mana character stays full after growth"), Character.Resources.CurrentMana, 19);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGMON153DeadCharacterTest, "Grimrock.RPG.MON15.3.DeadCharacterRemainsDead", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON153DeadCharacterTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeMON153Party(ClassDefinition, 1, 1000);
	FGridCharacterInventoryState& Character = Component->PartyInventoryState.ActiveCharacters[0];
	Character.Resources.CurrentHealth = 0;

	TestTrue(TEXT("A dead character may still resolve its pending level"), FRPGLevelUpService::ApplyPendingLevelUp(Component, 0));
	TestEqual(TEXT("The level is updated"), Character.Level, 2);
	TestEqual(TEXT("Level-up never resurrects a dead character"), Character.Resources.CurrentHealth, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGMON153AtomicFailureTest, "Grimrock.RPG.MON15.3.AtomicFailure", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON153AtomicFailureTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeMON153Party(ClassDefinition, 1, 1000);
	FGridCharacterInventoryState& Character = Component->PartyInventoryState.ActiveCharacters[0];
	Character.ClassId = TEXT("Wrong_Class_Id");
	Character.Resources.CurrentHealth = 13;
	Character.Resources.CurrentMana = 2;

	const int32 LevelBefore = Character.Level;
	const FRPGDerivedStats StatsBefore = Character.DerivedStats;
	const FRPGCharacterResources ResourcesBefore = Character.Resources;
	TestFalse(TEXT("A mismatched class definition rejects the transaction"), FRPGLevelUpService::ApplyPendingLevelUp(Component, 0));
	TestEqual(TEXT("Rejected transaction preserves Level"), Character.Level, LevelBefore);
	TestEqual(TEXT("Rejected transaction preserves MaxHealth"), Character.DerivedStats.MaxHealth, StatsBefore.MaxHealth);
	TestEqual(TEXT("Rejected transaction preserves CurrentHealth"), Character.Resources.CurrentHealth, ResourcesBefore.CurrentHealth);
	TestEqual(TEXT("Rejected transaction preserves MaxMana"), Character.DerivedStats.MaxMana, StatsBefore.MaxMana);
	TestEqual(TEXT("Rejected transaction preserves CurrentMana"), Character.Resources.CurrentMana, ResourcesBefore.CurrentMana);

	Character.ClassId = ClassDefinition->ClassId;
	Character.Level = 3;
	Character.Experience = 1000; // XP resolves to level two: never demote.
	const FRPGDerivedStats NoDemotionStatsBefore = Character.DerivedStats;
	TestFalse(TEXT("MON15.3 never demotes an inconsistent stored level"), FRPGLevelUpService::ApplyPendingLevelUp(Component, 0));
	TestEqual(TEXT("Would-demote state preserves Level"), Character.Level, 3);
	TestEqual(TEXT("Would-demote state preserves stats"), Character.DerivedStats.MaxHealth, NoDemotionStatsBefore.MaxHealth);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON153ExperienceIntegrationTest, "Grimrock.RPG.MON15.3.ExperienceAwardIntegration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON153ExperienceIntegrationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeMON153Party(ClassDefinition, 1, 999);
	FGridCharacterInventoryState& Character = Component->PartyInventoryState.ActiveCharacters[0];

	FGridItemInstance SentinelItem;
	SentinelItem.RuntimeObjectId = FGuid::NewGuid();
	SentinelItem.ItemDefinitionId = TEXT("MON153_SentinelItem");
	SentinelItem.Quantity = 2;
	SentinelItem.OwnerType = EGridItemOwnerType::CharacterInventory;
	SentinelItem.OwnerGuid = Character.CharacterId;
	SentinelItem.OwnerCharacterIndex = 0;
	Character.InventorySlots[0].bOccupied = true;
	Character.InventorySlots[0].Item = SentinelItem;
	const FGuid SentinelRuntimeId = SentinelItem.RuntimeObjectId;

	int32 LevelEventCount = 0;
	const FDelegateHandle LevelEventHandle = FRPGLevelUpService::OnCharacterLevelUpApplied().AddLambda(
		[&LevelEventCount](int32, int32, int32, int32)
		{
			++LevelEventCount;
		});

	TestEqual(TEXT("One XP is awarded"), FRPGExperienceRewardService::AwardToActiveParty(Component, 1), 1);
	FRPGLevelUpService::OnCharacterLevelUpApplied().Remove(LevelEventHandle);

	TestEqual(TEXT("XP crosses the level-two threshold"), Character.Experience, 1000);
	TestEqual(TEXT("The same XP transaction applies level two"), Character.Level, 2);
	TestEqual(TEXT("Exactly one level-up event is emitted"), LevelEventCount, 1);
	TestTrue(TEXT("Inventory runtime identity is preserved"), Character.InventorySlots[0].Item.RuntimeObjectId == SentinelRuntimeId);
	TestEqual(TEXT("Inventory quantity is preserved"), Character.InventorySlots[0].Item.Quantity, 2);
	TestTrue(TEXT("Hotbar remains present"), Character.CombatHotbarSlots.Num() == FGridCombatHotbarBinding::SlotCount);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGMON153PersistenceTest, "Grimrock.RPG.MON15.3.PersistenceState", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON153PersistenceTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* SourceComponent = MakeMON153Party(ClassDefinition, 1, 6000);
	TestTrue(TEXT("The source applies the pending multi-level transaction"), FRPGLevelUpService::ApplyPendingLevelUp(SourceComponent, 0));

	UGrimrockPartySaveGame* SaveGame = NewObject<UGrimrockPartySaveGame>();
	SaveGame->PartyInventoryState = SourceComponent->PartyInventoryState;
	TestEqual(TEXT("New save containers use the current save version"), SaveGame->SaveVersion, UGrimrockPartySaveGame::CurrentSaveVersion);

	UGridPartyInventoryComponent* RestoredComponent = NewObject<UGridPartyInventoryComponent>();
	FText RestoreError;
	TestTrue(TEXT("The existing party save state restores"), RestoredComponent->RestorePartyInventoryState(SaveGame->PartyInventoryState, RestoreError));
	TestTrue(TEXT("Restore produces no error"), RestoreError.IsEmpty());

	const FGridCharacterInventoryState& Restored = RestoredComponent->PartyInventoryState.ActiveCharacters[0];
	TestEqual(TEXT("Restored level is exact"), Restored.Level, 4);
	TestEqual(TEXT("Restored cumulative XP is exact"), Restored.Experience, 6000);
	TestEqual(TEXT("Restored max health is exact"), Restored.DerivedStats.MaxHealth, 39);
	TestEqual(TEXT("Restored max mana is exact"), Restored.DerivedStats.MaxMana, 19);
	TestFalse(TEXT("No duplicate level-up remains pending after restore"), FRPGLevelUpService::ApplyPendingLevelUp(RestoredComponent, 0));
	return true;
}

#endif
