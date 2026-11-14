#pragma once

#include "Runtime/Items/GridItemSubsystem.h"
#include "GridCanonicalActionIconReadModel.h"

/** Shared transient transport for one physically equipped canonical attack.
 * No legacy definition, catalogue, SaveGame write or implicit N1 fallback.
 */
struct FGridCanonicalEquippedAttackReadModel
{
	static bool TryResolve(const UGridItemSubsystem* Registry,
		const FGridItemInstance& Item, EGridEquipmentSlot Hand,
		FName ActionId, FGridCombatActionDefinition& OutAction)
	{
		OutAction = FGridCombatActionDefinition();
		if (!IsValid(Registry) || !Registry->HasCanonicalItem(Item.ItemDefinitionId)) return false;
		FGridResolvedItem Resolved;
		if (!Registry->ResolveItem(Item.ItemDefinitionId, Item.ItemLevel, Resolved) ||
			!Resolved.bIsArmament || !Resolved.Weapon.HasAttackSourceInHand(Hand)) return false;
		FGridResolvedBoundAction Bound;
		if (!Registry->ResolveAttackFromEquippedInstance(Item, ActionId, Hand,
			Item.RuntimeObjectId, Item.OwnerGuid, Item.OwnerCharacterIndex, Bound)) return false;

		FGridCombatActionDefinition Action;
		Action.ActionId = Bound.ActionId;
		Action.DisplayName = Resolved.DisplayName;
		Action.Description = Resolved.Description;
		Action.SourcePolicy = EGridCombatActionSourcePolicy::Equipment;
		Action.ActionType = Bound.Action.bRangedAttack
			? EGridCombatActionType::RangedAttack : EGridCombatActionType::MeleeAttack;
		Action.TargetingPolicy = Bound.Action.Targeting;
		Action.ResolutionProfile = EGridCombatActionResolutionProfile::Attack;
		Action.Icon = Registry->FindEquippedActionIcon(
			FGridCanonicalActionIconReadModel::Classify(Resolved, Action));
		Action.ActionPointCost = Resolved.Weapon.ActionPointCost;
		Action.RangeCells = Resolved.Weapon.RangeCells;
		Action.bRequiresLineOfSight = Bound.Action.bRequiresLineOfSight;
		Action.ResourceCosts.ManaCost = Bound.Action.ManaCost;
		Action.ResourceCosts.SourceItemQuantityCost = Bound.Action.SourceItemQuantityCost;
		Action.CooldownRounds = Bound.Action.CooldownRounds;
		for (const FName Tag : Resolved.Tags)
			if (!Tag.IsNone()) Action.SourceTags.AddUnique(Tag);
		FGridOffensiveEquipmentProfile& Profile = Action.OffensiveProfile;
		Profile.AttackId = Bound.ActionId;
		Profile.DamageScalingAttribute = Resolved.Weapon.ScalingAttribute;
		Profile.RangeCells = Resolved.Weapon.RangeCells;
		FGridAttackDefinition& Damage = Profile.AttackDefinition;
		Damage.DamageType = Resolved.Weapon.DamageType;
		Damage.PhysicalSubtype = Resolved.Weapon.PhysicalSubtype;
		Damage.DamageDice = Resolved.Weapon.DamageDice;
		Damage.AccuracyBonus = Resolved.Weapon.AccuracyBonus;
		Damage.MinDamage = Damage.DamageDice.GetMinimumDamage();
		Damage.MaxDamage = Damage.DamageDice.GetMaximumDamage();
		if (!Action.IsValid() || !Profile.IsValid()) return false;
		OutAction = MoveTemp(Action);
		return true;
	}
};
