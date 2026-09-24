#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Runtime/GridPartyInventoryComponent.h"
#include "UI/GridInventoryWidget.h"
#include "UObject/UnrealType.h"

namespace GridUIWeight01
{
	UGridPartyInventoryComponent* MakeInventory(int32 Strength, float ItemWeight)
	{
		UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
		Inventory->InitializeDefaultPartyIfNeeded();
		FGridCharacterInventoryState& Character = Inventory->PartyInventoryState.ActiveCharacters[0];
		Character.Attributes.Strength = Strength;

		if (ItemWeight > 0.0f)
		{
			FGridItemInstance Item;
			Item.RuntimeObjectId = FGuid::NewGuid();
			Item.ItemDefinitionId = TEXT("UI_WEIGHT01_Probe");
			Item.DisplayName = FText::FromString(TEXT("UI-WEIGHT01 Probe"));
			Item.Quantity = 1;
			Item.Weight = ItemWeight;
			Item.OwnerType = EGridItemOwnerType::World;
			Inventory->AddItemToCharacterInventory(0, Item);
		}
		return Inventory;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIWeight01InventoryOnlyContractTest, "Grimrock.UI.Weight01.InventoryOnlyContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIWeight01InventoryOnlyContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UClass* InventoryWidgetClass = UGridInventoryWidget::StaticClass();
	TestNotNull(TEXT("Inventory widget class exists"), InventoryWidgetClass);
	if (!InventoryWidgetClass)
	{
		return false;
	}

	TestNull(TEXT("Character sheet carry text binding is removed"), FindFProperty<FProperty>(InventoryWidgetClass, TEXT("Text_CharacterCarryWeight")));
	TestNull(TEXT("Character sheet carry progress binding is removed"), FindFProperty<FProperty>(InventoryWidgetClass, TEXT("ProgressBar_CharacterCarryWeight")));
	TestNotNull(TEXT("Inventory bag owner text binding exists"), FindFProperty<FProperty>(InventoryWidgetClass, TEXT("Text_InventoryBagOwner")));
	TestNotNull(TEXT("Inventory bag weight text binding remains"), FindFProperty<FProperty>(InventoryWidgetClass, TEXT("Text_InventoryBagWeight")));
	TestNull(TEXT("Inventory bag slot usage binding is removed"), FindFProperty<FProperty>(InventoryWidgetClass, TEXT("Text_InventoryBagSlotUsage")));
	TestNull(TEXT("Inventory bag progress binding is removed"), FindFProperty<FProperty>(InventoryWidgetClass, TEXT("ProgressBar_InventoryBagWeight")));
	TestNull(TEXT("Obsolete Blueprint weight presentation hook is removed"),
		UGridInventoryWidget::StaticClass()->FindFunctionByName(TEXT("PresentInventoryWeightState")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIWeight01DerivedPresentationStateTest, "Grimrock.UI.Weight01.DerivedPresentationState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIWeight01DerivedPresentationStateTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridUIWeight01;

	auto CheckState = [this](const TCHAR* Label, int32 Strength, float ItemWeight, EGridInventoryWeightState ExpectedState, bool bExpectedOverloaded)
	{
		UGridPartyInventoryComponent* Inventory = MakeInventory(Strength, ItemWeight);
		if (!TestNotNull(*FString::Printf(TEXT("%s inventory exists"), Label), Inventory))
		{
			return;
		}

		FGridInventoryCharacterSummary Summary;
		if (!TestTrue(*FString::Printf(TEXT("%s summary resolves"), Label), Inventory->GetCharacterSummary(0, Summary)))
		{
			return;
		}

		TestEqual(*FString::Printf(TEXT("%s weight state"), Label), static_cast<uint8>(Summary.WeightState), static_cast<uint8>(ExpectedState));
		TestEqual(*FString::Printf(TEXT("%s overload compatibility flag"), Label), Summary.bOverloaded, bExpectedOverloaded);
	};

	CheckState(TEXT("Below 80 percent"), 10, 39.9f, EGridInventoryWeightState::Normal, false);
	CheckState(TEXT("At 80 percent"), 10, 40.0f, EGridInventoryWeightState::Heavy, false);
	CheckState(TEXT("At capacity"), 10, 50.0f, EGridInventoryWeightState::Heavy, false);
	CheckState(TEXT("Above capacity"), 10, 50.1f, EGridInventoryWeightState::Overloaded, true);
	CheckState(TEXT("Zero weight at zero capacity"), 0, 0.0f, EGridInventoryWeightState::Normal, false);
	CheckState(TEXT("Positive weight at zero capacity"), 0, 0.1f, EGridInventoryWeightState::Overloaded, true);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
