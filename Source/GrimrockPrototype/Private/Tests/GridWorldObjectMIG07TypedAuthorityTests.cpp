#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridWorldObjectMIG07TypedAuthorityBridgeTest,
	"Grimrock.WorldObjects.MIG07.TypedAuthorityBridge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridWorldObjectMIG07TypedAuthorityBridgeTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridLevelAsset* Level = NewObject<UGridLevelAsset>();
	UGridItemDefinitionAsset* ItemDefinition = NewObject<UGridItemDefinitionAsset>();
	ItemDefinition->ItemDefinitionId = TEXT("MIG07B_Item");
	UGridMonsterDefinitionAsset* MonsterDefinition = NewObject<UGridMonsterDefinitionAsset>();

	FGridWorldObjectInstance Door;
	Door.InstanceId = FGuid::NewGuid();
	Door.Type = EGridLevelObjectType::Door;
	Door.WorldObjectDefinitionId = TEXT("Door_MIG07B");
	Door.CellX = 3;
	Door.CellY = 4;
	Door.WallSide = EGridEdge::East;
	Door.InstanceConfig.Transition.bIsTransition = true;
	Door.InstanceConfig.Transition.TargetLevelId = TEXT("Target_A");
	Door.InstanceConfig.bStartsUnlocked = true;
	Level->WorldObjectInstances.Add(Door);

	FGridLooseItemInstance Item;
	Item.InstanceId = FGuid::NewGuid();
	Item.ItemDefinition = ItemDefinition;
	Item.CellX = 5;
	Item.CellY = 6;
	Item.LocalYaw = 15.0f;
	Level->LooseItemInstances.Add(Item);

	FGridMonsterSpawnInstance Monster;
	Monster.SpawnId = FGuid::NewGuid();
	Monster.MonsterDefinition = MonsterDefinition;
	Monster.CellX = 7;
	Monster.CellY = 8;
	Monster.Facing = EGridEdge::West;
	Monster.InitialMonsterState = EGridMonsterState::Dormant;
	Level->MonsterSpawns.Add(Monster);

	FGridLogicObjectInstance Logic;
	Logic.InstanceId = FGuid::NewGuid();
	Logic.Type = EGridLevelObjectType::Logic;
	Logic.LogicId = TEXT("Logic_MIG07B");
	Logic.Logic.NodeType = EGridLogicNodeType::Latch;
	Level->LogicObjects.Add(Logic);

	TestEqual(TEXT("Native fixtures create four typed placements"), Level->GetTypedPlacementCount(), 4);
	TestEqual(TEXT("Door becomes one world-object instance"), Level->WorldObjectInstances.Num(), 1);
	TestEqual(TEXT("Item becomes one loose-item instance"), Level->LooseItemInstances.Num(), 1);
	TestEqual(TEXT("Monster becomes one monster-spawn instance"), Level->MonsterSpawns.Num(), 1);
	TestEqual(TEXT("Logic becomes one logic-object instance"), Level->LogicObjects.Num(), 1);
	TestTrue(TEXT("Typed world-object ids use sparse behavior resolution"), Level->UsesSparseBehaviorOverrides(Door.InstanceId));

	const FGridWorldObjectInstance* RestoredDoor = Level->FindWorldObjectInstanceById(Door.InstanceId);
	TestNotNull(TEXT("Door is found by native identity"), RestoredDoor);
	if (RestoredDoor)
	{
		TestEqual(TEXT("Door definition id comes from typed storage"), RestoredDoor->WorldObjectDefinitionId, FName(TEXT("Door_MIG07B")));
		TestEqual(TEXT("Door wall side comes from typed storage"), RestoredDoor->WallSide, EGridEdge::East);
		TestTrue(TEXT("Door transition remains sparse instance data"), RestoredDoor->InstanceConfig.Transition.bIsTransition);
		TestEqual(TEXT("Door transition target survives lookup"), RestoredDoor->InstanceConfig.Transition.TargetLevelId, FName(TEXT("Target_A")));
		TestTrue(TEXT("Door initial lock state survives lookup"), RestoredDoor->InstanceConfig.bStartsUnlocked);
	}

	const FGridLooseItemInstance* RestoredItem = Level->FindLooseItemInstanceById(Item.InstanceId);
	TestNotNull(TEXT("Loose item is found by native identity"), RestoredItem);
	if (RestoredItem)
	{
		TestTrue(TEXT("Loose item retains its single ItemDefinition"), RestoredItem->ItemDefinition == ItemDefinition);
		TestEqual(TEXT("Loose item local yaw survives projection"), RestoredItem->LocalYaw, 15.0f);
	}

	const FGridMonsterSpawnInstance* RestoredMonster = Level->FindMonsterSpawnInstanceById(Monster.SpawnId);
	TestNotNull(TEXT("Monster spawn is found by native identity"), RestoredMonster);
	if (RestoredMonster)
	{
		TestTrue(TEXT("Monster definition remains direct"), RestoredMonster->MonsterDefinition == MonsterDefinition);
		TestEqual(TEXT("Monster facing remains typed"), RestoredMonster->Facing, EGridEdge::West);
	}

	Level->WorldObjectInstances[0].InstanceConfig.Transition.TargetLevelId = TEXT("Target_B");
	Level->LooseItemInstances[0].LocalYaw = 42.0f;
	RestoredDoor = Level->FindWorldObjectInstanceById(Door.InstanceId);
	RestoredItem = Level->FindLooseItemInstanceById(Item.InstanceId);
	TestTrue(TEXT("Native lookup observes typed door updates"),
		RestoredDoor && RestoredDoor->InstanceConfig.Transition.TargetLevelId == FName(TEXT("Target_B")));
	TestTrue(TEXT("Native lookup observes typed item updates"), RestoredItem && FMath::IsNearlyEqual(RestoredItem->LocalYaw, 42.0f));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridWorldObjectMIG07TypedAuthoritySchemaTest,
	"Grimrock.WorldObjects.MIG07.TypedAuthoritySchema",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridWorldObjectMIG07TypedAuthoritySchemaTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const UClass* LevelClass = UGridLevelAsset::StaticClass();
	TestNull(TEXT("MIG09-E1 removes the authority migration marker"),
		LevelClass->FindPropertyByName(TEXT("bTypedPlacementStorageAuthoritative")));
	TestNotNull(TEXT("WorldObjectInstances remains reflected"), LevelClass->FindPropertyByName(TEXT("WorldObjectInstances")));
	TestNotNull(TEXT("LooseItemInstances remains reflected"), LevelClass->FindPropertyByName(TEXT("LooseItemInstances")));
	TestNotNull(TEXT("MonsterSpawns remains reflected"), LevelClass->FindPropertyByName(TEXT("MonsterSpawns")));
	TestNotNull(TEXT("ItemSpawns remains reflected"), LevelClass->FindPropertyByName(TEXT("ItemSpawns")));
	TestNotNull(TEXT("LogicObjects remains reflected"), LevelClass->FindPropertyByName(TEXT("LogicObjects")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
