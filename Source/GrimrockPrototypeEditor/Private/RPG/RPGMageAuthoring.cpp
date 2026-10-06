#include "RPG/RPGMageAuthoring.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "HAL/FileManager.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

namespace RPGMageAuthoring
{
	const FName MageClassId(TEXT("Mage"));
	const FName ElementalAffinityAlias(TEXT("Talent_Mage_Evoker_ElementalAffinity"));
	const FName ElementalAffinityGroup(TEXT("TalentGroup_Mage_Evoker_ElementalAffinity"));
	const FName ElementalOverloadTalentId(TEXT("Talent_Mage_Evoker_ElementalOverload"));
	const FName ControlledExplosionTalentId(TEXT("Talent_Mage_Evoker_ControlledExplosion"));
	const FName ElementalChainTalentId(TEXT("Talent_Mage_Evoker_ElementalChain"));
	const FName CataclysmTalentId(TEXT("Talent_Mage_Evoker_Cataclysm"));

	const FName ArcaneShieldTalentId(TEXT("Talent_Mage_Arcanist_ArcaneShield"));
	const FName DispelTalentId(TEXT("Talent_Mage_Arcanist_Dispel"));
	const FName RunicManipulationTalentId(TEXT("Talent_Mage_Arcanist_RunicManipulation"));
	const FName ShortTeleportTalentId(TEXT("Talent_Mage_Arcanist_ShortTeleport"));
	const FName ArcaneMasteryTalentId(TEXT("Talent_Mage_Arcanist_ArcaneMastery"));

	const FName ElementalOverloadActionId(TEXT("Action_Mage_ElementalOverload"));
	const FName ElementalChainActionId(TEXT("Action_Mage_ElementalChain"));
	const FName CataclysmActionId(TEXT("Action_Mage_Cataclysm"));
	const FName ArcaneShieldActionId(TEXT("Action_Mage_ArcaneShield"));
	const FName DispelActionId(TEXT("Action_Mage_Dispel"));
	const FName ShortTeleportActionId(TEXT("Action_Mage_ShortTeleport"));

	const FName ElementalOverloadStatusId(TEXT("Status_ElementalOverload"));
	const FName BurningStatusId(TEXT("Status_Burning"));
	const FName SlowStatusId(TEXT("Status_Slow"));
	const FName StunnedStatusId(TEXT("Status_Stunned"));
	const FName ImmobilizedStatusId(TEXT("Status_Immobilized"));

	struct FAffinityVariant
	{
		const TCHAR* Suffix;
		const TCHAR* DisplayName;
		const TCHAR* SchoolTag;
		EGridDamageType DamageType;
		FName CataclysmStatusId;
		int32 CataclysmStatusDuration;
	};

	const FAffinityVariant AffinityVariants[] = {
		{ TEXT("Fire"), TEXT("Affinité élémentaire — Feu"), TEXT("Spell.School.Fire"),
			EGridDamageType::Fire, BurningStatusId, 2 },
		{ TEXT("Frost"), TEXT("Affinité élémentaire — Glace"), TEXT("Spell.School.Frost"),
			EGridDamageType::Ice, SlowStatusId, 2 },
		{ TEXT("Air"), TEXT("Affinité élémentaire — Air"), TEXT("Spell.School.Air"),
			EGridDamageType::Lightning, StunnedStatusId, 1 },
		// RPG02 intentionally has no Earth damage enum. Until an Earth spell sub-choice exists,
		// the deterministic Evoker interpretation is Physical damage + Immobilized.
		{ TEXT("Earth"), TEXT("Affinité élémentaire — Terre"), TEXT("Spell.School.Earth"),
			EGridDamageType::Physical, ImmobilizedStatusId, 1 }
	};

	FName MakeAffinityChoiceId(const TCHAR* Suffix)
	{
		return FName(*FString::Printf(TEXT("Talent_Mage_Evoker_ElementalAffinity_%s"), Suffix));
	}

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

	FGridCombatStatusApplicationProfile MakeStatusApplication(
		FName StatusId, EGridCombatStatusApplicationTrigger Trigger, EGridCombatStatusArmorGate ArmorGate, int32 DurationOverride)
	{
		FGridCombatStatusApplicationProfile Profile;
		Profile.StatusEffectId = StatusId;
		Profile.Trigger = Trigger;
		Profile.ArmorGate = ArmorGate;
		Profile.DurationOverride = DurationOverride;
		return Profile;
	}

	FGridCombatActionOwnerVariantProfile MakeAffinityActionVariant(const FAffinityVariant& Variant, bool bAddCataclysmStatus)
	{
		FGridCombatActionOwnerVariantProfile Result;
		Result.RequiredOwnerRequirementIds = { MakeAffinityChoiceId(Variant.Suffix) };
		Result.AddedSourceTags = { FName(Variant.SchoolTag) };
		Result.bOverrideDamageDescriptor = true;
		Result.OverrideDamageType = Variant.DamageType;
		Result.OverridePhysicalSubtype = EGridPhysicalDamageSubtype::None;
		if (bAddCataclysmStatus)
		{
			Result.StatusApplications.Add(MakeStatusApplication(
				Variant.CataclysmStatusId, EGridCombatStatusApplicationTrigger::AfterSuccessfulHit,
				EGridCombatStatusArmorGate::MagicalArmorDepleted, Variant.CataclysmStatusDuration));
		}
		return Result;
	}

	FGridCombatActionDefinition MakeDirectSpellAttack(
		FName ActionId, const TCHAR* DisplayName, const TCHAR* Description, FName RequirementId,
		int32 ActionPointCost, int32 ManaCost, EGridCombatTargetingPolicy TargetingPolicy,
		int32 RangeCells, int32 BaseDamage, int32 CooldownRounds)
	{
		FGridCombatActionDefinition Action;
		Action.ActionId = ActionId;
		Action.DisplayName = FText::FromString(DisplayName);
		Action.Description = FText::FromString(Description);
		Action.ActionType = EGridCombatActionType::Ability;
		Action.SourcePolicy = EGridCombatActionSourcePolicy::Spell;
		Action.TargetingPolicy = TargetingPolicy;
		Action.ResolutionProfile = EGridCombatActionResolutionProfile::Attack;
		Action.ActionPointCost = ActionPointCost;
		Action.ResourceCosts.ManaCost = ManaCost;
		Action.RangeCells = RangeCells;
		Action.bRequiresLineOfSight = true;
		Action.CooldownRounds = CooldownRounds;
		Action.Requirements = { RequirementId };

		Action.OffensiveProfile.AttackId = ActionId;
		Action.OffensiveProfile.AttackDefinition.DamageType = EGridDamageType::Physical;
		Action.OffensiveProfile.AttackDefinition.PhysicalSubtype = EGridPhysicalDamageSubtype::None;
		Action.OffensiveProfile.AttackDefinition.MinDamage = BaseDamage;
		Action.OffensiveProfile.AttackDefinition.MaxDamage = BaseDamage;
		Action.OffensiveProfile.AttackDefinition.bAlwaysHits = true;
		Action.OffensiveProfile.AttackDefinition.bCanCriticalHit = false;
		Action.OffensiveProfile.DamageScalingAttribute = EGridAttackScalingAttribute::Intelligence;
		Action.OffensiveProfile.RangeCells = RangeCells;
		Action.DirectDamageScaling.ScalingSkillId = TEXT("Skill_Arcana");
		Action.DirectDamageScaling.SkillRankScale = 1;

		for (const FAffinityVariant& Variant : AffinityVariants)
		{
			Action.OwnerVariants.Add(MakeAffinityActionVariant(Variant, ActionId == CataclysmActionId));
		}
		return Action;
	}

	bool SaveAuthoredAsset(UObject* Asset, FString& OutError)
	{
		if (!IsValid(Asset))
		{
			OutError = TEXT("Cannot save a null authored asset.");
			return false;
		}
		UPackage* Package = Asset->GetOutermost();
		if (!Package)
		{
			OutError = FString::Printf(TEXT("Asset '%s' has no package."), *GetNameSafe(Asset));
			return false;
		}

		Package->MarkPackageDirty();
		const FString Filename =
			FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);

		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError;
		if (!UPackage::SavePackage(Package, Asset, *Filename, SaveArgs))
		{
			OutError = FString::Printf(TEXT("Failed to save '%s' to '%s'."), *Asset->GetPathName(), *Filename);
			return false;
		}
		return true;
	}

	UGridStatusEffectDefinitionAsset* FindOrCreateStatus(FName EffectId, FString& OutError)
	{
		if (EffectId.IsNone())
		{
			OutError = TEXT("Cannot author an empty Mage status id.");
			return nullptr;
		}

		const FString AssetName = FString::Printf(TEXT("DA_%s"), *EffectId.ToString());
		const FString PackageName =
			FString::Printf(TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/StatusEffects/%s"), *AssetName);
		if (FPackageName::DoesPackageExist(PackageName))
		{
			const FString ObjectPath = FRPGMageAuthoring::GetStatusObjectPath(EffectId);
			if (UGridStatusEffectDefinitionAsset* Existing =
					LoadObject<UGridStatusEffectDefinitionAsset>(nullptr, *ObjectPath))
			{
				return Existing;
			}
			OutError = FString::Printf(TEXT("Existing Mage status package could not load expected asset: %s"), *ObjectPath);
			return nullptr;
		}

		UPackage* Package = CreatePackage(*PackageName);
		if (!Package)
		{
			OutError = FString::Printf(TEXT("Failed to create package '%s'."), *PackageName);
			return nullptr;
		}

		UGridStatusEffectDefinitionAsset* Created = NewObject<UGridStatusEffectDefinitionAsset>(
			Package, *AssetName, RF_Public | RF_Standalone | RF_Transactional);
		if (!Created)
		{
			OutError = FString::Printf(TEXT("Failed to create '%s'."), *AssetName);
			return nullptr;
		}
		FAssetRegistryModule::AssetCreated(Created);
		return Created;
	}

	bool ValidateSharedStatus(FName EffectId, FString& OutError)
	{
		const FString ObjectPath = FRPGMageAuthoring::GetStatusObjectPath(EffectId);
		const UGridStatusEffectDefinitionAsset* Status =
			LoadObject<UGridStatusEffectDefinitionAsset>(nullptr, *ObjectPath);
		if (!IsValid(Status) || !Status->IsValidDefinition())
		{
			OutError = FString::Printf(TEXT("Required shared status is missing or invalid: %s"), *ObjectPath);
			return false;
		}
		return true;
	}
}

const TCHAR* FRPGMageAuthoring::MageAssetPath()
{
	return TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Mage.DA_Class_Mage");
}

const TCHAR* FRPGMageAuthoring::ElementalOverloadStatusPath()
{
	return TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/StatusEffects/DA_Status_ElementalOverload.DA_Status_ElementalOverload");
}

FString FRPGMageAuthoring::GetStatusObjectPath(FName EffectId)
{
	return FString::Printf(TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/StatusEffects/DA_%s.DA_%s"),
		*EffectId.ToString(), *EffectId.ToString());
}

void FRPGMageAuthoring::ConfigureClass(URPGClassAsset& ClassAsset)
{
	using namespace RPGMageAuthoring;

	ClassAsset.CombatActions.Reset();
	ClassAsset.ProgressionChoices.Reset();

	// Evoker actions.
	{
		FGridCombatActionDefinition OverloadAction;
		OverloadAction.ActionId = ElementalOverloadActionId;
		OverloadAction.DisplayName = FText::FromString(TEXT("Surcharge élémentaire"));
		OverloadAction.Description =
			FText::FromString(TEXT("Le prochain sort de l'affinité élémentaire inflige +35 % de dégâts avant de consommer la surcharge."));
		OverloadAction.ActionType = EGridCombatActionType::Ability;
		OverloadAction.SourcePolicy = EGridCombatActionSourcePolicy::Ability;
		OverloadAction.TargetingPolicy = EGridCombatTargetingPolicy::Self;
		OverloadAction.ResolutionProfile = EGridCombatActionResolutionProfile::Effect;
		OverloadAction.ActionPointCost = 1;
		OverloadAction.ResourceCosts.ManaCost = 4;
		OverloadAction.CooldownRounds = 3;
		OverloadAction.Requirements = { ElementalOverloadTalentId };
		OverloadAction.StatusApplications.Add(MakeStatusApplication(
			ElementalOverloadStatusId, EGridCombatStatusApplicationTrigger::AfterResolution,
			EGridCombatStatusArmorGate::None, 1));
		ClassAsset.CombatActions.Add(OverloadAction);
	}
	{
		FGridCombatActionDefinition Chain = MakeDirectSpellAttack(
			ElementalChainActionId, TEXT("Chaîne élémentaire"),
			TEXT("Frappe la cible primaire puis jusqu'à deux hostiles adjacents, une seule fois chacun."),
			ElementalChainTalentId, 3, 8, EGridCombatTargetingPolicy::Cell, 5, 7, 3);
		Chain.MaximumResolvedTargets = 3;
		Chain.ChainJumpRangeCells = 1;
		ClassAsset.CombatActions.Add(Chain);
	}
	{
		FGridCombatActionDefinition Cataclysm = MakeDirectSpellAttack(
			CataclysmActionId, TEXT("Cataclysme"),
			TEXT("Frappe tous les hostiles dans un rayon de 2 et applique le contrôle de l'affinité si l'armure magique est épuisée."),
			CataclysmTalentId, 4, 16, EGridCombatTargetingPolicy::Area, 5, 12, 5);
		Cataclysm.AreaRadiusCells = 2;
		Cataclysm.bAffectsAlliesInArea = true;
		ClassAsset.CombatActions.Add(Cataclysm);
	}

	// Evoker progression.
	for (const FAffinityVariant& Variant : AffinityVariants)
	{
		const FName ChoiceId = MakeAffinityChoiceId(Variant.Suffix);
		FRPGClassProgressionChoiceDefinition Choice = MakeChoice(
			ChoiceId, Variant.DisplayName, TEXT("Les sorts de l'école choisie infligent +15 % de dégâts."), 2);
		Choice.ExclusiveChoiceGroupId = ElementalAffinityGroup;
		Choice.GrantedRequirementIds = { ElementalAffinityAlias };

		FGridCombatModifierProfile Modifier;
		Modifier.SourcePolicies = { EGridCombatActionSourcePolicy::Spell };
		Modifier.RequiredSourceTags = { FName(Variant.SchoolTag) };
		Modifier.OutgoingDamagePercentModifier = 15;
		Choice.CombatModifiers.Add(Modifier);
		ClassAsset.ProgressionChoices.Add(Choice);
	}

	FRPGClassProgressionChoiceDefinition Overload = MakeChoice(
		ElementalOverloadTalentId, TEXT("Surcharge élémentaire"),
		TEXT("Débloque Surcharge élémentaire : le prochain sort de l'affinité gagne +35 % de dégâts puis consomme l'effet."), 6);
	Overload.PrerequisiteRequirementIds = { ElementalAffinityAlias };
	ClassAsset.ProgressionChoices.Add(Overload);

	FRPGClassProgressionChoiceDefinition ControlledExplosion = MakeChoice(
		ControlledExplosionTalentId, TEXT("Explosion contrôlée"),
		TEXT("Les sorts de zone du Mage ne lui infligent aucun dégât direct et en infligent 50 % de moins à ses alliés."),
		10, ElementalOverloadTalentId);
	FGridCombatModifierProfile AreaProtection;
	AreaProtection.SourcePolicies = { EGridCombatActionSourcePolicy::Spell };
	AreaProtection.TargetingPolicies = { EGridCombatTargetingPolicy::Area };
	AreaProtection.FriendlyDirectDamagePercentModifier = -50;
	AreaProtection.SelfDirectDamagePercentModifier = -100;
	ControlledExplosion.CombatModifiers.Add(AreaProtection);
	ClassAsset.ProgressionChoices.Add(ControlledExplosion);

	ClassAsset.ProgressionChoices.Add(MakeChoice(
		ElementalChainTalentId, TEXT("Chaîne élémentaire"),
		TEXT("Débloque Chaîne élémentaire."), 14, ControlledExplosionTalentId));

	ClassAsset.ProgressionChoices.Add(MakeChoice(
		CataclysmTalentId, TEXT("Cataclysme"),
		TEXT("Débloque Cataclysme."), 18, ElementalChainTalentId));

	// Arcanist actions.
	{
		FGridCombatActionDefinition Shield;
		Shield.ActionId = ArcaneShieldActionId;
		Shield.DisplayName = FText::FromString(TEXT("Bouclier arcanique"));
		Shield.Description = FText::FromString(TEXT("Restaure l'armure magique de 6 + modificateur d'INT + rang d'Arcane."));
		Shield.ActionType = EGridCombatActionType::Ability;
		Shield.SourcePolicy = EGridCombatActionSourcePolicy::Spell;
		Shield.SourceTags = { TEXT("Spell.School.Arcane") };
		Shield.TargetingPolicy = EGridCombatTargetingPolicy::Ally;
		Shield.ResolutionProfile = EGridCombatActionResolutionProfile::Effect;
		Shield.ActionPointCost = 2;
		Shield.ResourceCosts.ManaCost = 5;
		Shield.RangeCells = 3;
		Shield.CooldownRounds = 2;
		Shield.Requirements = { ArcaneShieldTalentId };

		FGridCombatArmorEffectProfile Restore;
		Restore.Pool = EGridCombatArmorPool::Magical;
		Restore.Operation = EGridCombatArmorEffectOperation::Restore;
		Restore.Magnitude = EGridCombatArmorEffectMagnitude::Flat;
		Restore.Trigger = EGridCombatArmorEffectTrigger::AfterResolution;
		Restore.Amount = 6;
		Restore.ScalingAttribute = EGridAttackScalingAttribute::Intelligence;
		Restore.AttributeModifierScale = 1;
		Restore.ScalingSkillId = TEXT("Skill_Arcana");
		Restore.SkillRankScale = 1;
		Shield.ArmorEffects.Add(Restore);
		ClassAsset.CombatActions.Add(Shield);
	}
	{
		FGridCombatActionDefinition Dispel;
		Dispel.ActionId = DispelActionId;
		Dispel.DisplayName = FText::FromString(TEXT("Dissipation"));
		Dispel.Description = FText::FromString(
			TEXT("Allié : retire le Debuff magique amovible prioritaire. Hostile : retire le Buff magique amovible prioritaire."));
		Dispel.ActionType = EGridCombatActionType::Ability;
		Dispel.SourcePolicy = EGridCombatActionSourcePolicy::Spell;
		Dispel.SourceTags = { TEXT("Spell.School.Arcane") };
		Dispel.TargetingPolicy = EGridCombatTargetingPolicy::AllyOrHostile;
		Dispel.ResolutionProfile = EGridCombatActionResolutionProfile::Effect;
		Dispel.ActionPointCost = 2;
		Dispel.ResourceCosts.ManaCost = 6;
		Dispel.RangeCells = 4;
		Dispel.bRequiresLineOfSight = true;
		Dispel.CooldownRounds = 2;
		Dispel.Requirements = { DispelTalentId };

		FGridCombatStatusRemovalProfile AllyRemoval;
		AllyRemoval.AnyStatusTags = { TEXT("Dispel.Magical") };
		AllyRemoval.AllowedDispositions = { EGridStatusEffectDisposition::Debuff };
		AllyRemoval.TargetSide = EGridCombatStatusRemovalTargetSide::Party;
		AllyRemoval.MaximumRemovals = 1;
		Dispel.StatusRemovals.Add(AllyRemoval);

		FGridCombatStatusRemovalProfile HostileRemoval;
		HostileRemoval.AnyStatusTags = { TEXT("Dispel.Magical") };
		HostileRemoval.AllowedDispositions = { EGridStatusEffectDisposition::Buff };
		HostileRemoval.TargetSide = EGridCombatStatusRemovalTargetSide::Hostile;
		HostileRemoval.MaximumRemovals = 1;
		Dispel.StatusRemovals.Add(HostileRemoval);
		ClassAsset.CombatActions.Add(Dispel);
	}
	{
		FGridCombatActionDefinition Teleport;
		Teleport.ActionId = ShortTeleportActionId;
		Teleport.DisplayName = FText::FromString(TEXT("Téléportation courte"));
		Teleport.Description = FText::FromString(TEXT("Téléporte le groupe vers une cellule visible, libre et marchable à portée 2, sans PAM."));
		Teleport.ActionType = EGridCombatActionType::Ability;
		Teleport.SourcePolicy = EGridCombatActionSourcePolicy::Spell;
		Teleport.SourceTags = { TEXT("Spell.School.Arcane") };
		Teleport.TargetingPolicy = EGridCombatTargetingPolicy::Cell;
		Teleport.ResolutionProfile = EGridCombatActionResolutionProfile::Effect;
		Teleport.ActionPointCost = 3;
		Teleport.ResourceCosts.ManaCost = 8;
		Teleport.RangeCells = 2;
		Teleport.bRequiresLineOfSight = true;
		Teleport.CooldownRounds = 4;
		Teleport.Requirements = { ShortTeleportTalentId };
		Teleport.bRelocatePartyToTargetCell = true;
		ClassAsset.CombatActions.Add(Teleport);
	}

	// Arcanist progression.
	ClassAsset.ProgressionChoices.Add(MakeChoice(
		ArcaneShieldTalentId, TEXT("Bouclier arcanique"),
		TEXT("Débloque Bouclier arcanique."), 2));

	ClassAsset.ProgressionChoices.Add(MakeChoice(
		DispelTalentId, TEXT("Dissipation"),
		TEXT("Débloque Dissipation."), 6, ArcaneShieldTalentId));

	FRPGClassProgressionChoiceDefinition RunicManipulation = MakeChoice(
		RunicManipulationTalentId, TEXT("Manipulation runique"),
		TEXT("Jets de Runes +2 ; dégâts Arcane +20 % contre Rune ou Construct."), 10, DispelTalentId);
	FRPGSkillProgressionModifier RuneSkill;
	RuneSkill.SkillId = TEXT("Skill_Runes");
	RuneSkill.CheckModifier = 2;
	RunicManipulation.SkillModifiers.Add(RuneSkill);
	FGridCombatModifierProfile RunicDamage;
	RunicDamage.SourcePolicies = { EGridCombatActionSourcePolicy::Spell };
	RunicDamage.RequiredSourceTags = { TEXT("Spell.School.Arcane") };
	RunicDamage.AnyTargetSemanticTags = { TEXT("Rune"), TEXT("Construct") };
	RunicDamage.OutgoingDamagePercentModifier = 20;
	RunicManipulation.CombatModifiers.Add(RunicDamage);
	ClassAsset.ProgressionChoices.Add(RunicManipulation);

	ClassAsset.ProgressionChoices.Add(MakeChoice(
		ShortTeleportTalentId, TEXT("Téléportation courte"),
		TEXT("Débloque Téléportation courte."), 14, RunicManipulationTalentId));

	FRPGClassProgressionChoiceDefinition ArcaneMastery = MakeChoice(
		ArcaneMasteryTalentId, TEXT("Maîtrise de l'Arcane"),
		TEXT("Sorts Arcane : mana -1 (minimum 1), portée +1 et dégâts +15 %."), 18, ShortTeleportTalentId);
	FGridCombatModifierProfile Mastery;
	Mastery.SourcePolicies = { EGridCombatActionSourcePolicy::Spell };
	Mastery.RequiredSourceTags = { TEXT("Spell.School.Arcane") };
	Mastery.ManaCostModifier = -1;
	Mastery.MinimumManaCost = 1;
	Mastery.RangeCellsModifier = 1;
	Mastery.OutgoingDamagePercentModifier = 15;
	ArcaneMastery.CombatModifiers.Add(Mastery);
	ClassAsset.ProgressionChoices.Add(ArcaneMastery);
}

bool FRPGMageAuthoring::ConfigureElementalOverloadStatus(UGridStatusEffectDefinitionAsset& StatusAsset)
{
	return ConfigureStatus(StatusAsset, RPGMageAuthoring::ElementalOverloadStatusId);
}

bool FRPGMageAuthoring::ConfigureStatus(UGridStatusEffectDefinitionAsset& StatusAsset, FName EffectId)
{
	using namespace RPGMageAuthoring;

	StatusAsset.EffectId = EffectId;
	StatusAsset.StatusTags.Reset();
	StatusAsset.Icon.Reset();
	StatusAsset.DefaultPotency = 0;
	StatusAsset.StackPolicy = EGridStatusEffectStackPolicy::RefreshDuration;
	StatusAsset.MaxStacks = 1;
	StatusAsset.bDistinctPerSource = false;
	StatusAsset.bUniquePerSourceAcrossMonsters = false;
	StatusAsset.PeriodicDamage = FGridStatusEffectPeriodicDamageProfile();
	StatusAsset.InitiativeModifier = 0;
	StatusAsset.Control = FGridStatusEffectControlProfile();
	StatusAsset.CombatModifiers.Reset();
	StatusAsset.CombatReactions.Reset();
	StatusAsset.bExpireAtOwnerNextActivation = false;

	if (EffectId == ElementalOverloadStatusId)
	{
		StatusAsset.DisplayName = FText::FromString(TEXT("Surcharge élémentaire"));
		StatusAsset.Description =
			FText::FromString(TEXT("Le prochain sort correspondant à l'affinité élémentaire inflige +35 % de dégâts puis consomme cet effet."));
		StatusAsset.StatusTags = { TEXT("Dispel.Magical") };
		StatusAsset.Disposition = EGridStatusEffectDisposition::Buff;
		StatusAsset.DurationUnit = EGridStatusEffectDurationUnit::Turns;
		StatusAsset.DefaultDuration = 1;
		StatusAsset.StackPolicy = EGridStatusEffectStackPolicy::NoStack;

		for (const FAffinityVariant& Variant : AffinityVariants)
		{
			const FName ChoiceId = MakeAffinityChoiceId(Variant.Suffix);
			const FName SchoolTag(Variant.SchoolTag);

			FGridCombatModifierProfile Modifier;
			Modifier.SourcePolicies = { EGridCombatActionSourcePolicy::Spell };
			Modifier.RequiredSourceTags = { SchoolTag };
			Modifier.RequiredOwnerRequirementIds = { ChoiceId };
			Modifier.OutgoingDamagePercentModifier = 35;
			StatusAsset.CombatModifiers.Add(Modifier);

			FGridCombatReactionProfile Reaction;
			Reaction.ReactionId = FName(*FString::Printf(TEXT("Reaction_Mage_ElementalOverload_%s"), Variant.Suffix));
			Reaction.Trigger = EGridCombatReactionTrigger::ActionResolved;
			Reaction.Limit = EGridCombatReactionLimit::OncePerAction;
			Reaction.SourcePolicies = { EGridCombatActionSourcePolicy::Spell };
			Reaction.RequiredSourceTags = { SchoolTag };
			Reaction.RequiredOwnerRequirementIds = { ChoiceId };
			Reaction.bConsumeOwningStatus = true;
			StatusAsset.CombatReactions.Add(Reaction);
		}
		return StatusAsset.IsValidDefinition();
	}

	if (EffectId == BurningStatusId)
	{
		StatusAsset.DisplayName = FText::FromString(TEXT("Brûlure"));
		StatusAsset.Description = FText::FromString(TEXT("Subit 2 dégâts de Feu à chaque tick pendant 2 tours."));
		StatusAsset.StatusTags = { TEXT("Purifiable"), TEXT("Dispel.Magical"), TEXT("Elemental.Fire") };
		StatusAsset.Disposition = EGridStatusEffectDisposition::Debuff;
		StatusAsset.DurationUnit = EGridStatusEffectDurationUnit::Turns;
		StatusAsset.DefaultDuration = 2;
		StatusAsset.PeriodicDamage.DamageType = EGridDamageType::Fire;
		StatusAsset.PeriodicDamage.DamagePerStack = 2;
		return StatusAsset.IsValidDefinition();
	}

	if (EffectId == SlowStatusId)
	{
		StatusAsset.DisplayName = FText::FromString(TEXT("Ralenti"));
		StatusAsset.Description = FText::FromString(TEXT("Initiative -6 pendant 2 rounds."));
		StatusAsset.StatusTags = { TEXT("Purifiable"), TEXT("Dispel.Magical"), TEXT("Control.Magical"), TEXT("Control.Slow") };
		StatusAsset.Disposition = EGridStatusEffectDisposition::Debuff;
		StatusAsset.DurationUnit = EGridStatusEffectDurationUnit::Rounds;
		StatusAsset.DefaultDuration = 2;
		StatusAsset.InitiativeModifier = -6;
		return StatusAsset.IsValidDefinition();
	}

	return false;
}

void FRPGMageAuthoring::GetRequiredMageStatusIds(TArray<FName>& OutStatusIds)
{
	OutStatusIds = {
		RPGMageAuthoring::ElementalOverloadStatusId,
		RPGMageAuthoring::BurningStatusId,
		RPGMageAuthoring::SlowStatusId
	};
}

bool FRPGMageAuthoring::AuthorProductionAssets(FString& OutError)
{
	using namespace RPGMageAuthoring;
	OutError.Reset();

	URPGClassAsset* Mage = LoadObject<URPGClassAsset>(nullptr, MageAssetPath());
	if (!IsValid(Mage))
	{
		OutError = FString::Printf(TEXT("Production Mage asset not found: %s"), MageAssetPath());
		return false;
	}
	if (Mage->ClassId != MageClassId)
	{
		OutError = FString::Printf(TEXT("Unexpected Mage ClassId '%s'."), *Mage->ClassId.ToString());
		return false;
	}

	if (!ValidateSharedStatus(StunnedStatusId, OutError) || !ValidateSharedStatus(ImmobilizedStatusId, OutError))
	{
		return false;
	}

	TArray<UGridStatusEffectDefinitionAsset*> MageStatuses;
	TArray<FName> StatusIds;
	GetRequiredMageStatusIds(StatusIds);
	for (const FName StatusId : StatusIds)
	{
		UGridStatusEffectDefinitionAsset* Status = FindOrCreateStatus(StatusId, OutError);
		if (!IsValid(Status))
		{
			return false;
		}
		Status->Modify();
		if (!ConfigureStatus(*Status, StatusId))
		{
			OutError = FString::Printf(TEXT("No Mage status authoring definition for '%s'."), *StatusId.ToString());
			return false;
		}
		MageStatuses.Add(Status);
	}

	Mage->Modify();
	ConfigureClass(*Mage);
	if (!Mage->IsValidDefinition())
	{
		OutError = TEXT("Authored DA_Class_Mage is structurally invalid.");
		return false;
	}

	for (UGridStatusEffectDefinitionAsset* Status : MageStatuses)
	{
		if (!SaveAuthoredAsset(Status, OutError))
		{
			return false;
		}
	}
	return SaveAuthoredAsset(Mage, OutError);
}
