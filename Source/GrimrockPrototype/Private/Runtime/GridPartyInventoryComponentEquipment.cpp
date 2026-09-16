#include "Runtime/GridPartyInventoryComponent.h"

#include "GridEquipmentSlotUtils.h"
#include "Runtime/GridItemDefinitionAsset.h"

namespace
{
	int32 GridPartyInventoryEquipmentFindFreeInventorySlotIndex(const FGridCharacterInventoryState& CharacterState)
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

	void GridPartyInventoryEquipmentAddStatBonus(FGridEquipmentStatBonus& InOutTotal, const FGridEquipmentStatBonus& Bonus)
	{
		InOutTotal.StrengthBonus += Bonus.StrengthBonus;
		InOutTotal.DexterityBonus += Bonus.DexterityBonus;
		InOutTotal.ConstitutionBonus += Bonus.ConstitutionBonus;
		InOutTotal.IntelligenceBonus += Bonus.IntelligenceBonus;
		InOutTotal.WisdomBonus += Bonus.WisdomBonus;
		InOutTotal.CharismaBonus += Bonus.CharismaBonus;
		InOutTotal.MaxHealthBonus += Bonus.MaxHealthBonus;
		InOutTotal.MaxManaBonus += Bonus.MaxManaBonus;
		InOutTotal.CarryWeightBonus += Bonus.CarryWeightBonus;
		InOutTotal.ArmorBonus += Bonus.ArmorBonus;
	}
}

bool UGridPartyInventoryComponent::CanEquipItemToSlot(int32 CharacterIndex, const FGridItemInstance& Item, EGridEquipmentSlot TargetSlot) const
{
	if (!IsValidCharacterIndex(CharacterIndex) || !PartyInventoryState.ActiveEquipment.IsValidIndex(CharacterIndex))
	{
		return false;
	}

	if (!Item.IsValid())
	{
		return false;
	}

	if (const UGridItemDefinitionAsset* Definition = FindItemDefinition(Item.ItemDefinitionId))
	{
		return Definition->CanEquipToSlot(TargetSlot);
	}

	if (!GridEquipmentSlotUtils::IsSupportedSlot(TargetSlot))
	{
		return false;
	}

	UE_LOG(LogTemp, Verbose, TEXT("GridInventory Equip Compatibility Fallback Item=%s Slot=%s"), *Item.ItemDefinitionId.ToString(),
		GridEquipmentSlotUtils::GetLogName(TargetSlot));

	return true;
}

bool UGridPartyInventoryComponent::EquipItemFromInventorySlot(int32 CharacterIndex, int32 InventorySlotIndex, EGridEquipmentSlot TargetSlot)
{
	EnsureEquipmentCountMatchesActiveCharacters();

	if (!IsValidCharacterIndex(CharacterIndex) || !PartyInventoryState.ActiveEquipment.IsValidIndex(CharacterIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("GridInventory Equip Failed Character=%d Slot=%s Reason=InvalidCharacter"), CharacterIndex,
			GridEquipmentSlotUtils::GetLogName(TargetSlot));
		return false;
	}

	FGridCharacterInventoryState& CharacterState = PartyInventoryState.ActiveCharacters[CharacterIndex];
	if (!CharacterState.InventorySlots.IsValidIndex(InventorySlotIndex) || CharacterState.InventorySlots[InventorySlotIndex].IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("GridInventory Equip Failed Character=%d Slot=%s Reason=InvalidInventorySlot"), CharacterIndex,
			GridEquipmentSlotUtils::GetLogName(TargetSlot));
		return false;
	}

	FGridInventorySlot& InventorySlot = CharacterState.InventorySlots[InventorySlotIndex];
	FGridItemInstance ItemToEquip = InventorySlot.Item;
	if (!CanEquipItemToSlot(CharacterIndex, ItemToEquip, TargetSlot))
	{
		UE_LOG(LogTemp, Warning, TEXT("GridInventory Equip Failed Character=%d Slot=%s Reason=UnsupportedSlot Item=%s"), CharacterIndex,
			GridEquipmentSlotUtils::GetLogName(TargetSlot), *ItemToEquip.ItemDefinitionId.ToString());
		return false;
	}

	FGridCharacterEquipmentState& EquipmentState = PartyInventoryState.ActiveEquipment[CharacterIndex];
	FGridItemInstance* TargetItem = EquipmentState.GetMutableSlot(TargetSlot);
	if (!TargetItem)
	{
		return false;
	}

	FGridItemInstance PreviouslyEquippedItem = *TargetItem;
	ItemToEquip.OwnerType = EGridItemOwnerType::EquipmentSlot;
	ItemToEquip.OwnerGuid = CharacterState.CharacterId;
	ItemToEquip.OwnerCharacterIndex = CharacterIndex;
	ItemToEquip.EquipmentSlot = TargetSlot;

	if (PreviouslyEquippedItem.IsValid())
	{
		PreviouslyEquippedItem.OwnerType = EGridItemOwnerType::CharacterInventory;
		PreviouslyEquippedItem.OwnerGuid = CharacterState.CharacterId;
		PreviouslyEquippedItem.OwnerCharacterIndex = CharacterIndex;
		PreviouslyEquippedItem.EquipmentSlot = EGridEquipmentSlot::None;
		InventorySlot.Item = PreviouslyEquippedItem;
		InventorySlot.bOccupied = true;
	}
	else
	{
		InventorySlot = FGridInventorySlot();
	}

	*TargetItem = ItemToEquip;
	NotifyPartyInventoryChanged(CharacterIndex);

	UE_LOG(LogTemp, Log, TEXT("GridInventory Equip Character=%d Slot=%s Item=%s RuntimeId=%s Result=true"), CharacterIndex,
		GridEquipmentSlotUtils::GetLogName(TargetSlot), *ItemToEquip.ItemDefinitionId.ToString(), *ItemToEquip.RuntimeObjectId.ToString());
	return true;
}

bool UGridPartyInventoryComponent::UnequipItemToInventory(int32 CharacterIndex, EGridEquipmentSlot SourceSlot)
{
	EnsureEquipmentCountMatchesActiveCharacters();

	if (!IsValidCharacterIndex(CharacterIndex) || !PartyInventoryState.ActiveEquipment.IsValidIndex(CharacterIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("GridInventory Unequip Failed Character=%d Slot=%s Reason=InvalidCharacter"), CharacterIndex,
			GridEquipmentSlotUtils::GetLogName(SourceSlot));
		return false;
	}

	if (!GridEquipmentSlotUtils::IsSupportedSlot(SourceSlot))
	{
		UE_LOG(LogTemp, Warning, TEXT("GridInventory Unequip Failed Character=%d Slot=%s Reason=UnsupportedSlot"), CharacterIndex,
			GridEquipmentSlotUtils::GetLogName(SourceSlot));
		return false;
	}

	FGridCharacterEquipmentState& EquipmentState = PartyInventoryState.ActiveEquipment[CharacterIndex];
	FGridItemInstance* EquippedItem = EquipmentState.GetMutableSlot(SourceSlot);
	if (!EquippedItem || !EquippedItem->IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("GridInventory Unequip Failed Character=%d Slot=%s Reason=EmptySlot"), CharacterIndex,
			GridEquipmentSlotUtils::GetLogName(SourceSlot));
		return false;
	}

	FGridCharacterInventoryState& CharacterState = PartyInventoryState.ActiveCharacters[CharacterIndex];
	const int32 FreeSlotIndex = GridPartyInventoryEquipmentFindFreeInventorySlotIndex(CharacterState);
	if (FreeSlotIndex == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridInventory Unequip Failed Character=%d Slot=%s Reason=InventoryFull"), CharacterIndex,
			GridEquipmentSlotUtils::GetLogName(SourceSlot));
		return false;
	}

	FGridItemInstance ItemToInventory = *EquippedItem;
	ItemToInventory.OwnerType = EGridItemOwnerType::CharacterInventory;
	ItemToInventory.OwnerGuid = CharacterState.CharacterId;
	ItemToInventory.OwnerCharacterIndex = CharacterIndex;
	ItemToInventory.EquipmentSlot = EGridEquipmentSlot::None;

	CharacterState.InventorySlots[FreeSlotIndex].bOccupied = true;
	CharacterState.InventorySlots[FreeSlotIndex].Item = ItemToInventory;
	*EquippedItem = FGridItemInstance();
	NotifyPartyInventoryChanged(CharacterIndex);

	UE_LOG(LogTemp, Log, TEXT("GridInventory Unequip Character=%d Slot=%s Item=%s RuntimeId=%s Result=true"), CharacterIndex,
		GridEquipmentSlotUtils::GetLogName(SourceSlot), *ItemToInventory.ItemDefinitionId.ToString(), *ItemToInventory.RuntimeObjectId.ToString());
	return true;
}

bool UGridPartyInventoryComponent::GetEquippedItem(int32 CharacterIndex, EGridEquipmentSlot Slot, FGridItemInstance& OutItem) const
{
	OutItem = FGridItemInstance();
	if (!IsValidCharacterIndex(CharacterIndex) || !PartyInventoryState.ActiveEquipment.IsValidIndex(CharacterIndex))
	{
		return false;
	}

	const FGridItemInstance* Item = PartyInventoryState.ActiveEquipment[CharacterIndex].GetSlot(Slot);
	if (!Item || !Item->IsValid())
	{
		return false;
	}

	OutItem = *Item;
	return true;
}

bool UGridPartyInventoryComponent::IsEquipmentSlotOccupied(int32 CharacterIndex, EGridEquipmentSlot Slot) const
{
	if (!IsValidCharacterIndex(CharacterIndex) || !PartyInventoryState.ActiveEquipment.IsValidIndex(CharacterIndex))
	{
		return false;
	}

	return PartyInventoryState.ActiveEquipment[CharacterIndex].IsSlotOccupied(Slot);
}

bool UGridPartyInventoryComponent::TryConsumeEquippedItemQuantityForCombatAction(
	int32 CharacterIndex, EGridEquipmentSlot SourceSlot, FName ExpectedItemDefinitionId, const FGuid& ExpectedRuntimeObjectId, int32 Quantity)
{
	EnsureEquipmentCountMatchesActiveCharacters();
	if (!IsValidCharacterIndex(CharacterIndex) || !PartyInventoryState.ActiveEquipment.IsValidIndex(CharacterIndex) ||
		!GridEquipmentSlotUtils::IsHandSlot(SourceSlot) || ExpectedItemDefinitionId.IsNone() || Quantity <= 0)
	{
		return false;
	}

	FGridItemInstance* EquippedItem = PartyInventoryState.ActiveEquipment[CharacterIndex].GetMutableSlot(SourceSlot);
	if (!EquippedItem || !EquippedItem->IsValid() || EquippedItem->ItemDefinitionId != ExpectedItemDefinitionId ||
		(ExpectedRuntimeObjectId.IsValid() && EquippedItem->RuntimeObjectId != ExpectedRuntimeObjectId) || FMath::Max(1, EquippedItem->Quantity) < Quantity)
	{
		return false;
	}

	const int32 QuantityBefore = FMath::Max(1, EquippedItem->Quantity);
	if (QuantityBefore == Quantity)
	{
		*EquippedItem = FGridItemInstance();
	}
	else
	{
		EquippedItem->Quantity = QuantityBefore - Quantity;
	}
	NotifyPartyInventoryChanged(CharacterIndex);
	return true;
}

FGridEquipmentStatBonus UGridPartyInventoryComponent::ComputeCharacterEquipmentStatBonus(int32 CharacterIndex) const
{
	FGridEquipmentStatBonus TotalBonus;
	if (!IsValidCharacterIndex(CharacterIndex) || !PartyInventoryState.ActiveEquipment.IsValidIndex(CharacterIndex))
	{
		return TotalBonus;
	}

	GridEquipmentSlotUtils::ForEachEquipmentItem(PartyInventoryState.ActiveEquipment[CharacterIndex],
		[this, &TotalBonus](EGridEquipmentSlot, const FGridItemInstance& Item)
		{
			if (!Item.IsValid())
			{
				return;
			}

			const UGridItemDefinitionAsset* Definition = FindItemDefinition(Item.ItemDefinitionId);
			if (!Definition)
			{
				return;
			}

			GridPartyInventoryEquipmentAddStatBonus(TotalBonus, Definition->EquipmentStatBonus);
		});

	return TotalBonus;
}

FGridDamageResistanceSet UGridPartyInventoryComponent::ComputeCharacterEquipmentResistances(int32 CharacterIndex) const
{
	FGridDamageResistanceSet TotalResistances;
	if (!IsValidCharacterIndex(CharacterIndex) || !PartyInventoryState.ActiveEquipment.IsValidIndex(CharacterIndex))
	{
		return TotalResistances;
	}

	GridEquipmentSlotUtils::ForEachEquipmentItem(PartyInventoryState.ActiveEquipment[CharacterIndex],
		[this, &TotalResistances](EGridEquipmentSlot, const FGridItemInstance& Item)
		{
			if (!Item.IsValid())
			{
				return;
			}

			const UGridItemDefinitionAsset* Definition = FindItemDefinition(Item.ItemDefinitionId);
			if (!Definition)
			{
				return;
			}

			TotalResistances.Add(Definition->EquipmentResistanceBonus);
		});

	return TotalResistances;
}
