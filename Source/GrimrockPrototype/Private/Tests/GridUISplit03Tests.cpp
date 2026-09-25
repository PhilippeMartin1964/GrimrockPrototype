#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "UI/GridCharacterSheetWidget.h"
#include "UI/GridInventoryBagWidget.h"
#include "UObject/UnrealType.h"
#include "UI/GridInventorySlotWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUISplit03PaperDollRoleTest, "Grimrock.UI.Split03.PaperDollRole",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUISplit03PaperDollRoleTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridInventoryBagWidget* Bag = NewObject<UGridInventoryBagWidget>();
	if (!TestNotNull(TEXT("Inventory bag exists"), Bag))
	{
		return false;
	}
	TestFalse(TEXT("A pure inventory bag has no paper-doll presentation"), Bag->HasPaperDollPresentation());

	UGridCharacterSheetWidget* Sheet = NewObject<UGridCharacterSheetWidget>();
	if (!TestNotNull(TEXT("Character sheet exists"), Sheet))
	{
		return false;
	}
	TestFalse(TEXT("An empty character-sheet test object has no authored paper doll yet"), Sheet->HasPaperDollPresentation());

	Sheet->SlotWidget_Head = NewObject<UGridInventorySlotWidget>(Sheet);
	TestTrue(TEXT("Any authored paper-doll slot marks the view as a paper-doll presentation"), Sheet->HasPaperDollPresentation());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUISplit03RemovedEquipmentSlotsTest, "Grimrock.UI.Split03.RemovedEquipmentSlots",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUISplit03RemovedEquipmentSlotsTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const UEnum* EquipmentSlotEnum = StaticEnum<EGridEquipmentSlot>();
	if (!TestNotNull(TEXT("Equipment slot enum exists"), EquipmentSlotEnum))
	{
		return false;
	}

	TestEqual(TEXT("Face equipment slot is removed"), EquipmentSlotEnum->GetIndexByNameString(TEXT("Face")), INDEX_NONE);
	TestEqual(TEXT("Earring1 equipment slot is removed"), EquipmentSlotEnum->GetIndexByNameString(TEXT("Earring1")), INDEX_NONE);
	TestEqual(TEXT("Earring2 equipment slot is removed"), EquipmentSlotEnum->GetIndexByNameString(TEXT("Earring2")), INDEX_NONE);

	UClass* InventoryWidgetClass = UGridInventoryWidget::StaticClass();
	TestNull(TEXT("Face paper-doll binding is removed"), FindFProperty<FProperty>(InventoryWidgetClass, TEXT("SlotWidget_Face")));
	TestNull(TEXT("Earring1 paper-doll binding is removed"), FindFProperty<FProperty>(InventoryWidgetClass, TEXT("SlotWidget_Earring1")));
	TestNull(TEXT("Earring2 paper-doll binding is removed"), FindFProperty<FProperty>(InventoryWidgetClass, TEXT("SlotWidget_Earring2")));
	return true;
}

#endif
