#if WITH_DEV_AUTOMATION_TESTS

#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "RPG/RPGLevelUpNotificationSubsystem.h"
#include "RPG/RPGSaveMigrationService.h"
#include "RPGMON155TestHelpers.h"

namespace
{
	struct FMON156StateGuard
	{
		FMON156StateGuard()
		{
			FRPGClassProgressionTransactionService::ResetRuntimeState();
			URPGLevelUpNotificationSubsystem::RestorePersistentState({});
		}

		~FMON156StateGuard()
		{
			FRPGClassProgressionTransactionService::ResetRuntimeState();
			URPGLevelUpNotificationSubsystem::RestorePersistentState({});
		}
	};

	UGrimrockPartySaveGame* MakeMON156Save(UGridPartyInventoryComponent* Component)
	{
		UGrimrockPartySaveGame* Save = NewObject<UGrimrockPartySaveGame>();
		Save->PartyInventoryState = Component->PartyInventoryState;
		return Save;
	}

	void AddEmptyProgressionState(UGrimrockPartySaveGame* Save, const FGuid& CharacterId)
	{
		FRPGCharacterProgressionSaveState Progression;
		Progression.CharacterId = CharacterId;
		Save->ClassProgressionStates.Add(Progression);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON156PersistentChoiceRoundTripTest, "Grimrock.RPG.MON15.6.PersistentChoiceRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON156PersistentChoiceRoundTripTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FMON156StateGuard StateGuard;

	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeMON155Inventory(3, 3000, ClassDefinition);
	const FGuid CharacterId = Component->PartyInventoryState.ActiveCharacters[0].CharacterId;

	FRPGClassProgressionCommitResult CommitResult;
	TestTrue(TEXT("Choice A and B commit before save"),
		FRPGClassProgressionTransactionService::TryCommitChoices(Component, 0, { TEXT("Choice_A"), TEXT("Choice_B") }, CommitResult));

	UGrimrockPartySaveGame* SourceSave = MakeMON156Save(Component);
	TArray<uint8> SaveBytes;
	TestTrue(TEXT("MON15.6 save serializes to memory"), UGameplayStatics::SaveGameToMemory(SourceSave, SaveBytes));

	FRPGClassProgressionTransactionService::ResetRuntimeState();
	UGrimrockPartySaveGame* LoadedSave = Cast<UGrimrockPartySaveGame>(UGameplayStatics::LoadGameFromMemory(SaveBytes));
	TestNotNull(TEXT("MON15.6 save deserializes from memory"), LoadedSave);
	if (!LoadedSave)
	{
		return false;
	}

	TestEqual(TEXT("Save contract is the current version"), LoadedSave->SaveVersion, UGrimrockPartySaveGame::CurrentSaveVersion);
	TestTrue(TEXT("Loaded progression is compatible"), LoadedSave->IsCompatible());
	TestEqual(TEXT("One character progression snapshot is restored"), LoadedSave->ClassProgressionStates.Num(), 1);
	TestTrue(TEXT("Choice A survives serialization"), LoadedSave->ClassProgressionStates[0].SelectedChoiceIds.Contains(TEXT("Choice_A")));
	TestTrue(TEXT("Choice B survives serialization"), LoadedSave->ClassProgressionStates[0].SelectedChoiceIds.Contains(TEXT("Choice_B")));

	TSet<FName> RestoredRequirements;
	FRPGClassProgressionTransactionService::AppendRuntimeSatisfiedRequirements(CharacterId, RestoredRequirements);
	TestTrue(TEXT("Choice A requirement is restored before live cache binding"), RestoredRequirements.Contains(TEXT("Choice_A")));
	TestTrue(TEXT("Choice B requirement is restored before live cache binding"), RestoredRequirements.Contains(TEXT("Choice_B")));
	TestTrue(TEXT("Choice A granted feature is restored"), RestoredRequirements.Contains(TEXT("Feature_A")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON156LegacyExperienceAheadMigrationTest, "Grimrock.RPG.MON15.6.LegacyExperienceAheadMigration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON156LegacyExperienceAheadMigrationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FMON156StateGuard StateGuard;

	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeMON155Inventory(1, 6000, ClassDefinition);
	FGridCharacterInventoryState& Character = Component->PartyInventoryState.ActiveCharacters[0];
	const int32 HealthDeficit = 5;
	const int32 ManaDeficit = 2;
	Character.DerivedStats.CurrentHealth = Character.DerivedStats.MaxHealth - HealthDeficit;
	Character.DerivedStats.CurrentMana = Character.DerivedStats.MaxMana - ManaDeficit;

	UGrimrockPartySaveGame* Save = MakeMON156Save(Component);
	Save->SaveVersion = 1;
	FText Error;
	FRPGSaveMigrationReport Report;
	TestTrue(TEXT("Version one migrates to the current contract"), FRPGSaveMigrationService::PrepareLoadedSave(Save, Error, &Report));
	TestTrue(TEXT("Migration report is marked migrated"), Report.bMigrated);
	TestEqual(TEXT("Migration source remains version one in report"), Report.SourceVersion, 1);
	TestEqual(TEXT("Migration target is the current version"), Save->SaveVersion, UGrimrockPartySaveGame::CurrentSaveVersion);
	TestEqual(TEXT("XP 6000 reconstructs level four"), Character.Level, 1);

	const FGridCharacterInventoryState& MigratedCharacter = Save->PartyInventoryState.ActiveCharacters[0];
	TestEqual(TEXT("Migrated character reaches level four"), MigratedCharacter.Level, 4);
	TestEqual(TEXT("Migrated XP remains 6000"), MigratedCharacter.Experience, 6000);
	TestEqual(TEXT("Health deficit is preserved"), MigratedCharacter.DerivedStats.CurrentHealth, MigratedCharacter.DerivedStats.MaxHealth - HealthDeficit);
	TestEqual(TEXT("Mana deficit is preserved"), MigratedCharacter.DerivedStats.CurrentMana, MigratedCharacter.DerivedStats.MaxMana - ManaDeficit);
	TestEqual(TEXT("Legacy migration creates one empty choice snapshot"), Save->ClassProgressionStates.Num(), 1);
	TestTrue(TEXT("Legacy character starts with no confirmed MON15.5 choice"), Save->ClassProgressionStates[0].SelectedChoiceIds.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON156LegacyStoredLevelAheadMigrationTest, "Grimrock.RPG.MON15.6.LegacyStoredLevelAheadMigration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON156LegacyStoredLevelAheadMigrationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FMON156StateGuard StateGuard;

	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeMON155Inventory(3, 1000, ClassDefinition);
	UGrimrockPartySaveGame* Save = MakeMON156Save(Component);
	Save->SaveVersion = 3;

	FText Error;
	FRPGSaveMigrationReport Report;
	TestTrue(TEXT("Version three with stored level ahead migrates"), FRPGSaveMigrationService::PrepareLoadedSave(Save, Error, &Report));
	const FGridCharacterInventoryState& MigratedCharacter = Save->PartyInventoryState.ActiveCharacters[0];
	TestEqual(TEXT("Stored level three is preserved"), MigratedCharacter.Level, 3);
	TestEqual(TEXT("XP is raised to the level-three floor"), MigratedCharacter.Experience, 3000);
	TestEqual(TEXT("One character was reconciled"), Report.ReconciledCharacterCount, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON156RejectCurrentMismatchTest, "Grimrock.RPG.MON15.6.RejectCurrentLevelExperienceMismatch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON156RejectCurrentMismatchTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FMON156StateGuard StateGuard;

	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeMON155Inventory(1, 1000, ClassDefinition);
	UGrimrockPartySaveGame* Save = MakeMON156Save(Component);
	Save->SaveVersion = UGrimrockPartySaveGame::CurrentSaveVersion;
	AddEmptyProgressionState(Save, Component->PartyInventoryState.ActiveCharacters[0].CharacterId);

	FText Error;
	TestFalse(TEXT("Current-version inconsistent Level/Experience is rejected"), FRPGSaveMigrationService::ValidateCurrentSave(Save, Error));
	TestTrue(TEXT("Mismatch rejection reports an error"), !Error.IsEmpty());
	TestEqual(TEXT("Strict current validation does not mutate the stored level"), Save->PartyInventoryState.ActiveCharacters[0].Level, 1);
	TestEqual(TEXT("Strict current validation does not mutate XP"), Save->PartyInventoryState.ActiveCharacters[0].Experience, 1000);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON156RejectInvalidChoiceSnapshotTest, "Grimrock.RPG.MON15.6.RejectInvalidChoiceSnapshot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON156RejectInvalidChoiceSnapshotTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FMON156StateGuard StateGuard;

	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeMON155Inventory(2, 1000, ClassDefinition);
	UGrimrockPartySaveGame* Save = MakeMON156Save(Component);
	Save->SaveVersion = UGrimrockPartySaveGame::CurrentSaveVersion;

	FRPGCharacterProgressionSaveState Progression;
	Progression.CharacterId = Component->PartyInventoryState.ActiveCharacters[0].CharacterId;
	Progression.SelectedChoiceIds.Add(TEXT("Choice_Unknown"));
	Save->ClassProgressionStates.Add(Progression);

	FText Error;
	TestFalse(TEXT("Unknown persisted class choice is rejected"), FRPGSaveMigrationService::ValidateCurrentSave(Save, Error));
	TestTrue(TEXT("Invalid choice rejection reports an error"), !Error.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON156PendingLevelUpRoundTripTest, "Grimrock.RPG.MON15.6.PendingLevelUpRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON156PendingLevelUpRoundTripTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FMON156StateGuard StateGuard;

	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeMON155Inventory(2, 1000, ClassDefinition);
	const FGuid CharacterId = Component->PartyInventoryState.ActiveCharacters[0].CharacterId;

	FRPGPendingLevelUpSaveState Pending;
	Pending.CharacterId = CharacterId;
	Pending.PreviousLevel = 1;
	Pending.NewLevel = 2;
	Pending.LevelsGained = 1;
	URPGLevelUpNotificationSubsystem::RestorePersistentState({ Pending });

	UGrimrockPartySaveGame* SourceSave = MakeMON156Save(Component);
	TArray<uint8> SaveBytes;
	TestTrue(TEXT("Save with pending Level Up serializes"), UGameplayStatics::SaveGameToMemory(SourceSave, SaveBytes));

	URPGLevelUpNotificationSubsystem::RestorePersistentState({});
	UGrimrockPartySaveGame* LoadedSave = Cast<UGrimrockPartySaveGame>(UGameplayStatics::LoadGameFromMemory(SaveBytes));
	TestNotNull(TEXT("Save with pending Level Up deserializes"), LoadedSave);
	if (!LoadedSave)
	{
		return false;
	}

	TestEqual(TEXT("One pending Level Up survives serialization"), LoadedSave->PendingLevelUpNotifications.Num(), 1);
	TestTrue(TEXT("Pending Level Up keeps CharacterId"), LoadedSave->PendingLevelUpNotifications[0].CharacterId == CharacterId);
	TestEqual(TEXT("Pending Level Up keeps previous level"), LoadedSave->PendingLevelUpNotifications[0].PreviousLevel, 1);
	TestEqual(TEXT("Pending Level Up keeps new level"), LoadedSave->PendingLevelUpNotifications[0].NewLevel, 2);

	TArray<FRPGPendingLevelUpSaveState> Recaptured;
	FText CaptureError;
	TestTrue(TEXT("Loaded notification is restored into the persistent mirror"),
		URPGLevelUpNotificationSubsystem::CapturePersistentState(LoadedSave->PartyInventoryState, Recaptured, CaptureError));
	TestEqual(TEXT("Persistent mirror contains the restored notification"), Recaptured.Num(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON156RejectInvalidPendingNotificationTest, "Grimrock.RPG.MON15.6.RejectInvalidPendingNotification",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON156RejectInvalidPendingNotificationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FMON156StateGuard StateGuard;

	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeMON155Inventory(2, 1000, ClassDefinition);
	UGrimrockPartySaveGame* Save = MakeMON156Save(Component);
	Save->SaveVersion = UGrimrockPartySaveGame::CurrentSaveVersion;
	const FGuid CharacterId = Component->PartyInventoryState.ActiveCharacters[0].CharacterId;
	AddEmptyProgressionState(Save, CharacterId);

	FRPGPendingLevelUpSaveState Pending;
	Pending.CharacterId = CharacterId;
	Pending.PreviousLevel = 1;
	Pending.NewLevel = 3;
	Pending.LevelsGained = 2;
	Save->PendingLevelUpNotifications.Add(Pending);

	FText Error;
	TestFalse(TEXT("Pending notification targeting a level other than current is rejected"), FRPGSaveMigrationService::ValidateCurrentSave(Save, Error));
	TestTrue(TEXT("Invalid pending notification reports an error"), !Error.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGMON156SaveVersionContractTest, "Grimrock.RPG.MON15.6.SaveVersionContract", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON156SaveVersionContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	TestEqual(TEXT("Current save contract includes MON19.2.2 version seven"), UGrimrockPartySaveGame::CurrentSaveVersion, 7);
	TestEqual(TEXT("Version one remains the minimum compatible save"), UGrimrockPartySaveGame::MinimumCompatibleSaveVersion, 1);

	UGrimrockPartySaveGame* VersionOne = NewObject<UGrimrockPartySaveGame>();
	VersionOne->SaveVersion = 1;
	TestTrue(TEXT("Version one is still inside the supported version range before migration"),
		VersionOne->SaveVersion >= UGrimrockPartySaveGame::MinimumCompatibleSaveVersion &&
			VersionOne->SaveVersion <= UGrimrockPartySaveGame::CurrentSaveVersion);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
