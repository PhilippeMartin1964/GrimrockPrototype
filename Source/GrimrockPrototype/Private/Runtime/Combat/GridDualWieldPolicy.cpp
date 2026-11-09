#include "Runtime/Combat/GridDualWieldPolicy.h"

#include "Runtime/GridPartyInventoryComponent.h"

namespace
{
	bool ResolveHandSource(const UGridPartyInventoryComponent* Inventory, int32 CharacterIndex, EGridEquipmentSlot Slot,
		FGridDualWieldSource& OutSource)
	{
		OutSource = FGridDualWieldSource();
		if (!IsValid(Inventory) || !Inventory->GetEquippedItem(CharacterIndex, Slot, OutSource.Item) ||
			!OutSource.Item.IsValid() || OutSource.Item.EquipmentSlot != Slot)
		{
			return false;
		}
		OutSource.Definition = Inventory->FindItemDefinition(OutSource.Item.ItemDefinitionId);
		if (!IsValid(OutSource.Definition) || !OutSource.Definition->IsValidDefinition() ||
			OutSource.Definition->GetEffectiveHandUsage() != EGridItemHandUsage::OneHanded ||
			OutSource.Definition->IsCombatThrowable() ||
			!OutSource.Definition->RequiredAmmunitionItemDefinitionId.IsNone() ||
			!OutSource.Definition->CanProvideAttackFromSlot(Slot))
		{
			return false;
		}

		const FGridCombatActionDefinition* Attack = OutSource.Definition->CombatActions.FindByPredicate(
			[](const FGridCombatActionDefinition& Candidate)
			{
				return Candidate.IsValid() &&
					Candidate.SourcePolicy == EGridCombatActionSourcePolicy::Equipment &&
					Candidate.ResolutionProfile == EGridCombatActionResolutionProfile::Attack &&
					Candidate.TargetingPolicy == EGridCombatTargetingPolicy::FirstAxialTarget &&
					Candidate.ActionType == EGridCombatActionType::MeleeAttack &&
					Candidate.RangeCells == 1 && Candidate.ResolutionCount == 1 &&
					Candidate.ResourceCosts.ManaCost == 0 &&
					Candidate.ResourceCosts.SourceItemQuantityCost == 0 &&
					Candidate.CooldownRounds == 0;
			});
		if (!Attack ||
			!OutSource.Definition->ResolveEquippedOffensiveProfile(OutSource.Item, *Attack, OutSource.Profile) ||
			OutSource.Profile.RangeCells != 1 || !OutSource.Profile.IsValid())
		{
			return false;
		}
		OutSource.Action = *Attack;
		return true;
	}
}

FName FGridDualWieldPolicy::ActionId()
{
	static const FName DualWieldId(TEXT("Attack_DualWield"));
	return DualWieldId;
}

bool FGridDualWieldPolicy::TryResolve(const UGridPartyInventoryComponent* Inventory, int32 CharacterIndex, FGridDualWieldPair& OutPair)
{
	OutPair = FGridDualWieldPair();
	if (!ResolveHandSource(Inventory, CharacterIndex, EGridEquipmentSlot::MainHand, OutPair.Main) ||
		!ResolveHandSource(Inventory, CharacterIndex, EGridEquipmentSlot::OffHand, OutPair.Off))
	{
		return false;
	}
	return OutPair.Main.Item.RuntimeObjectId.IsValid() && OutPair.Off.Item.RuntimeObjectId.IsValid() &&
		OutPair.Main.Item.RuntimeObjectId != OutPair.Off.Item.RuntimeObjectId;
}
