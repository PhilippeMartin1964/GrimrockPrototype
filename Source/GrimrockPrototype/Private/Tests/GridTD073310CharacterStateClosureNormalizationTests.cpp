#if WITH_DEV_AUTOMATION_TESTS

#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"

#include "RPG/RPGCharacterRulesLibrary.h"
#include "RPGMON155TestHelpers.h"
#include "Runtime/GridInventoryTypes.h"
#include "Save/GrimrockPartySaveGame.h"
#include "UObject/UnrealType.h"

namespace GridTD073310Normalization
{
	void TestTD073310DerivedStatsEqual(
		FAutomationTestBase& Test, const TCHAR* Prefix, const FRPGDerivedStats& Actual, const FRPGDerivedStats& Expected)
	{
		Test.TestEqual(FString::Printf(TEXT("%s MaxHealth"), Prefix), Actual.MaxHealth, Expected.MaxHealth);
		Test.TestEqual(FString::Printf(TEXT("%s MaxMana"), Prefix), Actual.MaxMana, Expected.MaxMana);
		Test.TestEqual(FString::Printf(TEXT("%s Initiative"), Prefix), Actual.Initiative, Expected.Initiative);
		Test.TestEqual(FString::Printf(TEXT("%s Accuracy"), Prefix), Actual.Accuracy, Expected.Accuracy);
		Test.TestEqual(FString::Printf(TEXT("%s Evasion"), Prefix), Actual.Evasion, Expected.Evasion);
	}

	void CorruptTD073310DerivedStats(FRPGDerivedStats& Stats)
	{
		Stats.MaxHealth += 777;
		Stats.MaxMana += 333;
		Stats.Initiative += 111;
		Stats.Accuracy += 222;
		Stats.Evasion += 444;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD073310SchemaAuthorityTest,
	"Grimrock.TechnicalDebt.TD07_3_3_10.Normalization.SchemaAuthority",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD073310SchemaAuthorityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const UScriptStruct* CharacterStruct = FGridCharacterInventoryState::StaticStruct();
	const FProperty* DerivedStatsProperty = FindFProperty<FProperty>(CharacterStruct, TEXT("DerivedStats"));
	const FProperty* ResourcesProperty = FindFProperty<FProperty>(CharacterStruct, TEXT("Resources"));
	const FProperty* LevelProperty = FindFProperty<FProperty>(CharacterStruct, TEXT("Level"));

	TestNotNull(TEXT("DerivedStats exists"), DerivedStatsProperty);
	TestTrue(TEXT("DerivedStats is a transient projection"),
		DerivedStatsProperty && DerivedStatsProperty->HasAnyPropertyFlags(CPF_Transient));
	TestTrue(TEXT("Resources remains durable mutable state"),
		ResourcesProperty && !ResourcesProperty->HasAnyPropertyFlags(CPF_Transient));
	TestTrue(TEXT("Level remains transient projection"),
		LevelProperty && LevelProperty->HasAnyPropertyFlags(CPF_Transient));

	TestTrue(TEXT("TD07.3.3.10 established SaveGame v20 or later"), UGrimrockPartySaveGame::CurrentSaveVersion >= 20);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD073310ActiveRoundTripTest,
	"Grimrock.TechnicalDebt.TD07_3_3_10.Normalization.ActiveDerivedStatsRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD073310ActiveRoundTripTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD073310Normalization;

	FMON155RuntimeStateGuard RuntimeGuard;
	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeMON155Inventory(3, 3000, ClassDefinition);
	if (!TestNotNull(TEXT("Active round-trip component exists"), Component) ||
		!TestNotNull(TEXT("Active round-trip class definition exists"), ClassDefinition))
	{
		return false;
	}

	FGridCharacterInventoryState& Character = Component->PartyInventoryState.ActiveCharacters[0];
	const FRPGDerivedStats Expected =
		URPGCharacterRulesLibrary::CalculateDerivedStats(Character.Attributes, ClassDefinition, Character.Level);
	CorruptTD073310DerivedStats(Character.DerivedStats);

	UGrimrockPartySaveGame* Save = NewObject<UGrimrockPartySaveGame>();
	Save->PartyInventoryState = Component->PartyInventoryState;

	TArray<uint8> Bytes;
	TestTrue(TEXT("v20 SaveGame serializes with corrupted transient DerivedStats"), UGameplayStatics::SaveGameToMemory(Save, Bytes));

	UGrimrockPartySaveGame* Loaded = Cast<UGrimrockPartySaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
	TestNotNull(TEXT("v20 SaveGame loads"), Loaded);
	if (!Loaded)
	{
		return false;
	}

	const FGridCharacterInventoryState& LoadedCharacter = Loaded->PartyInventoryState.ActiveCharacters[0];
	TestEqual(TEXT("Level is rebuilt from Experience"), LoadedCharacter.Level, 3);
	TestTD073310DerivedStatsEqual(*this, TEXT("Active DerivedStats rebuilds canonically"), LoadedCharacter.DerivedStats, Expected);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD073310PoolRoundTripTest,
	"Grimrock.TechnicalDebt.TD07_3_3_10.Normalization.PoolDerivedStatsRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD073310PoolRoundTripTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD073310Normalization;

	FMON155RuntimeStateGuard RuntimeGuard;
	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeMON155Inventory(1, 0, ClassDefinition);
	if (!TestNotNull(TEXT("Pool round-trip component exists"), Component) ||
		!TestNotNull(TEXT("Pool round-trip class definition exists"), ClassDefinition))
	{
		return false;
	}

	FGridCharacterInventoryState Reserve = MakeMON155Character(ClassDefinition, 2, 1000, TEXT("Reserve"));
	Reserve.LastAcknowledgedLevel = 2;
	const FRPGDerivedStats Expected =
		URPGCharacterRulesLibrary::CalculateDerivedStats(Reserve.Attributes, ClassDefinition, Reserve.Level);
	CorruptTD073310DerivedStats(Reserve.DerivedStats);
	Component->PartyInventoryState.CharacterPool.Add(Reserve);

	UGrimrockPartySaveGame* Save = NewObject<UGrimrockPartySaveGame>();
	Save->PartyInventoryState = Component->PartyInventoryState;

	TArray<uint8> Bytes;
	TestTrue(TEXT("v20 SaveGame serializes Active + Pool"), UGameplayStatics::SaveGameToMemory(Save, Bytes));

	UGrimrockPartySaveGame* Loaded = Cast<UGrimrockPartySaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
	TestNotNull(TEXT("v20 Active + Pool SaveGame loads"), Loaded);
	if (!Loaded)
	{
		return false;
	}

	TestEqual(TEXT("One reserve character survives"), Loaded->PartyInventoryState.CharacterPool.Num(), 1);
	if (Loaded->PartyInventoryState.CharacterPool.Num() != 1)
	{
		return false;
	}

	const FGridCharacterInventoryState& LoadedReserve = Loaded->PartyInventoryState.CharacterPool[0];
	TestEqual(TEXT("Pool Level is rebuilt from Experience"), LoadedReserve.Level, 2);
	TestTD073310DerivedStatsEqual(*this, TEXT("Pool DerivedStats rebuilds canonically"), LoadedReserve.DerivedStats, Expected);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD073310SaveSchemaVersionTest,
	"Grimrock.TechnicalDebt.TD07_3_3_10.Normalization.SaveSchemaVersion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD073310SaveSchemaVersionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGrimrockPartySaveGame* Current = NewObject<UGrimrockPartySaveGame>();
	TestEqual(TEXT("New SaveGame starts on the current schema"), Current->SaveVersion, UGrimrockPartySaveGame::CurrentSaveVersion);
	TestTrue(TEXT("Current schema is compatible"), Current->IsCompatible());

	UGrimrockPartySaveGame* Previous = NewObject<UGrimrockPartySaveGame>();
	Previous->SaveVersion = 19;
	FText Error;
	TestFalse(TEXT("Previous v19 is rejected without migration"), Previous->ValidateCurrentState(Error));
	TestFalse(TEXT("Previous v19 is incompatible"), Previous->IsCompatible());
	TestEqual(TEXT("Validation never rewrites v19"), Previous->SaveVersion, 19);
	TestTrue(TEXT("v19 rejection reports an error"), !Error.IsEmpty());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
