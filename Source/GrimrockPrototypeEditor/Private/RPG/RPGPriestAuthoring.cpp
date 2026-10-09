#include "RPG/RPGPriestAuthoring.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "HAL/FileManager.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGClassProgressionAuthoring.h"
#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

namespace RPGPriestAuthoring
{
	const FName PriestClassId(TEXT("Priest"));
	const FName RestorationBranchId(TEXT("Restoration"));
	const FName ProtectionBranchId(TEXT("Protection"));
	const FName ExorcismBranchId(TEXT("Exorcism"));

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

	const FName HolyLightTalentId(TEXT("Talent_Priest_Exorcism_HolyLight"));
	const FName TurnUndeadTalentId(TEXT("Talent_Priest_Exorcism_TurnUndead"));
	const FName HolyDispelTalentId(TEXT("Talent_Priest_Exorcism_HolyDispel"));
	const FName SmiteTalentId(TEXT("Talent_Priest_Exorcism_Smite"));
	const FName MajorExorcismTalentId(TEXT("Talent_Priest_Exorcism_MajorExorcism"));

	const FName RegenerationActionId(TEXT("Action_Priest_Regeneration"));
	const FName GroupHealActionId(TEXT("Action_Priest_GroupHeal"));
	const FName PurificationActionId(TEXT("Action_Priest_Purification"));
	const FName MiracleActionId(TEXT("Action_Priest_Miracle"));
	const FName BlessingActionId(TEXT("Action_Priest_Blessing"));
	const FName AegisActionId(TEXT("Action_Priest_Aegis"));
	const FName HolyProtectionActionId(TEXT("Action_Priest_HolyProtection"));
	const FName SanctuaryActionId(TEXT("Action_Priest_Sanctuary"));
	const FName DivineBastionActionId(TEXT("Action_Priest_DivineBastion"));
	const FName HolyLightActionId(TEXT("Action_Priest_HolyLight"));
	const FName TurnUndeadActionId(TEXT("Action_Priest_TurnUndead"));
	const FName HolyDispelActionId(TEXT("Action_Priest_HolyDispel"));
	const FName SmiteActionId(TEXT("Action_Priest_Smite"));
	const FName MajorExorcismActionId(TEXT("Action_Priest_MajorExorcism"));

	const FName RegenerationStatusId(TEXT("Status_Regeneration"));
	const FName BlessedStatusId(TEXT("Status_Blessed"));
	const FName HolyProtectionStatusId(TEXT("Status_HolyProtection"));
	const FName SanctuaryStatusId(TEXT("Status_Sanctuary"));
	const FName DivineBastionStatusId(TEXT("Status_DivineBastion"));
	const FName TurnedUndeadStatusId(TEXT("Status_TurnedUndead"));
	const FName BanishedStatusId(TEXT("Status_Banished"));


	ERPGTalentPresentationType ResolveTalentPresentationType(FName ChoiceId)
	{
		const FString Id = ChoiceId.ToString();
		if (Id == TEXT("Talent_Priest_Restoration_EnhancedHealing")) return ERPGTalentPresentationType::Passive;
		if (Id == TEXT("Talent_Priest_Restoration_Regeneration") || Id == TEXT("Talent_Priest_Restoration_GroupHeal") ||
			Id == TEXT("Talent_Priest_Restoration_Purification") || Id == TEXT("Talent_Priest_Restoration_Miracle") ||
			Id == TEXT("Talent_Priest_Protection_Blessing") || Id == TEXT("Talent_Priest_Protection_Aegis") ||
			Id == TEXT("Talent_Priest_Protection_HolyProtection") || Id == TEXT("Talent_Priest_Protection_Sanctuary") ||
			Id == TEXT("Talent_Priest_Protection_DivineBastion") || Id == TEXT("Talent_Priest_Exorcism_HolyLight") ||
			Id == TEXT("Talent_Priest_Exorcism_TurnUndead") || Id == TEXT("Talent_Priest_Exorcism_HolyDispel") ||
			Id == TEXT("Talent_Priest_Exorcism_Smite") || Id == TEXT("Talent_Priest_Exorcism_MajorExorcism"))
			return ERPGTalentPresentationType::ActiveSpell;
		return ERPGTalentPresentationType::None;
	}

	FRPGClassProgressionChoiceDefinition MakeChoice(
		FName ChoiceId, const TCHAR* DisplayName, const TCHAR* Description, int32 MinimumLevel, FName TalentBranchId, FName PrerequisiteChoiceId = NAME_None, FName TalentNodeId = NAME_None)
	{
		FRPGClassProgressionChoiceDefinition Choice;
		Choice.ChoiceId = ChoiceId;
		Choice.TalentBranchId = TalentBranchId;
		Choice.TalentNodeId = TalentNodeId.IsNone() ? ChoiceId : TalentNodeId;
		Choice.DisplayName = FText::FromString(DisplayName);
		Choice.Description = FText::FromString(Description);
		Choice.MinimumLevel = MinimumLevel;
		Choice.PointCost = 1;
		Choice.PresentationType = ResolveTalentPresentationType(ChoiceId);
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
		// Ally targeting uses the party-target path and does not participate in the grid LOS contract.
		Action.bRequiresLineOfSight =
			TargetingPolicy == EGridCombatTargetingPolicy::AllyOrHostile ||
			TargetingPolicy == EGridCombatTargetingPolicy::Hostile;
		Action.CooldownRounds = CooldownRounds;
		Action.Requirements = { RequirementId };
		return Action;
	}

	FGridCombatActionDefinition MakeHolyAttack(
		FName ActionId, const TCHAR* DisplayName, const TCHAR* Description, FName RequirementId,
		int32 ActionPointCost, int32 ManaCost, EGridCombatTargetingPolicy TargetingPolicy,
		int32 RangeCells, int32 BaseDamage, int32 CooldownRounds,
		int32 AdditionalWisdomModifierScale, bool bScaleReligion)
	{
		FGridCombatActionDefinition Action;
		Action.ActionId = ActionId;
		Action.DisplayName = FText::FromString(DisplayName);
		Action.Description = FText::FromString(Description);
		Action.ActionType = EGridCombatActionType::Ability;
		Action.SourcePolicy = EGridCombatActionSourcePolicy::Spell;
		Action.SourceTags = { TEXT("Spell.School.Holy") };
		Action.TargetingPolicy = TargetingPolicy;
		Action.ResolutionProfile = EGridCombatActionResolutionProfile::Attack;
		Action.ActionPointCost = ActionPointCost;
		Action.ResourceCosts.ManaCost = ManaCost;
		Action.RangeCells = RangeCells;
		Action.bRequiresLineOfSight =
			TargetingPolicy == EGridCombatTargetingPolicy::Hostile ||
			TargetingPolicy == EGridCombatTargetingPolicy::Cell ||
			TargetingPolicy == EGridCombatTargetingPolicy::Area;
		Action.CooldownRounds = CooldownRounds;
		Action.Requirements = { RequirementId };

		Action.OffensiveProfile.AttackId = ActionId;
		Action.OffensiveProfile.AttackDefinition.DamageType = EGridDamageType::Holy;
		Action.OffensiveProfile.AttackDefinition.PhysicalSubtype = EGridPhysicalDamageSubtype::None;
		Action.OffensiveProfile.AttackDefinition.MinDamage = BaseDamage;
		Action.OffensiveProfile.AttackDefinition.MaxDamage = BaseDamage;
		Action.OffensiveProfile.AttackDefinition.bAlwaysHits = true;
		Action.OffensiveProfile.AttackDefinition.bCanCriticalHit = false;
		Action.OffensiveProfile.DamageScalingAttribute = EGridAttackScalingAttribute::Wisdom;
		Action.OffensiveProfile.RangeCells = RangeCells;
		if (AdditionalWisdomModifierScale > 0)
		{
			Action.DirectDamageScaling.ScalingAttribute = EGridAttackScalingAttribute::Wisdom;
			Action.DirectDamageScaling.AttributeModifierScale = AdditionalWisdomModifierScale;
		}
		if (bScaleReligion)
		{
			Action.DirectDamageScaling.ScalingSkillId = TEXT("Skill_Religion");
			Action.DirectDamageScaling.SkillRankScale = 1;
		}
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
			OutError = TEXT("Cannot author an empty Priest status id.");
			return nullptr;
		}

		const FString AssetName = FString::Printf(TEXT("DA_%s"), *EffectId.ToString());
		const FString PackageName =
			FString::Printf(TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/StatusEffects/%s"), *AssetName);
		if (FPackageName::DoesPackageExist(PackageName))
		{
			const FString ObjectPath = FRPGPriestAuthoring::GetStatusObjectPath(EffectId);
			if (UGridStatusEffectDefinitionAsset* Existing =
					LoadObject<UGridStatusEffectDefinitionAsset>(nullptr, *ObjectPath))
			{
				return Existing;
			}
			OutError = FString::Printf(TEXT("Existing Priest status package could not load expected asset: %s"), *ObjectPath);
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
}

const TCHAR* FRPGPriestAuthoring::PriestAssetPath()
{
	return TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Priest.DA_Class_Priest");
}

FString FRPGPriestAuthoring::GetStatusObjectPath(FName EffectId)
{
	return FString::Printf(TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/StatusEffects/DA_%s.DA_%s"),
		*EffectId.ToString(), *EffectId.ToString());
}

void FRPGPriestAuthoring::ConfigureClass(URPGClassAsset& ClassAsset)
{
	using namespace RPGPriestAuthoring;

	FRPGClassProgressionAuthoring::ConfigureCanonicalTalentGrants(ClassAsset);
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
			TEXT("Tous les soins issus d'une action SourcePolicy=Spell du Prêtre sont multipliés par 1,25 après calcul de la magnitude, arrondi inférieur, minimum +1 si soin positif."), 2, RestorationBranchId);
		FGridCombatModifierProfile Modifier;
		Modifier.SourcePolicies = { EGridCombatActionSourcePolicy::Spell };
		Modifier.OutgoingHealingPercentModifier = 25;
		Choice.CombatModifiers.Add(Modifier);
		ClassAsset.ProgressionChoices.Add(Choice);
	}
	ClassAsset.ProgressionChoices.Add(MakeChoice(
		RegenerationTalentId, TEXT("Régénération"), TEXT("Applique Status_Regeneration 3 tours : à la fin de chaque activation de la cible, soigne 3 + WIS mod, minimum 1, puis décrémente la durée."), 6, RestorationBranchId, EnhancedHealingTalentId));
	ClassAsset.ProgressionChoices.Add(MakeChoice(
		GroupHealTalentId, TEXT("Soin de groupe"), TEXT("Chaque membre vivant du groupe récupère 5 + modificateur de SAG + rang de Médecine PV, sans dépasser ses PV maximum. Les personnages vaincus ne sont pas ciblés."), 10, RestorationBranchId, RegenerationTalentId));
	ClassAsset.ProgressionChoices.Add(MakeChoice(
		PurificationTalentId, TEXT("Purification"), TEXT("Retire jusqu'à 2 Debuffs amovibles parmi Poison, Burning, Bleeding, Slow, Silence, Immobilize et effets explicitement tagués Purifiable. Priorité : Potency puis EffectId."), 14, RestorationBranchId, GroupHealTalentId));
	ClassAsset.ProgressionChoices.Add(MakeChoice(
		MiracleTalentId, TEXT("Miracle"), TEXT("Soigne le maximum entre 12 + 2×WIS mod + Religion Rank et la quantité nécessaire pour atteindre 50 % PV maximum. Retire jusqu'à 3 Debuffs Purifiable et restaure 25 % du pool armure magique de référence. Ne ressuscite pas."), 18, RestorationBranchId, PurificationTalentId));

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
		BlessingTalentId, TEXT("Bénédiction"), TEXT("Status_Blessed 2 rounds : Précision +2 et InitiativeModifier +4. la réapplication rafraîchit la durée, non cumulable."), 2, ProtectionBranchId));
	ClassAsset.ProgressionChoices.Add(MakeChoice(
		AegisTalentId, TEXT("Égide"), TEXT("Restaure 8 + WIS mod + Religion Rank armure magique, clampé au pool de référence."), 6, ProtectionBranchId, BlessingTalentId));
	ClassAsset.ProgressionChoices.Add(MakeChoice(
		HolyProtectionTalentId, TEXT("Protection sacrée"), TEXT("Status_HolyProtection 3 rounds : résistances Holy/Necrotic/Arcane +25 % et Esquive +2 contre actions SourcePolicy=Spell ciblées."), 10, ProtectionBranchId, AegisTalentId));
	ClassAsset.ProgressionChoices.Add(MakeChoice(
		SanctuaryTalentId, TEXT("Sanctuaire"), TEXT("Jusqu'au début de la prochaine activation de la cible, max 2 rounds : attaques hostiles ciblées ne peuvent pas la sélectionner. Si la cible inflige des dégâts, le statut disparaît après cette action. AoE/DoT/surfaces restent valides."), 14, ProtectionBranchId, HolyProtectionTalentId));
	ClassAsset.ProgressionChoices.Add(MakeChoice(
		DivineBastionTalentId, TEXT("Bastion divin"), TEXT("Restaure 35 % du pool armure magique de référence à tous les alliés vivants puis applique Status_DivineBastion 2 rounds : dégâts non-Physical reçus -20 %."), 18, ProtectionBranchId, SanctuaryTalentId));

	// Exorcism actions.
	{
		FGridCombatActionDefinition Action = MakeHolyAttack(
			HolyLightActionId, TEXT("Lumière sacrée"),
			TEXT("Inflige 5 + modificateur de SAG + rang de Religion en dégâts Sacrés ; +50 % contre Undead ou Demon."),
			HolyLightTalentId, 2, 4, EGridCombatTargetingPolicy::Hostile, 5, 5, 0, 0, true);
		ClassAsset.CombatActions.Add(Action);
	}
	{
		FGridCombatActionDefinition Action = MakeHolyAttack(
			TurnUndeadActionId, TEXT("Repousser les morts-vivants"),
			TEXT("Tous les Undead à 2 cellules du groupe subissent 4 dégâts Sacrés ; sans armure magique, ils sont repoussés et perdent 4 Initiative pendant un round."),
			TurnUndeadTalentId, 3, 7, EGridCombatTargetingPolicy::Area, 1, 4, 3, 0, false);
		// D05: Repousser les morts-vivants is exactly 4 Holy damage; unlike the
		// other holy attacks, Wisdom must not contribute to the attack base damage.
		Action.OffensiveProfile.DamageScalingAttribute = EGridAttackScalingAttribute::None;
		Action.AreaRadiusCells = 2;
		Action.bAreaCenteredOnParty = true;
		Action.bRequiresLineOfSight = false;
		Action.TargetFilter.AllowedMonsterCategoryIds = { TEXT("Undead") };

		FGridCombatMovementEffectProfile Push;
		Push.Subject = EGridCombatMovementSubject::TargetCombatant;
		Push.Direction = EGridCombatMovementDirection::AwayFromSource;
		Push.DistanceCells = 1;
		Push.bForced = true;
		Push.ArmorGate = EGridCombatStatusArmorGate::MagicalArmorDepleted;
		Action.MovementEffects.Add(Push);

		FGridCombatStatusApplicationProfile InitiativeLoss;
		InitiativeLoss.StatusEffectId = TurnedUndeadStatusId;
		InitiativeLoss.Trigger = EGridCombatStatusApplicationTrigger::AfterSuccessfulHit;
		InitiativeLoss.ArmorGate = EGridCombatStatusArmorGate::MagicalArmorDepleted;
		InitiativeLoss.DurationOverride = 1;
		Action.StatusApplications.Add(InitiativeLoss);
		ClassAsset.CombatActions.Add(Action);
	}
	{
		FGridCombatActionDefinition Action = MakeEffectSpell(
			HolyDispelActionId, TEXT("Dissipation sacrée"),
			TEXT("Allié : retire jusqu'à deux Debuffs Nécrotiques ou Malédictions. Undead : retire un Buff magique amovible."),
			HolyDispelTalentId, 2, 6, EGridCombatTargetingPolicy::AllyOrHostile, 3, 2);
		Action.bRequiresLineOfSight = true;
		Action.TargetFilter.AllowedMonsterCategoryIds = { TEXT("Undead") };

		FGridCombatStatusRemovalProfile AllyRemoval;
		AllyRemoval.AnyStatusTags = { TEXT("Necrotic"), TEXT("Curse") };
		AllyRemoval.AllowedDispositions = { EGridStatusEffectDisposition::Debuff };
		AllyRemoval.TargetSide = EGridCombatStatusRemovalTargetSide::Party;
		AllyRemoval.MaximumRemovals = 2;
		Action.StatusRemovals.Add(AllyRemoval);

		FGridCombatStatusRemovalProfile HostileRemoval;
		HostileRemoval.AnyStatusTags = { TEXT("Dispel.Magical") };
		HostileRemoval.AllowedDispositions = { EGridStatusEffectDisposition::Buff };
		HostileRemoval.TargetSide = EGridCombatStatusRemovalTargetSide::Hostile;
		HostileRemoval.MaximumRemovals = 1;
		Action.StatusRemovals.Add(HostileRemoval);
		ClassAsset.CombatActions.Add(Action);
	}
	{
		FGridCombatActionDefinition Action = MakeHolyAttack(
			SmiteActionId, TEXT("Châtiment"),
			TEXT("Inflige 10 + 2×modificateur de SAG + rang de Religion en dégâts Sacrés ; +50 % contre Undead ou Demon."),
			SmiteTalentId, 3, 8, EGridCombatTargetingPolicy::Hostile, 4, 10, 2, 1, true);
		ClassAsset.CombatActions.Add(Action);
	}
	{
		FGridCombatActionDefinition Action = MakeHolyAttack(
			MajorExorcismActionId, TEXT("Exorcisme majeur"),
			TEXT("Zone sacrée n'affectant que Undead, Demon ou Summoned ; 14 + 2×SAG mod + Religion, et Banished si l'armure magique est épuisée."),
			MajorExorcismTalentId, 4, 14, EGridCombatTargetingPolicy::Area, 4, 14, 5, 1, true);
		Action.AreaRadiusCells = 2;
		Action.TargetFilter.AllowedMonsterCategoryIds = { TEXT("Undead"), TEXT("Demon"), TEXT("Summoned") };
		FGridCombatStatusApplicationProfile Banished;
		Banished.StatusEffectId = BanishedStatusId;
		Banished.Trigger = EGridCombatStatusApplicationTrigger::AfterSuccessfulHit;
		Banished.ArmorGate = EGridCombatStatusArmorGate::MagicalArmorDepleted;
		Banished.DurationOverride = 1;
		Action.StatusApplications.Add(Banished);
		ClassAsset.CombatActions.Add(Action);
	}

	// Exorcism progression.
	{
		FRPGClassProgressionChoiceDefinition Choice = MakeChoice(
			HolyLightTalentId, TEXT("Lumière sacrée"), TEXT("Inflige 5 + modificateur de Sagesse + rang de Religion en dégâts sacrés. Contre les Morts-vivants ou Démons, les dégâts sont multipliés par 1,5. Aucun statut supplémentaire."), 2, ExorcismBranchId);
		FGridCombatModifierProfile Bonus;
		Bonus.ActionIds = { HolyLightActionId };
		Bonus.AllowedTargetMonsterCategoryIds = { TEXT("Undead"), TEXT("Demon") };
		Bonus.OutgoingDamagePercentModifier = 50;
		Choice.CombatModifiers.Add(Bonus);
		ClassAsset.ProgressionChoices.Add(Choice);
	}
	ClassAsset.ProgressionChoices.Add(MakeChoice(
		TurnUndeadTalentId, TEXT("Repousser les morts-vivants"), TEXT("Chaque Undead dans la zone subit 4 Holy. Si armure magique=0 après dégâts, il est poussé d'1 cellule à l'opposé du groupe si possible et reçoit InitiativeModifier -4 pendant 1 round."), 6, ExorcismBranchId, HolyLightTalentId));
	ClassAsset.ProgressionChoices.Add(MakeChoice(
		HolyDispelTalentId, TEXT("Dissipation sacrée"), TEXT("Sur un allié : retire jusqu'à 2 affaiblissements Nécrotiques ou Malédictions. Sur un Mort-vivant hostile : retire 1 amélioration magique amovible. La priorité est déterministe : puissance, puis identifiant de l'effet."), 10, ExorcismBranchId, TurnUndeadTalentId));
	{
		FRPGClassProgressionChoiceDefinition Choice = MakeChoice(
			SmiteTalentId, TEXT("Châtiment"), TEXT("Inflige 10 + 2×modificateur de Sagesse + rang de Religion en dégâts sacrés. Contre les Morts-vivants ou Démons, les dégâts sont multipliés par 1,5. Ce sort ne peut pas infliger de coup critique."), 14, ExorcismBranchId, HolyDispelTalentId);
		FGridCombatModifierProfile Bonus;
		Bonus.ActionIds = { SmiteActionId };
		Bonus.AllowedTargetMonsterCategoryIds = { TEXT("Undead"), TEXT("Demon") };
		Bonus.OutgoingDamagePercentModifier = 50;
		Choice.CombatModifiers.Add(Bonus);
		ClassAsset.ProgressionChoices.Add(Choice);
	}
	ClassAsset.ProgressionChoices.Add(MakeChoice(
		MajorExorcismTalentId, TEXT("Exorcisme majeur"), TEXT("N'affecte que les Morts-vivants, Démons et créatures invoquées. Inflige 14 + 2×modificateur de Sagesse + rang de Religion en dégâts sacrés. Si l'armure magique est épuisée après les dégâts, applique Banni pendant 1 tour et fait perdre la prochaine activation."), 18, ExorcismBranchId, SmiteTalentId));
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

	if (EffectId == TurnedUndeadStatusId)
	{
		StatusAsset.DisplayName = FText::FromString(TEXT("Repoussé"));
		StatusAsset.Description = FText::FromString(TEXT("Initiative -4 pendant 1 round après Repousser les morts-vivants."));
		StatusAsset.StatusTags = { TEXT("Purifiable"), TEXT("Dispel.Magical"), TEXT("Control.Fear") };
		StatusAsset.Disposition = EGridStatusEffectDisposition::Debuff;
		StatusAsset.DurationUnit = EGridStatusEffectDurationUnit::Rounds;
		StatusAsset.DefaultDuration = 1;
		StatusAsset.InitiativeModifier = -4;
		return StatusAsset.IsValidDefinition();
	}

	if (EffectId == BanishedStatusId)
	{
		StatusAsset.DisplayName = FText::FromString(TEXT("Banni"));
		StatusAsset.Description = FText::FromString(TEXT("La prochaine activation est ignorée."));
		StatusAsset.StatusTags = { TEXT("Purifiable"), TEXT("Dispel.Magical"), TEXT("Control.Banish") };
		StatusAsset.Disposition = EGridStatusEffectDisposition::Debuff;
		StatusAsset.DurationUnit = EGridStatusEffectDurationUnit::Turns;
		StatusAsset.DefaultDuration = 1;
		StatusAsset.Control.bSkipActivation = true;
		return StatusAsset.IsValidDefinition();
	}

	return false;
}


void FRPGPriestAuthoring::GetRequiredPriestStatusIds(TArray<FName>& OutStatusIds)
{
	OutStatusIds = {
		RPGPriestAuthoring::RegenerationStatusId,
		RPGPriestAuthoring::BlessedStatusId,
		RPGPriestAuthoring::HolyProtectionStatusId,
		RPGPriestAuthoring::SanctuaryStatusId,
		RPGPriestAuthoring::DivineBastionStatusId,
		RPGPriestAuthoring::TurnedUndeadStatusId,
		RPGPriestAuthoring::BanishedStatusId
	};
}

bool FRPGPriestAuthoring::AuthorProductionAssets(FString& OutError)
{
	using namespace RPGPriestAuthoring;
	OutError.Reset();

	URPGClassAsset* Priest = LoadObject<URPGClassAsset>(nullptr, PriestAssetPath());
	if (!IsValid(Priest))
	{
		OutError = FString::Printf(TEXT("Production Priest asset not found: %s"), PriestAssetPath());
		return false;
	}
	if (Priest->ClassId != PriestClassId)
	{
		OutError = FString::Printf(TEXT("Unexpected Priest ClassId '%s'."), *Priest->ClassId.ToString());
		return false;
	}

	TArray<UGridStatusEffectDefinitionAsset*> PriestStatuses;
	TArray<FName> StatusIds;
	GetRequiredPriestStatusIds(StatusIds);
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
			OutError = FString::Printf(TEXT("No Priest status authoring definition for '%s'."), *StatusId.ToString());
			return false;
		}
		PriestStatuses.Add(Status);
	}

	Priest->Modify();
	ConfigureClass(*Priest);
	if (!Priest->IsValidDefinition())
	{
		OutError = TEXT("Authored DA_Class_Priest is structurally invalid.");
		return false;
	}

	for (UGridStatusEffectDefinitionAsset* Status : PriestStatuses)
	{
		if (!SaveAuthoredAsset(Status, OutError))
		{
			return false;
		}
	}
	return SaveAuthoredAsset(Priest, OutError);
}
