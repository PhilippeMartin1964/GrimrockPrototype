#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "UObject/UnrealType.h"

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

	FGridWorldObjectInstance Door;
	Door.InstanceId = FGuid::NewGuid();
	Door.Type = EGridLevelObjectType::Door;
	Door.WorldObjectDefinitionId = TEXT("Door_MIG07C");
	Door.CellX = 2;
	Door.CellY = 3;
	Door.WallSide = EGridEdge::North;
	Door.Tag = TEXT("DoorBefore");
	Door.InstanceConfig.Transition.bIsTransition = true;
	Door.InstanceConfig.Transition.TargetLevelId = TEXT("Target_A");
	Level->WorldObjectInstances.Add(Door);

	FGridLooseItemInstance Item;
	Item.InstanceId = FGuid::NewGuid();
	Item.ItemDefinition = ItemDefinition;
	Item.CellX = 4;
	Item.CellY = 5;
	Item.Tag = TEXT("ItemBefore");
	Level->LooseItemInstances.Add(Item);

	TestEqual(TEXT("Native construction creates two typed placements"), Level->GetTypedPlacementCount(), 2);
	TestEqual(TEXT("Door projects to one world-object instance"), Level->WorldObjectInstances.Num(), 1);
	TestEqual(TEXT("Item projects to one loose-item instance"), Level->LooseItemInstances.Num(), 1);
	if (Level->WorldObjectInstances.Num() != 1 || Level->LooseItemInstances.Num() != 1)
	{
		return false;
	}

	FGridWorldObjectInstance& TypedDoor = Level->WorldObjectInstances[0];
	TypedDoor.bHasLocalTransformOverride = true;
	TypedDoor.LocalTransformOverride = FTransform(FRotator(11.0f, 5.0f, 7.0f), FVector(1.0f, 2.0f, 3.0f), FVector(1.2f, 1.0f, 0.8f));

	FGridLooseItemInstance& TypedItem = Level->LooseItemInstances[0];
	TypedItem.Quantity = 6;
	TypedItem.LocalOffset = FVector(12.0f, -8.0f, 4.0f);

	FGridWorldObjectInstance* DoorEdit = Level->FindWorldObjectInstanceById(Door.InstanceId);
	FGridLooseItemInstance* ItemEdit = Level->FindLooseItemInstanceById(Item.InstanceId);
	TestNotNull(TEXT("Door native edit lookup exists"), DoorEdit);
	TestNotNull(TEXT("Item native edit lookup exists"), ItemEdit);
	if (!DoorEdit || !ItemEdit)
	{
		return false;
	}

	DoorEdit->Tag = TEXT("DoorAfter");
	DoorEdit->CellX = 8;
	FRotator Rotation = DoorEdit->LocalTransformOverride.Rotator();
	Rotation.Yaw = 55.0f;
	DoorEdit->LocalTransformOverride.SetRotation(Rotation.Quaternion());
	DoorEdit->InstanceConfig.Transition.TargetLevelId = TEXT("Target_B");
	ItemEdit->Tag = TEXT("ItemAfter");
	ItemEdit->CellY = 9;

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

	FGridLooseItemInstance AddedItem;
	AddedItem.ItemDefinition = ItemDefinition;
	AddedItem.CellX = 10;
	AddedItem.CellY = 11;
	Level->LooseItemInstances.Add(AddedItem);
	Level->EnsureObjectIds();
	const FGuid AddedItemId = Level->LooseItemInstances.Last().InstanceId;
	TestTrue(TEXT("EnsureObjectIds creates stable item id"), AddedItemId.IsValid());
	TestEqual(TEXT("Native addition writes loose-item collection"), Level->LooseItemInstances.Num(), 2);
	TestEqual(TEXT("Typed count includes the new item"), Level->GetTypedPlacementCount(), 3);

	FGridObjectLink Link;
	Link.SourceObjectId = Door.InstanceId;
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
	TestEqual(TEXT("ClearLevel clears world objects"), Level->WorldObjectInstances.Num(), 0);
	TestEqual(TEXT("ClearLevel clears loose items"), Level->LooseItemInstances.Num(), 0);
	TestEqual(TEXT("ClearLevel clears monster spawns"), Level->MonsterSpawns.Num(), 0);
	TestEqual(TEXT("ClearLevel clears item spawns"), Level->ItemSpawns.Num(), 0);
	TestEqual(TEXT("ClearLevel clears logic objects"), Level->LogicObjects.Num(), 0);
	TestEqual(TEXT("ClearLevel clears every typed placement"), Level->GetTypedPlacementCount(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridWorldObjectMIG07TypedLifecycleSchemaTest,
	"Grimrock.WorldObjects.MIG07.TypedLifecycleSchema",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridWorldObjectMIG07TypedLifecycleSchemaTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const UClass* LevelClass = UGridLevelAsset::StaticClass();
	TestNull(TEXT("MIG09-E1 removes the serialized authority marker"), LevelClass->FindPropertyByName(TEXT("bTypedPlacementStorageAuthoritative")));

	for (const FName Name : { FName(TEXT("WorldObjectInstances")), FName(TEXT("LooseItemInstances")), FName(TEXT("MonsterSpawns")),
		FName(TEXT("ItemSpawns")), FName(TEXT("LogicObjects")) })
	{
		const FProperty* Property = LevelClass->FindPropertyByName(Name);
		TestNotNull(*FString::Printf(TEXT("%s is reflected"), *Name.ToString()), Property);
		if (Property)
		{
			TestFalse(TEXT("Typed placements are persistent authoring data"), Property->HasAnyPropertyFlags(CPF_Transient));
		}
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
