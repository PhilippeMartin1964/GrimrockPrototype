#include "Runtime/GridPartyInventoryComponent.h"

namespace
{
	bool GridPartyInventoryWorldTransferIsHandEquipmentSlot(EGridEquipmentSlot Slot)
	{
		return Slot == EGridEquipmentSlot::MainHand || Slot == EGridEquipmentSlot::OffHand;
	}

	const TCHAR* GridPartyInventoryWorldTransferGetEquipmentSlotName(EGridEquipmentSlot Slot)
	{
		switch (Slot)
		{
			case EGridEquipmentSlot::MainHand:
				return TEXT("MainHand");
			case EGridEquipmentSlot::OffHand:
				return TEXT("OffHand");
			default:
				return TEXT("Unsupported");
		}
	}
}

bool UGridPartyInventoryComponent::TryExtractOneEquippedItemForWorldTransfer(
	int32 CharacterIndex, EGridEquipmentSlot SourceSlot, FName ExpectedItemDefinitionId, FGridItemInstance& OutWorldItem)
{
	OutWorldItem = FGridItemInstance();
	EnsureEquipmentCountMatchesActiveCharacters();
	if (!IsValidCharacterIndex(CharacterIndex) || !PartyInventoryState.ActiveEquipment.IsValidIndex(CharacterIndex) ||
		!GridPartyInventoryWorldTransferIsHandEquipmentSlot(SourceSlot) || ExpectedItemDefinitionId.IsNone())
	{
		return false;
	}

	FGridItemInstance* EquippedItem = PartyInventoryState.ActiveEquipment[CharacterIndex].GetMutableSlot(SourceSlot);
	if (!EquippedItem || !EquippedItem->IsValid() || EquippedItem->ItemDefinitionId != ExpectedItemDefinitionId)
	{
		return false;
	}

	OutWorldItem = *EquippedItem;
	OutWorldItem.Quantity = 1;
	OutWorldItem.OwnerType = EGridItemOwnerType::World;
	OutWorldItem.OwnerGuid = FGuid();
	OutWorldItem.OwnerCharacterIndex = INDEX_NONE;
	OutWorldItem.EquipmentSlot = EGridEquipmentSlot::None;

	const int32 QuantityBefore = FMath::Max(1, EquippedItem->Quantity);
	if (QuantityBefore > 1)
	{
		--EquippedItem->Quantity;
		OutWorldItem.RuntimeObjectId = FGuid::NewGuid();
	}
	else
	{
		*EquippedItem = FGridItemInstance();
	}
	RecalculateCharacterWeight(CharacterIndex);

	UE_LOG(LogTemp, Log, TEXT("GridInventory EquipmentWorldTransfer Extract Character=%d Slot=%s Item=%s RuntimeId=%s Quantity=%d->%d Result=true"),
		CharacterIndex, GridPartyInventoryWorldTransferGetEquipmentSlotName(SourceSlot), *OutWorldItem.ItemDefinitionId.ToString(),
		*OutWorldItem.RuntimeObjectId.ToString(), QuantityBefore, QuantityBefore - 1);
	return true;
}

bool UGridPartyInventoryComponent::TryRestoreExtractedItemToEquipment(
	int32 CharacterIndex, EGridEquipmentSlot TargetSlot, const FGridItemInstance& WorldItem)
{
	EnsureEquipmentCountMatchesActiveCharacters();
	if (!WorldItem.IsValid() || WorldItem.Quantity != 1 || !IsValidCharacterIndex(CharacterIndex) ||
		!PartyInventoryState.ActiveEquipment.IsValidIndex(CharacterIndex) || !GridPartyInventoryWorldTransferIsHandEquipmentSlot(TargetSlot))
	{
		return false;
	}

	FGridItemInstance* EquippedItem = PartyInventoryState.ActiveEquipment[CharacterIndex].GetMutableSlot(TargetSlot);
	if (!EquippedItem)
	{
		return false;
	}

	const FGridCharacterInventoryState& CharacterState = PartyInventoryState.ActiveCharacters[CharacterIndex];
	if (EquippedItem->IsValid())
	{
		if (EquippedItem->ItemDefinitionId != WorldItem.ItemDefinitionId)
		{
			return false;
		}
		++EquippedItem->Quantity;
	}
	else
	{
		*EquippedItem = WorldItem;
		EquippedItem->OwnerType = EGridItemOwnerType::EquipmentSlot;
		EquippedItem->OwnerGuid = CharacterState.CharacterId;
		EquippedItem->OwnerCharacterIndex = CharacterIndex;
		EquippedItem->EquipmentSlot = TargetSlot;
	}
	RecalculateCharacterWeight(CharacterIndex);

	UE_LOG(LogTemp, Warning, TEXT("GridInventory EquipmentWorldTransfer Restored Character=%d Slot=%s Item=%s RuntimeId=%s Result=true"), CharacterIndex,
		GridPartyInventoryWorldTransferGetEquipmentSlotName(TargetSlot), *WorldItem.ItemDefinitionId.ToString(), *WorldItem.RuntimeObjectId.ToString());
	return true;
}
