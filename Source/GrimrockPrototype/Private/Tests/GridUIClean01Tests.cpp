#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"

#include "UI/GridCharacterSheetWidget.h"
#include "UI/GridInventoryBagWidget.h"
#include "UI/GridInventoryWidget.h"
#include "UI/GrimrockMenuWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIClean01NoInventoryShellFallbackTest, "Grimrock.UI.Clean01.NoInventoryShellFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIClean01NoInventoryShellFallbackTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UClass* MenuClass = UGrimrockMenuWidget::StaticClass();
	UClass* InventoryBaseClass = UGridInventoryWidget::StaticClass();
	UClass* SheetClass = UGridCharacterSheetWidget::StaticClass();
	UClass* BagClass = UGridInventoryBagWidget::StaticClass();

	if (!TestNotNull(TEXT("Menu class exists"), MenuClass) ||
		!TestNotNull(TEXT("Inventory base class exists"), InventoryBaseClass) ||
		!TestNotNull(TEXT("Character sheet class exists"), SheetClass) ||
		!TestNotNull(TEXT("Inventory bag class exists"), BagClass))
	{
		return false;
	}

	TestNull(TEXT("Menu no longer exposes Page_Inventory"),
		FindFProperty<FProperty>(MenuClass, FName(TEXT("Page_Inventory"))));
	TestNull(TEXT("Menu no longer exposes Button_TabInventory"),
		FindFProperty<FProperty>(MenuClass, FName(TEXT("Button_TabInventory"))));
	TestNull(TEXT("Menu no longer exposes OpenInventoryWorkspace"),
		MenuClass->FindFunctionByName(FName(TEXT("OpenInventoryWorkspace"))));
	TestNull(TEXT("Menu no longer exposes RefreshInventory"),
		MenuClass->FindFunctionByName(FName(TEXT("RefreshInventory"))));
	TestNull(TEXT("Menu no longer exposes GetInventoryWidget"),
		MenuClass->FindFunctionByName(FName(TEXT("GetInventoryWidget"))));

	TestNull(TEXT("Shared inventory base no longer owns monolithic character panel"),
		FindFProperty<FProperty>(InventoryBaseClass, FName(TEXT("Panel_CharacterSheet"))));
	TestNull(TEXT("Shared inventory base no longer owns monolithic bag panel"),
		FindFProperty<FProperty>(InventoryBaseClass, FName(TEXT("Panel_InventoryBag"))));
	TestNull(TEXT("Shared inventory base no longer owns character close button"),
		FindFProperty<FProperty>(InventoryBaseClass, FName(TEXT("Button_CloseCharacterSheet"))));
	TestNull(TEXT("Shared inventory base no longer owns bag close button"),
		FindFProperty<FProperty>(InventoryBaseClass, FName(TEXT("Button_CloseInventoryBag"))));
	TestNull(TEXT("Shared inventory base no longer exposes ResetInventoryWorkspace"),
		InventoryBaseClass->FindFunctionByName(FName(TEXT("ResetInventoryWorkspace"))));

	TestNotNull(TEXT("Character sheet owns its close button"),
		FindFProperty<FProperty>(SheetClass, FName(TEXT("Button_CloseCharacterSheet"))));
	TestNotNull(TEXT("Inventory bag owns its close button"),
		FindFProperty<FProperty>(BagClass, FName(TEXT("Button_CloseInventoryBag"))));

	return true;
}

#endif
