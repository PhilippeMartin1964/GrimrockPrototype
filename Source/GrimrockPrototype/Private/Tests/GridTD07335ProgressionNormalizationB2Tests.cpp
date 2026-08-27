#if WITH_DEV_AUTOMATION_TESTS

#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#include "RPG/RPGClassProgressionTransactionService.h"
#include "RPGMON155TestHelpers.h"
#include "Save/GrimrockPartySaveGame.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07335B2SchemaPurgeTest,
	"Grimrock.TechnicalDebt.TD07_3_3_5.NormalizationB2.SchemaPurge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07335B2SchemaPurgeTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	TestNull(TEXT("Legacy ClassProgressionStates property is removed from SaveGame reflection"),
		FindFProperty<FProperty>(UGrimrockPartySaveGame::StaticClass(), TEXT("ClassProgressionStates")));

	FString SaveHeader;
	const FString SaveHeaderPath = FPaths::Combine(
		FPaths::ProjectDir(), TEXT("Source/GrimrockPrototype/Public/Save/GrimrockPartySaveGame.h"));
	TestTrue(TEXT("Current SaveGame header loads"), FFileHelper::LoadFileToString(SaveHeader, *SaveHeaderPath));
	TestFalse(TEXT("Legacy FRPGCharacterProgressionSaveState type is physically removed"),
		SaveHeader.Contains(TEXT("FRPGCharacterProgressionSaveState")));
	TestFalse(TEXT("Legacy ClassProgressionStates field is physically removed"),
		SaveHeader.Contains(TEXT("ClassProgressionStates")));

	UScriptStruct* CharacterStruct = FGridCharacterInventoryState::StaticStruct();
	FProperty* LevelProperty = FindFProperty<FProperty>(CharacterStruct, TEXT("Level"));
	FProperty* ExperienceProperty = FindFProperty<FProperty>(CharacterStruct, TEXT("Experience"));
	FProperty* ChoicesProperty = FindFProperty<FProperty>(CharacterStruct, TEXT("SelectedClassProgressionChoiceIds"));
	TestTrue(TEXT("Level remains transient runtime projection"), LevelProperty && LevelProperty->HasAnyPropertyFlags(CPF_Transient));
	TestTrue(TEXT("Experience remains durable authority"), ExperienceProperty && !ExperienceProperty->HasAnyPropertyFlags(CPF_Transient));
	TestTrue(TEXT("Selected class choices remain durable authority"), ChoicesProperty && !ChoicesProperty->HasAnyPropertyFlags(CPF_Transient));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07335B2CharacterChoiceRoundTripTest,
	"Grimrock.TechnicalDebt.TD07_3_3_5.NormalizationB2.CharacterChoiceRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07335B2CharacterChoiceRoundTripTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FMON155RuntimeStateGuard RuntimeGuard;

	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeMON155Inventory(2, 1000, ClassDefinition);

	FRPGClassProgressionCommitResult CommitResult;
	TestTrue(TEXT("Choice A commits into character authority"),
		FRPGClassProgressionTransactionService::TryCommitChoices(Component, 0, { TEXT("Choice_A") }, CommitResult));

	UGrimrockPartySaveGame* Save = NewObject<UGrimrockPartySaveGame>();
	Save->PartyInventoryState = Component->PartyInventoryState;

	TArray<uint8> Bytes;
	TestTrue(TEXT("v15 save serializes to memory without a separate progression snapshot"),
		UGameplayStatics::SaveGameToMemory(Save, Bytes));

	FRPGClassProgressionTransactionService::ResetRuntimeState();
	UGrimrockPartySaveGame* Loaded = Cast<UGrimrockPartySaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
	TestNotNull(TEXT("v15 save loads from memory"), Loaded);
	if (!Loaded)
	{
		return false;
	}

	const FGridCharacterInventoryState& Character = Loaded->PartyInventoryState.ActiveCharacters[0];
	TestEqual(TEXT("Experience survives as durable level authority"), Character.Experience, 1000);
	TestEqual(TEXT("Transient Level reconstructs from Experience"), Character.Level, 2);
	TestTrue(TEXT("Choice A survives directly on character state"),
		Character.SelectedClassProgressionChoiceIds.Contains(TEXT("Choice_A")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07335B2ProjectionRebuildTest,
	"Grimrock.TechnicalDebt.TD07_3_3_5.NormalizationB2.ProjectionRebuild",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07335B2ProjectionRebuildTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FMON155RuntimeStateGuard RuntimeGuard;

	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeMON155Inventory(3, 3000, ClassDefinition);

	FRPGClassProgressionCommitResult CommitResult;
	TestTrue(TEXT("Choice A commits before detached rebuild"),
		FRPGClassProgressionTransactionService::TryCommitChoices(Component, 0, { TEXT("Choice_A") }, CommitResult));

	const FGridPartyInventoryState Snapshot = Component->PartyInventoryState;
	const FGuid CharacterId = Snapshot.ActiveCharacters[0].CharacterId;

	FRPGClassProgressionTransactionService::ResetRuntimeState();
	TSet<FName> EmptyRequirements;
	FRPGClassProgressionTransactionService::AppendRuntimeSatisfiedRequirements(CharacterId, EmptyRequirements);
	TestTrue(TEXT("Runtime projection is empty after explicit cache reset"), EmptyRequirements.IsEmpty());

	FText RebuildError;
	TestTrue(TEXT("Runtime projection rebuilds only from authoritative character state"),
		FRPGClassProgressionTransactionService::RebuildRuntimeProjection(Snapshot, RebuildError));
	TestTrue(TEXT("Projection rebuild reports no error"), RebuildError.IsEmpty());

	TSet<FName> Requirements;
	FRPGClassProgressionTransactionService::AppendRuntimeSatisfiedRequirements(CharacterId, Requirements);
	TestTrue(TEXT("Choice A requirement is reconstructed"), Requirements.Contains(TEXT("Choice_A")));
	TestTrue(TEXT("Choice A granted feature is reconstructed"), Requirements.Contains(TEXT("Feature_A")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07335B2SaveSchemaVersionTest,
	"Grimrock.TechnicalDebt.TD07_3_3_5.NormalizationB2.SaveSchemaVersion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07335B2SaveSchemaVersionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	TestTrue(TEXT("Current exact-match schema remains at least the TD07.3.3.5 B2 v15 generation"),
		UGrimrockPartySaveGame::CurrentSaveVersion >= 15);

	UGrimrockPartySaveGame* Current = NewObject<UGrimrockPartySaveGame>();
	TestEqual(TEXT("New SaveGame starts on the current schema"), Current->SaveVersion, UGrimrockPartySaveGame::CurrentSaveVersion);
	TestTrue(TEXT("Current exact-match schema is compatible"), Current->IsCompatible());

	UGrimrockPartySaveGame* Previous = NewObject<UGrimrockPartySaveGame>();
	Previous->SaveVersion = UGrimrockPartySaveGame::CurrentSaveVersion - 1;
	FText Error;
	TestFalse(TEXT("Previous prototype schema is rejected without migration"), Previous->ValidateCurrentState(Error));
	TestFalse(TEXT("Previous prototype schema is incompatible"), Previous->IsCompatible());
	TestEqual(TEXT("Validation does not rewrite the previous schema"), Previous->SaveVersion, UGrimrockPartySaveGame::CurrentSaveVersion - 1);
	TestTrue(TEXT("Rejected previous schema reports an error"), !Error.IsEmpty());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
