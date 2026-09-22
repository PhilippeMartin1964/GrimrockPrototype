#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Runtime/GridPartyInventoryComponent.h"

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIWeight01PresentationHookTest, "Grimrock.UI.Weight01.PresentationHook",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIWeight01PresentationHookTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UFunction* PresentationHook = UGridInventoryWidget::StaticClass()->FindFunctionByName(TEXT("PresentInventoryWeightState"));
	TestNotNull(TEXT("Weight presentation hook is exposed to Blueprint"), PresentationHook);
	if (PresentationHook)
	{
		TestTrue(TEXT("Weight presentation hook is a Blueprint event"), PresentationHook->HasAnyFunctionFlags(FUNC_BlueprintEvent));
	}
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
