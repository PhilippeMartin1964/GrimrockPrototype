#include "ARMEMENT/GridArmClean06ActionProjection.h"
#include "UObject/Class.h"

GridArmClean06ActionProjection::EResult
GridArmClean06ActionProjection::TryProjectGeneratedAttack(
	const FGridWeaponPipelineRow& Manifest,
	const UGridItemDefinitionAsset& Item,
	const FGridWeaponLevelProfile& LevelOne,
	const FGridCombatActionDefinition& SourceAction,
	FGridCanonicalAction& OutAction)
{
	OutAction = FGridCanonicalAction();
	if (Manifest.IsPassive() || Manifest.AttackId.IsEmpty() ||
		Manifest.ItemDefinitionId != Item.ItemDefinitionId.ToString() ||
		Manifest.ArchetypeId != LevelOne.ArchetypeId.ToString() ||
		Item.WeaponArchetypeId != LevelOne.ArchetypeId ||
		SourceAction.ActionId != FName(*Manifest.AttackId) ||
		!SourceAction.IsValid() || !LevelOne.IsValidProfile())
	{
		return EResult::InvalidInput;
	}

	FGridAttackDefinition SourceAttack;
	if (!LevelOne.TryBuildAttackDefinition(SourceAttack))
	{
		return EResult::InvalidInput;
	}

	// Expected was independently reconstituted from the canonical production
	// authoring contract. Every legacy property is checked by reflection,
	// including optional effects, resource costs, requirements and soft refs.
	FGridCombatActionDefinition Expected;
	Expected.ActionId = FName(*Manifest.AttackId);
	Expected.DisplayName = LevelOne.DisplayName;
	Expected.Icon = Item.Icon;
	Expected.ActionType = Manifest.UsesRangedAction() ?
		EGridCombatActionType::RangedAttack : EGridCombatActionType::MeleeAttack;
	Expected.SourcePolicy = EGridCombatActionSourcePolicy::Equipment;
	Expected.TargetingPolicy = EGridCombatTargetingPolicy::FirstAxialTarget;
	Expected.ResolutionProfile = EGridCombatActionResolutionProfile::Attack;
	Expected.ActionPointCost = LevelOne.ActionPointCost;
	Expected.RangeCells = LevelOne.RangeCells;
	Expected.bRequiresLineOfSight = Manifest.UsesRangedAction();
	Expected.ResourceCosts.SourceItemQuantityCost = Manifest.IsThrowable() ? 1 : 0;
	Expected.OffensiveProfile.AttackId = Expected.ActionId;
	Expected.OffensiveProfile.AttackDefinition = SourceAttack;
	Expected.OffensiveProfile.DamageScalingAttribute = LevelOne.ScalingAttribute;
	Expected.OffensiveProfile.RangeCells = LevelOne.RangeCells;

	// Reject any source data not represented by this exact generator contract.
	// In particular, actions authored later with new effects remain untouched.
	if (!FGridCombatActionDefinition::StaticStruct()->CompareScriptStruct(
			&SourceAction, &Expected, 0))
	{
		return EResult::NonStandardAction;
	}

	OutAction.ActionId = SourceAction.ActionId;
	OutAction.Kind = EGridCanonicalActionKind::Attack;
	OutAction.Targeting = SourceAction.TargetingPolicy;
	OutAction.bRangedAttack = SourceAction.ActionType == EGridCombatActionType::RangedAttack;
	OutAction.bRequiresLineOfSight = SourceAction.bRequiresLineOfSight;
	OutAction.SourceItemQuantityCost = SourceAction.ResourceCosts.SourceItemQuantityCost;
	OutAction.ManaCost = SourceAction.ResourceCosts.ManaCost;
	OutAction.CooldownRounds = SourceAction.CooldownRounds;
	// Normal damage is owned by N1..N20, not duplicated in the ActionCatalogue.
	// A modifier for advanced actions must be independently authored and proven.
	OutAction.WeaponDamagePercent = 0;
	return EResult::ProvenStandard;
}
