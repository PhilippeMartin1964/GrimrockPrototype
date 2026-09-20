#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"

#include "UI/GridInventoryWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIClean02NoGeneratedPaperDollLegacyTest, "Grimrock.UI.Clean02.NoGeneratedPaperDollLegacy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIClean02NoGeneratedPaperDollLegacyTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UClass* InventoryClass = UGridInventoryWidget::StaticClass();
	if (!TestNotNull(TEXT("Inventory widget class exists"), InventoryClass))
	{
		return false;
	}

	TestNull(TEXT("Deprecated runtime paper-doll builder is removed"),
		InventoryClass->FindFunctionByName(FName(TEXT("BuildPaperDollEquipmentPanel"))));
	TestNull(TEXT("Deprecated runtime paper-doll container binding is removed"),
		FindFProperty<FProperty>(InventoryClass, FName(TEXT("Border_EquipmentPanel"))));
	TestNull(TEXT("Generated paper-doll slot state is removed"),
		FindFProperty<FProperty>(InventoryClass, FName(TEXT("GeneratedPaperDollSlotWidgets"))));
	TestNull(TEXT("Generated paper-doll build flag is removed"),
		FindFProperty<FProperty>(InventoryClass, FName(TEXT("bPaperDollEquipmentPanelBuilt"))));

	TestNotNull(TEXT("Manual authored Head slot binding remains"),
		FindFProperty<FProperty>(InventoryClass, FName(TEXT("SlotWidget_Head"))));
	TestNotNull(TEXT("Manual authored OffHand slot binding remains"),
		FindFProperty<FProperty>(InventoryClass, FName(TEXT("SlotWidget_OffHand"))));

	return true;
}

#endif
