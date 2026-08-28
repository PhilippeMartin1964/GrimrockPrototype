#include "Runtime/GridPartyInventoryComponent.h"

#include "Runtime/GridItemDefinitionAsset.h"

namespace
{
	int32 GridPartyInventoryCursorTransferFindFreeInventorySlotIndex(const FGridCharacterInventoryState& CharacterState)
	{
		for (int32 SlotIndex = 0; SlotIndex < CharacterState.InventorySlots.Num(); ++SlotIndex)
		{
			if (CharacterState.InventorySlots[SlotIndex].IsEmpty())
			{
				return SlotIndex;
			}
		}

		return INDEX_NONE;
	}

	const TCHAR* GridPartyInventoryCursorTransferGetEquipmentSlotName(EGridEquipmentSlot Slot)
	{
		switch (Slot)
		{
			case EGridEquipmentSlot::None:
				return TEXT("None");
			case EGridEquipmentSlot::MainHand:
				return TEXT("MainHand");
			case EGridEquipmentSlot::OffHand:
				return TEXT("OffHand");
			case EGridEquipmentSlot::Head:
				return TEXT("Head");
			case EGridEquipmentSlot::Chest:
				return TEXT("Chest");
			case EGridEquipmentSlot::Legs:
				return TEXT("Legs");
			case EGridEquipmentSlot::Feet:
				return TEXT("Feet");
			case EGridEquipmentSlot::Amulet:
				return TEXT("Amulet");
			case EGridEquipmentSlot::Ring1:
				return TEXT("Ring1");
			case EGridEquipmentSlot::Ring2:
				return TEXT("Ring2");
			case EGridEquipmentSlot::Shoulders:
				return TEXT("Shoulders");
			case EGridEquipmentSlot::Gloves:
				return TEXT("Gloves");
			case EGridEquipmentSlot::Belt:
				return TEXT("Belt");
			case EGridEquipmentSlot::Cloak:
				return TEXT("Cloak");
			case EGridEquipmentSlot::Talisman:
				return TEXT("Talisman");
			case EGridEquipmentSlot::QuickSlot1:
				return TEXT("QuickSlot1");
			case EGridEquipmentSlot::QuickSlot2:
				return TEXT("QuickSlot2");
			case EGridEquipmentSlot::Face:
				return TEXT("Visage");
			case EGridEquipmentSlot::Shirt:
				return TEXT("Chemise");
			case EGridEquipmentSlot::Bracers:
				return TEXT("Brassards");
			case EGridEquipmentSlot::Earring1:
				return TEXT("Bijou d'oreille I");
			case EGridEquipmentSlot::Earring2:
				return TEXT("Bijou d'oreille II");
			default:
				return TEXT("Unsupported");
		}
	}
}

bool UGridPartyInventoryComponent::TryTakeEquipmentSlotToCursor(int32 CharacterIndex, EGridEquipmentSlot SourceSlot)
{
	EnsureEquipmentCountMatchesActiveCharacters();

	if (!IsValidCharacterIndex(CharacterIndex) || !PartyInventoryState.ActiveEquipment.IsValidIndex(CharacterIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("GridInventory Cursor Take FromEquipment Failed Character=%d Slot=%s Reason=InvalidCharacter"), CharacterIndex,
			GridPartyInventoryCursorTransferGetEquipmentSlotName(SourceSlot));
		return false;
	}

	if (SourceSlot == EGridEquipmentSlot::None)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridInventory Cursor Take FromEquipment Failed Character=%d Slot=%s Reason=InvalidSlot"), CharacterIndex,
			GridPartyInventoryCursorTransferGetEquipmentSlotName(SourceSlot));
		return false;
	}

	if (HasCursorItem())
	{
		UE_LOG(LogTemp, Warning, TEXT("GridInventory Cursor Take FromEquipment Failed Character=%d Slot=%s Reason=CursorOccupied"), CharacterIndex,
			GridPartyInventoryCursorTransferGetEquipmentSlotName(SourceSlot));
		return false;
	}

	FGridCharacterEquipmentState& EquipmentState = PartyInventoryState.ActiveEquipment[CharacterIndex];
	FGridItemInstance* EquippedItem = EquipmentState.GetMutableSlot(SourceSlot);
	if (!EquippedItem)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridInventory Cursor Take FromEquipment Failed Character=%d Slot=%s Reason=InvalidSlot"), CharacterIndex,
			GridPartyInventoryCursorTransferGetEquipmentSlotName(SourceSlot));
		return false;
	}

	if (!EquippedItem->IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("GridInventory Cursor Take FromEquipment Failed Character=%d Slot=%s Reason=EmptySlot"), CharacterIndex,
			GridPartyInventoryCursorTransferGetEquipmentSlotName(SourceSlot));
		return false;
	}

	FGridItemInstance ItemToCursor = *EquippedItem;
	ItemToCursor.OwnerType = EGridItemOwnerType::Cursor;
	ItemToCursor.OwnerGuid = FGuid();
	ItemToCursor.OwnerCharacterIndex = INDEX_NONE;
	ItemToCursor.EquipmentSlot = EGridEquipmentSlot::None;

	*EquippedItem = FGridItemInstance();
	PartyInventoryState.CursorItem = ItemToCursor;
	PartyInventoryState.bHasCursorItem = true;
	NotifyPartyInventoryChanged(CharacterIndex);

	UE_LOG(LogTemp, Log, TEXT("GridInventory Cursor Take FromEquipment Character=%d Slot=%s Item=%s RuntimeId=%s Result=true"), CharacterIndex,
		GridPartyInventoryCursorTransferGetEquipmentSlotName(SourceSlot), *ItemToCursor.ItemDefinitionId.ToString(), *ItemToCursor.RuntimeObjectId.ToString());
	return true;
}

bool UGridPartyInventoryComponent::TryTakeSelectedCharacterEquipmentSlotToCursor(EGridEquipmentSlot SourceSlot)
{
	return TryTakeEquipmentSlotToCursor(PartyInventoryState.SelectedCharacterIndex, SourceSlot);
}

bool UGridPartyInventoryComponent::SetCursorItem(const FGridItemInstance& Item)
{
	if (!Item.IsValid())
	{
		return false;
	}

	PartyInventoryState.CursorItem = Item;
	PartyInventoryState.CursorItem.OwnerType = EGridItemOwnerType::Cursor;
	PartyInventoryState.CursorItem.OwnerGuid = FGuid();
	PartyInventoryState.CursorItem.OwnerCharacterIndex = INDEX_NONE;
	PartyInventoryState.CursorItem.EquipmentSlot = EGridEquipmentSlot::None;
	PartyInventoryState.bHasCursorItem = true;
	return true;
}

bool UGridPartyInventoryComponent::ClearCursorItem()
{
	if (!PartyInventoryState.bHasCursorItem)
	{
		return false;
	}

	PartyInventoryState.CursorItem = FGridItemInstance();
	PartyInventoryState.bHasCursorItem = false;
	return true;
}

bool UGridPartyInventoryComponent::HasCursorItem() const
{
	return PartyInventoryState.bHasCursorItem && PartyInventoryState.CursorItem.IsValid();
}

const FGridItemInstance& UGridPartyInventoryComponent::GetCursorItem() const
{
	return PartyInventoryState.CursorItem;
}

bool UGridPartyInventoryComponent::TryTakeInventorySlotToCursor(int32 CharacterIndex, int32 InventorySlotIndex)
{
	return TryTakeInventorySlotQuantityToCursor(CharacterIndex, InventorySlotIndex, MAX_int32);
}

bool UGridPartyInventoryComponent::TryTakeInventorySlotQuantityToCursor(int32 CharacterIndex, int32 InventorySlotIndex, int32 Quantity)
{
	if (HasCursorItem())
	{
		UE_LOG(LogTemp, Warning, TEXT("GridInventory Cursor Take Failed Character=%d Slot=%d Reason=CursorOccupied"), CharacterIndex, InventorySlotIndex);
		return false;
	}

	if (!IsValidCharacterIndex(CharacterIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("GridInventory Cursor Take Failed Character=%d Slot=%d Reason=InvalidCharacter"), CharacterIndex, InventorySlotIndex);
		return false;
	}

	if (Quantity <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridInventory Cursor Take Failed Character=%d Slot=%d Reason=InvalidQuantity Quantity=%d"), CharacterIndex,
			InventorySlotIndex, Quantity);
		return false;
	}

	FGridCharacterInventoryState& CharacterState = PartyInventoryState.ActiveCharacters[CharacterIndex];
	if (!CharacterState.InventorySlots.IsValidIndex(InventorySlotIndex) || CharacterState.InventorySlots[InventorySlotIndex].IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("GridInventory Cursor Take Failed Character=%d Slot=%d Reason=InvalidInventorySlot"), CharacterIndex, InventorySlotIndex);
		return false;
	}

	FGridInventorySlot& InventorySlot = CharacterState.InventorySlots[InventorySlotIndex];
	const UGridItemDefinitionAsset* Definition = FindItemDefinition(InventorySlot.Item.ItemDefinitionId);
	const bool bCanSplitStack = Definition && Definition->bStackable && InventorySlot.Item.Quantity > 1 && Quantity < InventorySlot.Item.Quantity;

	FGridItemInstance ItemToCursor = InventorySlot.Item;
	if (bCanSplitStack)
	{
		InventorySlot.Item.Quantity -= Quantity;
		ItemToCursor.Quantity = Quantity;
		ItemToCursor.RuntimeObjectId = FGuid::NewGuid();
	}
	else
	{
		InventorySlot = FGridInventorySlot();
	}

	ItemToCursor.OwnerType = EGridItemOwnerType::Cursor;
	ItemToCursor.OwnerGuid = FGuid();
	ItemToCursor.OwnerCharacterIndex = INDEX_NONE;
	ItemToCursor.EquipmentSlot = EGridEquipmentSlot::None;

	PartyInventoryState.CursorItem = ItemToCursor;
	PartyInventoryState.bHasCursorItem = true;
	NotifyPartyInventoryChanged(CharacterIndex);

	UE_LOG(LogTemp, Log, TEXT("GridInventory Cursor Take FromInventory Character=%d Slot=%d Item=%s RuntimeId=%s Quantity=%d Split=%s"), CharacterIndex,
		InventorySlotIndex, *ItemToCursor.ItemDefinitionId.ToString(), *ItemToCursor.RuntimeObjectId.ToString(), ItemToCursor.Quantity,
		bCanSplitStack ? TEXT("true") : TEXT("false"));
	return true;
}

bool UGridPartyInventoryComponent::TryPlaceCursorItemInCharacterInventorySlot(int32 CharacterIndex, int32 TargetSlotIndex)
{
	if (!HasCursorItem())
	{
		UE_LOG(LogTemp, Warning, TEXT("GridInventory Cursor Place ToInventorySlot Failed Character=%d Slot=%d Reason=NoCursorItem"), CharacterIndex,
			TargetSlotIndex);
		return false;
	}

	if (!IsValidCharacterIndex(CharacterIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("GridInventory Cursor Place ToInventorySlot Failed Character=%d Slot=%d Reason=InvalidCharacter"), CharacterIndex,
			TargetSlotIndex);
		return false;
	}

	FGridCharacterInventoryState& CharacterState = PartyInventoryState.ActiveCharacters[CharacterIndex];
	if (!CharacterState.InventorySlots.IsValidIndex(TargetSlotIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("GridInventory Cursor Place ToInventorySlot Failed Character=%d Slot=%d Reason=InvalidTargetSlot"), CharacterIndex,
			TargetSlotIndex);
		return false;
	}

	FGridInventorySlot& TargetSlot = CharacterState.InventorySlots[TargetSlotIndex];
	FGridItemInstance CursorItem = PartyInventoryState.CursorItem;
	ApplyItemDefinitionToInstance(CursorItem);
	CursorItem.OwnerType = EGridItemOwnerType::CharacterInventory;
	CursorItem.OwnerGuid = CharacterState.CharacterId;
	CursorItem.OwnerCharacterIndex = CharacterIndex;
	CursorItem.EquipmentSlot = EGridEquipmentSlot::None;

	if (TargetSlot.IsEmpty())
	{
		TargetSlot.bOccupied = true;
		TargetSlot.Item = CursorItem;
		PartyInventoryState.CursorItem = FGridItemInstance();
		PartyInventoryState.bHasCursorItem = false;
		NotifyPartyInventoryChanged(CharacterIndex);

		UE_LOG(LogTemp, Log, TEXT("GridInventory Cursor Place ToInventorySlot Character=%d Slot=%d Item=%s Result=true"), CharacterIndex, TargetSlotIndex,
			*CursorItem.ItemDefinitionId.ToString());
		return true;
	}

	FGridItemInstance SlotItem = TargetSlot.Item;
	SlotItem.OwnerType = EGridItemOwnerType::Cursor;
	SlotItem.OwnerGuid = FGuid();
	SlotItem.OwnerCharacterIndex = INDEX_NONE;
	SlotItem.EquipmentSlot = EGridEquipmentSlot::None;

	TargetSlot.Item = CursorItem;
	TargetSlot.bOccupied = true;
	PartyInventoryState.CursorItem = SlotItem;
	PartyInventoryState.bHasCursorItem = true;
	NotifyPartyInventoryChanged(CharacterIndex);

	UE_LOG(LogTemp, Log, TEXT("GridInventory Cursor Swap WithInventorySlot Character=%d Slot=%d CursorItem=%s SlotItem=%s Result=true"), CharacterIndex,
		TargetSlotIndex, *CursorItem.ItemDefinitionId.ToString(), *SlotItem.ItemDefinitionId.ToString());
	return true;
}

bool UGridPartyInventoryComponent::TryMoveCharacterInventorySlot(int32 CharacterIndex, int32 SourceSlotIndex, int32 TargetSlotIndex)
{
	if (!IsValidCharacterIndex(CharacterIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("GridInventory Move InventorySlot Failed Character=%d Source=%d Target=%d Reason=InvalidCharacter"), CharacterIndex,
			SourceSlotIndex, TargetSlotIndex);
		return false;
	}

	FGridCharacterInventoryState& CharacterState = PartyInventoryState.ActiveCharacters[CharacterIndex];
	if (!CharacterState.InventorySlots.IsValidIndex(SourceSlotIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("GridInventory Move InventorySlot Failed Character=%d Source=%d Target=%d Reason=InvalidSourceSlot"), CharacterIndex,
			SourceSlotIndex, TargetSlotIndex);
		return false;
	}

	if (!CharacterState.InventorySlots.IsValidIndex(TargetSlotIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("GridInventory Move InventorySlot Failed Character=%d Source=%d Target=%d Reason=InvalidTargetSlot"), CharacterIndex,
			SourceSlotIndex, TargetSlotIndex);
		return false;
	}

	if (SourceSlotIndex == TargetSlotIndex)
	{
		UE_LOG(LogTemp, Log, TEXT("GridInventory Move InventorySlot Character=%d Source=%d Target=%d Result=true Reason=SameSlot"), CharacterIndex,
			SourceSlotIndex, TargetSlotIndex);
		return true;
	}

	FGridInventorySlot& SourceSlot = CharacterState.InventorySlots[SourceSlotIndex];
	FGridInventorySlot& TargetSlot = CharacterState.InventorySlots[TargetSlotIndex];
	if (SourceSlot.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("GridInventory Move InventorySlot Failed Character=%d Source=%d Target=%d Reason=SourceEmpty"), CharacterIndex,
			SourceSlotIndex, TargetSlotIndex);
		return false;
	}

	FGridItemInstance SourceItem = SourceSlot.Item;
	SourceItem.OwnerType = EGridItemOwnerType::CharacterInventory;
	SourceItem.OwnerGuid = CharacterState.CharacterId;
	SourceItem.OwnerCharacterIndex = CharacterIndex;
	SourceItem.EquipmentSlot = EGridEquipmentSlot::None;

	if (TargetSlot.IsEmpty())
	{
		TargetSlot.bOccupied = true;
		TargetSlot.Item = SourceItem;
		SourceSlot = FGridInventorySlot();
		NotifyPartyInventoryChanged(CharacterIndex);

		UE_LOG(LogTemp, Log, TEXT("GridInventory Move InventorySlot Character=%d Source=%d Target=%d Item=%s Result=true"), CharacterIndex, SourceSlotIndex,
			TargetSlotIndex, *SourceItem.ItemDefinitionId.ToString());
		return true;
	}

	FGridItemInstance TargetItem = TargetSlot.Item;
	TargetItem.OwnerType = EGridItemOwnerType::CharacterInventory;
	TargetItem.OwnerGuid = CharacterState.CharacterId;
	TargetItem.OwnerCharacterIndex = CharacterIndex;
	TargetItem.EquipmentSlot = EGridEquipmentSlot::None;

	SourceSlot.bOccupied = true;
	SourceSlot.Item = TargetItem;
	TargetSlot.bOccupied = true;
	TargetSlot.Item = SourceItem;
	NotifyPartyInventoryChanged(CharacterIndex);

	UE_LOG(LogTemp, Log, TEXT("GridInventory Swap InventorySlots Character=%d A=%d B=%d ItemA=%s ItemB=%s Result=true"), CharacterIndex, SourceSlotIndex,
		TargetSlotIndex, *SourceItem.ItemDefinitionId.ToString(), *TargetItem.ItemDefinitionId.ToString());
	return true;
}

bool UGridPartyInventoryComponent::TryPlaceCursorItemInCharacterInventory(int32 CharacterIndex)
{
	if (!HasCursorItem())
	{
		UE_LOG(LogTemp, Warning, TEXT("GridInventory Cursor Place Failed Character=%d Reason=NoCursorItem"), CharacterIndex);
		return false;
	}

	if (!IsValidCharacterIndex(CharacterIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("GridInventory Cursor Place Failed Character=%d Reason=InvalidCharacter"), CharacterIndex);
		return false;
	}

	FGridCharacterInventoryState& CharacterState = PartyInventoryState.ActiveCharacters[CharacterIndex];
	const int32 FreeSlotIndex = GridPartyInventoryCursorTransferFindFreeInventorySlotIndex(CharacterState);
	if (FreeSlotIndex == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridInventory Cursor Place Failed Character=%d Item=%s RuntimeId=%s Reason=InventoryFull"), CharacterIndex,
			*PartyInventoryState.CursorItem.ItemDefinitionId.ToString(), *PartyInventoryState.CursorItem.RuntimeObjectId.ToString());
		return false;
	}

	FGridItemInstance ItemToInventory = PartyInventoryState.CursorItem;
	ApplyItemDefinitionToInstance(ItemToInventory);
	ItemToInventory.OwnerType = EGridItemOwnerType::CharacterInventory;
	ItemToInventory.OwnerGuid = CharacterState.CharacterId;
	ItemToInventory.OwnerCharacterIndex = CharacterIndex;
	ItemToInventory.EquipmentSlot = EGridEquipmentSlot::None;

	CharacterState.InventorySlots[FreeSlotIndex].bOccupied = true;
	CharacterState.InventorySlots[FreeSlotIndex].Item = ItemToInventory;
	PartyInventoryState.CursorItem = FGridItemInstance();
	PartyInventoryState.bHasCursorItem = false;
	NotifyPartyInventoryChanged(CharacterIndex);

	UE_LOG(LogTemp, Log, TEXT("GridInventory Cursor Place ToInventory Character=%d Slot=%d Item=%s RuntimeId=%s"), CharacterIndex, FreeSlotIndex,
		*ItemToInventory.ItemDefinitionId.ToString(), *ItemToInventory.RuntimeObjectId.ToString());
	return true;
}

bool UGridPartyInventoryComponent::TryPlaceCursorItemInSelectedCharacterInventory()
{
	return TryPlaceCursorItemInCharacterInventory(PartyInventoryState.SelectedCharacterIndex);
}

bool UGridPartyInventoryComponent::TryClearCursorToSelectedCharacterInventory()
{
	return TryPlaceCursorItemInSelectedCharacterInventory();
}

bool UGridPartyInventoryComponent::TryDropCursorItem()
{
	if (!HasCursorItem())
	{
		UE_LOG(LogTemp, Warning, TEXT("GridInventory Cursor Drop Failed Reason=NoCursorItem"));
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("GridInventory Cursor Drop Failed Item=%s RuntimeId=%s Reason=NotImplemented"),
		*PartyInventoryState.CursorItem.ItemDefinitionId.ToString(), *PartyInventoryState.CursorItem.RuntimeObjectId.ToString());
	return false;
}

bool UGridPartyInventoryComponent::CanEquipCursorItemToCharacterSlot(int32 CharacterIndex, EGridEquipmentSlot TargetSlot) const
{
	if (!HasCursorItem() || !IsValidCharacterIndex(CharacterIndex) || TargetSlot == EGridEquipmentSlot::None)
	{
		return false;
	}

	return CanEquipItemToSlot(CharacterIndex, PartyInventoryState.CursorItem, TargetSlot);
}

bool UGridPartyInventoryComponent::TryEquipCursorItemToCharacterSlot(int32 CharacterIndex, EGridEquipmentSlot TargetSlot)
{
	EnsureEquipmentCountMatchesActiveCharacters();

	if (!CanEquipCursorItemToCharacterSlot(CharacterIndex, TargetSlot) || !PartyInventoryState.ActiveEquipment.IsValidIndex(CharacterIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("GridInventory Cursor Equip Failed Character=%d Slot=%s Reason=InvalidOrIncompatible"), CharacterIndex,
			GridPartyInventoryCursorTransferGetEquipmentSlotName(TargetSlot));
		return false;
	}

	FGridCharacterInventoryState& CharacterState = PartyInventoryState.ActiveCharacters[CharacterIndex];
	FGridCharacterEquipmentState& EquipmentState = PartyInventoryState.ActiveEquipment[CharacterIndex];
	FGridItemInstance* TargetItem = EquipmentState.GetMutableSlot(TargetSlot);
	if (!TargetItem)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridInventory Cursor Equip Failed Character=%d Slot=%s Reason=InvalidEquipmentSlot"), CharacterIndex,
			GridPartyInventoryCursorTransferGetEquipmentSlotName(TargetSlot));
		return false;
	}

	FGridItemInstance ItemToEquip = PartyInventoryState.CursorItem;
	ItemToEquip.OwnerType = EGridItemOwnerType::EquipmentSlot;
	ItemToEquip.OwnerGuid = CharacterState.CharacterId;
	ItemToEquip.OwnerCharacterIndex = CharacterIndex;
	ItemToEquip.EquipmentSlot = TargetSlot;

	FGridItemInstance PreviouslyEquippedItem = *TargetItem;
	const bool bWasOccupied = PreviouslyEquippedItem.IsValid();
	if (bWasOccupied)
	{
		PreviouslyEquippedItem.OwnerType = EGridItemOwnerType::Cursor;
		PreviouslyEquippedItem.OwnerGuid = FGuid();
		PreviouslyEquippedItem.OwnerCharacterIndex = INDEX_NONE;
		PreviouslyEquippedItem.EquipmentSlot = EGridEquipmentSlot::None;
		PartyInventoryState.CursorItem = PreviouslyEquippedItem;
	}
	else
	{
		PartyInventoryState.CursorItem = FGridItemInstance();
		PartyInventoryState.bHasCursorItem = false;
	}

	*TargetItem = ItemToEquip;
	NotifyPartyInventoryChanged(CharacterIndex);

	if (bWasOccupied)
	{
		UE_LOG(LogTemp, Log, TEXT("GridInventory Cursor Equip Swap Character=%d Slot=%s NewItem=%s OldItem=%s Result=true"), CharacterIndex,
			GridPartyInventoryCursorTransferGetEquipmentSlotName(TargetSlot), *ItemToEquip.ItemDefinitionId.ToString(),
			*PreviouslyEquippedItem.ItemDefinitionId.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("GridInventory Cursor Equip Character=%d Slot=%s Item=%s RuntimeId=%s Result=true"), CharacterIndex,
			GridPartyInventoryCursorTransferGetEquipmentSlotName(TargetSlot), *ItemToEquip.ItemDefinitionId.ToString(),
			*ItemToEquip.RuntimeObjectId.ToString());
	}

	return true;
}

bool UGridPartyInventoryComponent::TryEquipCursorItemToSelectedCharacterSlot(EGridEquipmentSlot TargetSlot)
{
	return TryEquipCursorItemToCharacterSlot(PartyInventoryState.SelectedCharacterIndex, TargetSlot);
}
