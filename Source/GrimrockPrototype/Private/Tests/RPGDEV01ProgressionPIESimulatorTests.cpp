#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "HAL/IConsoleManager.h"
#include "RPG/RPGCharacterRulesLibrary.h"
#include "RPG/RPGClassProgressionTransactionService.h"
#include "RPG/RPGProgressionPIESimulator.h"
#include "RPGMON155TestHelpers.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGDEV01LevelTwentySimulationTest,
	"Grimrock.RPG.DEV01.ProgressionSimulator.Level20",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGDEV01LevelTwentySimulationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FMON155RuntimeStateGuard Guard;

	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeMON155Inventory(1, 0, ClassDefinition);
	FGridCharacterInventoryState& Character = Component->PartyInventoryState.ActiveCharacters[0];
	Character.LastAcknowledgedLevel = 1;

	int32 LevelEventCount = 0;
	int32 EventPreviousLevel = INDEX_NONE;
	int32 EventNewLevel = INDEX_NONE;
	int32 EventLevelsGained = 0;
	const FDelegateHandle Handle = FRPGLevelUpService::OnCharacterLevelUpApplied().AddLambda(
		[&](int32 CharacterIndex, int32 PreviousLevel, int32 NewLevel, int32 LevelsGained)
		{
			TestEqual(TEXT("Level event targets selected character"), CharacterIndex, 0);
			++LevelEventCount;
			EventPreviousLevel = PreviousLevel;
			EventNewLevel = NewLevel;
			EventLevelsGained = LevelsGained;
		});

	FText Feedback;
	TestTrue(TEXT("Selected character can jump from level one to level twenty"),
		FRPGProgressionPIESimulator::TrySetSelectedCharacterLevel(Component, 20, Feedback));
	FRPGLevelUpService::OnCharacterLevelUpApplied().Remove(Handle);

	TestEqual(TEXT("Stored level reaches twenty"), Character.Level, 20);
	TestEqual(TEXT("Level twenty uses the canonical cumulative XP threshold"), Character.Experience,
		URPGCharacterRulesLibrary::GetCumulativeExperienceRequiredForLevel(20));
	TestEqual(TEXT("Level twenty threshold is 190000 XP"), Character.Experience, 190000);
	TestEqual(TEXT("Non-modal Level-Up is acknowledged immediately"), Character.LastAcknowledgedLevel, 20);
	TestEqual(TEXT("Exactly one canonical level event is emitted"), LevelEventCount, 1);
	TestEqual(TEXT("Event previous level"), EventPreviousLevel, 1);
	TestEqual(TEXT("Event new level"), EventNewLevel, 20);
	TestEqual(TEXT("Nineteen levels are gained"), EventLevelsGained, 19);
	TestFalse(TEXT("Success feedback is not empty"), Feedback.IsEmpty());

	int32 GrantedPoints = 0;
	int32 SpentPoints = 0;
	int32 RemainingPoints = 0;
	TestTrue(TEXT("Class progression projection remains readable after the simulated level-up"),
		FRPGClassProgressionTransactionService::TryGetChoicePointBalance(
			Component, 0, GrantedPoints, SpentPoints, RemainingPoints));
	TestEqual(TEXT("Fixture grants four progression points by level twenty"), GrantedPoints, 4);
	TestEqual(TEXT("No progression choice was invented"), SpentPoints, 0);
	TestEqual(TEXT("All granted points remain available"), RemainingPoints, 4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGDEV01InvalidTargetTest,
	"Grimrock.RPG.DEV01.ProgressionSimulator.RejectInvalidTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGDEV01InvalidTargetTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FMON155RuntimeStateGuard Guard;

	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeMON155Inventory(1, 0, ClassDefinition);
	FGridCharacterInventoryState& Character = Component->PartyInventoryState.ActiveCharacters[0];
	Character.LastAcknowledgedLevel = 1;

	FText Feedback;
	TestFalse(TEXT("Current level is rejected as a no-op"),
		FRPGProgressionPIESimulator::TrySetSelectedCharacterLevel(Component, 1, Feedback));
	TestEqual(TEXT("No-op preserves level"), Character.Level, 1);
	TestEqual(TEXT("No-op preserves XP"), Character.Experience, 0);

	TestFalse(TEXT("Level zero is rejected"),
		FRPGProgressionPIESimulator::TrySetSelectedCharacterLevel(Component, 0, Feedback));
	TestFalse(TEXT("Level twenty-one is rejected"),
		FRPGProgressionPIESimulator::TrySetSelectedCharacterLevel(Component, 21, Feedback));
	TestEqual(TEXT("Invalid targets preserve level"), Character.Level, 1);
	TestEqual(TEXT("Invalid targets preserve XP"), Character.Experience, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGDEV01SuccessiveJumpTest,
	"Grimrock.RPG.DEV01.ProgressionSimulator.SuccessiveNonModalJumps",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGDEV01SuccessiveJumpTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FMON155RuntimeStateGuard Guard;

	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeMON155Inventory(1, 0, ClassDefinition);
	FGridCharacterInventoryState& Character = Component->PartyInventoryState.ActiveCharacters[0];
	Character.LastAcknowledgedLevel = 1;

	FText Feedback;
	TestTrue(TEXT("First jump reaches level two without a modal acknowledgement step"),
		FRPGProgressionPIESimulator::TrySetSelectedCharacterLevel(Component, 2, Feedback));
	TestEqual(TEXT("First jump auto-acknowledges level two"), Character.LastAcknowledgedLevel, 2);

	TestTrue(TEXT("Second jump can immediately continue to level six"),
		FRPGProgressionPIESimulator::TrySetSelectedCharacterLevel(Component, 6, Feedback));
	TestEqual(TEXT("Second jump reaches level six"), Character.Level, 6);
	TestEqual(TEXT("Second jump auto-acknowledges level six"), Character.LastAcknowledgedLevel, 6);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGDEV01RollbackTest,
	"Grimrock.RPG.DEV01.ProgressionSimulator.RollbackOnLevelUpFailure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGDEV01RollbackTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FMON155RuntimeStateGuard Guard;

	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeMON155Inventory(1, 0, ClassDefinition);
	FGridCharacterInventoryState& Character = Component->PartyInventoryState.ActiveCharacters[0];
	Character.LastAcknowledgedLevel = 1;
	Character.ClassId = TEXT("Wrong_Class_Id");

	AddExpectedError(TEXT("Reason=InvalidClassDefinition"), EAutomationExpectedErrorFlags::Contains, 1);

	FText Feedback;
	TestFalse(TEXT("Canonical Level-Up rejection propagates to the simulator"),
		FRPGProgressionPIESimulator::TrySetSelectedCharacterLevel(Component, 2, Feedback));
	TestEqual(TEXT("Rejected simulation restores previous XP"), Character.Experience, 0);
	TestEqual(TEXT("Rejected simulation preserves previous level"), Character.Level, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGDEV01ConsoleCommandRegistrationTest,
	"Grimrock.RPG.DEV01.ConsoleCommand.Registered",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGDEV01ConsoleCommandRegistrationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
#if !UE_BUILD_SHIPPING
	TestNotNull(
		TEXT("PIE progression console command is registered"),
		IConsoleManager::Get().FindConsoleObject(TEXT("Grimrock.RPG.SetSelectedLevel")));
#else
	AddInfo(TEXT("Shipping build intentionally omits the RPG-DEV01 console command."));
#endif
	return true;
}

#endif
