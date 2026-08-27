#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "RPG/RPGCharacterRulesLibrary.h"
#include "RPG/RPGClassProgressionTransactionService.h"
#include "RPG/RPGLevelUpService.h"
#include "RPGMON155TestHelpers.h"
#include "Save/GrimrockPartySaveGame.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07335ExperienceLevelContractTest,
	"Grimrock.TechnicalDebt.TD07_3_3_5.Characterization.ExperienceLevelContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07335ExperienceLevelContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	TestEqual(TEXT("Level 1 begins at zero XP"), URPGCharacterRulesLibrary::GetCumulativeExperienceRequiredForLevel(1), 0);
	TestEqual(TEXT("Level 2 begins at 1000 XP"), URPGCharacterRulesLibrary::GetCumulativeExperienceRequiredForLevel(2), 1000);
	TestEqual(TEXT("Level 3 begins at 3000 XP"), URPGCharacterRulesLibrary::GetCumulativeExperienceRequiredForLevel(3), 3000);
	TestEqual(TEXT("Level 4 begins at 6000 XP"), URPGCharacterRulesLibrary::GetCumulativeExperienceRequiredForLevel(4), 6000);

	TestEqual(TEXT("999 XP derives level 1"), URPGCharacterRulesLibrary::GetLevelForExperience(999), 1);
	TestEqual(TEXT("1000 XP derives level 2"), URPGCharacterRulesLibrary::GetLevelForExperience(1000), 2);
	TestEqual(TEXT("2999 XP derives level 2"), URPGCharacterRulesLibrary::GetLevelForExperience(2999), 2);
	TestEqual(TEXT("3000 XP derives level 3"), URPGCharacterRulesLibrary::GetLevelForExperience(3000), 3);

	TestTrue(TEXT("Level 3 and 3000 XP are consistent"), URPGCharacterRulesLibrary::IsLevelExperienceConsistent(3, 3000));
	TestFalse(TEXT("Stored level 2 and 3000 XP are inconsistent"), URPGCharacterRulesLibrary::IsLevelExperienceConsistent(2, 3000));
	TestFalse(TEXT("Stored level 3 and 1000 XP are inconsistent"), URPGCharacterRulesLibrary::IsLevelExperienceConsistent(3, 1000));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07335StoredLevelSynchronizationTest,
	"Grimrock.TechnicalDebt.TD07_3_3_5.Characterization.StoredLevelSynchronization",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07335StoredLevelSynchronizationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FMON155RuntimeStateGuard RuntimeGuard;

	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeMON155Inventory(1, 3000, ClassDefinition);
	FGridCharacterInventoryState& Character = Component->PartyInventoryState.ActiveCharacters[0];

	const int32 PreviousMaxHealth = Character.DerivedStats.MaxHealth;
	TestTrue(TEXT("Pending multi-level synchronization applies"), FRPGLevelUpService::ApplyPendingLevelUp(Component, 0, false));
	TestEqual(TEXT("Stored Level is synchronized from Experience"), Character.Level, 3);
	TestEqual(TEXT("Experience remains the source value"), Character.Experience, 3000);
	TestTrue(TEXT("Derived stats are rebuilt for the synchronized level"), Character.DerivedStats.MaxHealth > PreviousMaxHealth);

	Character.Level = 3;
	Character.Experience = 1000;
	const FRPGDerivedStats StatsBeforeRejectedDemotion = Character.DerivedStats;
	const FRPGCharacterResources ResourcesBeforeRejectedDemotion = Character.Resources;

	TestFalse(TEXT("Level-up service refuses to demote a stored level"), FRPGLevelUpService::ApplyPendingLevelUp(Component, 0, false));
	TestEqual(TEXT("Rejected demotion preserves stored Level"), Character.Level, 3);
	TestEqual(TEXT("Rejected demotion preserves Experience"), Character.Experience, 1000);
	TestEqual(TEXT("Rejected demotion preserves MaxHealth"), Character.DerivedStats.MaxHealth, StatsBeforeRejectedDemotion.MaxHealth);
	TestEqual(TEXT("Rejected demotion preserves CurrentHealth"), Character.Resources.CurrentHealth, ResourcesBeforeRejectedDemotion.CurrentHealth);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07335ProgressionMirrorBoundaryTest,
	"Grimrock.TechnicalDebt.TD07_3_3_5.Characterization.ProgressionMirrorBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07335ProgressionMirrorBoundaryTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FMON155RuntimeStateGuard RuntimeGuard;

	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeMON155Inventory(2, 1000, ClassDefinition);

	FRPGClassProgressionCommitResult CommitResult;
	TestTrue(TEXT("Choice A commits into runtime progression state"),
		FRPGClassProgressionTransactionService::TryCommitChoices(Component, 0, { TEXT("Choice_A") }, CommitResult));

	const FGridPartyInventoryState OrdinaryPartySnapshot = Component->PartyInventoryState;

	TArray<FRPGCharacterProgressionSaveState> SavedProgression;
	FText CaptureError;
	TestTrue(TEXT("Separate progression snapshot captures committed choices"),
		FRPGClassProgressionTransactionService::CapturePersistentState(Component->PartyInventoryState, SavedProgression, CaptureError));
	TestEqual(TEXT("One active character produces one separate progression record"), SavedProgression.Num(), 1);
	TestTrue(TEXT("Separate snapshot contains Choice A"),
		SavedProgression.Num() == 1 && SavedProgression[0].SelectedChoiceIds.Contains(TEXT("Choice_A")));

	FRPGClassProgressionTransactionService::ResetRuntimeState(Component);

	TArray<FName> ChoicesAfterRuntimeReset;
	TestTrue(TEXT("Runtime projection rebuilds from ordinary party state"),
		FRPGClassProgressionTransactionService::TryGetSelectedChoiceIds(Component, 0, ChoicesAfterRuntimeReset));
	TestTrue(TEXT("Ordinary party state alone does not retain committed Choice A"), ChoicesAfterRuntimeReset.IsEmpty());

	FText RestoreError;
	TestTrue(TEXT("Separate progression snapshot restores committed choices"),
		FRPGClassProgressionTransactionService::RestorePersistentState(OrdinaryPartySnapshot, SavedProgression, RestoreError));

	TArray<FName> ChoicesAfterRestore;
	TestTrue(TEXT("Restored runtime selection is readable"),
		FRPGClassProgressionTransactionService::TryGetSelectedChoiceIds(Component, 0, ChoicesAfterRestore));
	TestTrue(TEXT("Choice A returns only after the separate snapshot restore"), ChoicesAfterRestore.Contains(TEXT("Choice_A")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07335LevelUpProjectionRefreshTest,
	"Grimrock.TechnicalDebt.TD07_3_3_5.Characterization.LevelUpProjectionRefresh",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07335LevelUpProjectionRefreshTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FMON155RuntimeStateGuard RuntimeGuard;

	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeMON155Inventory(2, 1000, ClassDefinition);
	FGridCharacterInventoryState& Character = Component->PartyInventoryState.ActiveCharacters[0];

	FRPGClassProgressionCommitResult ChoiceAResult;
	TestTrue(TEXT("Choice A commits at level two"),
		FRPGClassProgressionTransactionService::TryCommitChoices(Component, 0, { TEXT("Choice_A") }, ChoiceAResult));

	Character.Experience = 3000;
	TestTrue(TEXT("XP threshold synchronizes stored level to three"), FRPGLevelUpService::ApplyPendingLevelUp(Component, 0, false));
	TestEqual(TEXT("Stored level becomes three"), Character.Level, 3);

	TArray<FName> SelectedAfterLevelUp;
	TestTrue(TEXT("Progression state remains readable after level-up refresh"),
		FRPGClassProgressionTransactionService::TryGetSelectedChoiceIds(Component, 0, SelectedAfterLevelUp));
	TestTrue(TEXT("Previously committed Choice A survives level-up projection refresh"), SelectedAfterLevelUp.Contains(TEXT("Choice_A")));

	int32 GrantedPoints = 0;
	int32 SpentPoints = 0;
	int32 RemainingPoints = 0;
	TestTrue(TEXT("Choice balance recomputes for the new level"),
		FRPGClassProgressionTransactionService::TryGetChoicePointBalance(Component, 0, GrantedPoints, SpentPoints, RemainingPoints));
	TestEqual(TEXT("Level three grants two total choice points"), GrantedPoints, 2);
	TestEqual(TEXT("Choice A still spends one point"), SpentPoints, 1);
	TestEqual(TEXT("One new point is available after level-up"), RemainingPoints, 1);

	FRPGClassProgressionCommitResult ChoiceBResult;
	TestTrue(TEXT("Choice B becomes commit-able after level-up while Choice A prerequisite remains"),
		FRPGClassProgressionTransactionService::TryCommitChoices(Component, 0, { TEXT("Choice_B") }, ChoiceBResult));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
