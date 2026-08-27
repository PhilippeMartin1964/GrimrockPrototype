#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "RPG/RPGCharacterRulesLibrary.h"
#include "Runtime/GridInventoryTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGMON151ProgressionCurveTest, "Grimrock.RPG.MON15.1.ProgressionCurve", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON151ProgressionCurveTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Minimum level is one"), URPGCharacterRulesLibrary::GetMinimumLevel(), 1);
	TestEqual(TEXT("Maximum level is twenty"), URPGCharacterRulesLibrary::GetMaximumLevel(), 20);
	TestEqual(TEXT("Level one starts at zero XP"), URPGCharacterRulesLibrary::GetCumulativeExperienceRequiredForLevel(1), 0);
	TestEqual(TEXT("Level two starts at 1000 XP"), URPGCharacterRulesLibrary::GetCumulativeExperienceRequiredForLevel(2), 1000);
	TestEqual(TEXT("Level three starts at 3000 XP"), URPGCharacterRulesLibrary::GetCumulativeExperienceRequiredForLevel(3), 3000);
	TestEqual(TEXT("Level four starts at 6000 XP"), URPGCharacterRulesLibrary::GetCumulativeExperienceRequiredForLevel(4), 6000);
	TestEqual(TEXT("Level twenty starts at 190000 XP"), URPGCharacterRulesLibrary::GetCumulativeExperienceRequiredForLevel(20), 190000);

	int32 PreviousThreshold = URPGCharacterRulesLibrary::GetCumulativeExperienceRequiredForLevel(1);
	for (int32 Level = 2; Level <= URPGCharacterRulesLibrary::GetMaximumLevel(); ++Level)
	{
		const int32 Threshold = URPGCharacterRulesLibrary::GetCumulativeExperienceRequiredForLevel(Level);
		TestTrue(*FString::Printf(TEXT("Level %d threshold is strictly greater than level %d"), Level, Level - 1), Threshold > PreviousThreshold);
		PreviousThreshold = Threshold;
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGMON151LevelFromExperienceTest, "Grimrock.RPG.MON15.1.LevelFromExperience", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON151LevelFromExperienceTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Negative XP normalizes to level one"), URPGCharacterRulesLibrary::GetLevelForExperience(-1), 1);
	TestEqual(TEXT("Zero XP is level one"), URPGCharacterRulesLibrary::GetLevelForExperience(0), 1);
	TestEqual(TEXT("One XP before level two remains level one"), URPGCharacterRulesLibrary::GetLevelForExperience(999), 1);
	TestEqual(TEXT("Exact level two threshold returns level two"), URPGCharacterRulesLibrary::GetLevelForExperience(1000), 2);
	TestEqual(TEXT("One XP after level two remains level two"), URPGCharacterRulesLibrary::GetLevelForExperience(1001), 2);
	TestEqual(TEXT("One XP before level three remains level two"), URPGCharacterRulesLibrary::GetLevelForExperience(2999), 2);
	TestEqual(TEXT("Exact level three threshold returns level three"), URPGCharacterRulesLibrary::GetLevelForExperience(3000), 3);
	TestEqual(TEXT("One XP before level four remains level three"), URPGCharacterRulesLibrary::GetLevelForExperience(5999), 3);
	TestEqual(TEXT("Exact level four threshold returns level four"), URPGCharacterRulesLibrary::GetLevelForExperience(6000), 4);
	TestEqual(TEXT("Exact maximum threshold returns maximum level"), URPGCharacterRulesLibrary::GetLevelForExperience(190000), 20);
	TestEqual(TEXT("XP above the maximum threshold remains maximum level"), URPGCharacterRulesLibrary::GetLevelForExperience(250000), 20);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGMON151ProgressionBoundariesTest, "Grimrock.RPG.MON15.1.ProgressionBoundaries", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON151ProgressionBoundariesTest::RunTest(const FString& Parameters)
{
	const int32 MaximumExperience = URPGCharacterRulesLibrary::GetCumulativeExperienceRequiredForLevel(URPGCharacterRulesLibrary::GetMaximumLevel());

	TestEqual(TEXT("Negative XP clamps to zero"), URPGCharacterRulesLibrary::NormalizeExperience(-500), 0);
	TestEqual(
		TEXT("XP above the cap clamps to the maximum threshold"), URPGCharacterRulesLibrary::NormalizeExperience(MaximumExperience + 50000), MaximumExperience);
	TestEqual(TEXT("Invalid low level clamps to the level one threshold"), URPGCharacterRulesLibrary::GetCumulativeExperienceRequiredForLevel(-10), 0);
	TestEqual(
		TEXT("Invalid high level clamps to the maximum threshold"), URPGCharacterRulesLibrary::GetCumulativeExperienceRequiredForLevel(999), MaximumExperience);

	TestEqual(TEXT("Level one begins with zero XP in the current level"), URPGCharacterRulesLibrary::GetExperienceInCurrentLevel(0), 0);
	TestEqual(TEXT("Level one needs 1000 XP for level two"), URPGCharacterRulesLibrary::GetExperienceRemainingToNextLevel(0), 1000);
	TestEqual(TEXT("Exact level two threshold resets in-level XP"), URPGCharacterRulesLibrary::GetExperienceInCurrentLevel(1000), 0);
	TestEqual(TEXT("One XP after level two is one XP into the level"), URPGCharacterRulesLibrary::GetExperienceInCurrentLevel(1001), 1);
	TestEqual(TEXT("One XP after level two leaves 1999 XP to level three"), URPGCharacterRulesLibrary::GetExperienceRemainingToNextLevel(1001), 1999);
	TestEqual(TEXT("Maximum level has no remaining XP requirement"), URPGCharacterRulesLibrary::GetExperienceRemainingToNextLevel(MaximumExperience), 0);
	TestEqual(
		TEXT("Maximum level stores no overflow XP inside the capped level"), URPGCharacterRulesLibrary::GetExperienceInCurrentLevel(MaximumExperience + 1), 0);

	TestTrue(TEXT("Level twenty is detected as maximum"), URPGCharacterRulesLibrary::IsMaximumLevel(20));
	TestFalse(TEXT("Level nineteen is not maximum"), URPGCharacterRulesLibrary::IsMaximumLevel(19));
	TestFalse(TEXT("An invalid level above the cap is not a valid maximum level"), URPGCharacterRulesLibrary::IsMaximumLevel(21));

	TestTrue(TEXT("Level one and zero XP are coherent"), URPGCharacterRulesLibrary::IsLevelExperienceConsistent(1, 0));
	TestTrue(TEXT("Level two and its exact threshold are coherent"), URPGCharacterRulesLibrary::IsLevelExperienceConsistent(2, 1000));
	TestFalse(TEXT("A stale level is detected from cumulative XP"), URPGCharacterRulesLibrary::IsLevelExperienceConsistent(1, 1000));
	TestFalse(TEXT("Negative raw XP is inconsistent"), URPGCharacterRulesLibrary::IsLevelExperienceConsistent(1, -1));
	TestFalse(TEXT("Raw XP above the cap is inconsistent"), URPGCharacterRulesLibrary::IsLevelExperienceConsistent(20, MaximumExperience + 1));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON151ExistingCharacterStateTest, "Grimrock.RPG.MON15.1.ExistingCharacterState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON151ExistingCharacterStateTest::RunTest(const FString& Parameters)
{
	FGridCharacterInventoryState Character;
	Character.CharacterId = FGuid::NewGuid();
	Character.DisplayName = FText::FromString(TEXT("MON15 Existing Hero"));
	Character.Level = 3;
	Character.Experience = 3500;
	Character.Attributes = FRPGAttributes{ 16, 12, 14, 10, 10, 10 };
	Character.DerivedStats.MaxHealth = 42;
	Character.Resources.CurrentHealth = 31;
	Character.DerivedStats.MaxMana = 12;
	Character.Resources.CurrentMana = 7;

	FGridInventorySlot InventorySlot;
	InventorySlot.bOccupied = true;
	InventorySlot.Item.RuntimeObjectId = FGuid::NewGuid();
	InventorySlot.Item.ItemDefinitionId = TEXT("MON15_TestItem");
	InventorySlot.Item.Quantity = 2;
	Character.InventorySlots.Add(InventorySlot);

	FGridCombatHotbarBinding HotbarBinding;
	HotbarBinding.SlotIndex = 0;
	HotbarBinding.ActionId = TEXT("Attack_Unarmed");
	HotbarBinding.SourcePolicy = EGridCombatActionSourcePolicy::Universal;
	Character.CombatHotbarSlots.Add(HotbarBinding);

	const int32 OriginalLevel = Character.Level;
	const int32 OriginalExperience = Character.Experience;
	const FRPGAttributes OriginalAttributes = Character.Attributes;
	const FRPGDerivedStats OriginalDerivedStats = Character.DerivedStats;
	const FRPGCharacterResources OriginalResources = Character.Resources;
	const FGuid OriginalItemRuntimeId = Character.InventorySlots[0].Item.RuntimeObjectId;
	const int32 OriginalItemQuantity = Character.InventorySlots[0].Item.Quantity;
	const FName OriginalHotbarActionId = Character.CombatHotbarSlots[0].ActionId;

	TestTrue(TEXT("Existing serialized Level and Experience are coherent"),
		URPGCharacterRulesLibrary::IsLevelExperienceConsistent(Character.Level, Character.Experience));
	TestEqual(TEXT("Level can be reconstructed from the existing Experience"), URPGCharacterRulesLibrary::GetLevelForExperience(Character.Experience),
		Character.Level);
	TestEqual(TEXT("Existing character is 500 XP into level three"), URPGCharacterRulesLibrary::GetExperienceInCurrentLevel(Character.Experience), 500);
	TestEqual(
		TEXT("Existing character needs 2500 XP for level four"), URPGCharacterRulesLibrary::GetExperienceRemainingToNextLevel(Character.Experience), 2500);

	TestEqual(TEXT("Pure helpers do not change the stored level"), Character.Level, OriginalLevel);
	TestEqual(TEXT("Pure helpers do not change stored cumulative XP"), Character.Experience, OriginalExperience);
	TestEqual(TEXT("Pure helpers do not change Strength"), Character.Attributes.Strength, OriginalAttributes.Strength);
	TestEqual(TEXT("Pure helpers do not change current health"), Character.Resources.CurrentHealth, OriginalResources.CurrentHealth);
	TestEqual(TEXT("Pure helpers do not change inventory size"), Character.InventorySlots.Num(), 1);
	TestTrue(TEXT("Pure helpers do not change inventory runtime identity"), Character.InventorySlots[0].Item.RuntimeObjectId == OriginalItemRuntimeId);
	TestEqual(TEXT("Pure helpers do not change inventory quantity"), Character.InventorySlots[0].Item.Quantity, OriginalItemQuantity);
	TestEqual(TEXT("Pure helpers do not change hotbar size"), Character.CombatHotbarSlots.Num(), 1);
	TestTrue(TEXT("Pure helpers do not change hotbar action identity"), Character.CombatHotbarSlots[0].ActionId == OriginalHotbarActionId);

	FGridCharacterInventoryState InconsistentCharacter = Character;
	InconsistentCharacter.Level = 2;
	TestFalse(TEXT("A mismatched existing character state is detected"),
		URPGCharacterRulesLibrary::IsLevelExperienceConsistent(InconsistentCharacter.Level, InconsistentCharacter.Experience));

	return true;
}

#endif
