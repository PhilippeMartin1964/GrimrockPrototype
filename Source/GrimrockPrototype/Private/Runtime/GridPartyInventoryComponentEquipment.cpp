#include "Runtime/GridPartyInventoryComponent.h"

#include "Runtime/GridItemDefinitionAsset.h"

namespace
{
		bool GridPartyInventoryEquipmentIsSupportedSlot(EGridEquipmentSlot Slot)
		{
			switch (Slot)
			{
				case EGridEquipmentSlot::MainHand:
				case EGridEquipmentSlot::OffHand:
				case EGridEquipmentSlot::Head:
				case EGridEquipmentSlot::Chest:
				case EGridEquipmentSlot::Legs:
				case EGridEquipmentSlot::Feet:
				case EGridEquipmentSlot::Amulet:
				case EGridEquipmentSlot::Ring1:
				case EGridEquipmentSlot::Ring2:
				case EGridEquipmentSlot::Shoulders:
				case EGridEquipmentSlot::Gloves:
				case EGridEquipmentSlot::Belt:
				case EGridEquipmentSlot::Cloak:
				case EGridEquipmentSlot::Talisman:
				case EGridEquipmentSlot::QuickSlot1:
				case EGridEquipmentSlot::QuickSlot2:
				case EGridEquipmentSlot::Face:
				case EGridEquipmentSlot::Shirt:
				case EGridEquipmentSlot::Bracers:
				case EGridEquipmentSlot::Earring1:
				case EGridEquipmentSlot::Earring2:
					return true;
				case EGridEquipmentSlot::None:
				default:
					return false;
			}
		}

		bool GridPartyInventoryEquipmentIsHandSlot(EGridEquipmentSlot Slot)
		{
			return Slot == EGridEquipmentSlot::MainHand || Slot == EGridEquipmentSlot::OffHand;
		}

		const TCHAR* GridPartyInventoryEquipmentGetSlotName(EGridEquipmentSlot Slot)
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

		void GridPartyInventoryEquipmentForEachItem(const FGridCharacterEquipmentState& EquipmentState, TFunctionRef<void(EGridEquipmentSlot, const FGridItemInstance&)> Visitor)
		{
			const EGridEquipmentSlot Slots[] = { EGridEquipmentSlot::MainHand, EGridEquipmentSlot::OffHand, EGridEquipmentSlot::Head, EGridEquipmentSlot::Chest,
				EGridEquipmentSlot::Legs, EGridEquipmentSlot::Feet, EGridEquipmentSlot::Amulet, EGridEquipmentSlot::Ring1, EGridEquipmentSlot::Ring2,
				EGridEquipmentSlot::Shoulders, EGridEquipmentSlot::Gloves, EGridEquipmentSlot::Belt, EGridEquipmentSlot::Cloak, EGridEquipmentSlot::Talisman,
				EGridEquipmentSlot::QuickSlot1, EGridEquipmentSlot::QuickSlot2, EGridEquipmentSlot::Face, EGridEquipmentSlot::Shirt, EGridEquipmentSlot::Bracers,
				EGridEquipmentSlot::Earring1, EGridEquipmentSlot::Earring2 };

			for (const EGridEquipmentSlot Slot : Slots)
			{
				if (const FGridItemInstance* Item = EquipmentState.GetSlot(Slot))
				{
					Visitor(Slot, *Item);
				}
			}
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

	if (!GridPartyInventoryEquipmentIsSupportedSlot(TargetSlot))
	{
		return false;
	}

	UE_LOG(LogTemp, Verbose, TEXT("GridInventory Equip Compatibility Fallback Item=%s Slot=%s"), *Item.ItemDefinitionId.ToString(),
		GridPartyInventoryEquipmentGetSlotName(TargetSlot));

	return true;
}

bool UGridPartyInventoryComponent::EquipItemFromInventorySlot(int32 CharacterIndex, int32 InventorySlotIndex, EGridEquipmentSlot TargetSlot)
{
	EnsureEquipmentCountMatchesActiveCharacters();

	if (!IsValidCharacterIndex(CharacterIndex) || !PartyInventoryState.ActiveEquipment.IsValidIndex(CharacterIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("GridInventory Equip Failed Character=%d Slot=%s Reason=InvalidCharacter"), CharacterIndex,
			GridPartyInventoryEquipmentGetSlotName(TargetSlot));
		return false;
	}

	FGridCharacterInventoryState& CharacterState = PartyInventoryState.ActiveCharacters[CharacterIndex];
	if (!CharacterState.InventorySlots.IsValidIndex(InventorySlotIndex) || CharacterState.InventorySlots[InventorySlotIndex].IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("GridInventory Equip Failed Character=%d Slot=%s Reason=InvalidInventorySlot"), CharacterIndex,
			GridPartyInventoryEquipmentGetSlotName(TargetSlot));
		return false;
	}

	FGridInventorySlot& InventorySlot = CharacterState.InventorySlots[InventorySlotIndex];
	FGridItemInstance ItemToEquip = InventorySlot.Item;
	if (!CanEquipItemToSlot(CharacterIndex, ItemToEquip, TargetSlot))
	{
		UE_LOG(LogTemp, Warning, TEXT("GridInventory Equip Failed Character=%d Slot=%s Reason=UnsupportedSlot Item=%s"), CharacterIndex,
			GridPartyInventoryEquipmentGetSlotName(TargetSlot), *ItemToEquip.ItemDefinitionId.ToString());
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

	UE_LOG(LogTemp, Log, TEXT("GridInventory Equip Character=%d Slot=%s Item=%s RuntimeId=%s Result=true"), CharacterIndex, GridPartyInventoryEquipmentGetSlotName(TargetSlot),
		*ItemToEquip.ItemDefinitionId.ToString(), *ItemToEquip.RuntimeObjectId.ToString());
	return true;
}

bool UGridPartyInventoryComponent::UnequipItemToInventory(int32 CharacterIndex, EGridEquipmentSlot SourceSlot)
{
	EnsureEquipmentCountMatchesActiveCharacters();

	if (!IsValidCharacterIndex(CharacterIndex) || !PartyInventoryState.ActiveEquipment.IsValidIndex(CharacterIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("GridInventory Unequip Failed Character=%d Slot=%s Reason=InvalidCharacter"), CharacterIndex,
			GridPartyInventoryEquipmentGetSlotName(SourceSlot));
		return false;
	}

	if (!GridPartyInventoryEquipmentIsSupportedSlot(SourceSlot))
	{
		UE_LOG(LogTemp, Warning, TEXT("GridInventory Unequip Failed Character=%d Slot=%s Reason=UnsupportedSlot"), CharacterIndex,
			GridPartyInventoryEquipmentGetSlotName(SourceSlot));
		return false;
	}

	FGridCharacterEquipmentState& EquipmentState = PartyInventoryState.ActiveEquipment[CharacterIndex];
	FGridItemInstance* EquippedItem = EquipmentState.GetMutableSlot(SourceSlot);
	if (!EquippedItem || !EquippedItem->IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("GridInventory Unequip Failed Character=%d Slot=%s Reason=EmptySlot"), CharacterIndex, GridPartyInventoryEquipmentGetSlotName(SourceSlot));
		return false;
	}

	FGridCharacterInventoryState& CharacterState = PartyInventoryState.ActiveCharacters[CharacterIndex];
	const int32 FreeSlotIndex = GridPartyInventoryEquipmentFindFreeInventorySlotIndex(CharacterState);
	if (FreeSlotIndex == INDEX_NONE)
	{
		UE_LOG(
			LogTemp, Warning, TEXT("GridInventory Unequip Failed Character=%d Slot=%s Reason=InventoryFull"), CharacterIndex, GridPartyInventoryEquipmentGetSlotName(SourceSlot));
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

	UE_LOG(LogTemp, Log, TEXT("GridInventory Unequip Character=%d Slot=%s Item=%s RuntimeId=%s Result=true"), CharacterIndex, GridPartyInventoryEquipmentGetSlotName(SourceSlot),
		*ItemToInventory.ItemDefinitionId.ToString(), *ItemToInventory.RuntimeObjectId.ToString());
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
	if (!IsValidCharacterIndex(CharacterIndex) || !PartyInventoryState.ActiveEquipment.IsValidIndex(CharacterIndex) || !GridPartyInventoryEquipmentIsHandSlot(SourceSlot) ||
		ExpectedItemDefinitionId.IsNone() || Quantity <= 0)
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

	GridPartyInventoryEquipmentForEachItem(PartyInventoryState.ActiveEquipment[CharacterIndex],
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

	GridPartyInventoryEquipmentForEachItem(PartyInventoryState.ActiveEquipment[CharacterIndex],
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
