#if WITH_DEV_AUTOMATION_TESTS

#include "GridEditorCanonicalStarterTestWorld.h"
#include "GridInventoryNotificationProbe.h"
#include "Misc/AutomationTest.h"
#include "Core/GridLevelAsset.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridItemTransferService.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridReceptacleActor.h"
#include "Runtime/GridWallLockActor.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Tests/GridAutomationSaveIsolation.h"
#include "../../../GrimrockPrototype/Private/Tests/RPGMON155TestHelpers.h"

namespace
{
	UGridPartyInventoryComponent* MakeParty(FGridEditorCanonicalStarterTestWorld& World)
	{
		auto* Inventory = World.MakeInventory();
		if (!Inventory)
			return nullptr;
		auto& Party = Inventory->PartyInventoryState;
		Party.ActiveCharacters.SetNum(2);
		Party.ActiveEquipment.SetNum(2);
		Party.SelectedCharacterIndex = 0;
		for (auto& Character : Party.ActiveCharacters)
		{
			Character.CharacterId = FGuid::NewGuid();
			Character.InventorySlots.SetNum(3);
		}
		return Inventory;
	}

	FGridItemInstance MakeItem(UGridPartyInventoryComponent& Inventory, int32 Index, FName Id, int32 Quantity = 1)
	{
		FGridItemInstance Item;
		Item.RuntimeObjectId = FGuid::NewGuid();
		Item.ItemDefinitionId = Id;
		Item.Quantity = Quantity;
		Item.OwnerType = EGridItemOwnerType::CharacterInventory;
		Item.OwnerGuid = Inventory.PartyInventoryState.ActiveCharacters[Index].CharacterId;
		Item.OwnerCharacterIndex = Index;
		return Item;
	}

	void PutItem(UGridPartyInventoryComponent& Inventory, int32 Index, const FGridItemInstance& Item)
	{
		auto& Slot = Inventory.PartyInventoryState.ActiveCharacters[Index].InventorySlots[0];
		Slot.bOccupied = true;
		Slot.Item = Item;
	}

	// Emulates a coherent native roster reorder, including the parallel equipment
	// array and derived ownership indices. No identity is changed.
	void PermuteAndReallocate(UGridPartyInventoryComponent& Inventory)
	{
		auto& Party = Inventory.PartyInventoryState;
		Party.ActiveCharacters.Swap(0, 1);
		Party.ActiveEquipment.Swap(0, 1);
		Party.ActiveCharacters.Reserve(Party.ActiveCharacters.Max() + 32);
		Party.ActiveEquipment.Reserve(Party.ActiveEquipment.Max() + 32);
		for (int32 Index = 0; Index < Party.ActiveCharacters.Num(); ++Index)
		{
			for (auto& Slot : Party.ActiveCharacters[Index].InventorySlots)
				if (!Slot.IsEmpty())
					Slot.Item.OwnerCharacterIndex = Index;
			for (int32 Raw = static_cast<int32>(EGridEquipmentSlot::MainHand); Raw <= static_cast<int32>(EGridEquipmentSlot::Bracers); ++Raw)
				if (auto* Item = Party.ActiveEquipment[Index].GetMutableSlot(static_cast<EGridEquipmentSlot>(Raw)); Item && Item->IsValid())
					Item->OwnerCharacterIndex = Index;
		}
	}

	int32 CountGuid(const UGridPartyInventoryComponent& Inventory, const FGuid& Id, const AGridReceptacleActor* Receptacle = nullptr)
	{
		int32 Count = 0;
		for (const auto& Character : Inventory.PartyInventoryState.ActiveCharacters)
			for (const auto& Slot : Character.InventorySlots)
				if (!Slot.IsEmpty() && Slot.Item.RuntimeObjectId == Id)
					++Count;
		for (const auto& Equipment : Inventory.PartyInventoryState.ActiveEquipment)
			for (int32 Raw = static_cast<int32>(EGridEquipmentSlot::MainHand); Raw <= static_cast<int32>(EGridEquipmentSlot::Bracers); ++Raw)
				if (const auto* Item = Equipment.GetSlot(static_cast<EGridEquipmentSlot>(Raw)); Item && Item->IsValid() && Item->RuntimeObjectId == Id)
					++Count;
		if (Receptacle)
			for (const auto& Item : Receptacle->ContainedItems)
				if (Item.RuntimeObjectId == Id)
					++Count;
		return Count;
	}

	AGridReceptacleActor* MakeReceptacle(FGridEditorCanonicalStarterTestWorld& World)
	{
		auto* Receptacle = World.World->SpawnActor<AGridReceptacleActor>();
		Receptacle->bAcceptAnyItem = true;
		Receptacle->bCanInsertItems = true;
		Receptacle->SetCanRemoveItem(true);
		return Receptacle;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridSecureEquipmentTransfer, "Grimrock.Invariants.Reentrance.EquipmentTransfer", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridSecureEquipmentTransfer::RunTest(const FString&)
{
	FGridEditorCanonicalStarterTestWorld World;
	if (!TestTrue(TEXT("Transient ItemSystem ready"), World.Ready()))
		return false;
	for (const bool FailInsertion : { false, true })
	{
		auto* Inventory = MakeParty(World);
		auto Item = MakeItem(*Inventory, 0, TEXT("Item_Potion_Healing_01"));
		Item.OwnerType = EGridItemOwnerType::EquipmentSlot;
		Item.EquipmentSlot = EGridEquipmentSlot::QuickSlot1;
		*Inventory->PartyInventoryState.ActiveEquipment[0].GetMutableSlot(Item.EquipmentSlot) = Item;
		auto* Receptacle = MakeReceptacle(World);
		auto* Probe = NewObject<UGridInventoryNotificationProbe>(Inventory);
		TArray<int32> Notifications;
		Probe->Callback = [&](int32 Index)
		{
			Notifications.Add(Index);
			if (Notifications.Num() != 1)
				return;
			TestTrue(TEXT("Source is removed before first notification"),
				!Inventory->PartyInventoryState.ActiveEquipment[Index].GetSlot(Item.EquipmentSlot)->IsValid());
			TestTrue(TEXT("Destination is still empty at notification"), Receptacle->ContainedItems.IsEmpty());
			PermuteAndReallocate(*Inventory);
			if (FailInsertion)
				Receptacle->bCanInsertItems = false;
		};
		Inventory->OnPartyInventoryChanged.AddDynamic(Probe, &UGridInventoryNotificationProbe::Observe);
		if (FailInsertion)
		{
			AddExpectedError(TEXT("Receptacle cursor insert refused:"), EAutomationExpectedErrorFlags::Contains, 1);
			AddExpectedError(TEXT("GridItemTransfer Failed"), EAutomationExpectedErrorFlags::Contains, 1);
		}
		const auto Result = UGridItemTransferService::TransferEquipmentSlotToReceptacle(Inventory, 0, Item.EquipmentSlot, Receptacle);
		Inventory->OnPartyInventoryChanged.RemoveDynamic(Probe, &UGridInventoryNotificationProbe::Observe);
		Probe->Callback = nullptr;
		TestEqual(TEXT("Return matches insertion outcome"), Result.bSuccess, !FailInsertion);
		TestEqual(TEXT("Source GUID remains owned exactly once"), CountGuid(*Inventory, Item.RuntimeObjectId, Receptacle), 1);
		const int32 CurrentIndex = Inventory->ResolveActiveCharacterIndexById(Item.OwnerGuid);
		const auto* Equipped = Inventory->PartyInventoryState.ActiveEquipment[CurrentIndex].GetSlot(Item.EquipmentSlot);
		TestEqual(TEXT("Exactly one source notification plus optional rollback"), Notifications.Num(), FailInsertion ? 2 : 1);
		if (FailInsertion)
		{
			TestEqual(TEXT("Rollback notification follows the original owner"), Notifications.Last(), CurrentIndex);
			TestEqual(TEXT("Restored equipment GUID"), Equipped->RuntimeObjectId, Item.RuntimeObjectId);
			TestEqual(TEXT("Restored equipment owner"), Equipped->OwnerGuid, Item.OwnerGuid);
			TestEqual(TEXT("Restored equipment index"), Equipped->OwnerCharacterIndex, CurrentIndex);
			TestEqual(TEXT("Restored equipment quantity"), Equipped->Quantity, 1);
		}
		else
		{
			TestFalse(TEXT("Successful transfer leaves equipment empty"), Equipped->IsValid());
			TestEqual(TEXT("Destination quantity preserved"), Receptacle->ContainedItems[0].Quantity, 1);
		}
		FString Error;
		TestTrue(TEXT("Final equipment ownership valid"), Inventory->ValidateInventoryOwnership(Error));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridRegisteredEquipmentStackRestore, "Grimrock.Inventory.Equipment.RegisteredStackExtractionAndRestore",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridRegisteredEquipmentStackRestore::RunTest(const FString&)
{
	FGridEditorCanonicalStarterTestWorld World;
	if (!TestTrue(TEXT("Registered test world"), World.Ready()))
		return false;
	auto* Inventory = MakeParty(World);
	if (!TestNotNull(TEXT("Test inventory"), Inventory))
		return false;

	auto Stack = MakeItem(*Inventory, 0, TEXT("Shuriken"), 2);
	Stack.OwnerType = EGridItemOwnerType::EquipmentSlot;
	Stack.EquipmentSlot = EGridEquipmentSlot::MainHand;
	FGridResolvedItem Resolved;
	if (!TestTrue(TEXT("Registered N0 shuriken is stackable and can be equipped"),
		World.Registry->ResolveItem(Stack.ItemDefinitionId, 0, Resolved) && Resolved.MaxStack >= 2 &&
		Inventory->CanEquipItemToSlot(0, Stack, Stack.EquipmentSlot)))
		return false;
	auto& MainHand = Inventory->PartyInventoryState.ActiveEquipment[0].MainHand;
	MainHand = Stack;
	auto* Probe = NewObject<UGridInventoryNotificationProbe>(Inventory);
	TArray<int32> ObservedQuantities;
	Probe->Callback = [&](int32 Index)
	{
		TestEqual(TEXT("Notification belongs to the source character"), Index, 0);
		ObservedQuantities.Add(MainHand.Quantity);
	};
	Inventory->OnPartyInventoryChanged.AddDynamic(Probe, &UGridInventoryNotificationProbe::Observe);
	FGridItemInstance Unit;
	const bool bExtracted = Inventory->TryExtractOneEquippedItemForWorldTransfer(0, Stack.EquipmentSlot, Stack.ItemDefinitionId, Unit);
	TestTrue(TEXT("One unit is extracted"), bExtracted);
	TestEqual(TEXT("Source stack retains its GUID"), MainHand.RuntimeObjectId, Stack.RuntimeObjectId);
	TestEqual(TEXT("One unit remains"), MainHand.Quantity, 1);
	TestEqual(TEXT("The extracted unit has quantity one"), Unit.Quantity, 1);
	TestNotEqual(TEXT("The extracted unit gets a separate GUID"), Unit.RuntimeObjectId, Stack.RuntimeObjectId);
	TestEqual(TEXT("The extracted unit belongs to the world"), Unit.OwnerType, EGridItemOwnerType::World);
	TestFalse(TEXT("The extracted unit has no character owner"), Unit.OwnerGuid.IsValid());
	TestTrue(TEXT("Failed spawn restores the unit"), bExtracted &&
		Inventory->TryRestoreExtractedItemToEquipment(0, Stack.EquipmentSlot, Unit));
	Inventory->OnPartyInventoryChanged.RemoveDynamic(Probe, &UGridInventoryNotificationProbe::Observe);
	Probe->Callback = nullptr;
	TestTrue(TEXT("Notifications occur after debit and restoration in order"), ObservedQuantities == TArray<int32>{1, 2});
	TestTrue(TEXT("Restored equipment matches original identity, owner and quantity"),
		FGridItemInstance::StaticStruct()->CompareScriptStruct(&MainHand, &Stack, 0));
	TestEqual(TEXT("Stack identity has one owner"), CountGuid(*Inventory, Stack.RuntimeObjectId), 1);
	TestEqual(TEXT("Merged unit has no separate owner"), CountGuid(*Inventory, Unit.RuntimeObjectId), 0);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridSecurePartyTransfer, "Grimrock.Invariants.Reentrance.PartyTransfer", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridSecurePartyTransfer::RunTest(const FString&)
{
	FGridEditorCanonicalStarterTestWorld World;
	if (!TestTrue(TEXT("Transient ItemSystem ready"), World.Ready()))
		return false;
	for (const bool Split : { false, true })
	{
		auto* Inventory = MakeParty(World);
		const auto Item = MakeItem(*Inventory, 0, TEXT("Item_Arrows_01"), 5);
		const auto TargetId = Inventory->PartyInventoryState.ActiveCharacters[1].CharacterId;
		PutItem(*Inventory, 0, Item);
		auto* Probe = NewObject<UGridInventoryNotificationProbe>(Inventory);
		TArray<FGuid> NotifiedCharacters;
		Probe->Callback = [&](int32 Index)
		{
			NotifiedCharacters.Add(Inventory->PartyInventoryState.ActiveCharacters[Index].CharacterId);
			if (NotifiedCharacters.Num() != 1)
				return;
			TestEqual(TEXT("Destination notified first"), NotifiedCharacters[0], TargetId);
			if (Split)
				TestEqual(TEXT("Source debit precedes destination notification"),
					Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots[0].Item.Quantity, 4);
			if (!Split)
				TestTrue(TEXT("Whole source is empty"), Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots[0].IsEmpty());
			PermuteAndReallocate(*Inventory);
			AddExpectedError(TEXT("GridItemTransfer Failed"), EAutomationExpectedErrorFlags::Contains, 1);
			const auto Nested = UGridItemTransferService::TransferInventorySlotToCharacter(Inventory, 0, 0, 1, 0);
			TestTrue(TEXT("Nested transfer conflicts before mutation"), Nested.Result == EGridItemTransferResult::TransactionConflict);
		};
		Inventory->OnPartyInventoryChanged.AddDynamic(Probe, &UGridInventoryNotificationProbe::Observe);
		const auto Result = UGridItemTransferService::TransferInventorySlotToCharacter(Inventory, 0, 0, 1, Split ? 1 : 0);
		Inventory->OnPartyInventoryChanged.RemoveDynamic(Probe, &UGridInventoryNotificationProbe::Observe);
		Probe->Callback = nullptr;
		TestTrue(TEXT("Ordinary transfer commits"), Result.bSuccess);
		TestEqual(TEXT("Exactly two inventory events"), NotifiedCharacters.Num(), 2);
		if (NotifiedCharacters.Num() == 2)
			TestEqual(TEXT("Source event follows stable source identity"), NotifiedCharacters[1], Item.OwnerGuid);
		const int32 Source = Inventory->ResolveActiveCharacterIndexById(Item.OwnerGuid);
		const int32 Target = Inventory->ResolveActiveCharacterIndexById(TargetId);
		const auto& Received = Inventory->PartyInventoryState.ActiveCharacters[Target].InventorySlots[0].Item;
		TestEqual(TEXT("Target quantity"), Received.Quantity, Split ? 1 : 5);
		TestEqual(TEXT("Target owner"), Received.OwnerGuid, TargetId);
		TestEqual(TEXT("Target current index"), Received.OwnerCharacterIndex, Target);
		TestEqual(TEXT("Original GUID owned once"), CountGuid(*Inventory, Item.RuntimeObjectId), 1);
		if (Split)
		{
			TestTrue(TEXT("Split GUID distinct"), Received.RuntimeObjectId != Item.RuntimeObjectId);
			TestEqual(TEXT("Split GUID owned once"), CountGuid(*Inventory, Received.RuntimeObjectId), 1);
			TestEqual(TEXT("Total quantity conserved"),
				Inventory->PartyInventoryState.ActiveCharacters[Source].InventorySlots[0].Item.Quantity + Received.Quantity, 5);
		}
		FString Error;
		TestTrue(TEXT("Final party transfer ownership valid"), Inventory->ValidateInventoryOwnership(Error));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridSecureReceptacleRecovery, "Grimrock.Invariants.Reentrance.ReceptacleRecovery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridSecureReceptacleRecovery::RunTest(const FString&)
{
	FGridEditorCanonicalStarterTestWorld World;
	if (!TestTrue(TEXT("Transient ItemSystem ready"), World.Ready()))
		return false;
	for (const bool FailInsertion : { false, true })
	{
		auto* Inventory = MakeParty(World);
		const FGuid TargetId = Inventory->PartyInventoryState.ActiveCharacters[0].CharacterId;
		auto* Receptacle = MakeReceptacle(World);
		const FGuid ItemId = FGuid::NewGuid();
		auto Seed = MakeItem(*Inventory, 0, TEXT("Item_Arrows_01"), 5);
		Seed.RuntimeObjectId = ItemId;
		FGridItemInstance Accepted;
		if (!TestTrue(TEXT("Seed contained source through public insertion"), Receptacle->TryInsertItemInstanceFromCursor(Seed, Accepted)))
			return false;
		const int32 SourceIndex = 0;
		if (!TestTrue(TEXT("Contained source materialized"), Receptacle->ContainedItems.IsValidIndex(SourceIndex)))
			return false;
		auto* Visual = Receptacle->ContainedItems[SourceIndex].ItemActor.Get();
		if (!TestNotNull(TEXT("Real contained visual"), Visual))
			return false;
		auto* Probe = NewObject<UGridInventoryNotificationProbe>(Inventory);
		int32 Removals = 0;
		int32 Notifications = 0;
		Probe->Callback = [&](int32 Index)
		{
			if (Index == INDEX_NONE)
			{
				++Removals;
				PermuteAndReallocate(*Inventory);
				if (FailInsertion)
					Inventory->PartyInventoryState.ActiveCharacters[1].InventorySlots.Reset();
			}
			else
			{
				++Notifications;
				TestEqual(
					TEXT("Insertion notification uses reacquired destination"), Inventory->PartyInventoryState.ActiveCharacters[Index].CharacterId, TargetId);
			}
		};
		Visual->OnDestroyed.AddDynamic(Probe, &UGridInventoryNotificationProbe::ObserveDestroyed);
		Inventory->OnPartyInventoryChanged.AddDynamic(Probe, &UGridInventoryNotificationProbe::Observe);
		if (FailInsertion)
			AddExpectedError(TEXT("GridItemTransfer Failed"), EAutomationExpectedErrorFlags::Contains, 1);
		const auto Result = UGridItemTransferService::TransferReceptacleItemToInventory(Receptacle, 0, Inventory, 0);
		Inventory->OnPartyInventoryChanged.RemoveDynamic(Probe, &UGridInventoryNotificationProbe::Observe);
		Probe->Callback = nullptr;
		TestEqual(TEXT("One source removal callback"), Removals, 1);
		TestEqual(TEXT("Insertion result"), Result.bSuccess, !FailInsertion);
		TestEqual(TEXT("No duplicate or lost GUID"), CountGuid(*Inventory, ItemId, Receptacle), 1);
		TestEqual(TEXT("Only a committed insertion notifies inventory"), Notifications, FailInsertion ? 0 : 1);
		if (FailInsertion)
		{
			TestTrue(TEXT("Insertion refused with successful source rollback"), Result.Result == EGridItemTransferResult::DestinationInsertFailed);
			TestEqual(TEXT("Receptacle source quantity restored"), Receptacle->ContainedItems[0].Quantity, 5);
		}
		else
		{
			const int32 Current = Inventory->ResolveActiveCharacterIndexById(TargetId);
			const auto& Item = Inventory->PartyInventoryState.ActiveCharacters[Current].InventorySlots[0].Item;
			TestEqual(TEXT("Destination quantity preserved"), Item.Quantity, 5);
			TestEqual(TEXT("Destination owner preserved"), Item.OwnerGuid, TargetId);
			TestEqual(TEXT("Destination index follows roster"), Item.OwnerCharacterIndex, Current);
			TestTrue(TEXT("Receptacle source removed"), Receptacle->ContainedItems.IsEmpty());
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridSecureIndependentTransfers, "Grimrock.Invariants.Reentrance.IndependentTransfers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridSecureIndependentTransfers::RunTest(const FString&)
{
	FGridEditorCanonicalStarterTestWorld World;
	if (!TestTrue(TEXT("Transient ItemSystem ready"), World.Ready()))
		return false;
	auto* Inventory = MakeParty(World);
	auto* ForeignInventory = MakeParty(World);
	const auto First = MakeItem(*Inventory, 0, TEXT("Item_Potion_Healing_01"));
	const auto Second = MakeItem(*Inventory, 1, TEXT("Item_Potion_Healing_01"));
	PutItem(*Inventory, 0, First);
	PutItem(*Inventory, 1, Second);
	PutItem(*ForeignInventory, 0, MakeItem(*ForeignInventory, 0, TEXT("Item_Potion_Healing_01")));
	auto* Destination = MakeReceptacle(World);
	auto* Independent = MakeReceptacle(World);
	auto* Probe = NewObject<UGridInventoryNotificationProbe>(Inventory);
	int32 Notifications = 0;
	Probe->Callback = [&](int32)
	{
		if (++Notifications != 1)
			return;
		AddExpectedError(TEXT("GridItemTransfer Failed"), EAutomationExpectedErrorFlags::Contains, 1);
		const auto Conflict = UGridItemTransferService::TransferInventorySlotToReceptacle(ForeignInventory, 0, 0, Destination);
		TestTrue(TEXT("Shared destination reserved across inventory instances"), Conflict.Result == EGridItemTransferResult::TransactionConflict);
		const auto Nested = UGridItemTransferService::TransferInventorySlotToReceptacle(Inventory, 1, 0, Independent);
		TestTrue(TEXT("Disjoint character and destination stay available"), Nested.bSuccess);
	};
	Inventory->OnPartyInventoryChanged.AddDynamic(Probe, &UGridInventoryNotificationProbe::Observe);
	TestTrue(TEXT("Outer transfer succeeds"), UGridItemTransferService::TransferInventorySlotToReceptacle(Inventory, 0, 0, Destination).bSuccess);
	Inventory->OnPartyInventoryChanged.RemoveDynamic(Probe, &UGridInventoryNotificationProbe::Observe);
	Probe->Callback = nullptr;
	TestEqual(TEXT("Each committed source notifies once"), Notifications, 2);
	TestEqual(TEXT("First GUID preserved"), CountGuid(*Inventory, First.RuntimeObjectId, Destination), 1);
	TestEqual(TEXT("Second GUID preserved"), CountGuid(*Inventory, Second.RuntimeObjectId, Independent), 1);
	TestFalse(TEXT("All reservations released"), Inventory->HasActiveCharacterTransactions());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridSecureWallLockTransfer, "Grimrock.Invariants.Reentrance.WallLockTransfer", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridSecureWallLockTransfer::RunTest(const FString&)
{
	FGridEditorCanonicalStarterTestWorld World;
	if (!TestTrue(TEXT("Transient ItemSystem ready"), World.Ready()))
		return false;
	auto* Runtime = World.World->SpawnActor<AGridLevelRuntimeActor>();
	auto* Level = NewObject<UGridLevelAsset>(Runtime);
	Level->Width = Level->Height = 2;
	Level->EnsureCellCount();
	for (auto& Cell : Level->Cells)
	{
		Cell.CellType = EGridCellType::Floor;
		Cell.bBlocksOccupancy = false;
	}
	Runtime->LevelAsset = Level;
	auto* Pawn = World.World->SpawnActor<AGrimrockPartyPawn>();
	Pawn->LevelRuntimeActor = Runtime;
	for (int32 Mode = 0; Mode < 4; ++Mode)
	{
		const bool FailInsertion = (Mode % 2) != 0;
		const bool SplitStack = Mode >= 2;
		auto* Inventory = MakeParty(World);
		// Locks accept authored ItemIds, so an actual stackable item exercises
		// the split path without changing a DataAsset or fabricating an instance.
		const auto Key = MakeItem(*Inventory, 0, SplitStack ? FName(TEXT("Item_Arrows_01")) : FName(TEXT("Key_Copper")), SplitStack ? 3 : 1);
		PutItem(*Inventory, 0, Key);
		FActorSpawnParameters Params;
		Params.Owner = Runtime;
		auto* Lock = World.World->SpawnActor<AGridWallLockActor>(AGridWallLockActor::StaticClass(), FTransform::Identity, Params);
		FGridRuntimeWorldObjectData Data;
		Data.ObjectId = FGuid::NewGuid();
		Data.Type = EGridLevelObjectType::Receptacle;
		Data.CellX = Data.CellY = 1;
		Data.Edge = EGridEdge::North;
		Data.Behavior.Lock.AcceptedKeyIds.Add(Key.ItemDefinitionId);
		Lock->InitializeRuntimeWorldObject(Data, nullptr, FTransform::Identity);
		auto* Probe = NewObject<UGridInventoryNotificationProbe>(Inventory);
		TArray<int32> Notifications;
		Probe->Callback = [&](int32 Index)
		{
			Notifications.Add(Index);
			if (Notifications.Num() != 1)
				return;
			TestFalse(TEXT("Notification still precedes unlock and dungeon links"), Lock->bIsUnlocked);
			TestTrue(TEXT("Notification precedes key insertion"), Lock->ContainedItems.IsEmpty());
			PermuteAndReallocate(*Inventory);
			if (FailInsertion)
				Lock->bIsUnlocked = true; // Destination changes after preflight.
		};
		Inventory->OnPartyInventoryChanged.AddDynamic(Probe, &UGridInventoryNotificationProbe::Observe);
		if (FailInsertion)
			AddExpectedError(TEXT("GridItemTransfer Failed"), EAutomationExpectedErrorFlags::Contains, 1);
		const auto Result = UGridItemTransferService::TransferInventorySlotToWallLock(Inventory, 0, 0, Lock, Pawn);
		Inventory->OnPartyInventoryChanged.RemoveDynamic(Probe, &UGridInventoryNotificationProbe::Observe);
		Probe->Callback = nullptr;
		TestEqual(TEXT("Lock transfer result"), Result.bSuccess, !FailInsertion);
		TestEqual(TEXT("Key GUID remains owned once"), CountGuid(*Inventory, Key.RuntimeObjectId, Lock), 1);
		TestEqual(TEXT("One debit notification plus optional rollback"), Notifications.Num(), FailInsertion ? 2 : 1);
		const int32 Source = Inventory->ResolveActiveCharacterIndexById(Key.OwnerGuid);
		if (FailInsertion)
		{
			const auto& Restored = Inventory->PartyInventoryState.ActiveCharacters[Source].InventorySlots[0].Item;
			TestEqual(TEXT("Failed lock restores original owner"), Restored.OwnerGuid, Key.OwnerGuid);
			TestEqual(TEXT("Failed lock restores original quantity"), Restored.Quantity, SplitStack ? 3 : 1);
			TestEqual(TEXT("Rollback notifies current source index"), Notifications.Last(), Source);
			TestTrue(TEXT("Failed lock has no inserted key"), Lock->ContainedItems.IsEmpty());
		}
		else
		{
			TestTrue(TEXT("Lock unlocked"), Lock->bIsUnlocked);
			TestEqual(TEXT("Exactly one physical key inserted"), Lock->ContainedItems.Num(), 1);
			if (SplitStack)
			{
				TestEqual(TEXT("Source remainder conserved"), Inventory->PartyInventoryState.ActiveCharacters[Source].InventorySlots[0].Item.Quantity, 2);
				TestTrue(TEXT("Split key identity is distinct"), Lock->ContainedItems[0].RuntimeObjectId != Key.RuntimeObjectId);
				TestEqual(TEXT("Split key identity owned once"), CountGuid(*Inventory, Lock->ContainedItems[0].RuntimeObjectId, Lock), 1);
			}
			else
				TestTrue(TEXT("Source key removed"), Inventory->PartyInventoryState.ActiveCharacters[Source].InventorySlots[0].IsEmpty());
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridSecurePersistenceEntryPoints, "Grimrock.Invariants.Reentrance.PersistenceEntryPoints",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridSecurePersistenceEntryPoints::RunTest(const FString&)
{
	if (!GridAutomationSaveIsolation::Require(*this))
		return false;
	FGridEditorCanonicalStarterTestWorld World;
	if (!TestTrue(TEXT("Transient ItemSystem ready"), World.Ready()))
		return false;
	FMON155RuntimeStateGuard RuntimeGuard;
	URPGClassAsset* Class = nullptr;
	auto* Fixture = MakeMON155Inventory(3, 3000, Class);
	auto* Pawn = World.World->SpawnActor<AGrimrockPartyPawn>();
	auto* Inventory = Pawn->PartyInventoryComponent.Get();
	Inventory->PartyInventoryState = Fixture->PartyInventoryState;
	Inventory->PartyInventoryState.bInitialCharacterCreationCompleted = true;
	Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots.SetNum(3);
	const auto Item = MakeItem(*Inventory, 0, TEXT("Item_Potion_Healing_01"));
	PutItem(*Inventory, 0, Item);
	Pawn->PartySaveSlotName = TEXT("TransactionPersistence_") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
	Pawn->PartySaveUserIndex = 0;
	auto* Save = NewObject<UGrimrockPartySaveGame>();
	Save->PartyInventoryState = Inventory->PartyInventoryState;
	if (!TestTrue(TEXT("Create only the isolated owned fixture slot"), UGameplayStatics::SaveGameToSlot(Save, Pawn->PartySaveSlotName, 0)))
		return false;
	TArray<uint8> Before;
	TestTrue(TEXT("Read fixture bytes"), UGameplayStatics::LoadDataFromSlot(Before, Pawn->PartySaveSlotName, 0));
	auto* Receptacle = MakeReceptacle(World);
	auto* Probe = NewObject<UGridInventoryNotificationProbe>(Inventory);
	int32 Notifications = 0;
	Probe->Callback = [&](int32)
	{
		++Notifications;
		FText Error;
		TestFalse(TEXT("Save refuses an in-flight snapshot"), Pawn->SaveCurrentGame(Error));
		TestFalse(TEXT("Continue refuses before loading/replacing state"), Pawn->LoadCurrentGame(Error));
		TestFalse(TEXT("NewGame refuses before deleting slot or resetting world"), Pawn->StartNewGame(Error));
		TestEqual(
			TEXT("Source identity survives refused lifecycle operations"), Inventory->PartyInventoryState.ActiveCharacters[0].CharacterId, Item.OwnerGuid);
	};
	Inventory->OnPartyInventoryChanged.AddDynamic(Probe, &UGridInventoryNotificationProbe::Observe);
	AddExpectedError(TEXT("PartySave TransactionRejected"), EAutomationExpectedErrorFlags::Contains, 3);
	TestTrue(TEXT("Outer physical transfer still commits"), UGridItemTransferService::TransferInventorySlotToReceptacle(Inventory, 0, 0, Receptacle).bSuccess);
	Inventory->OnPartyInventoryChanged.RemoveDynamic(Probe, &UGridInventoryNotificationProbe::Observe);
	Probe->Callback = nullptr;
	TArray<uint8> After;
	TestTrue(TEXT("Fixture still exists after refused NewGame"), UGameplayStatics::LoadDataFromSlot(After, Pawn->PartySaveSlotName, 0));
	TestTrue(TEXT("Persistence refusal preserves exact fixture bytes"), Before == After);
	TestEqual(TEXT("No lifecycle operation emits an extra inventory event"), Notifications, 1);
	TestEqual(TEXT("Outer item retains exactly one physical owner"), CountGuid(*Inventory, Item.RuntimeObjectId, Receptacle), 1);
	GridAutomationSaveIsolation::DeleteOwnedSlot(*this, Pawn->PartySaveSlotName, 0);
	return true;
}

#endif
