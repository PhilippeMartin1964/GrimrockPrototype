#include "Runtime/GridPartyInventoryComponent.h"

#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "CoreGlobals.h"
#include "GridItemInteractionReadModel.h"
#include "GridQuickItemUseReadModel.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

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

	// All live shortcut sources now resolve exclusively through ItemSystem.
	// Worldless historical Editor fixtures remain isolated below.
	const UWorld* World = GetWorld();
	UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
	const UGridItemSubsystem* Canonical = GI ? GI->GetSubsystem<UGridItemSubsystem>() : nullptr;
	const bool bRegisteredIdentity = Canonical &&
		Canonical->ContainsItem(SourceItem.ItemDefinitionId);
	const UGridItemDefinitionAsset* Definition = nullptr;
#if WITH_DEV_AUTOMATION_TESTS
	if (!Canonical && GIsAutomationTesting)
	{
		Definition = FindItemDefinition(SourceItem.ItemDefinitionId);
	}
#endif
	if (!bRegisteredIdentity &&
		(!IsValid(Definition) || !Definition->IsValidDefinition()))
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
		#if WITH_DEV_AUTOMATION_TESTS
		if (!bRegisteredIdentity && !Definition->CompatibleEquipmentSlots.Contains(SourceEquipmentSlot))
		{
			return false;
		}
#endif

		FGridItemInstance EquippedItem;
		if (!GetEquippedItem(CharacterIndex, SourceEquipmentSlot, EquippedItem) || EquippedItem.RuntimeObjectId != SourceItem.RuntimeObjectId ||
			EquippedItem.ItemDefinitionId != SourceItem.ItemDefinitionId)
		{
			return false;
		}

		FName PrimaryActionId = NAME_None;
		if (bRegisteredIdentity)
		{
			// The actual owned hand, not the drag snapshot, supplies the level
			// and all mutable source state for a new shortcut.
			FGridResolvedItem Item;
			if (!EquippedItem.IsValid() || EquippedItem.ItemLevel != SourceItem.ItemLevel ||
				EquippedItem.Quantity != 1 ||
				!Canonical->ResolveItem(EquippedItem.ItemDefinitionId, EquippedItem.ItemLevel, Item) ||
				!Item.IsValid() || !Item.bIsArmament)
			{
				return false; // No legacy fallback for a registered identity.
			}
			for (const FGridItemActionBinding& Authored : Item.AvailableActions)
			{
				FGridResolvedBoundAction Bound;
				if (Canonical->ResolveAttackFromEquippedInstance(
					EquippedItem, Authored.ActionId, SourceEquipmentSlot,
					EquippedItem.RuntimeObjectId, PartyInventoryState.ActiveCharacters[CharacterIndex].CharacterId,
					CharacterIndex, Bound))
				{
					PrimaryActionId = Bound.ActionId;
					break;
				}
			}
		}
		#if WITH_DEV_AUTOMATION_TESTS
		else if (!Canonical && GIsAutomationTesting && Definition)
		{
			const FGridCombatActionDefinition* PrimaryAction =
				Definition->CombatActions.FindByPredicate(
					[](const FGridCombatActionDefinition& Candidate)
					{
						return Candidate.IsValid() &&
							Candidate.SourcePolicy == EGridCombatActionSourcePolicy::Equipment;
					});
			if (PrimaryAction)
			{
				PrimaryActionId = PrimaryAction->ActionId;
			}
		}
#endif

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
		const FGridInventorySlot* OwnedSlot = Character.InventorySlots.FindByPredicate(
			[&SourceItem](const FGridInventorySlot& Candidate)
			{
				return !Candidate.IsEmpty() && Candidate.Item.RuntimeObjectId == SourceItem.RuntimeObjectId &&
					Candidate.Item.ItemDefinitionId == SourceItem.ItemDefinitionId;
			});
		if (!OwnedSlot)
		{
			return false;
		}
		const FGridItemInstance& OwnedItem = OwnedSlot->Item;
		if (bRegisteredIdentity)
		{
			FGridResolvedItem Physical;
			if (!OwnedItem.IsValid() || OwnedItem.ItemLevel != SourceItem.ItemLevel ||
				!Canonical->ResolveItem(OwnedItem.ItemDefinitionId, OwnedItem.ItemLevel, Physical) ||
				!Physical.IsValid() || OwnedItem.Quantity > FMath::Max(1, Physical.MaxStack))
			{
				return false;
			}
		}

		const FGridItemInteractionReadView Capabilities =
			FGridItemInteractionReadModel::Resolve(Canonical, Definition, OwnedItem);
		FGridCombatActionDefinition InventoryAction;
		if (Capabilities.bRegisteredIdentity && Capabilities.bPhysicalThrowHotbar)
		{
			Binding.ActionId = FGridCombatHotbarBinding::MakePhysicalThrowItemActionId(SourceItem.ItemDefinitionId);
			Binding.SourcePolicy = EGridCombatActionSourcePolicy::QuickItem;
			Binding.SourceDefinitionId = SourceItem.ItemDefinitionId;
		}
		else if (Capabilities.bRegisteredIdentity)
		{
			const FGridQuickItemUseReadView Use =
				FGridQuickItemUseReadModel::Resolve(Canonical, OwnedItem);
			if (!Use.bResolved)
			{
				return false; // No fallback to old QuickItem for known IDs.
			}
			Binding.ActionId = Use.Action.ActionId;
			Binding.SourcePolicy = EGridCombatActionSourcePolicy::QuickItem;
			Binding.SourceDefinitionId = SourceItem.ItemDefinitionId;
		}
		#if WITH_DEV_AUTOMATION_TESTS
		else if (!Canonical && GIsAutomationTesting && Definition &&
			Definition->BuildInventoryCombatActionDefinition(InventoryAction))
		{
			Binding.ActionId = InventoryAction.ActionId;
			Binding.SourcePolicy = EGridCombatActionSourcePolicy::QuickItem;
			Binding.SourceDefinitionId = SourceItem.ItemDefinitionId;
		}
		else if (!Canonical && GIsAutomationTesting && Definition && Definition->IsPhysicallyThrowable())
		{
			Binding.ActionId = FGridCombatHotbarBinding::MakePhysicalThrowItemActionId(SourceItem.ItemDefinitionId);
			Binding.SourcePolicy = EGridCombatActionSourcePolicy::QuickItem;
			Binding.SourceDefinitionId = SourceItem.ItemDefinitionId;
		}
#endif
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
