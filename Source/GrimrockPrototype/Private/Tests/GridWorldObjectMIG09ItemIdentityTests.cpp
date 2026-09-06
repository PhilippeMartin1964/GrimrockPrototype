#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Runtime/GridItemActor.h"
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

#endif // WITH_DEV_AUTOMATION_TESTS
