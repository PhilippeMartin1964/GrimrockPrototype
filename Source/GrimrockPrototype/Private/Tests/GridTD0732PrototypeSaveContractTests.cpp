#if WITH_DEV_AUTOMATION_TESTS

#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "RPG/RPGLevelUpNotificationSubsystem.h"
#include "RPGMON155TestHelpers.h"
#include "Save/GrimrockPartySaveGame.h"

namespace
{
	struct FGridTD0732StateGuard
	{
		FGridTD0732StateGuard()
		{
			FRPGClassProgressionTransactionService::ResetRuntimeState();
			URPGLevelUpNotificationSubsystem::RestorePersistentState({});
		}

		~FGridTD0732StateGuard()
		{
			FRPGClassProgressionTransactionService::ResetRuntimeState();
			URPGLevelUpNotificationSubsystem::RestorePersistentState({});
		}
	};

	UGrimrockPartySaveGame* MakeTD0732Save(UGridPartyInventoryComponent* Component)
	{
		UGrimrockPartySaveGame* Save = NewObject<UGrimrockPartySaveGame>();
		Save->PartyInventoryState = Component->PartyInventoryState;
		return Save;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD0732ExactVersionContractTest,
	"Grimrock.TechnicalDebt.TD07_3_2.SaveContract.ExactVersion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD0732ExactVersionContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	TestEqual(TEXT("TD07.3 exact-match contract tracks the current prototype schema generation"), UGrimrockPartySaveGame::CurrentSaveVersion, 14);

	UGrimrockPartySaveGame* Current = NewObject<UGrimrockPartySaveGame>();
	TestEqual(TEXT("A new SaveGame starts on the current schema"), Current->SaveVersion, UGrimrockPartySaveGame::CurrentSaveVersion);
	TestTrue(TEXT("A fresh current-schema SaveGame is compatible"), Current->IsCompatible());

	UGrimrockPartySaveGame* Previous = NewObject<UGrimrockPartySaveGame>();
	Previous->SaveVersion = UGrimrockPartySaveGame::CurrentSaveVersion - 1;
	FText PreviousError;
	TestFalse(TEXT("The previous prototype schema is rejected"), Previous->ValidateCurrentState(PreviousError));
	TestFalse(TEXT("The previous prototype schema is incompatible"), Previous->IsCompatible());
	TestEqual(TEXT("Validation never migrates the previous version"), Previous->SaveVersion, UGrimrockPartySaveGame::CurrentSaveVersion - 1);
	TestTrue(TEXT("Previous-version rejection reports an error"), !PreviousError.IsEmpty());

	UGrimrockPartySaveGame* Future = NewObject<UGrimrockPartySaveGame>();
	Future->SaveVersion = UGrimrockPartySaveGame::CurrentSaveVersion + 1;
	FText FutureError;
	TestFalse(TEXT("A future schema is rejected"), Future->ValidateCurrentState(FutureError));
	TestFalse(TEXT("A future schema is incompatible"), Future->IsCompatible());
	TestEqual(TEXT("Validation never rewrites a future version"), Future->SaveVersion, UGrimrockPartySaveGame::CurrentSaveVersion + 1);
	TestTrue(TEXT("Future-version rejection reports an error"), !FutureError.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD0732PersistentChoiceRoundTripTest,
	"Grimrock.TechnicalDebt.TD07_3_2.SaveContract.PersistentChoiceRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD0732PersistentChoiceRoundTripTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridTD0732StateGuard StateGuard;

	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeMON155Inventory(3, 3000, ClassDefinition);
	const FGuid CharacterId = Component->PartyInventoryState.ActiveCharacters[0].CharacterId;

	FRPGClassProgressionCommitResult CommitResult;
	TestTrue(TEXT("Choice A and B commit before save"),
		FRPGClassProgressionTransactionService::TryCommitChoices(Component, 0, { TEXT("Choice_A"), TEXT("Choice_B") }, CommitResult));

	UGrimrockPartySaveGame* SourceSave = MakeTD0732Save(Component);
	TArray<uint8> SaveBytes;
	TestTrue(TEXT("Current-schema save serializes to memory"), UGameplayStatics::SaveGameToMemory(SourceSave, SaveBytes));

	FRPGClassProgressionTransactionService::ResetRuntimeState();
	UGrimrockPartySaveGame* LoadedSave = Cast<UGrimrockPartySaveGame>(UGameplayStatics::LoadGameFromMemory(SaveBytes));
	TestNotNull(TEXT("Current-schema save deserializes from memory"), LoadedSave);
	if (!LoadedSave)
	{
		return false;
	}

	TestEqual(TEXT("Round trip keeps the current schema"), LoadedSave->SaveVersion, UGrimrockPartySaveGame::CurrentSaveVersion);
	TestTrue(TEXT("Loaded current-schema save is compatible"), LoadedSave->IsCompatible());
	TestEqual(TEXT("One active character is restored"), LoadedSave->PartyInventoryState.ActiveCharacters.Num(), 1);
	const FGridCharacterInventoryState& LoadedCharacter = LoadedSave->PartyInventoryState.ActiveCharacters[0];
	TestTrue(TEXT("Choice A survives serialization on the character"), LoadedCharacter.SelectedClassProgressionChoiceIds.Contains(TEXT("Choice_A")));
	TestTrue(TEXT("Choice B survives serialization on the character"), LoadedCharacter.SelectedClassProgressionChoiceIds.Contains(TEXT("Choice_B")));
	TestEqual(TEXT("Transient Level is rebuilt from Experience after load"), LoadedCharacter.Level, 3);

	TSet<FName> RestoredRequirements;
	FRPGClassProgressionTransactionService::AppendRuntimeSatisfiedRequirements(CharacterId, RestoredRequirements);
	TestTrue(TEXT("Choice A requirement is rebuilt before live cache binding"), RestoredRequirements.Contains(TEXT("Choice_A")));
	TestTrue(TEXT("Choice B requirement is rebuilt before live cache binding"), RestoredRequirements.Contains(TEXT("Choice_B")));
	TestTrue(TEXT("Choice A granted feature is rebuilt"), RestoredRequirements.Contains(TEXT("Feature_A")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD0732RejectCurrentMismatchTest,
	"Grimrock.TechnicalDebt.TD07_3_2.SaveContract.RejectLevelExperienceMismatch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD0732RejectCurrentMismatchTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridTD0732StateGuard StateGuard;

	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeMON155Inventory(1, 1000, ClassDefinition);
	UGrimrockPartySaveGame* Save = MakeTD0732Save(Component);

	FText Error;
	TestFalse(TEXT("Current-schema inconsistent transient Level/Experience is rejected before serialization"), Save->ValidateCurrentState(Error));
	TestTrue(TEXT("Mismatch rejection reports an error"), !Error.IsEmpty());
	TestEqual(TEXT("Strict validation does not mutate the runtime level cache"), Save->PartyInventoryState.ActiveCharacters[0].Level, 1);
	TestEqual(TEXT("Strict validation does not mutate XP"), Save->PartyInventoryState.ActiveCharacters[0].Experience, 1000);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD0732RejectInvalidChoiceSnapshotTest,
	"Grimrock.TechnicalDebt.TD07_3_2.SaveContract.RejectInvalidChoiceSnapshot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD0732RejectInvalidChoiceSnapshotTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridTD0732StateGuard StateGuard;

	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeMON155Inventory(2, 1000, ClassDefinition);
	UGrimrockPartySaveGame* Save = MakeTD0732Save(Component);
	Save->PartyInventoryState.ActiveCharacters[0].SelectedClassProgressionChoiceIds.Add(TEXT("Choice_Unknown"));

	FText Error;
	TestFalse(TEXT("Unknown persisted class choice is rejected"), Save->ValidateCurrentState(Error));
	TestTrue(TEXT("Invalid choice rejection reports an error"), !Error.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD0732PendingLevelUpRoundTripTest,
	"Grimrock.TechnicalDebt.TD07_3_2.SaveContract.PendingLevelUpRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD0732PendingLevelUpRoundTripTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridTD0732StateGuard StateGuard;

	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeMON155Inventory(2, 1000, ClassDefinition);
	const FGuid CharacterId = Component->PartyInventoryState.ActiveCharacters[0].CharacterId;

	FRPGPendingLevelUpSaveState Pending;
	Pending.CharacterId = CharacterId;
	Pending.PreviousLevel = 1;
	Pending.NewLevel = 2;
	Pending.LevelsGained = 1;
	URPGLevelUpNotificationSubsystem::RestorePersistentState({ Pending });

	UGrimrockPartySaveGame* SourceSave = MakeTD0732Save(Component);
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD0732RejectInvalidPendingNotificationTest,
	"Grimrock.TechnicalDebt.TD07_3_2.SaveContract.RejectInvalidPendingNotification",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD0732RejectInvalidPendingNotificationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridTD0732StateGuard StateGuard;

	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeMON155Inventory(2, 1000, ClassDefinition);
	UGrimrockPartySaveGame* Save = MakeTD0732Save(Component);
	const FGuid CharacterId = Component->PartyInventoryState.ActiveCharacters[0].CharacterId;

	FRPGPendingLevelUpSaveState Pending;
	Pending.CharacterId = CharacterId;
	Pending.PreviousLevel = 1;
	Pending.NewLevel = 3;
	Pending.LevelsGained = 2;
	Save->PendingLevelUpNotifications.Add(Pending);

	FText Error;
	TestFalse(TEXT("Pending notification targeting a level other than current is rejected"), Save->ValidateCurrentState(Error));
	TestTrue(TEXT("Invalid pending notification reports an error"), !Error.IsEmpty());
	return true;
}

#endif
