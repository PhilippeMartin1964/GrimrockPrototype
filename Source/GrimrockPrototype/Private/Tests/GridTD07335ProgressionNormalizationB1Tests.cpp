#if WITH_DEV_AUTOMATION_TESTS

#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"

#include "RPG/RPGClassProgressionTransactionService.h"
#include "RPGMON155TestHelpers.h"
#include "Save/GrimrockPartySaveGame.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07335SchemaAuthorityTest, "Grimrock.TechnicalDebt.TD07_3_3_5.NormalizationB1.SchemaAuthority",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07335SchemaAuthorityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UScriptStruct* CharacterStruct = FGridCharacterInventoryState::StaticStruct();
	TestNotNull(TEXT("Character state reflected"), CharacterStruct);

	FProperty* LevelProperty = FindFProperty<FProperty>(CharacterStruct, TEXT("Level"));
	FProperty* ExperienceProperty = FindFProperty<FProperty>(CharacterStruct, TEXT("Experience"));
	FProperty* ChoicesProperty = FindFProperty<FProperty>(CharacterStruct, TEXT("SelectedClassProgressionChoiceIds"));
	TestNotNull(TEXT("Runtime Level projection still exists"), LevelProperty);
	TestNotNull(TEXT("Experience authority exists"), ExperienceProperty);
	TestNotNull(TEXT("Character-owned class choices exist"), ChoicesProperty);
	TestTrue(TEXT("Level is transient and therefore not durable authority"), LevelProperty && LevelProperty->HasAnyPropertyFlags(CPF_Transient));
	TestFalse(TEXT("Experience remains durable"), ExperienceProperty && ExperienceProperty->HasAnyPropertyFlags(CPF_Transient));
	TestFalse(TEXT("Selected class choices remain durable"), ChoicesProperty && ChoicesProperty->HasAnyPropertyFlags(CPF_Transient));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07335CharacterChoiceAuthorityTest, "Grimrock.TechnicalDebt.TD07_3_3_5.NormalizationB1.CharacterChoiceAuthority",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07335CharacterChoiceAuthorityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FMON155RuntimeStateGuard RuntimeGuard;
	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeMON155Inventory(2, 1000, ClassDefinition);

	FRPGClassProgressionCommitResult Result;
	TestTrue(TEXT("Choice A commits"), FRPGClassProgressionTransactionService::TryCommitChoices(Component, 0, { TEXT("Choice_A") }, Result));
	TestTrue(
		TEXT("Character state owns Choice A"), Component->PartyInventoryState.ActiveCharacters[0].SelectedClassProgressionChoiceIds.Contains(TEXT("Choice_A")));

	FRPGClassProgressionTransactionService::ResetRuntimeState(Component);
	TArray<FName> Selected;
	TestTrue(TEXT("Derived runtime projection rebuilds after reset"), FRPGClassProgressionTransactionService::TryGetSelectedChoiceIds(Component, 0, Selected));
	TestTrue(TEXT("Choice A survives cache reset"), Selected.Contains(TEXT("Choice_A")));

	TSet<FName> Requirements;
	FRPGClassProgressionTransactionService::AppendRuntimeSatisfiedRequirements(Component->PartyInventoryState.ActiveCharacters[0].CharacterId, Requirements);
	TestTrue(TEXT("Choice A requirement is reconstructed"), Requirements.Contains(TEXT("Choice_A")));
	TestTrue(TEXT("Choice A granted feature is reconstructed"), Requirements.Contains(TEXT("Feature_A")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07335TransientLevelRoundTripTest, "Grimrock.TechnicalDebt.TD07_3_3_5.NormalizationB1.TransientLevelRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07335TransientLevelRoundTripTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FMON155RuntimeStateGuard RuntimeGuard;
	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeMON155Inventory(3, 3000, ClassDefinition);
	FRPGClassProgressionCommitResult Result;
	TestTrue(TEXT("Choice A commits before save"), FRPGClassProgressionTransactionService::TryCommitChoices(Component, 0, { TEXT("Choice_A") }, Result));

	UGrimrockPartySaveGame* Save = NewObject<UGrimrockPartySaveGame>();
	Save->PartyInventoryState = Component->PartyInventoryState;
	TArray<uint8> Bytes;
	TestTrue(TEXT("Current-schema save serializes to memory"), UGameplayStatics::SaveGameToMemory(Save, Bytes));

	FRPGClassProgressionTransactionService::ResetRuntimeState();
	UGrimrockPartySaveGame* Loaded = Cast<UGrimrockPartySaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
	TestNotNull(TEXT("Current-schema save loads from memory"), Loaded);
	if (!Loaded)
	{
		return false;
	}

	const FGridCharacterInventoryState& Character = Loaded->PartyInventoryState.ActiveCharacters[0];
	TestEqual(TEXT("Experience survives as authority"), Character.Experience, 3000);
	TestEqual(TEXT("Transient Level is reconstructed from Experience"), Character.Level, 3);
	TestTrue(TEXT("Character-owned Choice A survives serialization"), Character.SelectedClassProgressionChoiceIds.Contains(TEXT("Choice_A")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07335SaveSchemaVersionTest, "Grimrock.TechnicalDebt.TD07_3_3_5.NormalizationB1.SaveSchemaVersion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07335SaveSchemaVersionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	TestTrue(TEXT("Current exact-match schema remains at least the TD07.3.3.5 B1 v14 generation"), UGrimrockPartySaveGame::CurrentSaveVersion >= 14);
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
