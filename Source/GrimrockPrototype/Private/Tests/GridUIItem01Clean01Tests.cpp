#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"

#include "UI/GridInventorySlotWidget.h"
#include "UI/GridInventoryUiTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIItem01Clean01NoLegacyTooltipProjectionTest, "Grimrock.UI.Item01.Clean01.NoLegacyTooltipProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIItem01Clean01NoLegacyTooltipProjectionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UClass* SlotClass = UGridInventorySlotWidget::StaticClass();
	if (!TestNotNull(TEXT("Inventory slot widget class exists"), SlotClass))
	{
		return false;
	}

	const FName RemovedFunctions[] = {
		TEXT("GetItemTypeDisplayText"),
		TEXT("GetCompatibleEquipmentSlotsText"),
		TEXT("GetLightTooltipText")
	};
	for (const FName FunctionName : RemovedFunctions)
	{
		TestNull(*FString::Printf(TEXT("Legacy tooltip helper %s is removed"), *FunctionName.ToString()), SlotClass->FindFunctionByName(FunctionName));
	}

	TestNotNull(TEXT("Slot display-name helper remains for WBP_InventorySlot"),
		SlotClass->FindFunctionByName(TEXT("GetDisplayNameText")));
	TestNotNull(TEXT("Slot quantity helper remains for WBP_InventorySlot"),
		SlotClass->FindFunctionByName(TEXT("GetQuantityText")));
	TestNotNull(TEXT("Canonical structured tooltip projection remains"), SlotClass->FindFunctionByName(TEXT("GetTooltipView")));
	TestNotNull(TEXT("Examine fallback text remains"), SlotClass->FindFunctionByName(TEXT("GetTooltipText")));

	UScriptStruct* StatLineStruct = FGridItemTooltipStatLine::StaticStruct();
	UScriptStruct* ComparisonStruct = FGridItemTooltipEquipmentComparison::StaticStruct();
	UScriptStruct* ViewStruct = FGridItemTooltipView::StaticStruct();

	TestNull(TEXT("Legacy bIntegerValue is removed"), FindFProperty<FProperty>(StatLineStruct, TEXT("bIntegerValue")));
	TestNull(TEXT("Redundant bCanEquipToSlot is removed"), FindFProperty<FProperty>(ComparisonStruct, TEXT("bCanEquipToSlot")));
	TestNull(TEXT("Preformatted UsageSummary is removed"), FindFProperty<FProperty>(ViewStruct, TEXT("UsageSummary")));
	TestNull(TEXT("Preformatted StatSummary is removed"), FindFProperty<FProperty>(ViewStruct, TEXT("StatSummary")));
	TestNull(TEXT("Preformatted ComparisonSummary is removed"), FindFProperty<FProperty>(ViewStruct, TEXT("ComparisonSummary")));

	TestNotNull(TEXT("Structured StatLines remain"), FindFProperty<FProperty>(ViewStruct, TEXT("StatLines")));
	TestNotNull(TEXT("Structured EquipmentComparisons remain"), FindFProperty<FProperty>(ViewStruct, TEXT("EquipmentComparisons")));
	TestNull(TEXT("Old WBP equipped-item flag removed"),
		FindFProperty<FProperty>(ComparisonStruct, TEXT("bHasEquippedItem")));
	TestNull(TEXT("Duplicate equipped name removed"),
		FindFProperty<FProperty>(ComparisonStruct, TEXT("EquippedItemName")));
	TestNotNull(TEXT("Canonical equipped label remains"),
		FindFProperty<FProperty>(ComparisonStruct, TEXT("EquippedItemDisplayText")));
	TestNull(TEXT("Duplicated, preformat delta intermediate removed"),
		FindFProperty<FProperty>(StatLineStruct, TEXT("DeltaText")));
	TestNotNull(TEXT("Shared native/Examine delta text remains"),
		FindFProperty<FProperty>(StatLineStruct, TEXT("DeltaDisplayText")));

	return true;
}

#endif
