#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelPlacementTypes.h"
#include "Core/GridObjectBehavior.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridSemanticInitialStateContractTest,
	"Grimrock.WorldObjects.InitialState.SemanticContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridSemanticInitialStateContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UScriptStruct* WorldObjectStruct = FGridWorldObjectInstance::StaticStruct();
	UScriptStruct* LooseItemStruct = FGridLooseItemInstance::StaticStruct();
	UScriptStruct* LogicObjectStruct = FGridLogicObjectInstance::StaticStruct();
	UScriptStruct* ConfigStruct = FGridWorldObjectInstanceConfig::StaticStruct();
	UScriptStruct* MonsterSpawnStruct = FGridMonsterSpawnInstance::StaticStruct();
	UScriptStruct* ItemSpawnStruct = FGridItemSpawnInstance::StaticStruct();
	UScriptStruct* PitStruct = FGridPitBehaviorParams::StaticStruct();

	TestNotNull(TEXT("World object struct exists"), WorldObjectStruct);
	TestNotNull(TEXT("Loose item struct exists"), LooseItemStruct);
	TestNotNull(TEXT("Logic object struct exists"), LogicObjectStruct);
	TestNotNull(TEXT("World object config struct exists"), ConfigStruct);
	TestNotNull(TEXT("Monster spawn struct exists"), MonsterSpawnStruct);
	TestNotNull(TEXT("Item spawn struct exists"), ItemSpawnStruct);
	TestNotNull(TEXT("Pit behavior struct exists"), PitStruct);

	if (WorldObjectStruct)
	{
		TestNull(TEXT("World object has no generic bInitiallyEnabled property"), WorldObjectStruct->FindPropertyByName(TEXT("bInitiallyEnabled")));
		TestNull(TEXT("World object has no generic bInitiallyActive property"), WorldObjectStruct->FindPropertyByName(TEXT("bInitiallyActive")));
	}
	if (LooseItemStruct)
	{
		TestNull(TEXT("Loose item has no generic bInitiallyEnabled property"), LooseItemStruct->FindPropertyByName(TEXT("bInitiallyEnabled")));
		TestNull(TEXT("Loose item has no generic bInitiallyActive property"), LooseItemStruct->FindPropertyByName(TEXT("bInitiallyActive")));
	}
	if (LogicObjectStruct)
	{
		TestNull(TEXT("Logic object has no generic bInitiallyEnabled property"), LogicObjectStruct->FindPropertyByName(TEXT("bInitiallyEnabled")));
		TestNull(TEXT("Logic object has no generic bInitiallyActive property"), LogicObjectStruct->FindPropertyByName(TEXT("bInitiallyActive")));
	}
	if (ConfigStruct)
	{
		TestNull(TEXT("Instance config has no generic bInitiallyEnabled property"), ConfigStruct->FindPropertyByName(TEXT("bInitiallyEnabled")));
		TestNull(TEXT("Instance config has no generic bInitiallyActive property"), ConfigStruct->FindPropertyByName(TEXT("bInitiallyActive")));
		TestNotNull(TEXT("Door exposes semantic bDoorInitiallyOpen"), ConfigStruct->FindPropertyByName(TEXT("bDoorInitiallyOpen")));
		TestNotNull(TEXT("Teleporter exposes semantic bTeleporterInitiallyEnabled"), ConfigStruct->FindPropertyByName(TEXT("bTeleporterInitiallyEnabled")));
		TestNotNull(TEXT("Lock exposes semantic bStartsUnlocked"), ConfigStruct->FindPropertyByName(TEXT("bStartsUnlocked")));
		TestNull(TEXT("Lever has no authored bLeverInitiallyOn override"), ConfigStruct->FindPropertyByName(TEXT("bLeverInitiallyOn")));
		TestNull(TEXT("Pressure plate has no authored bPressurePlateInitiallyPressed override"),
			ConfigStruct->FindPropertyByName(TEXT("bPressurePlateInitiallyPressed")));
	}
	if (PitStruct)
	{
		TestNotNull(TEXT("Pit exposes semantic bInitiallyOpen inside Pit behavior"), PitStruct->FindPropertyByName(TEXT("bInitiallyOpen")));
	}
	if (MonsterSpawnStruct)
	{
		TestNotNull(TEXT("Monster spawn exposes bSpawnAtStart"), MonsterSpawnStruct->FindPropertyByName(TEXT("bSpawnAtStart")));
		TestNull(TEXT("Monster spawn no longer exposes bInitiallyEnabled"), MonsterSpawnStruct->FindPropertyByName(TEXT("bInitiallyEnabled")));
		TestNull(TEXT("Monster spawn has no generic bInitiallyActive"), MonsterSpawnStruct->FindPropertyByName(TEXT("bInitiallyActive")));
	}
	if (ItemSpawnStruct)
	{
		TestNotNull(TEXT("Item spawn exposes bSpawnAtStart"), ItemSpawnStruct->FindPropertyByName(TEXT("bSpawnAtStart")));
		TestNull(TEXT("Item spawn no longer exposes bInitiallyEnabled"), ItemSpawnStruct->FindPropertyByName(TEXT("bInitiallyEnabled")));
		TestNull(TEXT("Item spawn has no generic bInitiallyActive"), ItemSpawnStruct->FindPropertyByName(TEXT("bInitiallyActive")));
	}

	UClass* DefinitionClass = UGridWorldObjectDefinitionAsset::StaticClass();
	TestNull(TEXT("Definition has no bDefaultInitiallyEnabled property"), DefinitionClass->FindPropertyByName(TEXT("bDefaultInitiallyEnabled")));
	TestNull(TEXT("Definition has no bDefaultInitiallyActive property"), DefinitionClass->FindPropertyByName(TEXT("bDefaultInitiallyActive")));

	FGridWorldObjectInstanceConfig Config;
	TestFalse(TEXT("Doors start closed by default"), Config.bDoorInitiallyOpen);
	TestTrue(TEXT("Teleporters start enabled by default"), Config.bTeleporterInitiallyEnabled);
	TestFalse(TEXT("Locks start locked by default"), Config.bStartsUnlocked);
	TestTrue(TEXT("Pits preserve their explicit semantic default"), Config.Pit.bInitiallyOpen);

	FGridMonsterSpawnInstance MonsterSpawn;
	FGridItemSpawnInstance ItemSpawn;
	TestTrue(TEXT("Monster spawns are present at start by default"), MonsterSpawn.bSpawnAtStart);
	TestTrue(TEXT("Item spawns are present at start by default"), ItemSpawn.bSpawnAtStart);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
