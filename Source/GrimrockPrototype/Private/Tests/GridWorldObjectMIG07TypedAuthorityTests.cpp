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

	FGridLevelObjectData Door;
	Door.ObjectId = FGuid::NewGuid();
	Door.Type = EGridLevelObjectType::Door;
	Door.ArchetypeId = TEXT("Door_MIG07B");
	Door.CellX = 3;
	Door.CellY = 4;
	Door.Edge = EGridEdge::East;
	Door.Behavior.Transition.bIsTransition = true;
	Door.Behavior.Transition.TargetLevelId = TEXT("Target_A");
	Door.Behavior.Lock.bStartsUnlocked = true;
	Level->Objects.Add(Door);

	FGridLevelObjectData Item;
	Item.ObjectId = FGuid::NewGuid();
	Item.Type = EGridLevelObjectType::Item;
	Item.ItemDefinitionAsset = ItemDefinition;
	Item.CellX = 5;
	Item.CellY = 6;
	Item.LocalYaw = 15.0f;
	Level->Objects.Add(Item);

	FGridLevelObjectData Monster;
	Monster.ObjectId = FGuid::NewGuid();
	Monster.Type = EGridLevelObjectType::MonsterSpawn;
	Monster.MonsterDefinitionAsset = MonsterDefinition;
	Monster.CellX = 7;
	Monster.CellY = 8;
	Monster.InitialFacing = EGridEdge::West;
	Monster.InitialMonsterState = EGridMonsterState::Dormant;
	Level->Objects.Add(Monster);

	FGridLevelObjectData Logic;
	Logic.ObjectId = FGuid::NewGuid();
	Logic.Type = EGridLevelObjectType::Logic;
	Logic.LogicId = TEXT("Logic_MIG07B");
	Logic.Logic.NodeType = EGridLogicNodeType::Latch;
	Level->Objects.Add(Logic);

	Level->EnableTypedPlacementStorageFromLegacy();
	TestTrue(TEXT("Typed placement storage becomes authoritative only through explicit cut-over"), Level->bTypedPlacementStorageAuthoritative);
	TestEqual(TEXT("Four typed placements are created"), Level->GetTypedPlacementCount(), 4);
	TestEqual(TEXT("Door becomes one world-object instance"), Level->WorldObjectInstances.Num(), 1);
	TestEqual(TEXT("Item becomes one loose-item instance"), Level->LooseItemInstances.Num(), 1);
	TestEqual(TEXT("Monster becomes one monster-spawn instance"), Level->MonsterSpawns.Num(), 1);
	TestEqual(TEXT("Logic becomes one logic-object instance"), Level->LogicObjects.Num(), 1);
	TestTrue(TEXT("Typed world-object ids use sparse behavior resolution"), Level->UsesSparseBehaviorOverrides(Door.ObjectId));

	// Poison the old monolith. In typed-authority mode it is only a mirror and must
	// never become the source of truth again.
	Level->Objects.Reset();
	const TArray<FGridLevelObjectData>& RestoredView = Level->GetObjectCompatibilityView();
	TestEqual(TEXT("Compatibility view is rebuilt from typed storage"), RestoredView.Num(), 4);

	const FGridLevelObjectData* RestoredDoor = RestoredView.FindByPredicate(
		[&Door](const FGridLevelObjectData& Object)
		{
			return Object.ObjectId == Door.ObjectId;
		});
	TestNotNull(TEXT("Door survives typed -> legacy compatibility projection"), RestoredDoor);
	if (RestoredDoor)
	{
		TestEqual(TEXT("Door definition id comes from typed storage"), RestoredDoor->ArchetypeId, FName(TEXT("Door_MIG07B")));
		TestEqual(TEXT("Door wall side comes from typed storage"), RestoredDoor->Edge, EGridEdge::East);
		TestTrue(TEXT("Door transition remains sparse instance data"), RestoredDoor->Behavior.Transition.bIsTransition);
		TestEqual(TEXT("Door transition target survives compatibility view"), RestoredDoor->Behavior.Transition.TargetLevelId, FName(TEXT("Target_A")));
		TestTrue(TEXT("Door initial lock state survives compatibility view"), RestoredDoor->Behavior.Lock.bStartsUnlocked);
	}

	const FGridLevelObjectData* RestoredItem = RestoredView.FindByPredicate(
		[&Item](const FGridLevelObjectData& Object)
		{
			return Object.ObjectId == Item.ObjectId;
		});
	TestNotNull(TEXT("Loose item survives typed -> legacy compatibility projection"), RestoredItem);
	if (RestoredItem)
	{
		TestTrue(TEXT("Loose item retains its single ItemDefinition"), RestoredItem->ItemDefinitionAsset == ItemDefinition);
		TestEqual(TEXT("Loose item local yaw survives bridge"), RestoredItem->LocalYaw, 15.0f);
	}

	const FGridLevelObjectData* RestoredMonster = RestoredView.FindByPredicate(
		[&Monster](const FGridLevelObjectData& Object)
		{
			return Object.ObjectId == Monster.ObjectId;
		});
	TestNotNull(TEXT("Monster spawn survives typed -> legacy compatibility projection"), RestoredMonster);
	if (RestoredMonster)
	{
		TestTrue(TEXT("Monster definition remains direct"), RestoredMonster->MonsterDefinitionAsset == MonsterDefinition);
		TestEqual(TEXT("Monster facing remains typed"), RestoredMonster->InitialFacing, EGridEdge::West);
		TestEqual(TEXT("Monster yaw mirror is reconstructed"), RestoredMonster->LocalYaw, 270.0f);
	}

	// Mutate the typed source and verify that the next compatibility read follows it,
	// proving that Objects is not an authority in typed mode.
	Level->WorldObjectInstances[0].InstanceConfig.Transition.TargetLevelId = TEXT("Target_B");
	Level->LooseItemInstances[0].LocalYaw = 42.0f;
	const TArray<FGridLevelObjectData>& UpdatedView = Level->GetObjectCompatibilityView();
	RestoredDoor = UpdatedView.FindByPredicate(
		[&Door](const FGridLevelObjectData& Object)
		{
			return Object.ObjectId == Door.ObjectId;
		});
	RestoredItem = UpdatedView.FindByPredicate(
		[&Item](const FGridLevelObjectData& Object)
		{
			return Object.ObjectId == Item.ObjectId;
		});
	TestTrue(TEXT("Typed door update drives compatibility view"),
		RestoredDoor && RestoredDoor->Behavior.Transition.TargetLevelId == FName(TEXT("Target_B")));
	TestTrue(TEXT("Typed item update drives compatibility view"), RestoredItem && FMath::IsNearlyEqual(RestoredItem->LocalYaw, 42.0f));

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
	TestNotNull(TEXT("Typed placement authority marker exists"), LevelClass->FindPropertyByName(TEXT("bTypedPlacementStorageAuthoritative")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
