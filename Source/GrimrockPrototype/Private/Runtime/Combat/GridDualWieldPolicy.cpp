#include "Runtime/Combat/GridDualWieldPolicy.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "GridCanonicalEquippedAttackReadModel.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

namespace
{
	const UGridItemSubsystem* ResolveInventoryRegistry(const UGridPartyInventoryComponent* Inventory)
	{
		const UWorld* World = IsValid(Inventory) ? Inventory->GetWorld() : nullptr;
		UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		return GameInstance ? GameInstance->GetSubsystem<UGridItemSubsystem>() : nullptr;
	}

	bool ResolveHandSource(const UGridPartyInventoryComponent* Inventory, int32 CharacterIndex,
		EGridEquipmentSlot Slot, const UGridItemSubsystem* Registry, FGridDualWieldSource& OutSource)
	{
		OutSource = FGridDualWieldSource();
		if (!IsValid(Inventory) ||
			!Inventory->GetEquippedItem(CharacterIndex, Slot, OutSource.Item) ||
			!OutSource.Item.IsValid() || OutSource.Item.EquipmentSlot != Slot)
		{
			return false; // A two-handed item's virtual OffHand is not a second physical weapon.
		}
		if (Registry && Registry->HasCanonicalItem(OutSource.Item.ItemDefinitionId))
		{
			OutSource.bCanonicalIdentity = true;
			FGridResolvedItem Canonical;
			if (!Registry->ResolveItem(
					OutSource.Item.ItemDefinitionId, OutSource.Item.ItemLevel, Canonical) ||
				!Canonical.bIsArmament ||
				Canonical.Weapon.Kind != EGridCanonicalArmamentKind::Weapon ||
				Canonical.Weapon.HandPolicy == EGridCanonicalHandPolicy::TwoHanded ||
				!Canonical.Weapon.HasAttackSourceInHand(Slot) ||
				Canonical.Weapon.RangeCells != 1 ||
				!Canonical.Weapon.RequiredAmmoKindId.IsNone())
			{
				return false; // Known invalid/passive/ranged: NEVER read old CombatActions.
			}
			for (const FGridItemActionBinding& Binding : Canonical.AvailableActions)
			{
				FGridCombatActionDefinition Candidate;
				if (!FGridCanonicalEquippedAttackReadModel::TryResolve(
					Registry, OutSource.Item, Slot, Binding.ActionId, Candidate))
					continue;
				if (Candidate.ActionType != EGridCombatActionType::MeleeAttack ||
					Candidate.TargetingPolicy != EGridCombatTargetingPolicy::FirstAxialTarget ||
					Candidate.ResolutionProfile != EGridCombatActionResolutionProfile::Attack ||
					Candidate.RangeCells != 1 || Candidate.ResolutionCount != 1 ||
					Candidate.ResourceCosts.ManaCost != 0 ||
					Candidate.ResourceCosts.SourceItemQuantityCost != 0 ||
					Candidate.CooldownRounds != 0)
					continue;
				OutSource.Action = MoveTemp(Candidate);
				OutSource.Profile = OutSource.Action.OffensiveProfile;
				OutSource.SourceTags = OutSource.Action.SourceTags;
				return true;
			}
			return false;
		}

		// Only not-yet-migrated identities retain their historical path.
		OutSource.Definition = Inventory->FindItemDefinition(OutSource.Item.ItemDefinitionId);
		if (!IsValid(OutSource.Definition) || !OutSource.Definition->IsValidDefinition() ||
			OutSource.Definition->GetEffectiveHandUsage() != EGridItemHandUsage::OneHanded ||
			OutSource.Definition->IsCombatThrowable() ||
			!OutSource.Definition->RequiredAmmunitionItemDefinitionId.IsNone() ||
			!OutSource.Definition->CanProvideAttackFromSlot(Slot))
			return false;
		const FGridCombatActionDefinition* Attack =
			OutSource.Definition->CombatActions.FindByPredicate(
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
			!OutSource.Definition->ResolveEquippedOffensiveProfile(
				OutSource.Item, *Attack, OutSource.Profile) ||
			OutSource.Profile.RangeCells != 1 || !OutSource.Profile.IsValid())
			return false;
		OutSource.Action = *Attack;
		OutSource.SourceTags = OutSource.Definition->ItemTags;
		return true;
	}
}

FName FGridDualWieldPolicy::ActionId()
{
	static const FName DualWieldId(TEXT("Attack_DualWield"));
	return DualWieldId;
}

bool FGridDualWieldPolicy::TryResolve(const UGridPartyInventoryComponent* Inventory,
	int32 CharacterIndex, FGridDualWieldPair& OutPair,
	const UGridItemSubsystem* RegistryOverride)
{
	OutPair = FGridDualWieldPair();
	const UGridItemSubsystem* Registry =
		RegistryOverride ? RegistryOverride : ResolveInventoryRegistry(Inventory);
	if (!ResolveHandSource(Inventory, CharacterIndex, EGridEquipmentSlot::MainHand, Registry, OutPair.Main) ||
		!ResolveHandSource(Inventory, CharacterIndex, EGridEquipmentSlot::OffHand, Registry, OutPair.Off))
		return false;
	return OutPair.Main.Item.RuntimeObjectId.IsValid() && OutPair.Off.Item.RuntimeObjectId.IsValid() &&
		OutPair.Main.Item.RuntimeObjectId != OutPair.Off.Item.RuntimeObjectId;
}
