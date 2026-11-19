#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridObjectBehavior.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridReceptacleActor.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridWorldObjectITEMCLASS01ReflectionContractTest,
	"Grimrock.WorldObjects.ITEMCLASS01.ReflectionContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridWorldObjectITEMCLASS01ReflectionContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	TestNull(
		TEXT("World Object Definition no longer exposes ItemActorClass to Unreal reflection"),
		UGridWorldObjectDefinitionAsset::StaticClass()->FindPropertyByName(TEXT("ItemActorClass")));

	TestNull(
		TEXT("Receptacle actor no longer exposes ContainedItemActorClass to Unreal reflection"),
		AGridReceptacleActor::StaticClass()->FindPropertyByName(TEXT("ContainedItemActorClass")));

	TestNotNull(
		TEXT("Historical initial-content field retained only until schema retirement"),
		FGridReceptacleInitialItemConfig::StaticStruct()->FindPropertyByName(TEXT("ItemDefinition")));

	TestNotNull(
		TEXT("Generic GridItemActor remains the runtime item representation"),
		AGridItemActor::StaticClass());

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
