#include "Runtime/GridItemTransferService.h"

#include "Runtime/GridPartyTransactionScope.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GridReceptacleActor.h"
#include "Runtime/GridWallLockActor.h"
#include "Runtime/GrimrockPartyPawn.h"

DEFINE_LOG_CATEGORY_STATIC(LogGridItemTransfer, Log, All);

namespace
{
	FGridItemTransferResult MakeTransferResult(EGridItemTransferResult Result, const FString& Message)
	{
		FGridItemTransferResult TransferResult;
		TransferResult.bSuccess = Result == EGridItemTransferResult::Success;
		TransferResult.Result = Result;
		TransferResult.Message = FText::FromString(Message);
		return TransferResult;
	}

	FGridItemTransferResult LogTransferFailure(
		const TCHAR* Operation, EGridItemTransferResult Result, const FString& Message, const FGridItemInstance* Item = nullptr)
	{
		UE_LOG(LogGridItemTransfer, Warning, TEXT("GridItemTransfer Failed Operation=%s Result=%s Item=%s RuntimeId=%s Message=%s"), Operation,
			*UEnum::GetValueAsString(Result), Item ? *Item->ItemDefinitionId.ToString() : TEXT("None"), Item ? *Item->RuntimeObjectId.ToString() : TEXT("None"),
			*Message);
		return MakeTransferResult(Result, Message);
	}

	FGridItemTransferResult LogTransferSuccess(const TCHAR* Operation, const FGridItemInstance& Item, const FString& Message)
	{
		UE_LOG(LogGridItemTransfer, Verbose, TEXT("GridItemTransfer Success Operation=%s Item=%s RuntimeId=%s Message=%s"), Operation, *Item.ItemDefinitionId.ToString(),
			*Item.RuntimeObjectId.ToString(), *Message);
		return MakeTransferResult(EGridItemTransferResult::Success, Message);
	}

	bool RestoreInventorySource(UGridPartyInventoryComponent& Inventory, const FGridPartyTransactionScope& Transaction, int32 SlotIndex,
		const FGridInventorySlot& Snapshot, const FGridInventorySlot& ExpectedAfterDebit, bool bNotify = true)
	{
		const int32 Index = Transaction.ResolveIndex();
		if (!Inventory.IsValidCharacterIndex(Index))
			return false;
		auto& Slots = Inventory.PartyInventoryState.ActiveCharacters[Index].InventorySlots;
		if (!Slots.IsValidIndex(SlotIndex))
			return false;
		FGridInventorySlot Current = Slots[SlotIndex];
		FGridInventorySlot Expected = ExpectedAfterDebit;
		// OwnerCharacterIndex is derived from the roster. A coherent permutation
		// may update it while preserving the same physical owner and remainder.
		if (Current.Item.OwnerGuid == Transaction.GetCharacterId() && Expected.Item.OwnerGuid == Transaction.GetCharacterId())
		{
			Current.Item.OwnerCharacterIndex = Index;
			Expected.Item.OwnerCharacterIndex = Index;
		}
		if (!FGridInventorySlot::StaticStruct()->CompareScriptStruct(&Current, &Expected, 0))
			return false;
		FGridInventorySlot Restored = Snapshot;
		Restored.Item.OwnerGuid = Transaction.GetCharacterId();
		Restored.Item.OwnerCharacterIndex = Index;
		Slots[SlotIndex] = MoveTemp(Restored);
		if (bNotify)
			Inventory.NotifyPartyInventoryChanged(Index);
		return true;
	}

	bool ValidateReceptacleAcceptance(
		AGridReceptacleActor* Receptacle, const FGridItemInstance& Item, FGridItemTransferResult& OutFailure, const TCHAR* Operation)
	{
		FGridReceptacleAcceptanceResult AcceptanceResult;
		if (Receptacle->EvaluateItemAcceptance(Item, AcceptanceResult))
		{
			return true;
		}

		const FString Message = FString::Printf(TEXT("Destination rejected item with reason %d."), static_cast<int32>(AcceptanceResult.RejectReason));
		OutFailure = LogTransferFailure(Operation, EGridItemTransferResult::DestinationRejectsItem, Message, &Item);
		return false;
	}
}

FGridItemTransferResult UGridItemTransferService::TransferInventorySlotToReceptacle(
	UGridPartyInventoryComponent* Inventory, int32 CharacterIndex, int32 InventorySlotIndex, AGridReceptacleActor* Receptacle)
{
	static const TCHAR* Operation = TEXT("InventorySlotToReceptacle");
	if (!Inventory || !Inventory->IsValidCharacterIndex(CharacterIndex))
	{
		return LogTransferFailure(Operation, EGridItemTransferResult::InvalidSource, TEXT("Inventory or character source is invalid."));
	}
	if (!IsValid(Receptacle))
	{
		return LogTransferFailure(Operation, EGridItemTransferResult::InvalidDestination, TEXT("Receptacle destination is invalid."));
	}

	FGridPartyTransactionScope Transaction(Inventory, { CharacterIndex }, Receptacle);
	if (!Transaction.IsAcquired())
		return LogTransferFailure(Operation, EGridItemTransferResult::TransactionConflict,
			TEXT("TransactionConflict: character or destination is already reserved, or its identity is ambiguous."));

	FGridCharacterInventoryState& CharacterState = Inventory->PartyInventoryState.ActiveCharacters[CharacterIndex];
	if (!CharacterState.InventorySlots.IsValidIndex(InventorySlotIndex) || CharacterState.InventorySlots[InventorySlotIndex].IsEmpty())
	{
		return LogTransferFailure(Operation, EGridItemTransferResult::InvalidSource, TEXT("Inventory slot source is invalid or empty."));
	}

	const FGridInventorySlot SourceSnapshot = CharacterState.InventorySlots[InventorySlotIndex];
	const FGridItemInstance Candidate = SourceSnapshot.Item;
	if (!Candidate.IsValid())
	{
		return LogTransferFailure(Operation, EGridItemTransferResult::InvalidItem, TEXT("Inventory slot contains an invalid item."), &Candidate);
	}

	if (!Transaction.ReserveItem(Candidate.RuntimeObjectId))
		return LogTransferFailure(Operation, EGridItemTransferResult::TransactionConflict, TEXT("TransactionConflict: item already reserved."));

	FGridItemTransferResult AcceptanceFailure;
	if (!ValidateReceptacleAcceptance(Receptacle, Candidate, AcceptanceFailure, Operation))
	{
		return AcceptanceFailure;
	}

	const FGridInventorySlot ExpectedAfterDebit;
	CharacterState.InventorySlots[InventorySlotIndex] = ExpectedAfterDebit;
	Inventory->NotifyPartyInventoryChanged(CharacterIndex);

	FGridItemInstance AcceptedItem;
	if (!IsValid(Receptacle) || !Receptacle->TryInsertItemInstanceFromCursor(Candidate, AcceptedItem))
	{
		if (!RestoreInventorySource(*Inventory, Transaction, InventorySlotIndex, SourceSnapshot, ExpectedAfterDebit))
			return LogTransferFailure(Operation, EGridItemTransferResult::RollbackFailed,
				TEXT("Destination insertion failed; stable source is missing or concurrently modified."), &Candidate);
		return LogTransferFailure(
			Operation, EGridItemTransferResult::DestinationInsertFailed, TEXT("Destination insertion failed; inventory source was restored."), &Candidate);
	}

	return LogTransferSuccess(Operation, AcceptedItem, TEXT("Inventory item transferred to receptacle."));
}

FGridItemTransferResult UGridItemTransferService::TransferEquipmentSlotToReceptacle(
	UGridPartyInventoryComponent* Inventory, int32 CharacterIndex, EGridEquipmentSlot EquipmentSlot, AGridReceptacleActor* Receptacle)
{
	static const TCHAR* Operation = TEXT("EquipmentSlotToReceptacle");
	if (!Inventory || !Inventory->IsValidCharacterIndex(CharacterIndex) || !Inventory->PartyInventoryState.ActiveEquipment.IsValidIndex(CharacterIndex))
	{
		return LogTransferFailure(Operation, EGridItemTransferResult::InvalidSource, TEXT("Inventory or equipment character source is invalid."));
	}
	if (!IsValid(Receptacle))
	{
		return LogTransferFailure(Operation, EGridItemTransferResult::InvalidDestination, TEXT("Receptacle destination is invalid."));
	}

	FGridPartyTransactionScope Transaction(Inventory, { CharacterIndex }, Receptacle);
	if (!Transaction.IsAcquired())
		return LogTransferFailure(Operation, EGridItemTransferResult::TransactionConflict,
			TEXT("TransactionConflict: character or destination is already reserved, or its identity is ambiguous."));

	FGridCharacterEquipmentState& EquipmentState = Inventory->PartyInventoryState.ActiveEquipment[CharacterIndex];
	const EGridEquipmentSlot PhysicalSlot = Inventory->ResolveEquipmentSourceSlot(CharacterIndex, EquipmentSlot);
	FGridItemInstance* SourceItem = EquipmentState.GetMutableSlot(PhysicalSlot);
	if (!SourceItem)
	{
		return LogTransferFailure(Operation, EGridItemTransferResult::InvalidSource, TEXT("Equipment slot source is invalid."));
	}

	const FGridItemInstance SourceSnapshot = *SourceItem;
	if (!SourceSnapshot.IsValid())
	{
		return LogTransferFailure(
			Operation, EGridItemTransferResult::InvalidItem, TEXT("Equipment slot is empty or contains an invalid item."), &SourceSnapshot);
	}

	if (!Transaction.ReserveItem(SourceSnapshot.RuntimeObjectId))
		return LogTransferFailure(Operation, EGridItemTransferResult::TransactionConflict, TEXT("TransactionConflict: item already reserved."));

	FGridItemTransferResult AcceptanceFailure;
	if (!ValidateReceptacleAcceptance(Receptacle, SourceSnapshot, AcceptanceFailure, Operation))
	{
		return AcceptanceFailure;
	}

	*SourceItem = FGridItemInstance();
	Inventory->NotifyPartyInventoryChanged(CharacterIndex);

	FGridItemInstance AcceptedItem;
	if (!IsValid(Receptacle) || !Receptacle->TryInsertItemInstanceFromCursor(SourceSnapshot, AcceptedItem))
	{
		const int32 CurrentIndex = Transaction.ResolveIndex();
		FGridItemInstance* CurrentSlot = Inventory->PartyInventoryState.ActiveEquipment.IsValidIndex(CurrentIndex)
			? Inventory->PartyInventoryState.ActiveEquipment[CurrentIndex].GetMutableSlot(PhysicalSlot)
			: nullptr;
		const FGridItemInstance Empty;
		if (!CurrentSlot || !FGridItemInstance::StaticStruct()->CompareScriptStruct(CurrentSlot, &Empty, 0))
			return LogTransferFailure(Operation, EGridItemTransferResult::RollbackFailed,
				TEXT("Destination insertion failed; stable equipment source is missing or concurrently modified."), &SourceSnapshot);
		*CurrentSlot = SourceSnapshot;
		CurrentSlot->OwnerGuid = Transaction.GetCharacterId();
		CurrentSlot->OwnerCharacterIndex = CurrentIndex;
		Inventory->NotifyPartyInventoryChanged(CurrentIndex);
		return LogTransferFailure(
			Operation, EGridItemTransferResult::DestinationInsertFailed, TEXT("Destination insertion failed; equipment source was restored."), &SourceSnapshot);
	}

	return LogTransferSuccess(Operation, AcceptedItem, TEXT("Equipped item transferred to receptacle."));
}

FGridItemTransferResult UGridItemTransferService::TransferInventorySlotToWallLock(
	UGridPartyInventoryComponent* Inventory, int32 CharacterIndex, int32 InventorySlotIndex, AGridWallLockActor* WallLock, AGrimrockPartyPawn* PartyPawn)
{
	static const TCHAR* Operation = TEXT("InventorySlotToWallLock");
	if (!Inventory || !Inventory->IsValidCharacterIndex(CharacterIndex))
	{
		return LogTransferFailure(Operation, EGridItemTransferResult::InvalidSource, TEXT("Inventory or character source is invalid."));
	}
	if (!IsValid(WallLock) || !PartyPawn)
	{
		return LogTransferFailure(Operation, EGridItemTransferResult::InvalidDestination, TEXT("Wall lock destination or party pawn is invalid."));
	}

	FGridPartyTransactionScope Transaction(Inventory, { CharacterIndex }, WallLock);
	if (!Transaction.IsAcquired())
		return LogTransferFailure(Operation, EGridItemTransferResult::TransactionConflict,
			TEXT("TransactionConflict: character or destination is already reserved, or its identity is ambiguous."));
	TStrongObjectPtr<AGrimrockPartyPawn> PawnLifetime(PartyPawn);

	FGridCharacterInventoryState& CharacterState = Inventory->PartyInventoryState.ActiveCharacters[CharacterIndex];
	if (!CharacterState.InventorySlots.IsValidIndex(InventorySlotIndex) || CharacterState.InventorySlots[InventorySlotIndex].IsEmpty())
	{
		return LogTransferFailure(Operation, EGridItemTransferResult::InvalidSource, TEXT("Inventory slot source is invalid or empty."));
	}

	const FGridInventorySlot SourceSnapshot = CharacterState.InventorySlots[InventorySlotIndex];
	FGridItemInstance KeyItem = SourceSnapshot.Item;
	if (!WallLock->CanAcceptKeyInstance(KeyItem))
	{
		return LogTransferFailure(Operation, EGridItemTransferResult::DestinationRejectsItem, TEXT("Wall lock rejected the selected key."), &KeyItem);
	}

	if (!Transaction.ReserveItem(SourceSnapshot.Item.RuntimeObjectId))
		return LogTransferFailure(Operation, EGridItemTransferResult::TransactionConflict, TEXT("TransactionConflict: item already reserved."));

	KeyItem.Quantity = 1;
	if (SourceSnapshot.Item.Quantity > 1)
	{
		KeyItem.RuntimeObjectId = FGuid::NewGuid();
		if (!Transaction.ReserveItem(KeyItem.RuntimeObjectId))
			return LogTransferFailure(Operation, EGridItemTransferResult::TransactionConflict, TEXT("TransactionConflict: split item already reserved."));
		CharacterState.InventorySlots[InventorySlotIndex].Item.Quantity -= 1;
	}
	else
	{
		CharacterState.InventorySlots[InventorySlotIndex] = FGridInventorySlot();
	}
	const FGridInventorySlot ExpectedAfterDebit = CharacterState.InventorySlots[InventorySlotIndex];
	Inventory->NotifyPartyInventoryChanged(CharacterIndex);

	if (!IsValid(WallLock) || !IsValid(PartyPawn) || !WallLock->TryUnlockWithContextItem(PartyPawn, KeyItem))
	{
		if (!RestoreInventorySource(*Inventory, Transaction, InventorySlotIndex, SourceSnapshot, ExpectedAfterDebit))
			return LogTransferFailure(Operation, EGridItemTransferResult::RollbackFailed,
				TEXT("Wall lock insertion failed; stable source is missing or concurrently modified."), &KeyItem);
		return LogTransferFailure(
			Operation, EGridItemTransferResult::DestinationInsertFailed, TEXT("Wall lock insertion failed; inventory source was restored."), &KeyItem);
	}

	return LogTransferSuccess(Operation, KeyItem, TEXT("Inventory key transferred to wall lock."));
}

FGridItemTransferResult UGridItemTransferService::TransferReceptacleItemToInventory(
	AGridReceptacleActor* Receptacle, int32 ContainedItemIndex, UGridPartyInventoryComponent* Inventory, int32 CharacterIndex)
{
	return TransferReceptacleItemToInventoryInternal(Receptacle, ContainedItemIndex, Inventory, CharacterIndex, false);
}

FGridItemTransferResult UGridItemTransferService::TransferReceptacleItemToInventoryInternal(
	AGridReceptacleActor* Receptacle, int32 ContainedItemIndex, UGridPartyInventoryComponent* Inventory, int32 CharacterIndex,
	bool bPublishRemovalEvent)
{
	static const TCHAR* Operation = TEXT("ReceptacleItemToInventory");
	if (!IsValid(Receptacle) || !Receptacle->IsValidContainedItemIndex(ContainedItemIndex))
	{
		return LogTransferFailure(Operation, EGridItemTransferResult::InvalidSource, TEXT("Receptacle source or contained item index is invalid."));
	}
	if (!Receptacle->IsItemRemovalAllowed())
	{
		return LogTransferFailure(Operation, EGridItemTransferResult::InvalidSource,
			FString::Printf(TEXT("Receptacle item removal is disabled. CanRemove=%s."), Receptacle->bCanRemoveItem ? TEXT("true") : TEXT("false")));
	}
	if (!Inventory || !Inventory->IsValidCharacterIndex(CharacterIndex))
	{
		return LogTransferFailure(Operation, EGridItemTransferResult::InvalidDestination, TEXT("Inventory destination or character is invalid."));
	}

	FGridPartyTransactionScope Transaction(Inventory, { CharacterIndex }, Receptacle);
	if (!Transaction.IsAcquired())
		return LogTransferFailure(Operation, EGridItemTransferResult::TransactionConflict,
			TEXT("TransactionConflict: destination or receptacle is reserved, or character identity is ambiguous."));

	const FGridContainedReceptacleItem SourceSnapshot = Receptacle->ContainedItems[ContainedItemIndex];
	FGridItemInstance Candidate;
	Candidate.RuntimeObjectId = SourceSnapshot.RuntimeObjectId;
	Candidate.ItemDefinitionId = SourceSnapshot.ItemDefinitionId;
	Candidate.ItemLevel = SourceSnapshot.ItemLevel;
	Candidate.Quantity = SourceSnapshot.Quantity;
	Candidate.bLightsEnabled = SourceSnapshot.bLightsEnabled;
	Candidate.ReadableContentAsset = SourceSnapshot.ReadableContentAsset;
	Candidate.ReadableContentId = SourceSnapshot.ReadableContentId;
	Candidate.ReadTitleOverride = SourceSnapshot.ReadTitleOverride;
	Candidate.ReadTextOverride = SourceSnapshot.ReadTextOverride;
	Candidate.OwnerType = EGridItemOwnerType::Receptacle;
	Candidate.OwnerGuid = Receptacle->ObjectId;
	const bool bHadVisualActor = IsValid(SourceSnapshot.ItemActor.Get());
	Candidate.LastWorldTransform = bHadVisualActor ? SourceSnapshot.ItemActor->GetActorTransform() : FTransform::Identity;
	if (bHadVisualActor)
	{
		const AGridItemActor* Visual = SourceSnapshot.ItemActor.Get();
		if (Visual->ItemDefinitionId != Candidate.ItemDefinitionId ||
			Visual->GetRuntimeObjectId() != Candidate.RuntimeObjectId ||
			Visual->WorldItemLevel != Candidate.ItemLevel ||
			!Visual->ResolvedWorldPresentation.IsValid() || Visual->GetWorld() != Receptacle->GetWorld())
		{
			return LogTransferFailure(Operation, EGridItemTransferResult::InvalidItem, TEXT("Contained actor conflicts with physical source."), &Candidate);
		}
		Candidate.bLightsEnabled = Visual->AreItemLightsEnabled();
		Candidate.ReadableContentAsset = Visual->ReadableContentAsset;
		Candidate.ReadableContentId = Visual->ReadableContentId;
		Candidate.ReadTitleOverride = Visual->ReadTitleOverride;
		Candidate.ReadTextOverride = Visual->ReadTextOverride;
	}
	const UWorld* World = Receptacle->GetWorld();
	UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
	const UGridItemSubsystem* Registry = GI ? GI->GetSubsystem<UGridItemSubsystem>() : nullptr;
	if (!Registry || !Registry->ContainsItem(Candidate.ItemDefinitionId))
	{
		return LogTransferFailure(Operation, EGridItemTransferResult::InvalidItem, TEXT("Contained identity is not registered."), &Candidate);
	}
	FText CanonicalDisplayName;
	{
		FGridResolvedItem Resolved;
		if (!Registry->ResolveItem(Candidate.ItemDefinitionId, Candidate.ItemLevel, Resolved) ||
			!Resolved.IsValid() || Resolved.ItemLevel != Candidate.ItemLevel ||
			(!Resolved.bIsArmament && Candidate.ItemLevel != 0) ||
			Candidate.Quantity > FMath::Max(1, Resolved.MaxStack))
		{
			return LogTransferFailure(Operation, EGridItemTransferResult::InvalidItem, TEXT("Contained canonical instance is invalid."), &Candidate);
		}
		// Keep the notification label local; the physical instance has no cache.
		CanonicalDisplayName = Resolved.DisplayName.IsEmpty() ? FText::FromName(Candidate.ItemDefinitionId) : Resolved.DisplayName;
	}

	if (!Candidate.IsValid())
	{
		return LogTransferFailure(Operation, EGridItemTransferResult::InvalidItem, TEXT("Contained receptacle item is invalid."), &Candidate);
	}
	if (!Inventory->CanAddItemToCharacterInventory(CharacterIndex, Candidate))
	{
		return LogTransferFailure(Operation, EGridItemTransferResult::InventoryFull, TEXT("Destination inventory is full."), &Candidate);
	}

	if (!Transaction.ReserveItem(Candidate.RuntimeObjectId))
		return LogTransferFailure(Operation, EGridItemTransferResult::TransactionConflict, TEXT("TransactionConflict: item already reserved."));

	const bool bInitialRemovalAlreadyRecorded = Receptacle->RemovedInitialItemDefinitionIds.Contains(SourceSnapshot.ItemDefinitionId);
	FGridContainedReceptacleItem RemovedItem;
	if (!Receptacle->RemoveContainedItemAtIndex(ContainedItemIndex, RemovedItem))
	{
		return LogTransferFailure(Operation, EGridItemTransferResult::SourceRemoveFailed, TEXT("Failed to remove item from receptacle."), &Candidate);
	}

	const int32 CurrentTargetIndex = Transaction.ResolveIndex();
	if (CurrentTargetIndex == INDEX_NONE || !Inventory->AddItemToCharacterInventoryInternal(CurrentTargetIndex, Candidate))
	{
		const int32 RestoredIndex = Receptacle->AddContainedItem(SourceSnapshot.ItemDefinitionId, nullptr,
			SourceSnapshot.bWasInitialItem, SourceSnapshot.Quantity, SourceSnapshot.RuntimeObjectId,
			SourceSnapshot.ItemLevel);
		if (!Receptacle->ContainedItems.IsValidIndex(RestoredIndex))
		{
			return LogTransferFailure(
				Operation, EGridItemTransferResult::RollbackFailed, TEXT("Inventory insertion failed and receptacle rollback failed."), &Candidate);
		}

		FGridContainedReceptacleItem& RestoredItem = Receptacle->ContainedItems[RestoredIndex];
		RestoredItem.bLightsEnabled = Candidate.bLightsEnabled;
		RestoredItem.ReadableContentAsset = Candidate.ReadableContentAsset;
		RestoredItem.ReadableContentId = Candidate.ReadableContentId;
		RestoredItem.ReadTitleOverride = Candidate.ReadTitleOverride;
		RestoredItem.ReadTextOverride = Candidate.ReadTextOverride;
		if (SourceSnapshot.bWasInitialItem && !bInitialRemovalAlreadyRecorded)
		{
			Receptacle->RemovedInitialItemDefinitionIds.Remove(SourceSnapshot.ItemDefinitionId);
		}
		TStrongObjectPtr<AGridItemActor> RestoredActor(RestoredItem.ItemActor.Get());
		if (bHadVisualActor && IsValid(RestoredActor.Get()))
		{
			RestoredActor->SetActorTransform(Candidate.LastWorldTransform, false, nullptr, ETeleportType::TeleportPhysics);
			if (IsValid(RestoredActor.Get()))
				RestoredActor->SetItemLightsEnabled(Candidate.bLightsEnabled);
			if (IsValid(RestoredActor.Get()))
				RestoredActor->InitializeReadableContent(
					Candidate.ReadableContentAsset, Candidate.ReadableContentId, Candidate.ReadTitleOverride, Candidate.ReadTextOverride);
		}
		const int32 CurrentRestoredIndex = Receptacle->ContainedItems.IndexOfByPredicate(
			[&SourceSnapshot](const FGridContainedReceptacleItem& Item)
			{
				return Item.RuntimeObjectId == SourceSnapshot.RuntimeObjectId;
			});
		if (CurrentRestoredIndex == INDEX_NONE)
			return LogTransferFailure(
				Operation, EGridItemTransferResult::RollbackFailed, TEXT("Restored receptacle source was concurrently removed."), &Candidate);
		if (CurrentRestoredIndex != ContainedItemIndex && Receptacle->ContainedItems.IsValidIndex(ContainedItemIndex))
		{
			FGridContainedReceptacleItem Restored = MoveTemp(Receptacle->ContainedItems[CurrentRestoredIndex]);
			Receptacle->ContainedItems.RemoveAt(CurrentRestoredIndex);
			Receptacle->ContainedItems.Insert(MoveTemp(Restored), ContainedItemIndex);
			Receptacle->UpdateContainedItemInteractionCollision();
		}

		return LogTransferFailure(
			Operation, EGridItemTransferResult::DestinationInsertFailed, TEXT("Inventory insertion failed; receptacle source was restored."), &Candidate);
	}

	if (bPublishRemovalEvent)
	{
		Receptacle->ExecuteRemovalLinksWithItemEvent(CanonicalDisplayName);
	}
	else
	{
		Receptacle->ExecuteRemovalLinks();
	}
	return LogTransferSuccess(Operation, Candidate, TEXT("Receptacle item transferred to inventory."));
}

FGridItemTransferResult UGridItemTransferService::TransferInventorySlotToCharacter(
	UGridPartyInventoryComponent* Inventory, int32 SourceCharacterIndex, int32 SourceInventorySlotIndex, int32 TargetCharacterIndex, int32 RequestedQuantity)
{
	static const TCHAR* Operation = TEXT("InventorySlotToCharacter");
	if (!Inventory || !Inventory->IsValidCharacterIndex(SourceCharacterIndex))
	{
		return LogTransferFailure(Operation, EGridItemTransferResult::InvalidSource, TEXT("Inventory or source character is invalid."));
	}
	if (!Inventory->IsValidCharacterIndex(TargetCharacterIndex) || TargetCharacterIndex == SourceCharacterIndex)
	{
		return LogTransferFailure(Operation, EGridItemTransferResult::InvalidDestination, TEXT("Target character is invalid or identical to source."));
	}

	FGridPartyTransactionScope Transaction(Inventory, { SourceCharacterIndex, TargetCharacterIndex });
	if (!Transaction.IsAcquired())
		return LogTransferFailure(Operation, EGridItemTransferResult::TransactionConflict,
			TEXT("TransactionConflict: a source/destination character is reserved, or its identity is ambiguous."));

	FGridCharacterInventoryState& SourceCharacter = Inventory->PartyInventoryState.ActiveCharacters[SourceCharacterIndex];
	if (!SourceCharacter.InventorySlots.IsValidIndex(SourceInventorySlotIndex) || SourceCharacter.InventorySlots[SourceInventorySlotIndex].IsEmpty())
	{
		return LogTransferFailure(Operation, EGridItemTransferResult::InvalidSource, TEXT("Source inventory slot is invalid or empty."));
	}

	const FGridInventorySlot SourceSnapshot = SourceCharacter.InventorySlots[SourceInventorySlotIndex];
	const int32 SourceQuantity = FMath::Max(1, SourceSnapshot.Item.Quantity);
	const int32 QuantityToTransfer = RequestedQuantity <= 0 ? SourceQuantity : RequestedQuantity;
	if (!SourceSnapshot.Item.IsValid() || QuantityToTransfer <= 0 || QuantityToTransfer > SourceQuantity)
	{
		return LogTransferFailure(Operation, EGridItemTransferResult::InvalidItem, TEXT("Requested transfer quantity is invalid."), &SourceSnapshot.Item);
	}

	FGridItemInstance Candidate = SourceSnapshot.Item;
	Candidate.Quantity = QuantityToTransfer;
	if (QuantityToTransfer < SourceQuantity)
	{
		// A split creates a distinct runtime item; the source remainder keeps the
		// original identity so no duplicate RuntimeObjectId can exist.
		Candidate.RuntimeObjectId = FGuid::NewGuid();
	}

	if (!Transaction.ReserveItem(SourceSnapshot.Item.RuntimeObjectId) ||
		(Candidate.RuntimeObjectId != SourceSnapshot.Item.RuntimeObjectId && !Transaction.ReserveItem(Candidate.RuntimeObjectId)))
		return LogTransferFailure(Operation, EGridItemTransferResult::TransactionConflict, TEXT("TransactionConflict: item already reserved."));

	if (!Inventory->CanAddItemToCharacterInventory(TargetCharacterIndex, Candidate))
	{
		const FString Message = TEXT("Target character inventory has no capacity for the item.");
		UE_LOG(LogGridItemTransfer, Verbose, TEXT("GridItemTransfer Rejected Operation=%s Result=%s Item=%s RuntimeId=%s Message=%s"), Operation,
			*UEnum::GetValueAsString(EGridItemTransferResult::InventoryFull), *Candidate.ItemDefinitionId.ToString(), *Candidate.RuntimeObjectId.ToString(),
			*Message);
		return MakeTransferResult(EGridItemTransferResult::InventoryFull, Message);
	}

	if (QuantityToTransfer == SourceQuantity)
	{
		SourceCharacter.InventorySlots[SourceInventorySlotIndex] = FGridInventorySlot();
	}
	else
	{
		SourceCharacter.InventorySlots[SourceInventorySlotIndex].Item.Quantity = SourceQuantity - QuantityToTransfer;
	}

	const FGridInventorySlot ExpectedAfterDebit = SourceCharacter.InventorySlots[SourceInventorySlotIndex];

	// AddItemToCharacterInventory performs destination stacking, ownership
	// rewriting and its own destination notification. The source has already
	// reached its final state before that notification is broadcast.
	if (!Inventory->AddItemToCharacterInventoryInternal(Transaction.ResolveIndex(1), Candidate))
	{
		if (!RestoreInventorySource(*Inventory, Transaction, SourceInventorySlotIndex, SourceSnapshot, ExpectedAfterDebit, false))
			return LogTransferFailure(Operation, EGridItemTransferResult::RollbackFailed, TEXT("Target insertion failed; stable source changed."), &Candidate);
		return LogTransferFailure(Operation, EGridItemTransferResult::DestinationInsertFailed,
			TEXT("Target insertion failed; source inventory slot was restored."), &Candidate);
	}

	Inventory->NotifyPartyInventoryChanged(Transaction.ResolveIndex());
	return LogTransferSuccess(Operation, Candidate,
		FString::Printf(TEXT("Transferred quantity %d from character %d to character %d."), QuantityToTransfer, SourceCharacterIndex, TargetCharacterIndex));
}

