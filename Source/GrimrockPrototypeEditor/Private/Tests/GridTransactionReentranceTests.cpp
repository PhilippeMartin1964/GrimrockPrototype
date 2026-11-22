#include "GridInventoryNotificationProbe.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "GridEditorCanonicalStarterTestWorld.h"
#include "Misc/AutomationTest.h"
#include "RPG/RPGClassProgressionTransactionService.h"
#include "Runtime/GridItemTransferService.h"
#include "Runtime/GridReceptacleActor.h"
#include "../../../GrimrockPrototype/Private/Tests/RPGMON155TestHelpers.h"

namespace
{
	FGridItemInstance MakeTransferItem(const FGuid& Owner)
	{
		FGridItemInstance Item;
		Item.RuntimeObjectId = FGuid::NewGuid();
		Item.ItemDefinitionId = TEXT("Item_Potion_Healing_01");
		Item.Quantity = 1;
		Item.OwnerType = EGridItemOwnerType::CharacterInventory;
		Item.OwnerGuid = Owner;
		Item.OwnerCharacterIndex = 0;
		return Item;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridReentranceTransfer, "Grimrock.Invariants.Reentrance.TransferChronology", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridReentranceTransfer::RunTest(const FString&)
{
	FGridEditorCanonicalStarterTestWorld Scope;
	if (!TestTrue(TEXT("Real transient ItemSystem world"), Scope.Ready()))
		return false;
	for (int32 Mode = 0; Mode < 5; ++Mode)
	{
		auto* Inventory = Scope.MakeInventory();
		auto& Party = Inventory->PartyInventoryState;
		Party.ActiveCharacters.SetNum(2);
		Party.ActiveEquipment.SetNum(2);
		for (auto& C : Party.ActiveCharacters)
		{
			C.CharacterId = FGuid::NewGuid();
			C.InventorySlots.SetNum(2);
		}
		Party.SelectedCharacterIndex = 0;
		const FGuid SourceId = Party.ActiveCharacters[0].CharacterId;
		const FGridItemInstance Item = MakeTransferItem(SourceId);
		Party.ActiveCharacters[0].InventorySlots[0].bOccupied = true;
		Party.ActiveCharacters[0].InventorySlots[0].Item = Item;
		auto* Receptacle = Scope.World->SpawnActor<AGridReceptacleActor>();
		Receptacle->bAcceptAnyItem = true;
		Receptacle->bCanInsertItems = true;
		auto* Probe = NewObject<UGridInventoryNotificationProbe>(Inventory);
		int32 Notifications = 0;
		bool Entered = false;
		bool NestedSucceeded = false;
		Probe->Callback = [&](int32)
		{
			++Notifications;
			if (Entered)
				return;
			Entered = true;
			TestTrue(TEXT("First notification observes already removed source"), Party.ActiveCharacters[0].InventorySlots[0].IsEmpty());
			TestTrue(TEXT("First notification precedes destination insertion"), Receptacle->ContainedItems.IsEmpty());
			if (Mode == 0)
				Inventory->SetSelectedCharacterIndex(1);
			if (Mode == 1 || Mode == 2 || Mode == 3)
				Receptacle->bCanInsertItems = false;
			if (Mode == 2)
				Party.ActiveCharacters.Swap(0, 1); // No reallocation or dangling-reference dereference.
			if (Mode == 3)
				NestedSucceeded = Inventory->AddItemToCharacterInventory(0, MakeTransferItem(SourceId));
			if (Mode == 4)
			{
				auto& Slot = Party.ActiveCharacters[0].InventorySlots[1];
				Slot.bOccupied = true;
				Slot.Item = MakeTransferItem(SourceId);
				NestedSucceeded = UGridItemTransferService::TransferInventorySlotToCharacter(Inventory, 0, 1, 1, 0).bSuccess;
			}
		};
		Inventory->OnPartyInventoryChanged.AddDynamic(Probe, &UGridInventoryNotificationProbe::Observe);
		if (Mode >= 1 && Mode <= 3)
			AddExpectedError(TEXT("Receptacle cursor insert refused:"), EAutomationExpectedErrorFlags::Contains, 1);
		const auto Result = UGridItemTransferService::TransferInventorySlotToReceptacle(Inventory, 0, 0, Receptacle);
		Inventory->OnPartyInventoryChanged.RemoveDynamic(Probe, &UGridInventoryNotificationProbe::Observe);
		Probe->Callback = nullptr;
		if (Mode == 0 || Mode == 4)
		{
			TestTrue(TEXT("Selection change / disjoint nested transfer allows outer success"), Result.bSuccess);
			TestEqual(TEXT("Outer item reaches destination once"), Receptacle->ContainedItems.Num(), 1);
			TestEqual(TEXT("Notification count remains synchronous"), Notifications, Mode == 0 ? 2 : 3);
			if (Mode == 4)
				TestTrue(TEXT("Nested transfer commits"), NestedSucceeded);
		}
		else if (Mode == 3)
		{
			TestTrue(TEXT("Occupied rollback slot is refused"), Result.Result == EGridItemTransferResult::RollbackFailed);
			TestTrue(TEXT("Nested insertion succeeds"), NestedSucceeded);
			TestTrue(TEXT("Rollback does not overwrite nested item"), Party.ActiveCharacters[0].InventorySlots[0].Item.RuntimeObjectId != Item.RuntimeObjectId);
			TestEqual(TEXT("No rollback notification on failure"), Notifications, 2);
		}
		else
		{
			TestTrue(TEXT("Insertion failure reports restored source"), Result.Result == EGridItemTransferResult::DestinationInsertFailed);
			TestEqual(TEXT("Removal then rollback notify"), Notifications, 2);
			TestEqual(TEXT("Snapshot identity is retained"), Party.ActiveCharacters[0].InventorySlots[0].Item.RuntimeObjectId, Item.RuntimeObjectId);
			if (Mode == 2)
			{
				// Characterizes a pre-existing defect, without executing a use-after-free.
				TestTrue(TEXT("Known defect: rollback restores into the swapped character"), Party.ActiveCharacters[0].CharacterId != SourceId);
				TestEqual(TEXT("Known defect: restored owner still identifies original character"), Party.ActiveCharacters[0].InventorySlots[0].Item.OwnerGuid,
					SourceId);
				TestTrue(TEXT("Original character has lost its item"), Party.ActiveCharacters[1].InventorySlots[0].IsEmpty());
			}
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridReentranceProgression, "Grimrock.Invariants.Reentrance.ProgressionChronology",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridReentranceProgression::RunTest(const FString&)
{
	FMON155RuntimeStateGuard Guard;
	URPGClassAsset* Class = nullptr;
	auto* Inventory = MakeMON155Inventory(3, 3000, Class);
	auto& Characters = Inventory->PartyInventoryState.ActiveCharacters;
	Characters.Add(MakeMON155Character(Class, 3, 3000, TEXT("Second")));
	Inventory->PartyInventoryState.ActiveEquipment.SetNum(2);
	const FGuid Original = Characters[0].CharacterId;
	auto* Probe = NewObject<UGridInventoryNotificationProbe>(Inventory);
	TArray<FName> Events;
	Probe->Callback = [&](int32 Index)
	{
		Events.Add(TEXT("Inventory"));
		TestTrue(TEXT("Committed selection visible before inventory callback"), Characters[Index].SelectedClassProgressionChoiceIds.Contains(TEXT("Choice_A")));
		Characters.Swap(0, 1); // Stable allocation: reproduce stale identity, never a dangling pointer.
	};
	Inventory->OnPartyInventoryChanged.AddDynamic(Probe, &UGridInventoryNotificationProbe::Observe);
	const FDelegateHandle Handle = FRPGClassProgressionTransactionService::OnClassProgressionCommitted().AddLambda(
		[&](UGridPartyInventoryComponent* Component, int32 Index, const TArray<FName>&, int32)
		{
			if (Component != Inventory)
				return;
			Events.Add(TEXT("Progression"));
			TestTrue(TEXT("Known defect: event index now identifies another character"), Characters[Index].CharacterId != Original);
		});
	FRPGClassProgressionCommitResult Result;
	TestTrue(TEXT("Original transaction still reports success"),
		FRPGClassProgressionTransactionService::TryCommitChoices(Inventory, 0, { TEXT("Choice_A") }, Result));
	FRPGClassProgressionTransactionService::OnClassProgressionCommitted().Remove(Handle);
	Inventory->OnPartyInventoryChanged.RemoveDynamic(Probe, &UGridInventoryNotificationProbe::Observe);
	Probe->Callback = nullptr;
	TestEqual(TEXT("Exactly two ordered notifications"), Events.Num(), 2);
	if (Events.Num() == 2)
	{
		TestEqual(TEXT("Inventory first"), Events[0], FName(TEXT("Inventory")));
		TestEqual(TEXT("Progression second"), Events[1], FName(TEXT("Progression")));
	}
	TestEqual(TEXT("Original identity was not modified"), Characters[1].CharacterId, Original);
	TestTrue(TEXT("Original character owns the committed choice"), Characters[1].SelectedClassProgressionChoiceIds.Contains(TEXT("Choice_A")));
	// Demonstrate reference invalidation at the notification boundary without
	// allowing a production caller to dereference freed storage afterwards.
	const auto* Before = Characters.GetData();
	Probe->Callback = [&](int32)
	{
		Characters.Reserve(Characters.Max() + 16);
	};
	Inventory->OnPartyInventoryChanged.AddDynamic(Probe, &UGridInventoryNotificationProbe::Observe);
	Inventory->NotifyPartyInventoryChanged(0);
	Inventory->OnPartyInventoryChanged.RemoveDynamic(Probe, &UGridInventoryNotificationProbe::Observe);
	Probe->Callback = nullptr;
	TestTrue(TEXT("A callback can invalidate every retained character reference"), Characters.GetData() != Before);
	return true;
}

#endif
