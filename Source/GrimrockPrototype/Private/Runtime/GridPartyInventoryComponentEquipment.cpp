#include "Runtime/GridPartyInventoryComponent.h"

#include "GridEquipmentSlotUtils.h"
#include "Runtime/GridItemDefinitionAsset.h"

DEFINE_LOG_CATEGORY_STATIC(LogGridInventory, Log, All);

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

bool UGridPartyInventoryComponent::IsTwoHandedEquipped(int32 CharacterIndex) const
{
	if (!IsValidCharacterIndex(CharacterIndex) || !PartyInventoryState.ActiveEquipment.IsValidIndex(CharacterIndex))
	{
		return false;
	}
	const FGridItemInstance& Main = PartyInventoryState.ActiveEquipment[CharacterIndex].MainHand;
	const UGridItemDefinitionAsset* Definition = Main.IsValid() ? FindItemDefinition(Main.ItemDefinitionId) : nullptr;
	return Definition && Definition->GetEffectiveHandUsage() == EGridItemHandUsage::TwoHanded;
}

EGridEquipmentSlot UGridPartyInventoryComponent::ResolveEquipmentSourceSlot(int32 CharacterIndex, EGridEquipmentSlot RequestedSlot) const
{
	return RequestedSlot == EGridEquipmentSlot::OffHand && IsTwoHandedEquipped(CharacterIndex)
		? EGridEquipmentSlot::MainHand : RequestedSlot;
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

	if (!GridEquipmentSlotUtils::IsSupportedSlot(TargetSlot))
	{
		return false;
	}

	const UGridItemDefinitionAsset* Definition = FindItemDefinition(Item.ItemDefinitionId);
	if (!Definition || !Definition->CanEquipToSlot(TargetSlot))
	{
		return false;
	}
	if (Definition->bUseWeaponCatalogue)
	{
		const FGridWeaponLevelProfile* Profile = nullptr;
		return Definition->TryGetWeaponCatalogueProfile(Item.ItemLevel, Profile);
	}
	return true;
}

bool UGridPartyInventoryComponent::EquipItemFromInventorySlot(int32 CharacterIndex, int32 InventorySlotIndex, EGridEquipmentSlot TargetSlot)
{
	EnsureEquipmentCountMatchesActiveCharacters();
	if (!IsValidCharacterIndex(CharacterIndex) || !PartyInventoryState.ActiveEquipment.IsValidIndex(CharacterIndex))
	{
		UE_LOG(LogGridInventory, Warning, TEXT("GridInventory Equip Failed Character=%d Slot=%s Reason=InvalidCharacter"),
			CharacterIndex, GridEquipmentSlotUtils::GetLogName(TargetSlot));
		return false;
	}

	FGridCharacterInventoryState& Character = PartyInventoryState.ActiveCharacters[CharacterIndex];
	if (!Character.InventorySlots.IsValidIndex(InventorySlotIndex) || Character.InventorySlots[InventorySlotIndex].IsEmpty())
	{
		UE_LOG(LogGridInventory, Warning, TEXT("GridInventory Equip Failed Character=%d Slot=%s Reason=InvalidInventorySlot"),
			CharacterIndex, GridEquipmentSlotUtils::GetLogName(TargetSlot));
		return false;
	}

	const FGridItemInstance Source = Character.InventorySlots[InventorySlotIndex].Item;
	const UGridItemDefinitionAsset* Definition = FindItemDefinition(Source.ItemDefinitionId);
	if (!Definition)
	{
		UE_LOG(LogGridInventory, Warning, TEXT("GridInventory Equip Failed Character=%d Slot=%s Reason=MissingDefinition Item=%s"),
			CharacterIndex, GridEquipmentSlotUtils::GetLogName(TargetSlot), *Source.ItemDefinitionId.ToString());
		return false;
	}
	if (!CanEquipItemToSlot(CharacterIndex, Source, TargetSlot))
	{
		UE_LOG(LogGridInventory, Warning, TEXT("GridInventory Equip Failed Character=%d Slot=%s Reason=UnsupportedSlot Item=%s"),
			CharacterIndex, GridEquipmentSlotUtils::GetLogName(TargetSlot), *Source.ItemDefinitionId.ToString());
		return false;
	}

	FGridCharacterEquipmentState& Equipment = PartyInventoryState.ActiveEquipment[CharacterIndex];
	const bool bIncomingTwoHanded = GridEquipmentSlotUtils::IsHandSlot(TargetSlot) &&
		Definition->GetEffectiveHandUsage() == EGridItemHandUsage::TwoHanded;
	const bool bReleaseBoth = bIncomingTwoHanded || (GridEquipmentSlotUtils::IsHandSlot(TargetSlot) && IsTwoHandedEquipped(CharacterIndex));
	const EGridEquipmentSlot ActualTarget = bIncomingTwoHanded ? EGridEquipmentSlot::MainHand : TargetSlot;
	FGridItemInstance* TargetItem = Equipment.GetMutableSlot(ActualTarget);
	if (!TargetItem)
	{
		return false;
	}

	// Verify all displaced items fit before changing ownership or clearing a slot.
	TArray<FGridItemInstance> Displaced;
	if (bReleaseBoth)
	{
		if (Equipment.MainHand.IsValid()) Displaced.Add(Equipment.MainHand);
		if (Equipment.OffHand.IsValid()) Displaced.Add(Equipment.OffHand);
	}
	else if (TargetItem->IsValid())
	{
		Displaced.Add(*TargetItem);
	}
	TArray<int32> Destinations;
	Destinations.Add(InventorySlotIndex); // The source inventory slot becomes available at commit.
	for (int32 Index = 0; Index < Character.InventorySlots.Num() && Destinations.Num() < Displaced.Num(); ++Index)
	{
		if (Index != InventorySlotIndex && Character.InventorySlots[Index].IsEmpty()) Destinations.Add(Index);
	}
	if (Destinations.Num() < Displaced.Num())
	{
		UE_LOG(LogGridInventory, Warning, TEXT("GridInventory Equip Failed Character=%d Slot=%s Reason=InventoryFullForDisplacedItems Count=%d"),
			CharacterIndex, GridEquipmentSlotUtils::GetLogName(TargetSlot), Displaced.Num());
		return false;
	}

	Character.InventorySlots[InventorySlotIndex] = FGridInventorySlot();
	if (bReleaseBoth)
	{
		Equipment.MainHand = FGridItemInstance();
		Equipment.OffHand = FGridItemInstance();
	}
	for (int32 Index = 0; Index < Displaced.Num(); ++Index)
	{
		FGridItemInstance Item = Displaced[Index];
		Item.OwnerType = EGridItemOwnerType::CharacterInventory;
		Item.OwnerGuid = Character.CharacterId;
		Item.OwnerCharacterIndex = CharacterIndex;
		Item.EquipmentSlot = EGridEquipmentSlot::None;
		FGridInventorySlot& Slot = Character.InventorySlots[Destinations[Index]];
		Slot.bOccupied = true;
		Slot.Item = Item;
	}
	FGridItemInstance Equipped = Source;
	Equipped.OwnerType = EGridItemOwnerType::EquipmentSlot;
	Equipped.OwnerGuid = Character.CharacterId;
	Equipped.OwnerCharacterIndex = CharacterIndex;
	Equipped.EquipmentSlot = ActualTarget;
	*TargetItem = Equipped;
	NotifyPartyInventoryChanged(CharacterIndex);
	return true;
}

bool UGridPartyInventoryComponent::UnequipItemToInventory(int32 CharacterIndex, EGridEquipmentSlot SourceSlot)
{
	EnsureEquipmentCountMatchesActiveCharacters();

	if (!IsValidCharacterIndex(CharacterIndex) || !PartyInventoryState.ActiveEquipment.IsValidIndex(CharacterIndex))
	{
		UE_LOG(LogGridInventory, Warning, TEXT("GridInventory Unequip Failed Character=%d Slot=%s Reason=InvalidCharacter"), CharacterIndex,
			GridEquipmentSlotUtils::GetLogName(SourceSlot));
		return false;
	}

	if (!GridEquipmentSlotUtils::IsSupportedSlot(SourceSlot))
	{
		UE_LOG(LogGridInventory, Warning, TEXT("GridInventory Unequip Failed Character=%d Slot=%s Reason=UnsupportedSlot"), CharacterIndex,
			GridEquipmentSlotUtils::GetLogName(SourceSlot));
		return false;
	}

	FGridCharacterEquipmentState& EquipmentState = PartyInventoryState.ActiveEquipment[CharacterIndex];
	FGridItemInstance* EquippedItem = EquipmentState.GetMutableSlot(ResolveEquipmentSourceSlot(CharacterIndex, SourceSlot));
	if (!EquippedItem || !EquippedItem->IsValid())
	{
		UE_LOG(LogGridInventory, Warning, TEXT("GridInventory Unequip Failed Character=%d Slot=%s Reason=EmptySlot"), CharacterIndex,
			GridEquipmentSlotUtils::GetLogName(SourceSlot));
		return false;
	}

	FGridCharacterInventoryState& CharacterState = PartyInventoryState.ActiveCharacters[CharacterIndex];
	const int32 FreeSlotIndex = GridPartyInventoryEquipmentFindFreeInventorySlotIndex(CharacterState);
	if (FreeSlotIndex == INDEX_NONE)
	{
		UE_LOG(LogGridInventory, Warning, TEXT("GridInventory Unequip Failed Character=%d Slot=%s Reason=InventoryFull"), CharacterIndex,
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

	UE_LOG(LogGridInventory, Verbose, TEXT("GridInventory Unequip Character=%d Slot=%s Item=%s RuntimeId=%s Result=true"), CharacterIndex,
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

	const FGridItemInstance* Item = PartyInventoryState.ActiveEquipment[CharacterIndex].GetSlot(ResolveEquipmentSourceSlot(CharacterIndex, Slot));
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

	return PartyInventoryState.ActiveEquipment[CharacterIndex].IsSlotOccupied(ResolveEquipmentSourceSlot(CharacterIndex, Slot));
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
