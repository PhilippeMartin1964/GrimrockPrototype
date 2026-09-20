#include "Runtime/GridItemTransferService.h"

#include "Runtime/GridItemActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GridReceptacleActor.h"
#include "Runtime/GridWallLockActor.h"
#include "Runtime/GrimrockPartyPawn.h"

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
		UE_LOG(LogTemp, Warning, TEXT("GridItemTransfer Failed Operation=%s Result=%s Item=%s RuntimeId=%s Message=%s"), Operation,
			*UEnum::GetValueAsString(Result), Item ? *Item->ItemDefinitionId.ToString() : TEXT("None"), Item ? *Item->RuntimeObjectId.ToString() : TEXT("None"),
			*Message);
		return MakeTransferResult(Result, Message);
	}

	FGridItemTransferResult LogTransferSuccess(const TCHAR* Operation, const FGridItemInstance& Item, const FString& Message)
	{
		UE_LOG(LogTemp, Log, TEXT("GridItemTransfer Success Operation=%s Item=%s RuntimeId=%s Message=%s"), Operation, *Item.ItemDefinitionId.ToString(),
			*Item.RuntimeObjectId.ToString(), *Message);
		return MakeTransferResult(EGridItemTransferResult::Success, Message);
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

	FGridItemTransferResult AcceptanceFailure;
	if (!ValidateReceptacleAcceptance(Receptacle, Candidate, AcceptanceFailure, Operation))
	{
		return AcceptanceFailure;
	}

	CharacterState.InventorySlots[InventorySlotIndex] = FGridInventorySlot();
	Inventory->NotifyPartyInventoryChanged(CharacterIndex);

	FGridItemInstance AcceptedItem;
	if (!Receptacle->TryInsertItemInstanceFromCursor(Candidate, AcceptedItem))
	{
		if (!CharacterState.InventorySlots[InventorySlotIndex].IsEmpty())
		{
			return LogTransferFailure(Operation, EGridItemTransferResult::RollbackFailed,
				TEXT("Destination insertion failed and the source slot is no longer available."), &Candidate);
		}

		CharacterState.InventorySlots[InventorySlotIndex] = SourceSnapshot;
		Inventory->NotifyPartyInventoryChanged(CharacterIndex);
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

	FGridCharacterEquipmentState& EquipmentState = Inventory->PartyInventoryState.ActiveEquipment[CharacterIndex];
	FGridItemInstance* SourceItem = EquipmentState.GetMutableSlot(EquipmentSlot);
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

	FGridItemTransferResult AcceptanceFailure;
	if (!ValidateReceptacleAcceptance(Receptacle, SourceSnapshot, AcceptanceFailure, Operation))
	{
		return AcceptanceFailure;
	}

	*SourceItem = FGridItemInstance();
	Inventory->NotifyPartyInventoryChanged(CharacterIndex);

	FGridItemInstance AcceptedItem;
	if (!Receptacle->TryInsertItemInstanceFromCursor(SourceSnapshot, AcceptedItem))
	{
		if (SourceItem->IsValid())
		{
			return LogTransferFailure(Operation, EGridItemTransferResult::RollbackFailed,
				TEXT("Destination insertion failed and the equipment slot is no longer available."), &SourceSnapshot);
		}

		*SourceItem = SourceSnapshot;
		Inventory->NotifyPartyInventoryChanged(CharacterIndex);
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

	FGridCharacterInventoryState& CharacterState = Inventory->PartyInventoryState.ActiveCharacters[CharacterIndex];
	if (!CharacterState.InventorySlots.IsValidIndex(InventorySlotIndex) || CharacterState.InventorySlots[InventorySlotIndex].IsEmpty())
	{
		return LogTransferFailure(Operation, EGridItemTransferResult::InvalidSource, TEXT("Inventory slot source is invalid or empty."));
	}

	const FGridInventorySlot SourceSnapshot = CharacterState.InventorySlots[InventorySlotIndex];
	FGridItemInstance KeyItem = SourceSnapshot.Item;
	if (!KeyItem.IsValid() || !WallLock->CanAcceptKeyDefinition(KeyItem.ItemDefinitionId))
	{
		return LogTransferFailure(Operation, EGridItemTransferResult::DestinationRejectsItem, TEXT("Wall lock rejected the selected key."), &KeyItem);
	}

	KeyItem.Quantity = 1;
	if (SourceSnapshot.Item.Quantity > 1)
	{
		KeyItem.RuntimeObjectId = FGuid::NewGuid();
		CharacterState.InventorySlots[InventorySlotIndex].Item.Quantity -= 1;
	}
	else
	{
		CharacterState.InventorySlots[InventorySlotIndex] = FGridInventorySlot();
	}
	Inventory->NotifyPartyInventoryChanged(CharacterIndex);

	if (!WallLock->TryUnlockWithContextItem(PartyPawn, KeyItem))
	{
		CharacterState.InventorySlots[InventorySlotIndex] = SourceSnapshot;
		Inventory->NotifyPartyInventoryChanged(CharacterIndex);
		return LogTransferFailure(
			Operation, EGridItemTransferResult::DestinationInsertFailed, TEXT("Wall lock insertion failed; inventory source was restored."), &KeyItem);
	}

	return LogTransferSuccess(Operation, KeyItem, TEXT("Inventory key transferred to wall lock."));
}

FGridItemTransferResult UGridItemTransferService::TransferReceptacleItemToInventory(
	AGridReceptacleActor* Receptacle, int32 ContainedItemIndex, UGridPartyInventoryComponent* Inventory, int32 CharacterIndex)
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

	const FGridContainedReceptacleItem SourceSnapshot = Receptacle->ContainedItems[ContainedItemIndex];
	FGridItemInstance Candidate;
	Candidate.RuntimeObjectId = SourceSnapshot.RuntimeObjectId;
	Candidate.ItemDefinitionId = SourceSnapshot.ItemDefinitionId;
	Candidate.DisplayName = SourceSnapshot.DisplayName;
	Candidate.Quantity = SourceSnapshot.Quantity;
	Candidate.Weight = SourceSnapshot.Weight;
	Candidate.bLightsEnabled = SourceSnapshot.bLightsEnabled;
	const bool bHadVisualActor = IsValid(SourceSnapshot.ItemActor.Get());
	Candidate.LastWorldTransform = bHadVisualActor ? SourceSnapshot.ItemActor->GetActorTransform() : FTransform::Identity;

	if (!Candidate.IsValid())
	{
		return LogTransferFailure(Operation, EGridItemTransferResult::InvalidItem, TEXT("Contained receptacle item is invalid."), &Candidate);
	}
	if (!Inventory->CanAddItemToCharacterInventory(CharacterIndex, Candidate))
	{
		return LogTransferFailure(Operation, EGridItemTransferResult::InventoryFull, TEXT("Destination inventory is full."), &Candidate);
	}

	FGridContainedReceptacleItem RemovedItem;
	if (!Receptacle->RemoveContainedItemAtIndex(ContainedItemIndex, RemovedItem))
	{
		return LogTransferFailure(Operation, EGridItemTransferResult::SourceRemoveFailed, TEXT("Failed to remove item from receptacle."), &Candidate);
	}

	if (!Inventory->AddItemToCharacterInventory(CharacterIndex, Candidate))
	{
		const int32 RestoredIndex = Receptacle->AddContainedItem(SourceSnapshot.ItemDefinitionId, SourceSnapshot.ItemDefinition, nullptr,
			SourceSnapshot.bWasInitialItem, SourceSnapshot.Quantity, SourceSnapshot.RuntimeObjectId);
		if (!Receptacle->ContainedItems.IsValidIndex(RestoredIndex))
		{
			return LogTransferFailure(
				Operation, EGridItemTransferResult::RollbackFailed, TEXT("Inventory insertion failed and receptacle rollback failed."), &Candidate);
		}

		FGridContainedReceptacleItem& RestoredItem = Receptacle->ContainedItems[RestoredIndex];
		RestoredItem.Weight = SourceSnapshot.Weight;
		RestoredItem.DisplayName = SourceSnapshot.DisplayName;
		RestoredItem.bLightsEnabled = SourceSnapshot.bLightsEnabled;
		if (SourceSnapshot.bWasInitialItem)
		{
			Receptacle->RemovedInitialItemDefinitionIds.Remove(SourceSnapshot.ItemDefinitionId);
		}
		if (bHadVisualActor && IsValid(RestoredItem.ItemActor.Get()))
		{
			RestoredItem.ItemActor->SetActorTransform(Candidate.LastWorldTransform, false, nullptr, ETeleportType::TeleportPhysics);
			RestoredItem.ItemActor->SetItemLightsEnabled(SourceSnapshot.bLightsEnabled);
		}

		return LogTransferFailure(
			Operation, EGridItemTransferResult::DestinationInsertFailed, TEXT("Inventory insertion failed; receptacle source was restored."), &Candidate);
	}

	Receptacle->ExecuteRemovalLinks();
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

	if (!Inventory->CanAddItemToCharacterInventory(TargetCharacterIndex, Candidate))
	{
		return LogTransferFailure(Operation, EGridItemTransferResult::InventoryFull, TEXT("Target character inventory has no capacity for the item."), &Candidate);
	}

	if (QuantityToTransfer == SourceQuantity)
	{
		SourceCharacter.InventorySlots[SourceInventorySlotIndex] = FGridInventorySlot();
	}
	else
	{
		SourceCharacter.InventorySlots[SourceInventorySlotIndex].Item.Quantity = SourceQuantity - QuantityToTransfer;
	}

	// AddItemToCharacterInventory performs destination stacking, ownership
	// rewriting and its own destination notification. The source has already
	// reached its final state before that notification is broadcast.
	if (!Inventory->AddItemToCharacterInventory(TargetCharacterIndex, Candidate))
	{
		SourceCharacter.InventorySlots[SourceInventorySlotIndex] = SourceSnapshot;
		return LogTransferFailure(Operation, EGridItemTransferResult::DestinationInsertFailed,
			TEXT("Target insertion failed; source inventory slot was restored."), &Candidate);
	}

	Inventory->NotifyPartyInventoryChanged(SourceCharacterIndex);
	return LogTransferSuccess(Operation, Candidate,
		FString::Printf(TEXT("Transferred quantity %d from character %d to character %d."), QuantityToTransfer, SourceCharacterIndex, TargetCharacterIndex));
}

