#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Runtime/GridPartyInventoryComponent.h"
#include "UI/GridInventoryBagWidget.h"
#include "UI/GridInventoryUiTypes.h"
#include "UI/GridInventoryWidget.h"
#include "UObject/UnrealType.h"

namespace GridUIInventoryProjection02Private
{
	void ExpectProjection(FAutomationTestBase& Test, UGridInventoryWidget* Widget, const TArray<int32>& Expected, const TCHAR* Label)
	{
		TArray<int32> Actual;
		Widget->GetInventoryProjectionSourceSlotIndices(Actual);
		Test.TestEqual(*FString::Printf(TEXT("%s count"), Label), Actual.Num(), Expected.Num());
		for (int32 Index = 0; Index < FMath::Min(Actual.Num(), Expected.Num()); ++Index)
		{
			Test.TestEqual(*FString::Printf(TEXT("%s[%d]"), Label, Index), Actual[Index], Expected[Index]);
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIInventory02SortingProjectionTest, "Grimrock.UI.Inventory02.SortingProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIInventory02SortingProjectionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridUIInventoryProjection02Private;

	UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
	UGridInventoryWidget* Widget = NewObject<UGridInventoryWidget>();
	if (!TestNotNull(TEXT("Party inventory exists"), Inventory) || !TestNotNull(TEXT("Inventory widget exists"), Widget))
	{
		return false;
	}

	Inventory->PartyInventoryState.ActiveCharacters.SetNum(1);
	Inventory->PartyInventoryState.SelectedCharacterIndex = 0;
	FGridCharacterInventoryState& Character = Inventory->PartyInventoryState.ActiveCharacters[0];
	Character.InventorySlots.SetNum(6);

	auto RegisterDefinition = [Inventory](FName Id, const TCHAR* DisplayName, EGridItemType Type, float Weight)
	{
		UGridItemDefinitionAsset* Definition = NewObject<UGridItemDefinitionAsset>(Inventory);
		Definition->ItemDefinitionId = Id;
		Definition->DisplayName = FText::FromString(DisplayName);
		Definition->ItemType = Type;
		Definition->Weight = Weight;
		Inventory->RegisterItemDefinition(Definition);
	};

	auto PutItem = [&Character](int32 SlotIndex, FName Id, const TCHAR* DisplayName, float Weight, int32 Quantity)
	{
		FGridInventorySlot& Slot = Character.InventorySlots[SlotIndex];
		Slot.bOccupied = true;
		Slot.Item.RuntimeObjectId = FGuid::NewGuid();
		Slot.Item.ItemDefinitionId = Id;
		Slot.Item.DisplayName = FText::FromString(DisplayName);
		Slot.Item.Weight = Weight;
		Slot.Item.Quantity = Quantity;
	};

	RegisterDefinition(TEXT("SortTorch"), TEXT("Zeta Torche"), EGridItemType::Torch, 0.5f);
	RegisterDefinition(TEXT("SortPotion"), TEXT("Alpha Potion"), EGridItemType::Potion, 1.0f);
	RegisterDefinition(TEXT("SortSword"), TEXT("Beta Épée"), EGridItemType::Weapon, 2.0f);
	RegisterDefinition(TEXT("SortBook"), TEXT("Gamma Livre"), EGridItemType::Book, 0.1f);

	PutItem(0, TEXT("SortTorch"), TEXT("Zeta Torche"), 0.5f, 1);
	PutItem(2, TEXT("SortPotion"), TEXT("Alpha Potion"), 1.0f, 3);
	PutItem(4, TEXT("SortSword"), TEXT("Beta Épée"), 2.0f, 1);
	PutItem(5, TEXT("SortBook"), TEXT("Gamma Livre"), 0.1f, 1);

	const FGuid Slot0Id = Character.InventorySlots[0].Item.RuntimeObjectId;
	const FGuid Slot2Id = Character.InventorySlots[2].Item.RuntimeObjectId;
	const FGuid Slot4Id = Character.InventorySlots[4].Item.RuntimeObjectId;
	const FGuid Slot5Id = Character.InventorySlots[5].Item.RuntimeObjectId;

	Widget->InventoryComponent = Inventory;

	ExpectProjection(*this, Widget, { 0, 1, 2, 3, 4, 5 }, TEXT("Physical order preserves complete bag grid"));
	TestEqual(TEXT("Physical order reports four visible items"), Widget->GetVisibleInventoryItemCount(), 4);

	Widget->SetInventorySortMode(EGridInventorySortMode::Name);
	ExpectProjection(*this, Widget, { 2, 4, 5, 0 }, TEXT("Name sort"));

	Widget->SetInventorySortMode(EGridInventorySortMode::Type);
	ExpectProjection(*this, Widget, { 0, 4, 2, 5 }, TEXT("Type sort"));

	Widget->SetInventorySortMode(EGridInventorySortMode::Weight);
	ExpectProjection(*this, Widget, { 5, 0, 4, 2 }, TEXT("Total weight sort"));

	Widget->SetInventoryFilterCategory(EGridInventoryFilterCategory::Equipment);
	Widget->SetInventorySortMode(EGridInventorySortMode::Name);
	ExpectProjection(*this, Widget, { 4, 0 }, TEXT("Filter plus name sort"));
	TestEqual(TEXT("Equipment filter reports two visible items"), Widget->GetVisibleInventoryItemCount(), 2);

	Widget->SetInventoryFilterCategory(EGridInventoryFilterCategory::Magic);
	TestEqual(TEXT("Empty filter reports zero visible items"), Widget->GetVisibleInventoryItemCount(), 0);

	TestEqual(TEXT("Slot 0 identity unchanged"), Character.InventorySlots[0].Item.RuntimeObjectId, Slot0Id);
	TestTrue(TEXT("Slot 1 remains empty"), Character.InventorySlots[1].IsEmpty());
	TestEqual(TEXT("Slot 2 identity unchanged"), Character.InventorySlots[2].Item.RuntimeObjectId, Slot2Id);
	TestTrue(TEXT("Slot 3 remains empty"), Character.InventorySlots[3].IsEmpty());
	TestEqual(TEXT("Slot 4 identity unchanged"), Character.InventorySlots[4].Item.RuntimeObjectId, Slot4Id);
	TestEqual(TEXT("Slot 5 identity unchanged"), Character.InventorySlots[5].Item.RuntimeObjectId, Slot5Id);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIInventory02SortCycleTest, "Grimrock.UI.Inventory02.SortCycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIInventory02SortCycleTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridInventoryWidget* Widget = NewObject<UGridInventoryWidget>();
	if (!TestNotNull(TEXT("Inventory widget exists"), Widget))
	{
		return false;
	}

	TestEqual(TEXT("Default sort is physical order"), Widget->GetInventorySortMode(), EGridInventorySortMode::PhysicalOrder);
	Widget->CycleInventorySortMode();
	TestEqual(TEXT("Cycle 1 is name"), Widget->GetInventorySortMode(), EGridInventorySortMode::Name);
	Widget->CycleInventorySortMode();
	TestEqual(TEXT("Cycle 2 is type"), Widget->GetInventorySortMode(), EGridInventorySortMode::Type);
	Widget->CycleInventorySortMode();
	TestEqual(TEXT("Cycle 3 is weight"), Widget->GetInventorySortMode(), EGridInventorySortMode::Weight);
	Widget->CycleInventorySortMode();
	TestEqual(TEXT("Cycle wraps to physical order"), Widget->GetInventorySortMode(), EGridInventorySortMode::PhysicalOrder);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIInventory02ControlsContractTest, "Grimrock.UI.Inventory02.ControlsContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIInventory02ControlsContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UClass* BagClass = UGridInventoryBagWidget::StaticClass();
	UClass* BaseClass = UGridInventoryWidget::StaticClass();
	if (!TestNotNull(TEXT("Inventory bag class exists"), BagClass) || !TestNotNull(TEXT("Inventory widget class exists"), BaseClass))
	{
		return false;
	}

	TestNotNull(TEXT("Sort button binding exists"), FindFProperty<FProperty>(BagClass, TEXT("Button_SortInventory")));
	TestNotNull(TEXT("Sort label binding exists"), FindFProperty<FProperty>(BagClass, TEXT("Text_SortInventory")));
	TestNotNull(TEXT("Empty projection binding exists"), FindFProperty<FProperty>(BaseClass, TEXT("Text_InventoryEmptyState")));
	TestNotNull(TEXT("Native sort click handler exists"), BagClass->FindFunctionByName(TEXT("HandleSortInventoryClicked")));
	TestNotNull(TEXT("Sort setter is reflected"), BaseClass->FindFunctionByName(TEXT("SetInventorySortMode")));
	TestNotNull(TEXT("Sort cycle is reflected"), BaseClass->FindFunctionByName(TEXT("CycleInventorySortMode")));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
