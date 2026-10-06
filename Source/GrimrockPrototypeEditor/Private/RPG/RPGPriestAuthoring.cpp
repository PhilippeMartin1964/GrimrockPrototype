#include "RPG/RPGPriestAuthoring.h"

#include "RPG/RPGClassAsset.h"
#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"

namespace RPGPriestAuthoring
{
	const FName EnhancedHealingTalentId(TEXT("Talent_Priest_Restoration_EnhancedHealing"));
	const FName RegenerationTalentId(TEXT("Talent_Priest_Restoration_Regeneration"));
	const FName GroupHealTalentId(TEXT("Talent_Priest_Restoration_GroupHeal"));
	const FName PurificationTalentId(TEXT("Talent_Priest_Restoration_Purification"));
	const FName MiracleTalentId(TEXT("Talent_Priest_Restoration_Miracle"));

	const FName BlessingTalentId(TEXT("Talent_Priest_Protection_Blessing"));
	const FName AegisTalentId(TEXT("Talent_Priest_Protection_Aegis"));
	const FName HolyProtectionTalentId(TEXT("Talent_Priest_Protection_HolyProtection"));
	const FName SanctuaryTalentId(TEXT("Talent_Priest_Protection_Sanctuary"));
	const FName DivineBastionTalentId(TEXT("Talent_Priest_Protection_DivineBastion"));

	const FName RegenerationActionId(TEXT("Action_Priest_Regeneration"));
	const FName GroupHealActionId(TEXT("Action_Priest_GroupHeal"));
	const FName PurificationActionId(TEXT("Action_Priest_Purification"));
	const FName MiracleActionId(TEXT("Action_Priest_Miracle"));
	const FName BlessingActionId(TEXT("Action_Priest_Blessing"));
	const FName AegisActionId(TEXT("Action_Priest_Aegis"));
	const FName HolyProtectionActionId(TEXT("Action_Priest_HolyProtection"));
	const FName SanctuaryActionId(TEXT("Action_Priest_Sanctuary"));
	const FName DivineBastionActionId(TEXT("Action_Priest_DivineBastion"));

	const FName RegenerationStatusId(TEXT("Status_Regeneration"));
	const FName BlessedStatusId(TEXT("Status_Blessed"));
	const FName HolyProtectionStatusId(TEXT("Status_HolyProtection"));
	const FName SanctuaryStatusId(TEXT("Status_Sanctuary"));
	const FName DivineBastionStatusId(TEXT("Status_DivineBastion"));

	FRPGClassProgressionChoiceDefinition MakeChoice(
		FName ChoiceId, const TCHAR* DisplayName, const TCHAR* Description, int32 MinimumLevel, FName PrerequisiteChoiceId = NAME_None)
	{
		FRPGClassProgressionChoiceDefinition Choice;
		Choice.ChoiceId = ChoiceId;
		Choice.DisplayName = FText::FromString(DisplayName);
		Choice.Description = FText::FromString(Description);
		Choice.MinimumLevel = MinimumLevel;
		Choice.PointCost = 1;
		if (!PrerequisiteChoiceId.IsNone())
		{
			Choice.PrerequisiteChoiceIds.Add(PrerequisiteChoiceId);
		}
		return Choice;
	}

	FGridCombatActionDefinition MakeEffectSpell(
		FName ActionId, const TCHAR* DisplayName, const TCHAR* Description, FName RequirementId,
		int32 ActionPointCost, int32 ManaCost, EGridCombatTargetingPolicy TargetingPolicy,
		int32 RangeCells, int32 CooldownRounds)
	{
		FGridCombatActionDefinition Action;
		Action.ActionId = ActionId;
		Action.DisplayName = FText::FromString(DisplayName);
		Action.Description = FText::FromString(Description);
		Action.ActionType = EGridCombatActionType::Ability;
		Action.SourcePolicy = EGridCombatActionSourcePolicy::Spell;
		Action.TargetingPolicy = TargetingPolicy;
		Action.ResolutionProfile = EGridCombatActionResolutionProfile::Effect;
		Action.ActionPointCost = ActionPointCost;
		Action.ResourceCosts.ManaCost = ManaCost;
		Action.RangeCells = RangeCells;
		Action.bRequiresLineOfSight =
			TargetingPolicy == EGridCombatTargetingPolicy::Ally ||
			TargetingPolicy == EGridCombatTargetingPolicy::AllyOrHostile ||
			TargetingPolicy == EGridCombatTargetingPolicy::Hostile;
		Action.CooldownRounds = CooldownRounds;
		Action.Requirements = { RequirementId };
		return Action;
	}

	FGridCombatStatusApplicationProfile MakeStatusApplication(FName StatusId, int32 DurationOverride)
	{
		FGridCombatStatusApplicationProfile Profile;
		Profile.StatusEffectId = StatusId;
		Profile.Trigger = EGridCombatStatusApplicationTrigger::AfterResolution;
		Profile.ArmorGate = EGridCombatStatusArmorGate::None;
		Profile.DurationOverride = DurationOverride;
		return Profile;
	}

	FGridCombatArmorEffectProfile MakeMagicalArmorRestore(
		EGridCombatArmorEffectMagnitude Magnitude, int32 Amount,
		EGridAttackScalingAttribute ScalingAttribute = EGridAttackScalingAttribute::None,
		int32 AttributeModifierScale = 0, FName ScalingSkillId = NAME_None, int32 SkillRankScale = 0)
	{
		FGridCombatArmorEffectProfile Profile;
		Profile.Pool = EGridCombatArmorPool::Magical;
		Profile.Operation = EGridCombatArmorEffectOperation::Restore;
		Profile.Magnitude = Magnitude;
		Profile.Trigger = EGridCombatArmorEffectTrigger::AfterResolution;
		Profile.Amount = Amount;
		Profile.ScalingAttribute = ScalingAttribute;
		Profile.AttributeModifierScale = AttributeModifierScale;
		Profile.ScalingSkillId = ScalingSkillId;
		Profile.SkillRankScale = SkillRankScale;
		return Profile;
	}
}

const TCHAR* FRPGPriestAuthoring::PriestAssetPath()
{
	return TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Priest.DA_Class_Priest");
}

void FRPGPriestAuthoring::ConfigureClass(URPGClassAsset& ClassAsset)
{
	using namespace RPGPriestAuthoring;

	ClassAsset.CombatActions.Reset();
	ClassAsset.ProgressionChoices.Reset();

	// Restoration actions.
	{
		FGridCombatActionDefinition Action = MakeEffectSpell(
			RegenerationActionId, TEXT("Régénération"),
			TEXT("Applique Régénération pendant 3 tours : 3 + modificateur de SAG PV à la fin de chaque activation."),
			RegenerationTalentId, 2, 5, EGridCombatTargetingPolicy::Ally, 3, 2);
		Action.StatusApplications.Add(MakeStatusApplication(RegenerationStatusId, 3));
		ClassAsset.CombatActions.Add(Action);
	}
	{
		FGridCombatActionDefinition Action = MakeEffectSpell(
			GroupHealActionId, TEXT("Soin de groupe"),
			TEXT("Soigne tous les membres vivants du groupe de 5 + modificateur de SAG + rang de Médecine."),
			GroupHealTalentId, 3, 8, EGridCombatTargetingPolicy::Party, 0, 3);
		Action.EffectProfile.RestoreHealth = 5;
		Action.HealingScaling.ScalingAttribute = EGridAttackScalingAttribute::Wisdom;
		Action.HealingScaling.AttributeModifierScale = 1;
		Action.HealingScaling.ScalingSkillId = TEXT("Skill_Medicine");
		Action.HealingScaling.SkillRankScale = 1;
		ClassAsset.CombatActions.Add(Action);
	}
	{
		FGridCombatActionDefinition Action = MakeEffectSpell(
			PurificationActionId, TEXT("Purification"),
			TEXT("Retire jusqu'à deux afflictions purifiables, avec résolution déterministe."),
			PurificationTalentId, 2, 6, EGridCombatTargetingPolicy::Ally, 3, 2);
		FGridCombatStatusRemovalProfile Removal;
		Removal.EffectIds = {
			TEXT("Status_Poison"), TEXT("Status_Burning"), TEXT("Status_Bleeding"),
			TEXT("Status_Slow"), TEXT("Status_Silence"), TEXT("Status_Immobilized")
		};
		Removal.AnyStatusTags = { TEXT("Purifiable") };
		Removal.AllowedDispositions = { EGridStatusEffectDisposition::Debuff };
		Removal.TargetSide = EGridCombatStatusRemovalTargetSide::Party;
		Removal.MaximumRemovals = 2;
		Action.StatusRemovals.Add(Removal);
		ClassAsset.CombatActions.Add(Action);
	}
	{
		FGridCombatActionDefinition Action = MakeEffectSpell(
			MiracleActionId, TEXT("Miracle"),
			TEXT("Soigne 12 + 2×modificateur de SAG + rang de Religion, au minimum jusqu'à 50 % des PV ; purifie et restaure 25 % d'armure magique."),
			MiracleTalentId, 4, 15, EGridCombatTargetingPolicy::Ally, 3, 5);
		Action.EffectProfile.RestoreHealth = 12;
		Action.HealingScaling.ScalingAttribute = EGridAttackScalingAttribute::Wisdom;
		Action.HealingScaling.AttributeModifierScale = 2;
		Action.HealingScaling.ScalingSkillId = TEXT("Skill_Religion");
		Action.HealingScaling.SkillRankScale = 1;
		Action.HealingScaling.MinimumTargetHealthPercent = 50;

		FGridCombatStatusRemovalProfile Removal;
		Removal.AnyStatusTags = { TEXT("Purifiable") };
		Removal.AllowedDispositions = { EGridStatusEffectDisposition::Debuff };
		Removal.TargetSide = EGridCombatStatusRemovalTargetSide::Party;
		Removal.MaximumRemovals = 3;
		Action.StatusRemovals.Add(Removal);
		Action.ArmorEffects.Add(MakeMagicalArmorRestore(EGridCombatArmorEffectMagnitude::ReferencePercent, 25));
		ClassAsset.CombatActions.Add(Action);
	}

	// Restoration progression.
	{
		FRPGClassProgressionChoiceDefinition Choice = MakeChoice(
			EnhancedHealingTalentId, TEXT("Soin renforcé"),
			TEXT("Les soins issus des sorts du Prêtre gagnent +25 % après calcul de leur magnitude."), 2);
		FGridCombatModifierProfile Modifier;
		Modifier.SourcePolicies = { EGridCombatActionSourcePolicy::Spell };
		Modifier.OutgoingHealingPercentModifier = 25;
		Choice.CombatModifiers.Add(Modifier);
		ClassAsset.ProgressionChoices.Add(Choice);
	}
	ClassAsset.ProgressionChoices.Add(MakeChoice(
		RegenerationTalentId, TEXT("Régénération"), TEXT("Débloque Régénération."), 6, EnhancedHealingTalentId));
	ClassAsset.ProgressionChoices.Add(MakeChoice(
		GroupHealTalentId, TEXT("Soin de groupe"), TEXT("Débloque Soin de groupe."), 10, RegenerationTalentId));
	ClassAsset.ProgressionChoices.Add(MakeChoice(
		PurificationTalentId, TEXT("Purification"), TEXT("Débloque Purification."), 14, GroupHealTalentId));
	ClassAsset.ProgressionChoices.Add(MakeChoice(
		MiracleTalentId, TEXT("Miracle"), TEXT("Débloque Miracle."), 18, PurificationTalentId));

	// Protection actions.
	{
		FGridCombatActionDefinition Action = MakeEffectSpell(
			BlessingActionId, TEXT("Bénédiction"),
			TEXT("Accorde +2 Précision et +4 Initiative pendant 2 rounds."),
			BlessingTalentId, 2, 5, EGridCombatTargetingPolicy::Ally, 3, 2);
		Action.StatusApplications.Add(MakeStatusApplication(BlessedStatusId, 2));
		ClassAsset.CombatActions.Add(Action);
	}
	{
		FGridCombatActionDefinition Action = MakeEffectSpell(
			AegisActionId, TEXT("Égide"),
			TEXT("Restaure 8 + modificateur de SAG + rang de Religion en armure magique."),
			AegisTalentId, 2, 6, EGridCombatTargetingPolicy::Ally, 3, 2);
		Action.ArmorEffects.Add(MakeMagicalArmorRestore(
			EGridCombatArmorEffectMagnitude::Flat, 8, EGridAttackScalingAttribute::Wisdom, 1, TEXT("Skill_Religion"), 1));
		ClassAsset.CombatActions.Add(Action);
	}
	{
		FGridCombatActionDefinition Action = MakeEffectSpell(
			HolyProtectionActionId, TEXT("Protection sacrée"),
			TEXT("Accorde une protection de 3 rounds contre les attaques magiques ciblées."),
			HolyProtectionTalentId, 2, 7, EGridCombatTargetingPolicy::Ally, 3, 3);
		Action.StatusApplications.Add(MakeStatusApplication(HolyProtectionStatusId, 3));
		ClassAsset.CombatActions.Add(Action);
	}
	{
		FGridCombatActionDefinition Action = MakeEffectSpell(
			SanctuaryActionId, TEXT("Sanctuaire"),
			TEXT("Empêche les attaques hostiles directes de cibler l'allié jusqu'à sa prochaine activation ou jusqu'à ce qu'il inflige des dégâts."),
			SanctuaryTalentId, 3, 10, EGridCombatTargetingPolicy::Ally, 3, 4);
		Action.StatusApplications.Add(MakeStatusApplication(SanctuaryStatusId, 2));
		ClassAsset.CombatActions.Add(Action);
	}
	{
		FGridCombatActionDefinition Action = MakeEffectSpell(
			DivineBastionActionId, TEXT("Bastion divin"),
			TEXT("Restaure 35 % de l'armure magique de référence du groupe puis réduit de 20 % les dégâts non physiques pendant 2 rounds."),
			DivineBastionTalentId, 4, 12, EGridCombatTargetingPolicy::Party, 0, 5);
		Action.ArmorEffects.Add(MakeMagicalArmorRestore(EGridCombatArmorEffectMagnitude::ReferencePercent, 35));
		Action.StatusApplications.Add(MakeStatusApplication(DivineBastionStatusId, 2));
		ClassAsset.CombatActions.Add(Action);
	}

	// Protection progression.
	ClassAsset.ProgressionChoices.Add(MakeChoice(
		BlessingTalentId, TEXT("Bénédiction"), TEXT("Débloque Bénédiction."), 2));
	ClassAsset.ProgressionChoices.Add(MakeChoice(
		AegisTalentId, TEXT("Égide"), TEXT("Débloque Égide."), 6, BlessingTalentId));
	ClassAsset.ProgressionChoices.Add(MakeChoice(
		HolyProtectionTalentId, TEXT("Protection sacrée"), TEXT("Débloque Protection sacrée."), 10, AegisTalentId));
	ClassAsset.ProgressionChoices.Add(MakeChoice(
		SanctuaryTalentId, TEXT("Sanctuaire"), TEXT("Débloque Sanctuaire."), 14, HolyProtectionTalentId));
	ClassAsset.ProgressionChoices.Add(MakeChoice(
		DivineBastionTalentId, TEXT("Bastion divin"), TEXT("Débloque Bastion divin."), 18, SanctuaryTalentId));
}

bool FRPGPriestAuthoring::ConfigureStatus(UGridStatusEffectDefinitionAsset& StatusAsset, FName EffectId)
{
	using namespace RPGPriestAuthoring;

	StatusAsset.EffectId = EffectId;
	StatusAsset.StatusTags.Reset();
	StatusAsset.Icon.Reset();
	StatusAsset.DefaultPotency = 0;
	StatusAsset.StackPolicy = EGridStatusEffectStackPolicy::RefreshDuration;
	StatusAsset.MaxStacks = 1;
	StatusAsset.bDistinctPerSource = false;
	StatusAsset.bUniquePerSourceAcrossMonsters = false;
	StatusAsset.PeriodicDamage = FGridStatusEffectPeriodicDamageProfile();
	StatusAsset.PeriodicHealing = FGridStatusEffectPeriodicHealingProfile();
	StatusAsset.InitiativeModifier = 0;
	StatusAsset.Control = FGridStatusEffectControlProfile();
	StatusAsset.CombatModifiers.Reset();
	StatusAsset.CombatReactions.Reset();
	StatusAsset.bExpireAtOwnerNextActivation = false;

	if (EffectId == RegenerationStatusId)
	{
		StatusAsset.DisplayName = FText::FromString(TEXT("Régénération"));
		StatusAsset.Description = FText::FromString(TEXT("Rend 3 + modificateur de SAG du Prêtre source à la fin de chaque activation."));
		StatusAsset.StatusTags = { TEXT("Dispel.Magical") };
		StatusAsset.Disposition = EGridStatusEffectDisposition::Buff;
		StatusAsset.DurationUnit = EGridStatusEffectDurationUnit::Turns;
		StatusAsset.DefaultDuration = 3;
		StatusAsset.PeriodicHealing.HealingPerStack = 3;
		StatusAsset.PeriodicHealing.SourcePolicy = EGridCombatActionSourcePolicy::Spell;
		StatusAsset.PeriodicHealing.ScalingAttribute = EGridAttackScalingAttribute::Wisdom;
		StatusAsset.PeriodicHealing.AttributeModifierScale = 1;
		return StatusAsset.IsValidDefinition();
	}

	if (EffectId == BlessedStatusId)
	{
		StatusAsset.DisplayName = FText::FromString(TEXT("Bénédiction"));
		StatusAsset.Description = FText::FromString(TEXT("+2 Précision et +4 Initiative pendant 2 rounds."));
		StatusAsset.StatusTags = { TEXT("Dispel.Magical") };
		StatusAsset.Disposition = EGridStatusEffectDisposition::Buff;
		StatusAsset.DurationUnit = EGridStatusEffectDurationUnit::Rounds;
		StatusAsset.DefaultDuration = 2;
		StatusAsset.InitiativeModifier = 4;
		FGridCombatModifierProfile Modifier;
		Modifier.AccuracyModifier = 2;
		StatusAsset.CombatModifiers.Add(Modifier);
		return StatusAsset.IsValidDefinition();
	}

	if (EffectId == HolyProtectionStatusId)
	{
		StatusAsset.DisplayName = FText::FromString(TEXT("Protection sacrée"));
		StatusAsset.Description = FText::FromString(TEXT("+25 % résistances Sacré/Nécrotique/Arcane et +2 Esquive contre les sorts ciblés."));
		StatusAsset.StatusTags = { TEXT("Dispel.Magical") };
		StatusAsset.Disposition = EGridStatusEffectDisposition::Buff;
		StatusAsset.DurationUnit = EGridStatusEffectDurationUnit::Rounds;
		StatusAsset.DefaultDuration = 3;
		FGridCombatModifierProfile Modifier;
		Modifier.SourcePolicies = { EGridCombatActionSourcePolicy::Spell };
		Modifier.bExcludeAreaActions = true;
		Modifier.EvasionModifier = 2;
		Modifier.ResistanceModifiers.HolyResistance = 25;
		Modifier.ResistanceModifiers.NecroticResistance = 25;
		Modifier.ResistanceModifiers.ArcaneResistance = 25;
		StatusAsset.CombatModifiers.Add(Modifier);
		return StatusAsset.IsValidDefinition();
	}

	if (EffectId == SanctuaryStatusId)
	{
		StatusAsset.DisplayName = FText::FromString(TEXT("Sanctuaire"));
		StatusAsset.Description = FText::FromString(TEXT("Bloque le ciblage hostile direct jusqu'à la prochaine activation ou jusqu'à ce que le bénéficiaire inflige des dégâts."));
		StatusAsset.StatusTags = { TEXT("Dispel.Magical") };
		StatusAsset.Disposition = EGridStatusEffectDisposition::Buff;
		StatusAsset.DurationUnit = EGridStatusEffectDurationUnit::Rounds;
		StatusAsset.DefaultDuration = 2;
		StatusAsset.bExpireAtOwnerNextActivation = true;
		StatusAsset.Control.bBlockDirectHostileTargeting = true;

		FGridCombatReactionProfile BreakReaction;
		BreakReaction.ReactionId = TEXT("Reaction_Priest_Sanctuary_BreakOnDamage");
		BreakReaction.Trigger = EGridCombatReactionTrigger::AttackHit;
		BreakReaction.Limit = EGridCombatReactionLimit::OncePerAction;
		BreakReaction.bRequireOwnerAsEventSource = true;
		BreakReaction.bRequireAppliedDamage = true;
		BreakReaction.bConsumeOwningStatus = true;
		StatusAsset.CombatReactions.Add(BreakReaction);
		return StatusAsset.IsValidDefinition();
	}

	if (EffectId == DivineBastionStatusId)
	{
		StatusAsset.DisplayName = FText::FromString(TEXT("Bastion divin"));
		StatusAsset.Description = FText::FromString(TEXT("Réduit de 20 % les dégâts non physiques reçus pendant 2 rounds."));
		StatusAsset.StatusTags = { TEXT("Dispel.Magical") };
		StatusAsset.Disposition = EGridStatusEffectDisposition::Buff;
		StatusAsset.DurationUnit = EGridStatusEffectDurationUnit::Rounds;
		StatusAsset.DefaultDuration = 2;
		FGridCombatModifierProfile Modifier;
		Modifier.DamageTypes = {
			EGridDamageType::Fire, EGridDamageType::Ice, EGridDamageType::Lightning, EGridDamageType::Poison,
			EGridDamageType::Holy, EGridDamageType::Necrotic, EGridDamageType::Arcane
		};
		Modifier.IncomingDamagePercentModifier = -20;
		StatusAsset.CombatModifiers.Add(Modifier);
		return StatusAsset.IsValidDefinition();
	}

	return false;
}
