#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Core/GridLevelPlacementTypes.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridWorldObjectMIG07TypedPlacementProjectionTest,
	"Grimrock.WorldObjects.MIG07.TypedPlacementProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridWorldObjectMIG07TypedPlacementProjectionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridLevelAsset* Level = NewObject<UGridLevelAsset>();

	UGridItemDefinitionAsset* ItemDefinition = NewObject<UGridItemDefinitionAsset>();
	ItemDefinition->ItemDefinitionId = TEXT("MIG07_Item");

	UGridMonsterDefinitionAsset* MonsterDefinition = NewObject<UGridMonsterDefinitionAsset>();

	FGridWorldObjectInstance Door;
	Door.InstanceId = FGuid::NewGuid();
	Door.Type = EGridLevelObjectType::Door;
	Door.WorldObjectDefinitionId = TEXT("Door_Iron");
	Door.CellX = 3;
	Door.CellY = 4;
	Door.WallSide = EGridEdge::North;
	Door.bInitiallyEnabled = true;
	Door.bInitiallyActive = false;
	Door.Tag = TEXT("MainDoor");
	Door.InstanceConfig.Transition.bIsTransition = true;
	Door.InstanceConfig.Transition.TargetLevelId = TEXT("LowerLevel");
	Door.InstanceConfig.ReceptacleInitialContent.AddDefaulted_GetRef().ItemDefinition = ItemDefinition;
	Door.InstanceConfig.bStartsUnlocked = true;
	Level->WorldObjectInstances.Add(Door);

	FGridLooseItemInstance LooseItem;
	LooseItem.InstanceId = FGuid::NewGuid();
	LooseItem.ItemDefinition = ItemDefinition;
	LooseItem.CellX = 5;
	LooseItem.CellY = 6;
	LooseItem.LocalYaw = 22.5f;
	Level->LooseItemInstances.Add(LooseItem);

	FGridMonsterSpawnInstance Monster;
	Monster.SpawnId = FGuid::NewGuid();
	Monster.MonsterDefinition = MonsterDefinition;
	Monster.CellX = 7;
	Monster.CellY = 8;
	Monster.Facing = EGridEdge::West;
	Monster.InitialMonsterState = EGridMonsterState::Dormant;
	Monster.PatrolMode = EGridMonsterPatrolMode::Loop;
	Monster.EncounterGroupId = TEXT("Encounter_A");
	Monster.EncounterWaveIndex = 2;
	Level->MonsterSpawns.Add(Monster);

	FGridItemSpawnInstance ItemSpawn;
	ItemSpawn.SpawnId = FGuid::NewGuid();
	ItemSpawn.ItemDefinition = ItemDefinition;
	ItemSpawn.CellX = 9;
	ItemSpawn.CellY = 10;
	Level->ItemSpawns.Add(ItemSpawn);

	FGridLogicObjectInstance Logic;
	Logic.InstanceId = FGuid::NewGuid();
	Logic.Type = EGridLevelObjectType::Logic;
	Logic.LogicId = TEXT("PuzzleRelay");
	Logic.CellX = 1;
	Logic.CellY = 2;
	Logic.Logic.NodeType = EGridLogicNodeType::Latch;
	Level->LogicObjects.Add(Logic);

	FGridLogicObjectInstance Story;
	Story.InstanceId = FGuid::NewGuid();
	Story.Type = EGridLevelObjectType::StoryCompanion;
	Story.LogicId = TEXT("CompanionOffer");
	Level->LogicObjects.Add(Story);

	TestEqual(TEXT("One reusable world object is projected"), Level->WorldObjectInstances.Num(), 1);
	TestEqual(TEXT("One loose item is projected"), Level->LooseItemInstances.Num(), 1);
	TestEqual(TEXT("One monster spawn is projected"), Level->MonsterSpawns.Num(), 1);
	TestEqual(TEXT("One item spawn is projected"), Level->ItemSpawns.Num(), 1);
	TestEqual(TEXT("Logic and story objects share the logical bucket"), Level->LogicObjects.Num(), 2);
	TestEqual(TEXT("Typed placement total excludes no valid placement"), Level->GetTypedPlacementCount(), 6);

	if (Level->WorldObjectInstances.Num() == 1)
	{
		const FGridWorldObjectInstance& Instance = Level->WorldObjectInstances[0];
		TestEqual(TEXT("World object keeps stable instance id"), Instance.InstanceId, Door.InstanceId);
		TestEqual(TEXT("World object references its reusable definition"), Instance.WorldObjectDefinitionId, FName(TEXT("Door_Iron")));
		TestEqual(TEXT("Wall side is separated from generic placement"), Instance.WallSide, EGridEdge::North);
		TestTrue(TEXT("Transition is retained in minimal instance config"), Instance.InstanceConfig.Transition.bIsTransition);
		TestEqual(TEXT("Transition target is retained"), Instance.InstanceConfig.Transition.TargetLevelId, FName(TEXT("LowerLevel")));
		TestTrue(TEXT("Lock initial state is instance-owned"), Instance.InstanceConfig.bStartsUnlocked);
		TestEqual(TEXT("Initial receptacle content is instance-owned"), Instance.InstanceConfig.ReceptacleInitialContent.Num(), 1);
	}

	if (Level->LooseItemInstances.Num() == 1)
	{
		const FGridLooseItemInstance& Instance = Level->LooseItemInstances[0];
		TestEqual(TEXT("Loose item keeps stable instance id"), Instance.InstanceId, LooseItem.InstanceId);
		TestTrue(TEXT("Loose item directly references ItemDefinition"), Instance.ItemDefinition == ItemDefinition);
		TestEqual(TEXT("Loose item defaults to quantity one"), Instance.Quantity, 1);
		TestEqual(TEXT("Loose item preserves local yaw bridge"), Instance.LocalYaw, 22.5f);
	}

	if (Level->MonsterSpawns.Num() == 1)
	{
		const FGridMonsterSpawnInstance& Spawn = Level->MonsterSpawns[0];
		TestEqual(TEXT("Monster SpawnId remains stable"), Spawn.SpawnId, Monster.SpawnId);
		TestTrue(TEXT("Monster spawn directly references MonsterDefinition"), Spawn.MonsterDefinition == MonsterDefinition);
		TestEqual(TEXT("Monster facing is typed"), Spawn.Facing, EGridEdge::West);
		TestEqual(TEXT("Monster initial state is typed"), Spawn.InitialMonsterState, EGridMonsterState::Dormant);
		TestEqual(TEXT("Encounter group is typed"), Spawn.EncounterGroupId, FName(TEXT("Encounter_A")));
		TestEqual(TEXT("Encounter wave is typed"), Spawn.EncounterWaveIndex, 2);
	}

	if (Level->ItemSpawns.Num() == 1)
	{
		const FGridItemSpawnInstance& Spawn = Level->ItemSpawns[0];
		TestEqual(TEXT("Item SpawnId remains stable"), Spawn.SpawnId, ItemSpawn.SpawnId);
		TestTrue(TEXT("ItemSpawn references its item definition directly"), Spawn.ItemDefinition == ItemDefinition);
		TestEqual(TEXT("ItemSpawn defaults to quantity one"), Spawn.Quantity, 1);
	}

	if (Level->LogicObjects.Num() == 2)
	{
		TestEqual(TEXT("Logic object keeps LogicId"), Level->LogicObjects[0].LogicId, FName(TEXT("PuzzleRelay")));
		TestEqual(TEXT("Logic node configuration remains typed"), Level->LogicObjects[0].Logic.NodeType, EGridLogicNodeType::Latch);
		TestEqual(TEXT("Story companion remains a logical/narrative target"), Level->LogicObjects[1].Type, EGridLevelObjectType::StoryCompanion);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridWorldObjectMIG07SchemaReflectionTest,
	"Grimrock.WorldObjects.MIG07.TypedPlacementSchema",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridWorldObjectMIG07SchemaReflectionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const UClass* LevelClass = UGridLevelAsset::StaticClass();
	TestNotNull(TEXT("WorldObjectInstances collection exists"), LevelClass->FindPropertyByName(TEXT("WorldObjectInstances")));
	TestNotNull(TEXT("LooseItemInstances collection exists"), LevelClass->FindPropertyByName(TEXT("LooseItemInstances")));
	TestNotNull(TEXT("MonsterSpawns collection exists"), LevelClass->FindPropertyByName(TEXT("MonsterSpawns")));
	TestNotNull(TEXT("ItemSpawns collection exists"), LevelClass->FindPropertyByName(TEXT("ItemSpawns")));
	TestNotNull(TEXT("LogicObjects collection exists"), LevelClass->FindPropertyByName(TEXT("LogicObjects")));

	TestEqual(TEXT("Item classifies as loose item"), GridLevelPlacementConversion::GetBucket(EGridLevelObjectType::Item), EGridLevelPlacementBucket::LooseItem);
	TestEqual(TEXT("MonsterSpawn classifies independently"), GridLevelPlacementConversion::GetBucket(EGridLevelObjectType::MonsterSpawn),
		EGridLevelPlacementBucket::MonsterSpawn);
	TestEqual(TEXT("ItemSpawn is distinct from loose item"), GridLevelPlacementConversion::GetBucket(EGridLevelObjectType::ItemSpawn),
		EGridLevelPlacementBucket::ItemSpawn);
	TestEqual(TEXT("Logic classifies as data-only logic"), GridLevelPlacementConversion::GetBucket(EGridLevelObjectType::Logic), EGridLevelPlacementBucket::LogicObject);
	TestEqual(TEXT("Door classifies as reusable world object"), GridLevelPlacementConversion::GetBucket(EGridLevelObjectType::Door),
		EGridLevelPlacementBucket::WorldObject);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
