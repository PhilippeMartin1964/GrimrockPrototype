#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "GridCanonicalWeightTestWorld.h"

#include "Runtime/GridPartyInventoryComponent.h"
#include "UI/GridInventoryWidget.h"
#include "UObject/UnrealType.h"

namespace GridUIWeight01
{
	UGridPartyInventoryComponent* MakeInventory(FGridCanonicalWeightTestWorld& Fixture, int32 Strength, FName ItemId)
	{
		UGridPartyInventoryComponent* Inventory = Fixture.MakeInventory();
		if (!Inventory) return nullptr;
		Inventory->InitializeDefaultPartyIfNeeded();
		FGridCharacterInventoryState& Character = Inventory->PartyInventoryState.ActiveCharacters[0];
		Character.Attributes.Strength = Strength;

		if (!ItemId.IsNone())
		{
			FGridItemInstance Item;
			Item.RuntimeObjectId = FGuid::NewGuid();
			Item.ItemDefinitionId = ItemId;
			Item.Quantity = 1;
			Item.OwnerType = EGridItemOwnerType::World;
			if (!Inventory->AddItemToCharacterInventory(0, Item)) return nullptr;
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

	const TMap<FName, float> Weights = {{TEXT("Weight39_9"), 39.9f}, {TEXT("Weight40"), 40.0f},
		{TEXT("Weight50"), 50.0f}, {TEXT("Weight50_1"), 50.1f}, {TEXT("Weight0_1"), 0.1f}};
	FGridCanonicalWeightTestWorld Fixture(Weights);
	if (!TestTrue(TEXT("Transient canonical weight registry"), Fixture.Scope.Registry != nullptr)) return false;
	auto CheckState = [this, &Fixture, &Weights](const TCHAR* Label, int32 Strength, FName ItemId, EGridInventoryWeightState ExpectedState, bool bExpectedOverloaded)
	{
		UGridPartyInventoryComponent* Inventory = MakeInventory(Fixture, Strength, ItemId);
		if (!TestNotNull(*FString::Printf(TEXT("%s inventory exists"), Label), Inventory))
		{
			return;
		}

		FGridInventoryCharacterSummary Summary;
		if (!TestTrue(*FString::Printf(TEXT("%s summary resolves"), Label), Inventory->GetCharacterSummary(0, Summary)))
		{
			return;
		}

		TestEqual(*FString::Printf(TEXT("%s exact canonical carry weight"), Label), Summary.CurrentWeight, Weights.FindRef(ItemId));
		TestEqual(*FString::Printf(TEXT("%s weight state"), Label), static_cast<uint8>(Summary.WeightState), static_cast<uint8>(ExpectedState));
		TestEqual(*FString::Printf(TEXT("%s overload compatibility flag"), Label), Summary.bOverloaded, bExpectedOverloaded);
		TestEqual(*FString::Printf(TEXT("%s inventory slot count stays independent from weight"), Label), Summary.MaxInventorySlots, 40);
	};

	CheckState(TEXT("Below 80 percent"), 10, TEXT("Weight39_9"), EGridInventoryWeightState::Normal, false);
	CheckState(TEXT("At 80 percent"), 10, TEXT("Weight40"), EGridInventoryWeightState::Heavy, false);
	CheckState(TEXT("At capacity"), 10, TEXT("Weight50"), EGridInventoryWeightState::Heavy, false);
	CheckState(TEXT("Above capacity"), 10, TEXT("Weight50_1"), EGridInventoryWeightState::Overloaded, true);
	CheckState(TEXT("Zero weight at zero capacity"), 0, NAME_None, EGridInventoryWeightState::Normal, false);
	CheckState(TEXT("Positive weight at zero capacity"), 0, TEXT("Weight0_1"), EGridInventoryWeightState::Overloaded, true);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
