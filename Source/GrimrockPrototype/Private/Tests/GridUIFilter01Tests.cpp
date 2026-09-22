#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "UI/GridInventoryUiTypes.h"

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

#endif // WITH_DEV_AUTOMATION_TESTS
