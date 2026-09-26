#include "Runtime/GridPartyInventoryComponent.h"

#include "Runtime/GridItemDefinitionAsset.h"

namespace
{
	bool GridPartyInventoryHotbarCharacterHasInventoryItemDefinition(const FGridCharacterInventoryState& CharacterState, FName ItemDefinitionId)
	{
		return !ItemDefinitionId.IsNone() &&
			CharacterState.InventorySlots.ContainsByPredicate(
				[ItemDefinitionId](const FGridInventorySlot& Slot)
				{
					return !Slot.IsEmpty() && Slot.Item.ItemDefinitionId == ItemDefinitionId && Slot.Item.Quantity > 0;
				});
	}
}

int32 UGridPartyInventoryComponent::GetCombatHotbarSlotCount() const
{
	return IsValidCharacterIndex(PartyInventoryState.SelectedCharacterIndex)
		? PartyInventoryState.ActiveCharacters[PartyInventoryState.SelectedCharacterIndex].CombatHotbarSlots.Num()
		: FGridCombatHotbarBinding::MinimumSlotCount;
}

int32 UGridPartyInventoryComponent::GetCharacterCombatHotbarSlotCount(int32 CharacterIndex) const
{
	return IsValidCharacterIndex(CharacterIndex) ? PartyInventoryState.ActiveCharacters[CharacterIndex].CombatHotbarSlots.Num() : 0;
}

int32 UGridPartyInventoryComponent::GetCharacterCombatHotbarUsedSlotCount(int32 CharacterIndex) const
{
	if (!IsValidCharacterIndex(CharacterIndex))
	{
		return FGridCombatHotbarBinding::MinimumSlotCount;
	}

	const TArray<FGridCombatHotbarBinding>& Slots = PartyInventoryState.ActiveCharacters[CharacterIndex].CombatHotbarSlots;
	for (int32 SlotIndex = Slots.Num() - 1; SlotIndex >= FGridCombatHotbarBinding::MinimumSlotCount; --SlotIndex)
	{
		if (!Slots[SlotIndex].IsEmpty())
		{
			return SlotIndex + 1;
		}
	}
	return FGridCombatHotbarBinding::MinimumSlotCount;
}

bool UGridPartyInventoryComponent::EnsureCharacterCombatHotbarCapacity(int32 CharacterIndex, int32 RequiredSlotCount)
{
	if (!IsValidCharacterIndex(CharacterIndex))
	{
		return false;
	}

	FGridCharacterInventoryState& Character = PartyInventoryState.ActiveCharacters[CharacterIndex];
	const int32 TargetSlotCount = FMath::Max(FGridCombatHotbarBinding::MinimumSlotCount, RequiredSlotCount);
	if (Character.CombatHotbarSlots.Num() >= TargetSlotCount)
	{
		return true;
	}

	const int32 PreviousCount = Character.CombatHotbarSlots.Num();
	Character.CombatHotbarSlots.SetNum(TargetSlotCount);
	for (int32 SlotIndex = PreviousCount; SlotIndex < TargetSlotCount; ++SlotIndex)
	{
		Character.CombatHotbarSlots[SlotIndex].Reset(SlotIndex);
	}
	NotifyPartyInventoryChanged(CharacterIndex);
	return true;
}

bool UGridPartyInventoryComponent::GetCharacterCombatHotbarBinding(int32 CharacterIndex, int32 SlotIndex, FGridCombatHotbarBinding& OutBinding) const
{
	OutBinding = FGridCombatHotbarBinding();
	if (!IsValidCharacterIndex(CharacterIndex) || SlotIndex < 0)
	{
		return false;
	}

	const FGridCharacterInventoryState& Character = PartyInventoryState.ActiveCharacters[CharacterIndex];
	if (!Character.CombatHotbarSlots.IsValidIndex(SlotIndex))
	{
		return false;
	}

	OutBinding = Character.CombatHotbarSlots[SlotIndex];
	return true;
}

bool UGridPartyInventoryComponent::SetCharacterCombatHotbarBinding(int32 CharacterIndex, int32 SlotIndex, const FGridCombatHotbarBinding& Binding)
{
	if (!IsValidCharacterIndex(CharacterIndex) || SlotIndex < 0)
	{
		return false;
	}
	FGridCharacterInventoryState& Character = PartyInventoryState.ActiveCharacters[CharacterIndex];
	if (!Character.CombatHotbarSlots.IsValidIndex(SlotIndex))
	{
		return false;
	}
	if ((SlotIndex == FGridCombatHotbarBinding::PrimaryAttackSlotIndex && !Binding.IsPrimaryAttackBinding()) ||
		(SlotIndex != FGridCombatHotbarBinding::PrimaryAttackSlotIndex && Binding.IsPrimaryAttackBinding()))
	{
		return false;
	}

	FGridCombatHotbarBinding NormalizedBinding = Binding;
	if (NormalizedBinding.IsEmpty())
	{
		NormalizedBinding.Reset(SlotIndex);
	}
	else
	{
		NormalizedBinding.SlotIndex = SlotIndex;
	}

	if (!NormalizedBinding.IsValid())
	{
		return false;
	}

	if (NormalizedBinding.SourcePolicy == EGridCombatActionSourcePolicy::QuickItem &&
		!GridPartyInventoryHotbarCharacterHasInventoryItemDefinition(Character, NormalizedBinding.SourceDefinitionId))
	{
		return false;
	}

	if (NormalizedBinding.SourcePolicy == EGridCombatActionSourcePolicy::Equipment ||
		NormalizedBinding.SourcePolicy == EGridCombatActionSourcePolicy::QuickItem)
	{
		for (int32 ExistingSlotIndex = 0; ExistingSlotIndex < Character.CombatHotbarSlots.Num(); ++ExistingSlotIndex)
		{
			if (ExistingSlotIndex == SlotIndex)
			{
				continue;
			}

			FGridCombatHotbarBinding& ExistingBinding = Character.CombatHotbarSlots[ExistingSlotIndex];
			const bool bSameEquipmentItem = NormalizedBinding.SourcePolicy == EGridCombatActionSourcePolicy::Equipment &&
				ExistingBinding.SourcePolicy == EGridCombatActionSourcePolicy::Equipment &&
				ExistingBinding.PreferredSourceRuntimeId == NormalizedBinding.PreferredSourceRuntimeId;
			const bool bSameQuickItemDefinition = NormalizedBinding.SourcePolicy == EGridCombatActionSourcePolicy::QuickItem &&
				ExistingBinding.SourcePolicy == EGridCombatActionSourcePolicy::QuickItem &&
				ExistingBinding.SourceDefinitionId == NormalizedBinding.SourceDefinitionId;
			if (bSameEquipmentItem || bSameQuickItemDefinition)
			{
				ExistingBinding.Reset(ExistingSlotIndex);
			}
		}
	}

	Character.CombatHotbarSlots[SlotIndex] = MoveTemp(NormalizedBinding);
	NotifyPartyInventoryChanged(CharacterIndex);
	return true;
}

bool UGridPartyInventoryComponent::ClearCharacterCombatHotbarBinding(int32 CharacterIndex, int32 SlotIndex)
{
	if (SlotIndex == FGridCombatHotbarBinding::PrimaryAttackSlotIndex)
	{
		return false;
	}
	FGridCombatHotbarBinding EmptyBinding;
	EmptyBinding.Reset(SlotIndex);
	return SetCharacterCombatHotbarBinding(CharacterIndex, SlotIndex, EmptyBinding);
}

bool UGridPartyInventoryComponent::SetCharacterCombatHotbarBindingFromItem(
	int32 CharacterIndex, int32 SlotIndex, const FGridItemInstance& SourceItem, EGridEquipmentSlot SourceEquipmentSlot)
{
	if (!IsValidCharacterIndex(CharacterIndex) || SlotIndex < 0 || SlotIndex == FGridCombatHotbarBinding::PrimaryAttackSlotIndex || !SourceItem.IsValid() ||
		!PartyInventoryState.ActiveCharacters[CharacterIndex].CombatHotbarSlots.IsValidIndex(SlotIndex))
	{
		return false;
	}

	const UGridItemDefinitionAsset* Definition = FindItemDefinition(SourceItem.ItemDefinitionId);
	if (!IsValid(Definition) || !Definition->IsValidDefinition())
	{
		return false;
	}

	FGridCombatHotbarBinding Binding;
	if (SourceEquipmentSlot != EGridEquipmentSlot::None)
	{
		if (SourceEquipmentSlot != EGridEquipmentSlot::MainHand && SourceEquipmentSlot != EGridEquipmentSlot::OffHand)
		{
			return false;
		}
		if (!Definition->CompatibleEquipmentSlots.Contains(SourceEquipmentSlot))
		{
			return false;
		}

		FGridItemInstance EquippedItem;
		if (!GetEquippedItem(CharacterIndex, SourceEquipmentSlot, EquippedItem) || EquippedItem.RuntimeObjectId != SourceItem.RuntimeObjectId ||
			EquippedItem.ItemDefinitionId != SourceItem.ItemDefinitionId)
		{
			return false;
		}

		FName PrimaryActionId = NAME_None;
		const FGridCombatActionDefinition* PrimaryAction = Definition->CombatActions.FindByPredicate(
			[](const FGridCombatActionDefinition& Candidate)
			{
				return Candidate.IsValid() && Candidate.SourcePolicy == EGridCombatActionSourcePolicy::Equipment;
			});
		if (PrimaryAction)
		{
			PrimaryActionId = PrimaryAction->ActionId;
		}

		if (PrimaryActionId.IsNone())
		{
			return false;
		}

		Binding.ActionId = PrimaryActionId;
		Binding.SourcePolicy = EGridCombatActionSourcePolicy::Equipment;
		Binding.SourceDefinitionId = SourceItem.ItemDefinitionId;
		Binding.PreferredSourceRuntimeId = SourceItem.RuntimeObjectId;
		Binding.PreferredEquipmentSlot = SourceEquipmentSlot;
	}
	else
	{
		const FGridCharacterInventoryState& Character = PartyInventoryState.ActiveCharacters[CharacterIndex];
		const bool bItemStillOwned = Character.InventorySlots.ContainsByPredicate(
			[&SourceItem](const FGridInventorySlot& Candidate)
			{
				return !Candidate.IsEmpty() && Candidate.Item.RuntimeObjectId == SourceItem.RuntimeObjectId &&
					Candidate.Item.ItemDefinitionId == SourceItem.ItemDefinitionId;
			});
		if (!bItemStillOwned)
		{
			return false;
		}

		FGridCombatActionDefinition InventoryAction;
		if (Definition->BuildInventoryCombatActionDefinition(InventoryAction))
		{
			Binding.ActionId = InventoryAction.ActionId;
			Binding.SourcePolicy = EGridCombatActionSourcePolicy::QuickItem;
			Binding.SourceDefinitionId = SourceItem.ItemDefinitionId;
		}
		else if (Definition->IsPhysicallyThrowable())
		{
			Binding.ActionId = FGridCombatHotbarBinding::MakePhysicalThrowItemActionId(SourceItem.ItemDefinitionId);
			Binding.SourcePolicy = EGridCombatActionSourcePolicy::QuickItem;
			Binding.SourceDefinitionId = SourceItem.ItemDefinitionId;
		}
		else
		{
			return false;
		}
	}

	return SetCharacterCombatHotbarBinding(CharacterIndex, SlotIndex, Binding);
}

bool UGridPartyInventoryComponent::MoveOrSwapCharacterCombatHotbarBinding(int32 CharacterIndex, int32 SourceSlotIndex, int32 TargetSlotIndex)
{
	if (!IsValidCharacterIndex(CharacterIndex) || SourceSlotIndex < 0 || TargetSlotIndex < 0 ||
		SourceSlotIndex == FGridCombatHotbarBinding::PrimaryAttackSlotIndex ||
		TargetSlotIndex == FGridCombatHotbarBinding::PrimaryAttackSlotIndex)
	{
		return false;
	}

	FGridCharacterInventoryState& Character = PartyInventoryState.ActiveCharacters[CharacterIndex];
	if (!Character.CombatHotbarSlots.IsValidIndex(SourceSlotIndex) || !Character.CombatHotbarSlots.IsValidIndex(TargetSlotIndex) ||
		Character.CombatHotbarSlots[SourceSlotIndex].IsEmpty())
	{
		return false;
	}
	if (SourceSlotIndex == TargetSlotIndex)
	{
		return true;
	}

	FGridCombatHotbarBinding SourceBinding = Character.CombatHotbarSlots[SourceSlotIndex];
	FGridCombatHotbarBinding TargetBinding = Character.CombatHotbarSlots[TargetSlotIndex];
	SourceBinding.SlotIndex = TargetSlotIndex;
	if (TargetBinding.IsEmpty())
	{
		TargetBinding.Reset(SourceSlotIndex);
	}
	else
	{
		TargetBinding.SlotIndex = SourceSlotIndex;
	}

	if (!SourceBinding.IsValid() || !TargetBinding.IsValid())
	{
		return false;
	}

	Character.CombatHotbarSlots[SourceSlotIndex] = MoveTemp(TargetBinding);
	Character.CombatHotbarSlots[TargetSlotIndex] = MoveTemp(SourceBinding);
	NotifyPartyInventoryChanged(CharacterIndex);
	return true;
}
