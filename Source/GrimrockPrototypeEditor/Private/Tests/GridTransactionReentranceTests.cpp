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
	for (int32 Mode = 0; Mode < 9; ++Mode)
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
			if (Mode == 1 || Mode == 2 || Mode == 3 || Mode == 5)
				Receptacle->bCanInsertItems = false;
			if (Mode == 2)
				Party.ActiveCharacters.Swap(0, 1); // No reallocation or dangling-reference dereference.
			if (Mode == 3)
				NestedSucceeded = Inventory->AddItemToCharacterInventory(0, MakeTransferItem(SourceId));
			if (Mode == 5)
				Party.ActiveCharacters.Reserve(Party.ActiveCharacters.Max() + 64);
			if (Mode == 7)
			{
				FText Error;
				const auto Before = Party;
				Inventory->ResetPartyForNewGame();
				TestFalse(TEXT("Roster replacement is refused while source is reserved"), Inventory->RestorePartyInventoryState(Before, Error));
				TestEqual(TEXT("Roster remains intact"), Party.ActiveCharacters[0].CharacterId, SourceId);
			}
			if (Mode == 8)
			{
				TestFalse(TEXT("In-flight GUID cannot be inserted into another character"), Inventory->AddItemToCharacterInventory(1, Item));
				TestFalse(TEXT("In-flight GUID cannot be copied to cursor"), Inventory->SetCursorItem(Item));
				auto Secondary = MakeTransferItem(SourceId);
				auto& SecondarySlot = Party.ActiveCharacters[0].InventorySlots[1];
				SecondarySlot.bOccupied = true;
				SecondarySlot.Item = Secondary;
				FText Error;
				const auto ResourcesBefore = Party.ActiveCharacters[0].Resources;
				TestFalse(TEXT("Reserved character cannot consume another physical item"),
					Inventory->TryConsumeSelfRestorativeQuickItem(0, Secondary.RuntimeObjectId, Error));
				TestEqual(TEXT("Refusal happens before effect projection"), Error.ToString(),
					NSLOCTEXT("GridItemActions", "ConsumeInvalidSource", "Objet ou personnage indisponible.").ToString());
				TestEqual(TEXT("Concurrent consumption keeps physical quantity"), SecondarySlot.Item.Quantity, 1);
				TestEqual(TEXT("Concurrent consumption keeps health"), Party.ActiveCharacters[0].Resources.CurrentHealth, ResourcesBefore.CurrentHealth);
				TestEqual(TEXT("Concurrent consumption keeps mana"), Party.ActiveCharacters[0].Resources.CurrentMana, ResourcesBefore.CurrentMana);
			}
			if (Mode == 4)
			{
				auto& Slot = Party.ActiveCharacters[0].InventorySlots[1];
				Slot.bOccupied = true;
				Slot.Item = MakeTransferItem(SourceId);
				NestedSucceeded = UGridItemTransferService::TransferInventorySlotToCharacter(Inventory, 0, 1, 1, 0).bSuccess;
			}
		};
		Inventory->OnPartyInventoryChanged.AddDynamic(Probe, &UGridInventoryNotificationProbe::Observe);
		if ((Mode >= 1 && Mode <= 3) || Mode == 5)
			AddExpectedError(TEXT("Receptacle cursor insert refused:"), EAutomationExpectedErrorFlags::Contains, 1);
		if ((Mode >= 1 && Mode <= 5))
			AddExpectedError(TEXT("GridItemTransfer Failed"), EAutomationExpectedErrorFlags::Contains, 1);
		if (Mode == 7)
			AddExpectedError(TEXT("RosterMutationRejected"), EAutomationExpectedErrorFlags::Contains, 2);
		const auto Result = UGridItemTransferService::TransferInventorySlotToReceptacle(Inventory, 0, 0, Receptacle);
		Inventory->OnPartyInventoryChanged.RemoveDynamic(Probe, &UGridInventoryNotificationProbe::Observe);
		Probe->Callback = nullptr;
		if (Mode == 0 || Mode == 4 || Mode >= 6)
		{
			TestTrue(TEXT("Ordinary outer transfer succeeds despite rejected nested mutation"), Result.bSuccess);
			TestEqual(TEXT("Outer item reaches destination once"), Receptacle->ContainedItems.Num(), 1);
			TestEqual(TEXT("Notification count remains synchronous"), Notifications, Mode == 0 ? 2 : 1);
			if (Mode == 4)
				TestFalse(TEXT("Nested transfer touching a reserved character is refused"), NestedSucceeded);
		}
		else
		{
			TestTrue(TEXT("Insertion failure reports restored source"), Result.Result == EGridItemTransferResult::DestinationInsertFailed);
			TestEqual(TEXT("Removal then rollback notify"), Notifications, 2);
			const int32 CurrentIndex = Inventory->ResolveActiveCharacterIndexById(SourceId);
			TestTrue(TEXT("Stable source is available after callback"), Party.ActiveCharacters.IsValidIndex(CurrentIndex));
			if (Party.ActiveCharacters.IsValidIndex(CurrentIndex))
			{
				const auto& Restored = Party.ActiveCharacters[CurrentIndex].InventorySlots[0].Item;
				TestEqual(TEXT("Snapshot GUID returns to original character"), Restored.RuntimeObjectId, Item.RuntimeObjectId);
				TestEqual(TEXT("Restored owner identity"), Restored.OwnerGuid, SourceId);
				TestEqual(TEXT("Restored current owner index"), Restored.OwnerCharacterIndex, CurrentIndex);
			}
			if (Mode == 3)
				TestFalse(TEXT("Concurrent source insertion was rejected before mutation"), NestedSucceeded);
			if (Mode == 2)
				TestTrue(TEXT("Other character is not overwritten by rollback"), Party.ActiveCharacters[0].InventorySlots[0].IsEmpty());
		}
		int32 Occurrences = 0;
		for (const auto& C : Party.ActiveCharacters)
			for (const auto& Slot : C.InventorySlots)
				if (!Slot.IsEmpty() && Slot.Item.RuntimeObjectId == Item.RuntimeObjectId)
					++Occurrences;
		for (const auto& Contained : Receptacle->ContainedItems)
			if (Contained.RuntimeObjectId == Item.RuntimeObjectId)
				++Occurrences;
		TestEqual(TEXT("Original GUID is neither lost nor duplicated"), Occurrences, 1);
		FString OwnershipError;
		TestTrue(TEXT("Final inventory ownership is valid"), Inventory->ValidateInventoryOwnership(OwnershipError));
		TestFalse(TEXT("Character reservation released on all exits"), Inventory->HasActiveCharacterTransactions());
		TestFalse(TEXT("Item reservation released"), Inventory->IsItemMutationReserved(Item.RuntimeObjectId));
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
	FRPGClassProgressionCommitResult Result;
	Probe->Callback = [&](int32 Index)
	{
		Events.Add(TEXT("Inventory"));
		TestTrue(TEXT("Committed selection visible before inventory callback"), Characters[Index].SelectedClassProgressionChoiceIds.Contains(TEXT("Choice_A")));
		Characters.Swap(0, 1);
		Characters.Reserve(Characters.Max() + 64); // Exercise the former dangling log pointer on the corrected path.
		TestFalse(TEXT("Nested commit on reserved identity is refused"),
			FRPGClassProgressionTransactionService::TryCommitChoices(Inventory, 1, { TEXT("Choice_B") }, Result));
		TestTrue(TEXT("Nested progression has explicit conflict reason"), Result.RejectReason == ERPGClassProgressionCommitRejectReason::TransactionConflict);
	};
	Inventory->OnPartyInventoryChanged.AddDynamic(Probe, &UGridInventoryNotificationProbe::Observe);
	const FDelegateHandle Handle = FRPGClassProgressionTransactionService::OnClassProgressionCommitted().AddLambda(
		[&](UGridPartyInventoryComponent* Component, int32 Index, const TArray<FName>&, int32)
		{
			if (Component != Inventory)
				return;
			Events.Add(TEXT("Progression"));
			TestEqual(TEXT("Progression event resolves stable character after permutation"), Characters[Index].CharacterId, Original);
		});
	TestTrue(TEXT("Original transaction still reports success"),
		FRPGClassProgressionTransactionService::TryCommitChoices(Inventory, 0, { TEXT("Choice_A") }, Result));
	TestTrue(TEXT("Callback cannot replace outer commit result with nested rejection"),
		Result.bCommitted && Result.RejectReason == ERPGClassProgressionCommitRejectReason::None);
	TestEqual(TEXT("Committed choices stable despite output alias in callback"), Result.CommittedChoiceIds.Num(), 1);
	TestEqual(TEXT("Committed remaining points stable"), Result.RemainingPoints, 1);
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
	const UPTRINT Before = reinterpret_cast<UPTRINT>(Characters.GetData());
	Probe->Callback = [&](int32)
	{
		Characters.Reserve(Characters.Max() + 16);
	};
	Inventory->OnPartyInventoryChanged.AddDynamic(Probe, &UGridInventoryNotificationProbe::Observe);
	Inventory->NotifyPartyInventoryChanged(0);
	Inventory->OnPartyInventoryChanged.RemoveDynamic(Probe, &UGridInventoryNotificationProbe::Observe);
	Probe->Callback = nullptr;
	TestTrue(TEXT("A callback can invalidate every retained character reference"), reinterpret_cast<UPTRINT>(Characters.GetData()) != Before);
	return true;
}

#endif
