#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Runtime/GridDungeonRuntimeState.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridReceptacleActor.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridWorldObjectMIG09ReceptacleItemIdentityTest,
	"Grimrock.WorldObjects.MIG09.ReceptacleItemDefinitionAuthority",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridWorldObjectMIG09ReceptacleItemIdentityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	TestNull(TEXT("AGridItemActor no longer exposes duplicate ArchetypeId state"),
		AGridItemActor::StaticClass()->FindPropertyByName(TEXT("ArchetypeId")));
	TestNull(TEXT("Contained receptacle items no longer expose ItemArchetypeId"),
		FGridContainedReceptacleItem::StaticStruct()->FindPropertyByName(TEXT("ItemArchetypeId")));
	TestNull(TEXT("Receptacle actor no longer exposes ContainedItemArchetypeId"),
		AGridReceptacleActor::StaticClass()->FindPropertyByName(TEXT("ContainedItemArchetypeId")));

	FGridContainedReceptacleItem Item;
	Item.ItemDefinitionId = TEXT("MIG09_Item");
	TestEqual(TEXT("Contained item identity is ItemDefinitionId only"), Item.ItemDefinitionId, FName(TEXT("MIG09_Item")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridWorldObjectMIG09RuntimeItemSaveIdentityTest,
	"Grimrock.WorldObjects.MIG09.RuntimeItemSaveDefinitionAuthority",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridWorldObjectMIG09RuntimeItemSaveIdentityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	TestNull(TEXT("Runtime item save state no longer serializes ArchetypeId"),
		FGridRuntimeItemState::StaticStruct()->FindPropertyByName(TEXT("ArchetypeId")));
	TestNotNull(TEXT("Runtime item save state serializes ItemDefinitionId"),
		FGridRuntimeItemState::StaticStruct()->FindPropertyByName(TEXT("ItemDefinitionId")));

	FGridRuntimeItemState ItemState;
	ItemState.ItemDefinitionId = TEXT("MIG09_SaveItem");
	TestEqual(TEXT("Runtime item state identity is ItemDefinitionId only"), ItemState.ItemDefinitionId, FName(TEXT("MIG09_SaveItem")));

	TestNull(TEXT("Spawned item runtime entry no longer exposes ItemArchetypeId"),
		FGridSpawnedItemRuntimeEntry::StaticStruct()->FindPropertyByName(TEXT("ItemArchetypeId")));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
