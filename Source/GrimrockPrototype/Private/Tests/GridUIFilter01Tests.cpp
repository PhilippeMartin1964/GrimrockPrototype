#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Runtime/GridPartyInventoryComponent.h"
#include "UI/GridInventoryBagWidget.h"
#include "UI/GridInventoryUiTypes.h"
#include "UI/GridInventoryWidget.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIFilter01CategoryMappingTest, "Grimrock.UI.Filter01.CategoryMapping",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIFilter01CategoryMappingTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	auto ExpectCategory = [this](EGridItemType ItemType, EGridInventoryFilterCategory ExpectedCategory, const TCHAR* Label)
	{
		TestEqual(Label, static_cast<uint8>(ResolveGridInventoryFilterCategory(ItemType)), static_cast<uint8>(ExpectedCategory));
		TestTrue(*FString::Printf(TEXT("%s matches All"), Label),
			DoesGridItemTypeMatchInventoryFilter(ItemType, EGridInventoryFilterCategory::All));
		TestTrue(*FString::Printf(TEXT("%s matches its category"), Label),
			DoesGridItemTypeMatchInventoryFilter(ItemType, ExpectedCategory));
	};

	ExpectCategory(EGridItemType::Torch, EGridInventoryFilterCategory::Equipment, TEXT("Torch"));
	ExpectCategory(EGridItemType::Weapon, EGridInventoryFilterCategory::Equipment, TEXT("Weapon"));
	ExpectCategory(EGridItemType::Shield, EGridInventoryFilterCategory::Equipment, TEXT("Shield"));
	ExpectCategory(EGridItemType::Armor, EGridInventoryFilterCategory::Equipment, TEXT("Armor"));
	ExpectCategory(EGridItemType::Jewelry, EGridInventoryFilterCategory::Equipment, TEXT("Jewelry"));
	ExpectCategory(EGridItemType::Potion, EGridInventoryFilterCategory::Consumables, TEXT("Potion"));
	ExpectCategory(EGridItemType::Food, EGridInventoryFilterCategory::Consumables, TEXT("Food"));
	ExpectCategory(EGridItemType::Scroll, EGridInventoryFilterCategory::Magic, TEXT("Scroll"));
	ExpectCategory(EGridItemType::Gem, EGridInventoryFilterCategory::Magic, TEXT("Gem"));
	ExpectCategory(EGridItemType::Component, EGridInventoryFilterCategory::Ingredients, TEXT("Component"));
	ExpectCategory(EGridItemType::Key, EGridInventoryFilterCategory::BooksAndKeys, TEXT("Key"));
	ExpectCategory(EGridItemType::Book, EGridInventoryFilterCategory::BooksAndKeys, TEXT("Book"));
	ExpectCategory(EGridItemType::Quest, EGridInventoryFilterCategory::Misc, TEXT("Quest"));
	ExpectCategory(EGridItemType::Misc, EGridInventoryFilterCategory::Misc, TEXT("Misc"));
	ExpectCategory(EGridItemType::None, EGridInventoryFilterCategory::Misc, TEXT("None"));

	TestFalse(TEXT("Weapon does not match Consumables"),
		DoesGridItemTypeMatchInventoryFilter(EGridItemType::Weapon, EGridInventoryFilterCategory::Consumables));
	TestFalse(TEXT("Potion does not match Equipment"),
		DoesGridItemTypeMatchInventoryFilter(EGridItemType::Potion, EGridInventoryFilterCategory::Equipment));
	TestFalse(TEXT("Book does not match Magic"),
		DoesGridItemTypeMatchInventoryFilter(EGridItemType::Book, EGridInventoryFilterCategory::Magic));

	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIFilter01GridProjectionTest, "Grimrock.UI.Filter01.GridProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIFilter01GridProjectionTest::RunTest(const FString& Parameters)
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
	Character.InventorySlots.SetNum(6);

	auto RegisterDefinition = [Inventory](FName Id, EGridItemType Type)
	{
		UGridItemDefinitionAsset* Definition = NewObject<UGridItemDefinitionAsset>(Inventory);
		Definition->ItemDefinitionId = Id;
		Definition->DisplayName = FText::FromName(Id);
		Definition->ItemType = Type;
		Inventory->RegisterItemDefinition(Definition);
	};

	auto PutItem = [&Character](int32 SlotIndex, FName Id)
	{
		FGridInventorySlot& Slot = Character.InventorySlots[SlotIndex];
		Slot.bOccupied = true;
		Slot.Item.RuntimeObjectId = FGuid::NewGuid();
		Slot.Item.ItemDefinitionId = Id;
		Slot.Item.DisplayName = FText::FromName(Id);
		Slot.Item.Quantity = 1;
	};

	RegisterDefinition(TEXT("FilterWeapon"), EGridItemType::Weapon);
	RegisterDefinition(TEXT("FilterPotion"), EGridItemType::Potion);
	RegisterDefinition(TEXT("FilterBook"), EGridItemType::Book);
	PutItem(0, TEXT("FilterWeapon"));
	PutItem(2, TEXT("FilterPotion"));
	PutItem(4, TEXT("FilterBook"));
	PutItem(5, TEXT("FilterMissingDefinition"));

	const FGuid WeaponRuntimeId = Character.InventorySlots[0].Item.RuntimeObjectId;
	const FGuid PotionRuntimeId = Character.InventorySlots[2].Item.RuntimeObjectId;
	const FGuid BookRuntimeId = Character.InventorySlots[4].Item.RuntimeObjectId;
	const FGuid MissingRuntimeId = Character.InventorySlots[5].Item.RuntimeObjectId;

	Widget->InventoryComponent = Inventory;

	TestEqual(TEXT("All keeps the full physical grid capacity"), Widget->ResolveInventorySlotWidgetCount(), 6);

	Widget->SetInventoryFilterCategory(EGridInventoryFilterCategory::Equipment);
	TestEqual(TEXT("Equipment projects only the weapon"), Widget->ResolveInventorySlotWidgetCount(), 1);

	Widget->SetInventoryFilterCategory(EGridInventoryFilterCategory::Consumables);
	TestEqual(TEXT("Consumables projects only the potion"), Widget->ResolveInventorySlotWidgetCount(), 1);

	Widget->SetInventoryFilterCategory(EGridInventoryFilterCategory::BooksAndKeys);
	TestEqual(TEXT("Books and keys projects only the book"), Widget->ResolveInventorySlotWidgetCount(), 1);

	Widget->SetInventoryFilterCategory(EGridInventoryFilterCategory::Misc);
	TestEqual(TEXT("Missing definition falls back to Misc"), Widget->ResolveInventorySlotWidgetCount(), 1);

	Widget->SetInventoryFilterCategory(EGridInventoryFilterCategory::Magic);
	TestEqual(TEXT("No magic item yields an empty filtered projection"), Widget->ResolveInventorySlotWidgetCount(), 0);

	Widget->SetInventoryFilterCategory(EGridInventoryFilterCategory::All);
	TestEqual(TEXT("Returning to All restores the full grid"), Widget->ResolveInventorySlotWidgetCount(), 6);

	TestEqual(TEXT("Weapon remains in physical slot 0"), Character.InventorySlots[0].Item.RuntimeObjectId, WeaponRuntimeId);
	TestTrue(TEXT("Physical slot 1 remains empty"), Character.InventorySlots[1].IsEmpty());
	TestEqual(TEXT("Potion remains in physical slot 2"), Character.InventorySlots[2].Item.RuntimeObjectId, PotionRuntimeId);
	TestTrue(TEXT("Physical slot 3 remains empty"), Character.InventorySlots[3].IsEmpty());
	TestEqual(TEXT("Book remains in physical slot 4"), Character.InventorySlots[4].Item.RuntimeObjectId, BookRuntimeId);
	TestEqual(TEXT("Unresolved item remains in physical slot 5"), Character.InventorySlots[5].Item.RuntimeObjectId, MissingRuntimeId);

	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIFilter01BagControlsContractTest, "Grimrock.UI.Filter01.BagControlsContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIFilter01BagControlsContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UClass* BagClass = UGridInventoryBagWidget::StaticClass();
	if (!TestNotNull(TEXT("Inventory bag class exists"), BagClass))
	{
		return false;
	}

	const FName ButtonNames[] = {
		TEXT("Button_FilterAll"),
		TEXT("Button_FilterEquipment"),
		TEXT("Button_FilterConsumables"),
		TEXT("Button_FilterMagic"),
		TEXT("Button_FilterIngredients"),
		TEXT("Button_FilterBooksAndKeys"),
		TEXT("Button_FilterMisc")
	};

	for (const FName ButtonName : ButtonNames)
	{
		TestNotNull(*FString::Printf(TEXT("%s is part of the native bag contract"), *ButtonName.ToString()),
			FindFProperty<FProperty>(BagClass, ButtonName));
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
