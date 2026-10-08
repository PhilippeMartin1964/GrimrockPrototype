#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "RPGMON155TestHelpers.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGMON155CatalogUnlockTest, "Grimrock.RPG.MON15.5.CombatCatalogUnlock", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON155CatalogUnlockTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FMON155RuntimeStateGuard RuntimeGuard;
	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeMON155Inventory(2, 1000, ClassDefinition);
	TestTrue(TEXT("Initial progression projection is built"), FRPGClassProgressionTransactionService::RefreshCharacterProjection(Component, 0));

	FGridAvailableCombatAction Available = BuildMON155ChoiceActionAvailability(Component, ClassDefinition);
	TestFalse(TEXT("Choice-gated action starts locked"), Available.bEnabled);
	TestTrue(TEXT("The lock is a missing requirement"), Available.AvailabilityReason == EGridCombatActionAvailabilityReason::MissingRequirement);

	FRPGClassProgressionCommitResult Result;
	TestTrue(TEXT("Choice A commits"), FRPGClassProgressionTransactionService::TryCommitChoices(Component, 0, { TEXT("Choice_A") }, Result));
	Available = BuildMON155ChoiceActionAvailability(Component, ClassDefinition);
	TestTrue(TEXT("Committed Choice A unlocks the same catalog action"), Available.bEnabled);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON155LevelUpNotificationSourceTest, "Grimrock.RPG.MON15.5.LevelUpNotificationSource",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON155LevelUpNotificationSourceTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FMON155RuntimeStateGuard RuntimeGuard;
	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeMON155Inventory(1, 1000, ClassDefinition);

	int32 EventCount = 0;
	UGridPartyInventoryComponent* EventComponent = nullptr;
	int32 EventCharacterIndex = INDEX_NONE;
	int32 EventPreviousLevel = 0;
	int32 EventNewLevel = 0;
	int32 EventLevelsGained = 0;
	const FDelegateHandle Handle = FRPGLevelUpService::OnCharacterLevelUpAppliedWithSource().AddLambda(
		[&](UGridPartyInventoryComponent* SourceComponent, int32 CharacterIndex, int32 PreviousLevel, int32 NewLevel, int32 LevelsGained)
		{
			++EventCount;
			EventComponent = SourceComponent;
			EventCharacterIndex = CharacterIndex;
			EventPreviousLevel = PreviousLevel;
			EventNewLevel = NewLevel;
			EventLevelsGained = LevelsGained;
		});

	TestTrue(TEXT("MON15.3 level-up still applies"), FRPGLevelUpService::ApplyPendingLevelUp(Component, 0, false));
	FRPGLevelUpService::OnCharacterLevelUpAppliedWithSource().Remove(Handle);

	TestEqual(TEXT("One source-aware level-up event fires"), EventCount, 1);
	TestTrue(TEXT("The event identifies the source inventory"), EventComponent == Component);
	TestEqual(TEXT("The event identifies character zero"), EventCharacterIndex, 0);
	TestEqual(TEXT("The event reports previous level one"), EventPreviousLevel, 1);
	TestEqual(TEXT("The event reports new level two"), EventNewLevel, 2);
	TestEqual(TEXT("The event reports one level gained"), EventLevelsGained, 1);

	const TSet<FName> Requirements = GetMON155RuntimeRequirements(Component, 0);
	TestTrue(TEXT("Level-up projects the level-two automatic feature"), Requirements.Contains(TEXT("Feature_Level2")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
