#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "UI/GridInventoryWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIFoundation01WorkspaceStateTest, "Grimrock.UI.Foundation01.WorkspaceState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIFoundation01WorkspaceStateTest::RunTest(const FString& Parameters)
{
	UGridInventoryWidget* InventoryWidget = NewObject<UGridInventoryWidget>();
	TestNotNull(TEXT("Inventory widget can be created for workspace-state characterization"), InventoryWidget);
	if (!InventoryWidget)
	{
		return false;
	}

	TestTrue(TEXT("Character sheet starts visible"), InventoryWidget->IsCharacterSheetPanelVisible());
	TestTrue(TEXT("Inventory bag starts visible"), InventoryWidget->IsInventoryBagPanelVisible());

	InventoryWidget->SetCharacterSheetPanelVisible(false);
	TestFalse(TEXT("Character sheet can be independently closed"), InventoryWidget->IsCharacterSheetPanelVisible());
	TestTrue(TEXT("Closing character sheet does not close inventory bag"), InventoryWidget->IsInventoryBagPanelVisible());

	InventoryWidget->SetInventoryBagPanelVisible(false);
	TestFalse(TEXT("Inventory bag can be independently closed"), InventoryWidget->IsInventoryBagPanelVisible());

	InventoryWidget->ResetInventoryWorkspace();
	TestTrue(TEXT("Workspace reset restores character sheet"), InventoryWidget->IsCharacterSheetPanelVisible());
	TestTrue(TEXT("Workspace reset restores inventory bag"), InventoryWidget->IsInventoryBagPanelVisible());

	return true;
}

#endif
