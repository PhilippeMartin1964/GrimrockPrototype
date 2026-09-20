#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "UI/GridInventoryWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIInventory01SelectedBagProjectionTest, "Grimrock.UI.Inventory01.SelectedBagProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIInventory01SelectedBagProjectionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
	UGridInventoryWidget* Widget = NewObject<UGridInventoryWidget>();
	TestNotNull(TEXT("Party inventory exists"), Inventory);
	TestNotNull(TEXT("Inventory widget exists"), Widget);
	if (!Inventory || !Widget)
	{
		return false;
	}

	Inventory->PartyInventoryState.ActiveCharacters.SetNum(2);
	Inventory->PartyInventoryState.SelectedCharacterIndex = 0;

	FGridCharacterInventoryState& First = Inventory->PartyInventoryState.ActiveCharacters[0];
	First.DisplayName = FText::FromString(TEXT("Ariadne"));
	First.InventorySlots.SetNum(3);

	FGridCharacterInventoryState& Second = Inventory->PartyInventoryState.ActiveCharacters[1];
	Second.DisplayName = FText::FromString(TEXT("Borin"));
	Second.InventorySlots.SetNum(5);

	Widget->InventoryComponent = Inventory;
	Widget->Text_InventoryBagTitle = NewObject<UTextBlock>(Widget);
	Widget->Text_InventoryBagSlotUsage = NewObject<UTextBlock>(Widget);
	Widget->Text_InventoryBagWeight = NewObject<UTextBlock>(Widget);
	Widget->ProgressBar_InventoryBagWeight = NewObject<UProgressBar>(Widget);

	Widget->RefreshInventory();
	TestEqual(TEXT("Bag title follows selected character 0"), Widget->Text_InventoryBagTitle->GetText().ToString(), FString(TEXT("Ariadne")));
	TestEqual(TEXT("Bag slot usage follows character 0"), Widget->Text_InventoryBagSlotUsage->GetText().ToString(), FString(TEXT("0 / 3")));
	TestEqual(TEXT("Resolved slot count follows character 0 capacity"), Widget->ResolveInventorySlotWidgetCount(), 3);
	TestEqual(TEXT("Inventory slot count reads character 0 only"), Widget->GetInventorySlotCount(), 3);

	TestTrue(TEXT("Selecting character 1 succeeds"), Widget->SelectCharacter(1));
	TestEqual(TEXT("Bag title switches to character 1"), Widget->Text_InventoryBagTitle->GetText().ToString(), FString(TEXT("Borin")));
	TestEqual(TEXT("Bag slot usage switches to character 1"), Widget->Text_InventoryBagSlotUsage->GetText().ToString(), FString(TEXT("0 / 5")));
	TestEqual(TEXT("Resolved slot count switches to character 1 capacity"), Widget->ResolveInventorySlotWidgetCount(), 5);
	TestEqual(TEXT("Inventory slot count now reads character 1 only"), Widget->GetInventorySlotCount(), 5);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIInventory01SingleBagAuthorityTest, "Grimrock.UI.Inventory01.SingleBagAuthority",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIInventory01SingleBagAuthorityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
	UGridInventoryWidget* Widget = NewObject<UGridInventoryWidget>();
	if (!TestNotNull(TEXT("Party inventory exists"), Inventory) || !TestNotNull(TEXT("Inventory widget exists"), Widget))
	{
		return false;
	}

	Inventory->PartyInventoryState.ActiveCharacters.SetNum(2);
	Inventory->PartyInventoryState.SelectedCharacterIndex = 0;
	Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots.SetNum(4);
	Inventory->PartyInventoryState.ActiveCharacters[1].InventorySlots.SetNum(9);
	Widget->InventoryComponent = Inventory;

	TestEqual(TEXT("Default UI slot override is disabled"), Widget->InventorySlotCountOverride, 0);
	TestEqual(TEXT("Single bag resolves first selected capacity"), Widget->ResolveInventorySlotWidgetCount(), 4);

	TestTrue(TEXT("Authoritative selection changes"), Inventory->SetSelectedCharacterIndex(1));
	TestEqual(TEXT("The same single bag resolves second selected capacity"), Widget->ResolveInventorySlotWidgetCount(), 9);

	Widget->InventorySlotCountOverride = 6;
	TestEqual(TEXT("Explicit development override remains available"), Widget->ResolveInventorySlotWidgetCount(), 6);

	return true;
}

#endif
