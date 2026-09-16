#include "Runtime/GridPartyInventoryComponent.h"

#include "GridEquipmentSlotUtils.h"

bool UGridPartyInventoryComponent::TryExtractOneEquippedItemForWorldTransfer(
	int32 CharacterIndex, EGridEquipmentSlot SourceSlot, FName ExpectedItemDefinitionId, FGridItemInstance& OutWorldItem)
{
	OutWorldItem = FGridItemInstance();
	EnsureEquipmentCountMatchesActiveCharacters();
	if (!IsValidCharacterIndex(CharacterIndex) || !PartyInventoryState.ActiveEquipment.IsValidIndex(CharacterIndex) ||
		!GridEquipmentSlotUtils::IsHandSlot(SourceSlot) || ExpectedItemDefinitionId.IsNone())
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
	NotifyPartyInventoryChanged(CharacterIndex);

	UE_LOG(LogTemp, Log, TEXT("GridInventory EquipmentWorldTransfer Extract Character=%d Slot=%s Item=%s RuntimeId=%s Quantity=%d->%d Result=true"),
		CharacterIndex, GridEquipmentSlotUtils::GetLogName(SourceSlot), *OutWorldItem.ItemDefinitionId.ToString(),
		*OutWorldItem.RuntimeObjectId.ToString(), QuantityBefore, QuantityBefore - 1);
	return true;
}

bool UGridPartyInventoryComponent::TryRestoreExtractedItemToEquipment(int32 CharacterIndex, EGridEquipmentSlot TargetSlot, const FGridItemInstance& WorldItem)
{
	EnsureEquipmentCountMatchesActiveCharacters();
	if (!WorldItem.IsValid() || WorldItem.Quantity != 1 || !IsValidCharacterIndex(CharacterIndex) ||
		!PartyInventoryState.ActiveEquipment.IsValidIndex(CharacterIndex) || !GridEquipmentSlotUtils::IsHandSlot(TargetSlot))
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
	NotifyPartyInventoryChanged(CharacterIndex);

	UE_LOG(LogTemp, Log, TEXT("GridInventory EquipmentWorldTransfer Restored Character=%d Slot=%s Item=%s RuntimeId=%s Result=true"), CharacterIndex,
		GridEquipmentSlotUtils::GetLogName(TargetSlot), *WorldItem.ItemDefinitionId.ToString(), *WorldItem.RuntimeObjectId.ToString());
	return true;
}
