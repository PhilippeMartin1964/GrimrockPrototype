#if WITH_DEV_AUTOMATION_TESTS

#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "RPGMON155TestHelpers.h"
#include "Save/GrimrockPartySaveGame.h"
#include "UObject/UnrealType.h"

namespace
{
	struct FGridTD0732StateGuard
	{
		FGridTD0732StateGuard()
		{
			FRPGClassProgressionTransactionService::ResetRuntimeState();
		}

		~FGridTD0732StateGuard()
		{
			FRPGClassProgressionTransactionService::ResetRuntimeState();
		}
	};

	UGrimrockPartySaveGame* MakeTD0732Save(UGridPartyInventoryComponent* Component)
	{
		UGrimrockPartySaveGame* Save = NewObject<UGrimrockPartySaveGame>();
		Save->PartyInventoryState = Component->PartyInventoryState;
		return Save;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD0732ExactVersionContractTest, "Grimrock.TechnicalDebt.TD07_3_2.SaveContract.ExactVersion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD0732ExactVersionContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	TestTrue(TEXT("TD07.3 exact-match contract remains on or beyond the v10 reset generation"), UGrimrockPartySaveGame::CurrentSaveVersion >= 10);

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD0732PersistentChoiceRoundTripTest, "Grimrock.TechnicalDebt.TD07_3_2.SaveContract.PersistentChoiceRoundTrip",
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD0732RejectCurrentMismatchTest, "Grimrock.TechnicalDebt.TD07_3_2.SaveContract.RejectLevelExperienceMismatch",
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD0732RejectInvalidChoiceSnapshotTest, "Grimrock.TechnicalDebt.TD07_3_2.SaveContract.RejectInvalidChoiceSnapshot",
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD0732PendingLevelUpRoundTripTest, "Grimrock.TechnicalDebt.TD07_3_2.SaveContract.PendingLevelUpRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD0732PendingLevelUpRoundTripTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridTD0732StateGuard StateGuard;

	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeMON155Inventory(2, 1000, ClassDefinition);
	FGridCharacterInventoryState& Character = Component->PartyInventoryState.ActiveCharacters[0];
	Character.LastAcknowledgedLevel = 1;

	UGrimrockPartySaveGame* SourceSave = MakeTD0732Save(Component);
	TArray<uint8> SaveBytes;
	TestTrue(TEXT("Save with an unacknowledged Level Up serializes"), UGameplayStatics::SaveGameToMemory(SourceSave, SaveBytes));

	UGrimrockPartySaveGame* LoadedSave = Cast<UGrimrockPartySaveGame>(UGameplayStatics::LoadGameFromMemory(SaveBytes));
	TestNotNull(TEXT("Save with an unacknowledged Level Up deserializes"), LoadedSave);
	if (!LoadedSave)
	{
		return false;
	}

	const FGridCharacterInventoryState& LoadedCharacter = LoadedSave->PartyInventoryState.ActiveCharacters[0];
	TestEqual(TEXT("Current level survives as rebuilt projection"), LoadedCharacter.Level, 2);
	TestEqual(TEXT("Durable acknowledgement survives serialization"), LoadedCharacter.LastAcknowledgedLevel, 1);
	TestEqual(TEXT("Pending Level-Up delta remains derivable"), LoadedCharacter.Level - LoadedCharacter.LastAcknowledgedLevel, 1);
	TestNull(TEXT("Separate PendingLevelUpNotifications SaveGame mirror is absent"),
		FindFProperty<FProperty>(UGrimrockPartySaveGame::StaticClass(), TEXT("PendingLevelUpNotifications")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD0732RejectInvalidPendingNotificationTest,
	"Grimrock.TechnicalDebt.TD07_3_2.SaveContract.RejectInvalidPendingNotification", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD0732RejectInvalidPendingNotificationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridTD0732StateGuard StateGuard;

	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeMON155Inventory(2, 1000, ClassDefinition);
	UGrimrockPartySaveGame* Save = MakeTD0732Save(Component);
	Save->PartyInventoryState.ActiveCharacters[0].LastAcknowledgedLevel = 3;

	FText Error;
	TestFalse(TEXT("Acknowledgement above the current level is rejected"), Save->ValidateCurrentState(Error));
	TestTrue(TEXT("Invalid Level-Up acknowledgement reports an error"), !Error.IsEmpty());
	return true;
}

#endif
