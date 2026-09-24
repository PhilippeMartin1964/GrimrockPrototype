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

	ExpectProjection(*this, Widget, { 2, 4, 5, 0, INDEX_NONE, INDEX_NONE }, TEXT("Default name ascending keeps six visible cells"));
	TestEqual(TEXT("Default name ascending reports four visible items"), Widget->GetVisibleInventoryItemCount(), 4);

	Widget->SetInventorySortMode(EGridInventorySortMode::NameDescending);
	ExpectProjection(*this, Widget, { 0, 5, 4, 2, INDEX_NONE, INDEX_NONE }, TEXT("Name descending keeps six visible cells"));

	Widget->SetInventorySortMode(EGridInventorySortMode::TypeAscending);
	ExpectProjection(*this, Widget, { 0, 4, 2, 5, INDEX_NONE, INDEX_NONE }, TEXT("Type ascending keeps six visible cells"));

	Widget->SetInventorySortMode(EGridInventorySortMode::TypeDescending);
	ExpectProjection(*this, Widget, { 5, 2, 4, 0, INDEX_NONE, INDEX_NONE }, TEXT("Type descending keeps six visible cells"));

	Widget->SetInventorySortMode(EGridInventorySortMode::WeightAscending);
	ExpectProjection(*this, Widget, { 5, 0, 4, 2, INDEX_NONE, INDEX_NONE }, TEXT("Weight ascending keeps six visible cells"));

	Widget->SetInventorySortMode(EGridInventorySortMode::WeightDescending);
	ExpectProjection(*this, Widget, { 2, 4, 0, 5, INDEX_NONE, INDEX_NONE }, TEXT("Weight descending keeps six visible cells"));
	TestEqual(TEXT("Sorted view keeps fixed widget capacity"), Widget->ResolveInventorySlotWidgetCount(), 6);

	Widget->SetInventoryFilterCategory(EGridInventoryFilterCategory::Equipment);
	Widget->SetInventorySortMode(EGridInventorySortMode::NameAscending);
	ExpectProjection(*this, Widget, { 4, 0, INDEX_NONE, INDEX_NONE, INDEX_NONE, INDEX_NONE }, TEXT("Filter plus name ascending keeps six visible cells"));
	TestEqual(TEXT("Equipment filter reports two visible items"), Widget->GetVisibleInventoryItemCount(), 2);
	TestEqual(TEXT("Equipment filter keeps fixed widget capacity"), Widget->ResolveInventorySlotWidgetCount(), 6);

	Widget->SetInventoryFilterCategory(EGridInventoryFilterCategory::Magic);
	ExpectProjection(*this, Widget,
		{ INDEX_NONE, INDEX_NONE, INDEX_NONE, INDEX_NONE, INDEX_NONE, INDEX_NONE }, TEXT("Empty filter still keeps six visible cells"));
	TestEqual(TEXT("Empty filter reports zero visible items"), Widget->GetVisibleInventoryItemCount(), 0);
	TestEqual(TEXT("Empty filter keeps fixed widget capacity"), Widget->ResolveInventorySlotWidgetCount(), 6);

	TestEqual(TEXT("Slot 0 identity unchanged"), Character.InventorySlots[0].Item.RuntimeObjectId, Slot0Id);
	TestTrue(TEXT("Slot 1 remains empty"), Character.InventorySlots[1].IsEmpty());
	TestEqual(TEXT("Slot 2 identity unchanged"), Character.InventorySlots[2].Item.RuntimeObjectId, Slot2Id);
	TestTrue(TEXT("Slot 3 remains empty"), Character.InventorySlots[3].IsEmpty());
	TestEqual(TEXT("Slot 4 identity unchanged"), Character.InventorySlots[4].Item.RuntimeObjectId, Slot4Id);
	TestEqual(TEXT("Slot 5 identity unchanged"), Character.InventorySlots[5].Item.RuntimeObjectId, Slot5Id);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIInventory021FixedCapacityTest, "Grimrock.UI.Inventory02.FixedCapacity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIInventory021FixedCapacityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
	UGridInventoryWidget* Widget = NewObject<UGridInventoryWidget>();
	if (!TestNotNull(TEXT("Party inventory exists"), Inventory) || !TestNotNull(TEXT("Inventory widget exists"), Widget))
	{
		return false;
	}

	Inventory->PartyInventoryState.ActiveCharacters.SetNum(1);
	Inventory->PartyInventoryState.SelectedCharacterIndex = 0;
	FGridCharacterInventoryState& Character = Inventory->PartyInventoryState.ActiveCharacters[0];
	Character.InventorySlots.SetNum(40);

	UGridItemDefinitionAsset* Definition = NewObject<UGridItemDefinitionAsset>(Inventory);
	Definition->ItemDefinitionId = TEXT("FixedCapacityPotion");
	Definition->DisplayName = FText::FromString(TEXT("Potion"));
	Definition->ItemType = EGridItemType::Potion;
	Definition->Weight = 0.5f;
	Inventory->RegisterItemDefinition(Definition);

	Character.InventorySlots[17].bOccupied = true;
	Character.InventorySlots[17].Item.RuntimeObjectId = FGuid::NewGuid();
	Character.InventorySlots[17].Item.ItemDefinitionId = Definition->ItemDefinitionId;
	Character.InventorySlots[17].Item.DisplayName = Definition->DisplayName;
	Character.InventorySlots[17].Item.Quantity = 1;
	Character.InventorySlots[17].Item.Weight = Definition->Weight;

	Widget->InventoryComponent = Inventory;

	TestEqual(TEXT("Default bag shows all forty cells"), Widget->ResolveInventorySlotWidgetCount(), 40);

	Widget->SetInventorySortMode(EGridInventorySortMode::NameAscending);
	TestEqual(TEXT("Sorted bag still shows all forty cells"), Widget->ResolveInventorySlotWidgetCount(), 40);
	TArray<int32> Projection;
	Widget->GetInventoryProjectionSourceSlotIndices(Projection);
	TestEqual(TEXT("Sorted item moves to first projected cell"), Projection[0], 17);
	TestEqual(TEXT("Remaining projected cells are virtual empty cells"), Projection[1], INDEX_NONE);

	Widget->SetInventoryFilterCategory(EGridInventoryFilterCategory::Equipment);
	TestEqual(TEXT("Filtered bag still shows all forty cells"), Widget->ResolveInventorySlotWidgetCount(), 40);
	TestEqual(TEXT("Filtered bag has no visible matching items"), Widget->GetVisibleInventoryItemCount(), 0);

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

	TestNotNull(TEXT("Sort combo binding exists"), FindFProperty<FProperty>(BagClass, TEXT("ComboBox_SortInventory")));
	TestNull(TEXT("Legacy sort button binding is removed"), FindFProperty<FProperty>(BagClass, TEXT("Button_SortInventory")));
	TestNull(TEXT("Legacy sort label binding is removed"), FindFProperty<FProperty>(BagClass, TEXT("Text_SortInventory")));
	TestNotNull(TEXT("Empty projection binding exists"), FindFProperty<FProperty>(BaseClass, TEXT("Text_InventoryEmptyState")));
	TestNotNull(TEXT("Native sort selection handler exists"), BagClass->FindFunctionByName(TEXT("HandleSortInventorySelectionChanged")));
	TestNotNull(TEXT("Sort setter is reflected"), BaseClass->FindFunctionByName(TEXT("SetInventorySortMode")));
	TestNull(TEXT("Legacy sort cycle API is removed"), BaseClass->FindFunctionByName(TEXT("CycleInventorySortMode")));
	TestEqual(TEXT("Name ascending label"), GetGridInventorySortModeDisplayName(EGridInventorySortMode::NameAscending).ToString(), FString(TEXT("Nom ordre croissant")));
	TestEqual(TEXT("Name descending label"), GetGridInventorySortModeDisplayName(EGridInventorySortMode::NameDescending).ToString(), FString(TEXT("Nom ordre décroissant")));
	TestEqual(TEXT("Type ascending label"), GetGridInventorySortModeDisplayName(EGridInventorySortMode::TypeAscending).ToString(), FString(TEXT("Type ordre croissant")));
	TestEqual(TEXT("Type descending label"), GetGridInventorySortModeDisplayName(EGridInventorySortMode::TypeDescending).ToString(), FString(TEXT("Type ordre décroissant")));
	TestEqual(TEXT("Weight ascending label"), GetGridInventorySortModeDisplayName(EGridInventorySortMode::WeightAscending).ToString(), FString(TEXT("Poids ordre croissant")));
	TestEqual(TEXT("Weight descending label"), GetGridInventorySortModeDisplayName(EGridInventorySortMode::WeightDescending).ToString(), FString(TEXT("Poids ordre décroissant")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
