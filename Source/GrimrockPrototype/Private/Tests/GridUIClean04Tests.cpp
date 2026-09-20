#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "UI/GridInventoryWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIClean04NoPrimitiveDisplayHelpersTest, "Grimrock.UI.Clean04.NoPrimitiveDisplayHelpers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIClean04NoPrimitiveDisplayHelpersTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UClass* InventoryClass = UGridInventoryWidget::StaticClass();
	if (!TestNotNull(TEXT("Inventory widget class exists"), InventoryClass))
	{
		return false;
	}

	const FName RemovedFunctions[] = {
		TEXT("GetItemDisplayString"),
		TEXT("GetCursorItemDisplayText"),
		TEXT("GetMainHandDisplayText"),
		TEXT("GetOffHandDisplayText"),
		TEXT("GetInventorySlotDisplayText"),
		TEXT("GetCharacterDisplayText"),
		TEXT("GetSelectedCharacterDisplayText")
	};

	for (const FName FunctionName : RemovedFunctions)
	{
		TestNull(*FString::Printf(TEXT("Legacy primitive display helper %s is removed"), *FunctionName.ToString()),
			InventoryClass->FindFunctionByName(FunctionName));
	}

	TestNotNull(TEXT("Canonical character summary projection remains"),
		InventoryClass->FindFunctionByName(TEXT("GetCharacterSummary")));
	TestNotNull(TEXT("Canonical refresh remains"),
		InventoryClass->FindFunctionByName(TEXT("RefreshInventory")));

	return true;
}

#endif
