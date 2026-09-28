#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Kismet/GameplayStatics.h"
#include "Runtime/GridDungeonRuntimeState.h"
#include "Runtime/Map/GridMapExplorationState.h"
#include "Save/GrimrockPartySaveGame.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMapMON2165SchemaFlagsTest, "Grimrock.Map.MON21_6_5.Persistence.SchemaFlags",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMapMON2165SchemaFlagsTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UScriptStruct* LevelStateStruct = FGridLevelRuntimeState::StaticStruct();
	const FProperty* ExplorationProperty = LevelStateStruct ? LevelStateStruct->FindPropertyByName(TEXT("MapExploration")) : nullptr;
	TestNotNull(TEXT("Level runtime state owns MapExploration"), ExplorationProperty);
	TestTrue(TEXT("MapExploration is SaveGame-persistent from MON21.6.5"),
		ExplorationProperty && ExplorationProperty->HasAnyPropertyFlags(CPF_SaveGame));

	UScriptStruct* ExplorationStruct = FGridMapExplorationState::StaticStruct();
	const FProperty* CellsProperty = ExplorationStruct ? ExplorationStruct->FindPropertyByName(TEXT("ExploredCells")) : nullptr;
	const FProperty* SecretsProperty = ExplorationStruct ? ExplorationStruct->FindPropertyByName(TEXT("DiscoveredSecretObjectIds")) : nullptr;
	TestNotNull(TEXT("Exploration state owns ExploredCells"), CellsProperty);
	TestNotNull(TEXT("Exploration state owns DiscoveredSecretObjectIds"), SecretsProperty);
	TestTrue(TEXT("ExploredCells is SaveGame-persistent"), CellsProperty && CellsProperty->HasAnyPropertyFlags(CPF_SaveGame));
	TestTrue(TEXT("DiscoveredSecretObjectIds is SaveGame-persistent"), SecretsProperty && SecretsProperty->HasAnyPropertyFlags(CPF_SaveGame));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMapMON2165RoundTripTest, "Grimrock.Map.MON21_6_5.Persistence.RoundTripPerLevel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMapMON2165RoundTripTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGrimrockPartySaveGame* Source = NewObject<UGrimrockPartySaveGame>(GetTransientPackage());
	Source->DungeonRuntimeState.LevelStates.Add(TEXT("MapTile_A")).LevelId = TEXT("MapTile_A");
	Source->DungeonRuntimeState.LevelStates.Add(TEXT("MapTile_B")).LevelId = TEXT("MapTile_B");
	FGridLevelRuntimeState* LevelA = Source->DungeonRuntimeState.LevelStates.Find(TEXT("MapTile_A"));
	FGridLevelRuntimeState* LevelB = Source->DungeonRuntimeState.LevelStates.Find(TEXT("MapTile_B"));
	TestNotNull(TEXT("Source Level A exists"), LevelA);
	TestNotNull(TEXT("Source Level B exists"), LevelB);
	if (!LevelA || !LevelB)
	{
		return false;
	}

	bool bNewlyExplored = false;
	TestTrue(TEXT("Level A first explored cell is accepted"), LevelA->MapExploration.TryMarkExplored(FIntPoint(4, 7), bNewlyExplored));
	TestTrue(TEXT("Level A second explored cell is accepted"), LevelA->MapExploration.TryMarkExplored(FIntPoint(5, 7), bNewlyExplored));
	const FGuid SecretA = FGuid::NewGuid();
	bool bNewlyDiscovered = false;
	TestTrue(TEXT("Level A secret discovery is accepted"), LevelA->MapExploration.TryMarkSecretDiscovered(SecretA, bNewlyDiscovered));

	TestTrue(TEXT("Level B explored cell is accepted"), LevelB->MapExploration.TryMarkExplored(FIntPoint(20, 21), bNewlyExplored));
	const FGuid SecretB = FGuid::NewGuid();
	TestTrue(TEXT("Level B secret discovery is accepted"), LevelB->MapExploration.TryMarkSecretDiscovered(SecretB, bNewlyDiscovered));

	TArray<uint8> SaveBytes;
	TestTrue(TEXT("Current v23 SaveGame serializes map exploration"), UGameplayStatics::SaveGameToMemory(Source, SaveBytes));
	UGrimrockPartySaveGame* Loaded = Cast<UGrimrockPartySaveGame>(UGameplayStatics::LoadGameFromMemory(SaveBytes));
	if (!TestNotNull(TEXT("Serialized map SaveGame loads"), Loaded))
	{
		return false;
	}
	TestTrue(TEXT("Loaded map SaveGame is compatible"), Loaded->IsCompatible());

	const FGridLevelRuntimeState* LoadedA = Loaded->DungeonRuntimeState.LevelStates.Find(TEXT("MapTile_A"));
	const FGridLevelRuntimeState* LoadedB = Loaded->DungeonRuntimeState.LevelStates.Find(TEXT("MapTile_B"));
	TestNotNull(TEXT("Level A survives round trip"), LoadedA);
	TestNotNull(TEXT("Level B survives round trip"), LoadedB);
	TestEqual(TEXT("Level A preserves exactly two explored cells"), LoadedA ? LoadedA->MapExploration.GetExploredCellCount() : 0, 2);
	TestTrue(TEXT("Level A explored cell survives"), LoadedA && LoadedA->MapExploration.IsExplored(FIntPoint(4, 7)));
	TestTrue(TEXT("Level A secret survives"), LoadedA && LoadedA->MapExploration.IsSecretDiscovered(SecretA));
	TestFalse(TEXT("Level A does not inherit Level B secret"), LoadedA && LoadedA->MapExploration.IsSecretDiscovered(SecretB));
	TestEqual(TEXT("Level B preserves exactly one explored cell"), LoadedB ? LoadedB->MapExploration.GetExploredCellCount() : 0, 1);
	TestTrue(TEXT("Level B explored cell survives"), LoadedB && LoadedB->MapExploration.IsExplored(FIntPoint(20, 21)));
	TestTrue(TEXT("Level B secret survives"), LoadedB && LoadedB->MapExploration.IsSecretDiscovered(SecretB));
	TestFalse(TEXT("Level B does not inherit Level A secret"), LoadedB && LoadedB->MapExploration.IsSecretDiscovered(SecretA));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMapMON2165MalformedTest, "Grimrock.Map.MON21_6_5.Persistence.MalformedStateRejected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMapMON2165MalformedTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGrimrockPartySaveGame* Save = NewObject<UGrimrockPartySaveGame>(GetTransientPackage());
	FGridLevelRuntimeState& Level = Save->DungeonRuntimeState.LevelStates.Add(TEXT("BrokenMap"));
	Level.LevelId = TEXT("BrokenMap");

	UScriptStruct* ExplorationStruct = FGridMapExplorationState::StaticStruct();
	FArrayProperty* CellsProperty = ExplorationStruct ? CastField<FArrayProperty>(ExplorationStruct->FindPropertyByName(TEXT("ExploredCells"))) : nullptr;
	if (!TestNotNull(TEXT("ExploredCells reflection property exists"), CellsProperty))
	{
		return false;
	}

	void* ArrayAddress = CellsProperty->ContainerPtrToValuePtr<void>(&Level.MapExploration);
	FScriptArrayHelper ArrayHelper(CellsProperty, ArrayAddress);
	ArrayHelper.Resize(3);

	TestFalse(TEXT("Malformed three-cell exploration storage is structurally invalid"), Level.MapExploration.IsStructurallyValid());
	FText Error;
	TestFalse(TEXT("SaveGame validation rejects malformed map exploration"), Save->ValidateCurrentState(Error));
	TestTrue(TEXT("Malformed map rejection identifies the affected level"), Error.ToString().Contains(TEXT("BrokenMap")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMapMON2165VersionTest, "Grimrock.Map.MON21_6_5.Persistence.ExactMatchVersion23",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMapMON2165VersionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	TestEqual(TEXT("MON21.6.5 opens exact-match SaveGame v23"), UGrimrockPartySaveGame::CurrentSaveVersion, 23);
	UGrimrockPartySaveGame* Current = NewObject<UGrimrockPartySaveGame>(GetTransientPackage());
	TestEqual(TEXT("Fresh SaveGame defaults to v23"), Current->SaveVersion, 23);
	TestTrue(TEXT("Fresh v23 SaveGame is compatible"), Current->IsCompatible());

	UGrimrockPartySaveGame* Previous = NewObject<UGrimrockPartySaveGame>(GetTransientPackage());
	Previous->SaveVersion = 22;
	FText Error;
	TestFalse(TEXT("Previous v22 is rejected without migration"), Previous->ValidateCurrentState(Error));
	TestFalse(TEXT("Previous v22 is incompatible"), Previous->IsCompatible());
	TestEqual(TEXT("Validation never rewrites v22"), Previous->SaveVersion, 22);
	TestTrue(TEXT("v22 rejection reports an error"), !Error.IsEmpty());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
