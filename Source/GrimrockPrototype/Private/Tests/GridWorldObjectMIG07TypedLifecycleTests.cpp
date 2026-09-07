#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Runtime/GridItemDefinitionAsset.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridWorldObjectMIG07TypedLifecycleTest,
	"Grimrock.WorldObjects.MIG07.TypedLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridWorldObjectMIG07TypedLifecycleTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridLevelAsset* Level = NewObject<UGridLevelAsset>();
	UGridItemDefinitionAsset* ItemDefinition = NewObject<UGridItemDefinitionAsset>();
	ItemDefinition->ItemDefinitionId = TEXT("MIG07C_Item");

	FGridLevelObjectData Door;
	Door.ObjectId = FGuid::NewGuid();
	Door.Type = EGridLevelObjectType::Door;
	Door.ArchetypeId = TEXT("Door_MIG07C");
	Door.CellX = 2;
	Door.CellY = 3;
	Door.Edge = EGridEdge::North;
	Door.LocalYaw = 5.0f;
	Door.Tag = TEXT("DoorBefore");
	Door.Behavior.Transition.bIsTransition = true;
	Door.Behavior.Transition.TargetLevelId = TEXT("Target_A");
	Level->Objects.Add(Door);

	FGridLevelObjectData Item;
	Item.ObjectId = FGuid::NewGuid();
	Item.Type = EGridLevelObjectType::Item;
	Item.ItemDefinitionAsset = ItemDefinition;
	Item.CellX = 4;
	Item.CellY = 5;
	Item.Tag = TEXT("ItemBefore");
	Level->Objects.Add(Item);

	Level->EnableTypedPlacementStorageFromLegacy();
	TestTrue(TEXT("Typed storage is authoritative after explicit cut-over"), Level->bTypedPlacementStorageAuthoritative);
	TestEqual(TEXT("Door projects to one world-object instance"), Level->WorldObjectInstances.Num(), 1);
	TestEqual(TEXT("Item projects to one loose-item instance"), Level->LooseItemInstances.Num(), 1);
	if (Level->WorldObjectInstances.Num() != 1 || Level->LooseItemInstances.Num() != 1)
	{
		return false;
	}

	// Add data that cannot be represented by FGridLevelObjectData. Direct typed
	// snapshot updates must preserve these typed-only values.
	FGridWorldObjectInstance& TypedDoor = Level->WorldObjectInstances[0];
	TypedDoor.bHasLocalTransformOverride = true;
	TypedDoor.LocalTransformOverride = FTransform(FRotator(11.0f, 5.0f, 7.0f), FVector(1.0f, 2.0f, 3.0f), FVector(1.2f, 1.0f, 0.8f));

	FGridLooseItemInstance& TypedItem = Level->LooseItemInstances[0];
	TypedItem.Quantity = 6;
	TypedItem.LocalOffset = FVector(12.0f, -8.0f, 4.0f);
	Level->RefreshLegacyObjectMirrorFromTyped();

	const FGridLevelObjectData* DoorMirror = Level->Objects.FindByPredicate(
		[&Door](const FGridLevelObjectData& Object)
		{
			return Object.ObjectId == Door.ObjectId;
		});
	const FGridLevelObjectData* ItemMirror = Level->Objects.FindByPredicate(
		[&Item](const FGridLevelObjectData& Object)
		{
			return Object.ObjectId == Item.ObjectId;
		});
	TestNotNull(TEXT("Door compatibility read view exists"), DoorMirror);
	TestNotNull(TEXT("Item compatibility read view exists"), ItemMirror);
	if (!DoorMirror || !ItemMirror)
	{
		return false;
	}

	FGridLevelObjectData DoorEdit = *DoorMirror;
	DoorEdit.Tag = TEXT("DoorAfter");
	DoorEdit.CellX = 8;
	DoorEdit.LocalYaw = 55.0f;
	DoorEdit.Behavior.Transition.TargetLevelId = TEXT("Target_B");
	TestEqual(TEXT("Door snapshot writes directly into typed authority"), Level->AddObject(DoorEdit), Door.ObjectId);

	ItemMirror = Level->Objects.FindByPredicate(
		[&Item](const FGridLevelObjectData& Object)
		{
			return Object.ObjectId == Item.ObjectId;
		});
	TestNotNull(TEXT("Item read view is rebuilt after direct typed update"), ItemMirror);
	if (!ItemMirror)
	{
		return false;
	}
	FGridLevelObjectData ItemEdit = *ItemMirror;
	ItemEdit.Tag = TEXT("ItemAfter");
	ItemEdit.CellY = 9;
	TestEqual(TEXT("Item snapshot writes directly into typed authority"), Level->AddObject(ItemEdit), Item.ObjectId);

	TestEqual(TEXT("Door typed Tag follows direct editor snapshot"), Level->WorldObjectInstances[0].Tag, FName(TEXT("DoorAfter")));
	TestEqual(TEXT("Door typed CellX follows direct editor snapshot"), Level->WorldObjectInstances[0].CellX, 8);
	TestEqual(TEXT("Door transition remains instance-owned"), Level->WorldObjectInstances[0].InstanceConfig.Transition.TargetLevelId, FName(TEXT("Target_B")));
	const FTransform& PreservedTransform = Level->WorldObjectInstances[0].LocalTransformOverride;
	TestTrue(TEXT("Door typed local location survives direct snapshot edit"), PreservedTransform.GetLocation().Equals(FVector(1.0f, 2.0f, 3.0f)));
	TestTrue(TEXT("Door typed local scale survives direct snapshot edit"), PreservedTransform.GetScale3D().Equals(FVector(1.2f, 1.0f, 0.8f)));
	TestTrue(TEXT("Door typed pitch survives direct snapshot edit"), FMath::IsNearlyEqual(PreservedTransform.Rotator().Pitch, 11.0f, 0.1f));
	TestTrue(TEXT("Door typed roll survives direct snapshot edit"), FMath::IsNearlyEqual(PreservedTransform.Rotator().Roll, 7.0f, 0.1f));
	TestTrue(TEXT("Door typed yaw follows direct snapshot edit"), FMath::IsNearlyEqual(PreservedTransform.Rotator().Yaw, 55.0f, 0.1f));

	TestEqual(TEXT("Loose item Tag follows direct editor snapshot"), Level->LooseItemInstances[0].Tag, FName(TEXT("ItemAfter")));
	TestEqual(TEXT("Loose item CellY follows direct editor snapshot"), Level->LooseItemInstances[0].CellY, 9);
	TestEqual(TEXT("Loose item typed-only quantity survives direct snapshot edit"), Level->LooseItemInstances[0].Quantity, 6);
	TestTrue(TEXT("Loose item typed-only offset survives direct snapshot edit"), Level->LooseItemInstances[0].LocalOffset.Equals(FVector(12.0f, -8.0f, 4.0f)));

	FGridLevelObjectData AddedItem;
	AddedItem.Type = EGridLevelObjectType::Item;
	AddedItem.ItemDefinitionAsset = ItemDefinition;
	AddedItem.CellX = 10;
	AddedItem.CellY = 11;
	const FGuid AddedItemId = Level->AddObject(AddedItem);
	TestTrue(TEXT("Typed AddObject creates stable id"), AddedItemId.IsValid());
	TestEqual(TEXT("Typed AddObject writes loose-item collection"), Level->LooseItemInstances.Num(), 2);
	TestEqual(TEXT("Compatibility mirror follows typed AddObject"), Level->Objects.Num(), 3);

	FGridObjectLink Link;
	Link.SourceObjectId = Door.ObjectId;
	Link.TargetObjectId = AddedItemId;
	Level->Links.Add(Link);
	TestTrue(TEXT("Typed RemoveObjectById removes placement"), Level->RemoveObjectById(AddedItemId));
	TestEqual(TEXT("Typed loose-item collection shrinks after remove"), Level->LooseItemInstances.Num(), 1);
	TestEqual(TEXT("Links to removed typed placement are removed"), Level->Links.Num(), 0);

	FGridLogicObjectInstance& MissingIdLogic = Level->LogicObjects.AddDefaulted_GetRef();
	MissingIdLogic.Type = EGridLevelObjectType::Logic;
	TestFalse(TEXT("Test precondition: added logic id starts invalid"), MissingIdLogic.InstanceId.IsValid());
	Level->EnsureObjectIds();
	TestTrue(TEXT("EnsureObjectIds repairs typed logic id"), Level->LogicObjects.Last().InstanceId.IsValid());

	Level->ClearLevel();
	TestTrue(TEXT("ClearLevel preserves typed-authority mode"), Level->bTypedPlacementStorageAuthoritative);
	TestEqual(TEXT("ClearLevel clears world objects"), Level->WorldObjectInstances.Num(), 0);
	TestEqual(TEXT("ClearLevel clears loose items"), Level->LooseItemInstances.Num(), 0);
	TestEqual(TEXT("ClearLevel clears monster spawns"), Level->MonsterSpawns.Num(), 0);
	TestEqual(TEXT("ClearLevel clears item spawns"), Level->ItemSpawns.Num(), 0);
	TestEqual(TEXT("ClearLevel clears logic objects"), Level->LogicObjects.Num(), 0);
	TestEqual(TEXT("ClearLevel clears compatibility mirror"), Level->Objects.Num(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridWorldObjectMIG07TypedLifecycleSchemaTest,
	"Grimrock.WorldObjects.MIG07.TypedLifecycleSchema",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridWorldObjectMIG07TypedLifecycleSchemaTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	TestNotNull(TEXT("Typed placement authority remains reflected"),
		UGridLevelAsset::StaticClass()->FindPropertyByName(TEXT("bTypedPlacementStorageAuthoritative")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
