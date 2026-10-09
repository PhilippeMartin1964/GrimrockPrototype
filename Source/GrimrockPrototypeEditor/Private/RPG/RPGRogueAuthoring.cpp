#include "RPG/RPGRogueAuthoring.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "HAL/FileManager.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGClassProgressionAuthoring.h"
#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

namespace RPGRogueAuthoring
{
	const FName RogueClassId(TEXT("Rogue"));
	const FName AssassinBranchId(TEXT("Assassin"));
	const FName ShadowBranchId(TEXT("Shadow"));
	const FName SaboteurBranchId(TEXT("Saboteur"));
	const FName LightWeaponTag(TEXT("Weapon.Light"));

	FGridCombatStatusApplicationProfile MakeStatusApplication(FName StatusId, EGridCombatStatusApplicationTrigger Trigger,
		EGridCombatStatusArmorGate Gate = EGridCombatStatusArmorGate::None, int32 DurationOverride = INDEX_NONE)
	{
		FGridCombatStatusApplicationProfile Profile;
		Profile.StatusEffectId = StatusId;
		Profile.Trigger = Trigger;
		Profile.ArmorGate = Gate;
		Profile.DurationOverride = DurationOverride;
		return Profile;
	}


	ERPGTalentPresentationType ResolveTalentPresentationType(FName ChoiceId)
	{
		const FString Id = ChoiceId.ToString();
		if (Id == TEXT("Talent_Rogue_Shadow_Elusive")) return ERPGTalentPresentationType::AutomaticReaction;
		if (Id == TEXT("Talent_Rogue_Assassin_Backstab") || Id == TEXT("Talent_Rogue_Saboteur_ExpertDisarm") ||
			Id == TEXT("Talent_Rogue_Saboteur_MasterLocksmith")) return ERPGTalentPresentationType::Passive;
		if (Id == TEXT("Talent_Rogue_Assassin_SneakAttack") || Id == TEXT("Talent_Rogue_Assassin_Hemorrhage") ||
			Id == TEXT("Talent_Rogue_Assassin_WeakPoint") || Id == TEXT("Talent_Rogue_Assassin_Finisher") ||
			Id == TEXT("Talent_Rogue_Shadow_Dodge") || Id == TEXT("Talent_Rogue_Shadow_ShortVanish") ||
			Id == TEXT("Talent_Rogue_Shadow_ShadowStep") || Id == TEXT("Talent_Rogue_Shadow_PerfectShadow") ||
			Id == TEXT("Talent_Rogue_Saboteur_QuickTrap") || Id == TEXT("Talent_Rogue_Saboteur_SmokeBomb") ||
			Id == TEXT("Talent_Rogue_Saboteur_Sabotage")) return ERPGTalentPresentationType::Active;
		return ERPGTalentPresentationType::None;
	}

	FRPGClassProgressionChoiceDefinition MakeChoice(FName ChoiceId, const TCHAR* DisplayName, const TCHAR* Description,
		int32 MinimumLevel, FName TalentBranchId, FName PrerequisiteChoiceId = NAME_None, FName TalentNodeId = NAME_None)
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

	FGridCombatActionDefinition MakeEffectAction(FName ActionId, const TCHAR* DisplayName, const TCHAR* Description,
		FName RequirementId, int32 ActionPointCost, EGridCombatTargetingPolicy TargetingPolicy, int32 RangeCells, int32 CooldownRounds)
	{
		FGridCombatActionDefinition Action;
		Action.ActionId = ActionId;
		Action.DisplayName = FText::FromString(DisplayName);
		Action.Description = FText::FromString(Description);
		Action.ActionType = EGridCombatActionType::Ability;
		Action.SourcePolicy = EGridCombatActionSourcePolicy::Ability;
		Action.TargetingPolicy = TargetingPolicy;
		Action.ResolutionProfile = EGridCombatActionResolutionProfile::Effect;
		Action.ActionPointCost = ActionPointCost;
		Action.RangeCells = RangeCells;
		Action.CooldownRounds = CooldownRounds;
		Action.Requirements = { RequirementId };
		return Action;
	}

	FGridCombatActionDefinition MakeWeaponAction(FName ActionId, const TCHAR* DisplayName, const TCHAR* Description,
		FName RequirementId, int32 ActionPointCost, int32 CooldownRounds, int32 WeaponDamagePercent)
	{
		FGridCombatActionDefinition Action;
		Action.ActionId = ActionId;
		Action.DisplayName = FText::FromString(DisplayName);
		Action.Description = FText::FromString(Description);
		Action.ActionType = EGridCombatActionType::MeleeAttack;
		Action.SourcePolicy = EGridCombatActionSourcePolicy::Ability;
		Action.TargetingPolicy = EGridCombatTargetingPolicy::FirstAxialTarget;
		Action.ResolutionProfile = EGridCombatActionResolutionProfile::Attack;
		Action.ActionPointCost = ActionPointCost;
		Action.RangeCells = 1;
		Action.CooldownRounds = CooldownRounds;
		Action.Requirements = { RequirementId };
		Action.SourceTags = { LightWeaponTag };
		Action.WeaponAttackProfile.bUseEquippedWeapon = true;
		Action.WeaponAttackProfile.WeaponDamagePercent = WeaponDamagePercent;
		Action.WeaponAttackProfile.RequiredItemTags = { LightWeaponTag };
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
		const FString ObjectPath = FRPGRogueAuthoring::GetStatusObjectPath(EffectId);
		if (UGridStatusEffectDefinitionAsset* Existing = LoadObject<UGridStatusEffectDefinitionAsset>(nullptr, *ObjectPath))
		{
			return Existing;
		}

		const FString AssetName = FString::Printf(TEXT("DA_%s"), *EffectId.ToString());
		const FString PackageName =
			FString::Printf(TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/StatusEffects/%s"), *AssetName);
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
			OutError = FString::Printf(TEXT("Failed to create status asset '%s'."), *ObjectPath);
			return nullptr;
		}
		FAssetRegistryModule::AssetCreated(Created);
		return Created;
	}
}

const TCHAR* FRPGRogueAuthoring::RogueAssetPath()
{
	return TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Rogue.DA_Class_Rogue");
}

void FRPGRogueAuthoring::GetRequiredStatusIds(TArray<FName>& OutStatusIds)
{
	OutStatusIds = {
		TEXT("Status_Bleeding"),
		TEXT("Status_ExposedPhysical"),
		TEXT("Status_Evasive"),
		TEXT("Status_Hidden"),
		TEXT("Status_ShadowReach"),
		TEXT("Status_Elusive"),
		TEXT("Status_HiddenPerfect"),
		TEXT("Status_Immobilized"),
		TEXT("Status_Sabotaged")
	};
}

FString FRPGRogueAuthoring::GetStatusObjectPath(FName EffectId)
{
	return FString::Printf(TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/StatusEffects/DA_%s.DA_%s"),
		*EffectId.ToString(), *EffectId.ToString());
}

void FRPGRogueAuthoring::ConfigureClass(URPGClassAsset& ClassAsset)
{
	using namespace RPGRogueAuthoring;

	FRPGClassProgressionAuthoring::ConfigureCanonicalTalentGrants(ClassAsset);
	ClassAsset.CombatActions.Reset();
	ClassAsset.ProgressionChoices.Reset();

	// Assassin actions.
	{
		FGridCombatActionDefinition Action = MakeWeaponAction(
			TEXT("Action_Rogue_SneakAttack"), TEXT("Attaque sournoise"),
			TEXT("125 % WD avec une arme légère ; 175 % si la cible n'a pas encore agi, subit un contrôle physique ou expose son arc arrière."),
			TEXT("Talent_Rogue_Assassin_SneakAttack"), 2, 1, 125);
		ClassAsset.CombatActions.Add(Action);
	}
	{
		FGridCombatActionDefinition Action = MakeWeaponAction(
			TEXT("Action_Rogue_Hemorrhage"), TEXT("Hémorragie"),
			TEXT("100 % WD tranchant ou perforant ; applique Saignement si l'armure physique est épuisée."),
			TEXT("Talent_Rogue_Assassin_Hemorrhage"), 2, 2, 100);
		Action.WeaponAttackProfile.AllowedPhysicalSubtypes = {
			EGridPhysicalDamageSubtype::Slashing,
			EGridPhysicalDamageSubtype::Piercing
		};
		Action.StatusApplications.Add(MakeStatusApplication(
			TEXT("Status_Bleeding"), EGridCombatStatusApplicationTrigger::AfterSuccessfulHit,
			EGridCombatStatusArmorGate::PhysicalArmorDepleted, 3));
		ClassAsset.CombatActions.Add(Action);
	}
	{
		FGridCombatActionDefinition Action = MakeEffectAction(
			TEXT("Action_Rogue_WeakPoint"), TEXT("Point faible"),
			TEXT("Expose une cible dont l'armure physique est épuisée : dégâts physiques reçus +20 % pendant 2 rounds."),
			TEXT("Talent_Rogue_Assassin_WeakPoint"), 1, EGridCombatTargetingPolicy::Hostile, 1, 3);
		Action.TargetFilter.bRequirePhysicalArmorDepleted = true;
		Action.StatusApplications.Add(MakeStatusApplication(
			TEXT("Status_ExposedPhysical"), EGridCombatStatusApplicationTrigger::AfterResolution,
			EGridCombatStatusArmorGate::None, 2));
		ClassAsset.CombatActions.Add(Action);
	}
	{
		FGridCombatActionDefinition Action = MakeWeaponAction(
			TEXT("Action_Rogue_Finisher"), TEXT("Finisseur"),
			TEXT("220 % WD contre une cible sans armure physique et à 30 % de ses PV maximum ou moins."),
			TEXT("Talent_Rogue_Assassin_Finisher"), 3, 4, 220);
		Action.TargetFilter.bRequirePhysicalArmorDepleted = true;
		Action.TargetFilter.MaximumHealthPercent = 30;
		ClassAsset.CombatActions.Add(Action);
	}

	// Shadow actions.
	{
		FGridCombatActionDefinition Action = MakeEffectAction(
			TEXT("Action_Rogue_Dodge"), TEXT("Esquive"),
			TEXT("Applique Esquive pendant 1 round."), TEXT("Talent_Rogue_Shadow_Dodge"),
			1, EGridCombatTargetingPolicy::Self, 0, 3);
		Action.StatusApplications.Add(MakeStatusApplication(
			TEXT("Status_Evasive"), EGridCombatStatusApplicationTrigger::AfterResolution, EGridCombatStatusArmorGate::None, 1));
		ClassAsset.CombatActions.Add(Action);
	}
	{
		FGridCombatActionDefinition Action = MakeEffectAction(
			TEXT("Action_Rogue_ShortVanish"), TEXT("Disparition courte"),
			TEXT("Devient caché jusqu'à la prochaine activation ou la première action offensive."),
			TEXT("Talent_Rogue_Shadow_ShortVanish"), 2, EGridCombatTargetingPolicy::Self, 0, 4);
		Action.StatusApplications.Add(MakeStatusApplication(
			TEXT("Status_Hidden"), EGridCombatStatusApplicationTrigger::AfterResolution));
		ClassAsset.CombatActions.Add(Action);
	}
	{
		FGridCombatActionDefinition Action = MakeEffectAction(
			TEXT("Action_Rogue_ShadowStep"), TEXT("Pas de l'ombre"),
			TEXT("La prochaine attaque de mêlée avec arme légère gagne +1 cellule de portée et consomme l'effet."),
			TEXT("Talent_Rogue_Shadow_ShadowStep"), 1, EGridCombatTargetingPolicy::Self, 0, 2);
		Action.StatusApplications.Add(MakeStatusApplication(
			TEXT("Status_ShadowReach"), EGridCombatStatusApplicationTrigger::AfterResolution, EGridCombatStatusArmorGate::None, 1));
		ClassAsset.CombatActions.Add(Action);
	}
	{
		FGridCombatActionDefinition Action = MakeEffectAction(
			TEXT("Action_Rogue_PerfectShadow"), TEXT("Ombre parfaite"),
			TEXT("Devient caché pendant 2 rounds ; la première offense gagne +50 % dégâts et Accuracy +2 puis rompt l'effet."),
			TEXT("Talent_Rogue_Shadow_PerfectShadow"), 2, EGridCombatTargetingPolicy::Self, 0, 5);
		Action.StatusApplications.Add(MakeStatusApplication(
			TEXT("Status_HiddenPerfect"), EGridCombatStatusApplicationTrigger::AfterResolution,
			EGridCombatStatusArmorGate::None, 2));
		ClassAsset.CombatActions.Add(Action);
	}

	// Saboteur actions.
	{
		FGridCombatActionDefinition Action = MakeEffectAction(
			TEXT("Action_Rogue_QuickTrap"), TEXT("Piège rapide"),
			TEXT("Pose un piège temporaire ; le premier hostile entrant subit 6 + mod DEX dégâts perforants."),
			TEXT("Talent_Rogue_Saboteur_QuickTrap"), 2, EGridCombatTargetingPolicy::Cell, 1, 2);
		Action.TrapEffect.bPlaceTrap = true;
		Action.TrapEffect.TrapId = TEXT("Trap_Quick");
		Action.TrapEffect.DurationRounds = 3;
		Action.TrapEffect.BaseDamage = 6;
		Action.TrapEffect.DamageType = EGridDamageType::Physical;
		Action.TrapEffect.PhysicalSubtype = EGridPhysicalDamageSubtype::Piercing;
		Action.TrapEffect.DamageScalingAttribute = EGridAttackScalingAttribute::Dexterity;
		Action.TrapEffect.bConsumeOnTrigger = true;
		Action.TrapEffect.StatusApplications.Add(MakeStatusApplication(
			TEXT("Status_Immobilized"), EGridCombatStatusApplicationTrigger::AfterSuccessfulHit,
			EGridCombatStatusArmorGate::PhysicalArmorDepleted, 1));
		ClassAsset.CombatActions.Add(Action);
	}
	{
		FGridCombatActionDefinition Action = MakeEffectAction(
			TEXT("Action_Rogue_SmokeBomb"), TEXT("Bombe fumigène"),
			TEXT("Crée une zone de fumée de rayon 1 pendant 2 rounds."),
			TEXT("Talent_Rogue_Saboteur_SmokeBomb"), 2, EGridCombatTargetingPolicy::Area, 3, 3);
		Action.AreaRadiusCells = 1;
		FGridCombatSurfaceEffectProfile Smoke;
		Smoke.SurfaceType = EGridCombatSurfaceType::Smoke;
		Smoke.DurationRounds = 2;
		Action.SurfaceEffects.Add(Smoke);
		ClassAsset.CombatActions.Add(Action);
	}
	{
		FGridCombatActionDefinition Action = MakeEffectAction(
			TEXT("Action_Rogue_Sabotage"), TEXT("Sabotage"),
			TEXT("Teste Mécanique contre le DD de la cible Mechanical/Construct ; succès : Saboté pendant 2 rounds."),
			TEXT("Talent_Rogue_Saboteur_Sabotage"), 2, EGridCombatTargetingPolicy::Hostile, 1, 3);
		Action.TargetFilter.AllowedMonsterCategoryIds = { TEXT("Mechanical"), TEXT("Construct") };
		Action.SkillCheck.SkillId = TEXT("Skill_Mechanics");
		Action.SkillCheck.bUseTargetDifficulty = true;
		Action.StatusApplications.Add(MakeStatusApplication(
			TEXT("Status_Sabotaged"), EGridCombatStatusApplicationTrigger::AfterResolution,
			EGridCombatStatusArmorGate::None, 2));
		ClassAsset.CombatActions.Add(Action);
	}

	// Assassin progression.
	FRPGClassProgressionChoiceDefinition SneakAttack = MakeChoice(
		TEXT("Talent_Rogue_Assassin_SneakAttack"), TEXT("Attaque sournoise"),
		TEXT("Requiert arme légère. 125 % dégâts de l'arme. Si la cible n'a pas encore agi ce round, subit un contrôle physique, ou si le groupe se trouve dans son arc arrière : 175 % dégâts de l'arme à la place."), 2, AssassinBranchId);
	{
		FGridCombatModifierProfile Modifier;
		Modifier.ActionIds = { TEXT("Action_Rogue_SneakAttack") };
		Modifier.AnyTargetConditions = {
			EGridCombatTargetCondition::HasNotActedThisRound,
			EGridCombatTargetCondition::PhysicalControl,
			EGridCombatTargetCondition::RearArc
		};
		Modifier.WeaponDamagePercentModifier = 50;
		SneakAttack.CombatModifiers.Add(Modifier);
	}
	ClassAsset.ProgressionChoices.Add(SneakAttack);

	FRPGClassProgressionChoiceDefinition Backstab = MakeChoice(
		TEXT("Talent_Rogue_Assassin_Backstab"), TEXT("Frappe dans le dos"),
		TEXT("Avec arme légère contre une cible dont le groupe occupe l'arc arrière : dégâts +20 % et chance de critique +20 points. Ne s'applique pas aux AoE."),
		6, AssassinBranchId, TEXT("Talent_Rogue_Assassin_SneakAttack"));
	{
		FGridCombatModifierProfile Modifier;
		Modifier.RequiredSourceTags = { LightWeaponTag };
		Modifier.ActionTypes = { EGridCombatActionType::MeleeAttack };
		Modifier.RequiredTargetConditions = { EGridCombatTargetCondition::RearArc };
		Modifier.bExcludeAreaActions = true;
		Modifier.OutgoingDamagePercentModifier = 20;
		Modifier.CriticalChancePercentModifier = 20;
		Backstab.CombatModifiers.Add(Modifier);
	}
	ClassAsset.ProgressionChoices.Add(Backstab);
	ClassAsset.ProgressionChoices.Add(MakeChoice(
		TEXT("Talent_Rogue_Assassin_Hemorrhage"), TEXT("Hémorragie"),
		TEXT("100 % dégâts de l'arme Tranchant/Perforant. Si armure physique=0 après dégâts, applique Status_Bleeding 3 tours : 2 dégâts Physical par tick, la réapplication rafraîchit la durée, MaxStacks=1."), 10, AssassinBranchId, TEXT("Talent_Rogue_Assassin_Backstab")));
	ClassAsset.ProgressionChoices.Add(MakeChoice(
		TEXT("Talent_Rogue_Assassin_WeakPoint"), TEXT("Point faible"),
		TEXT("Aucun dégât. Disponible seulement si armure physique=0. Applique Status_ExposedPhysical 2 rounds : dégâts Physical reçus +20 %."), 14, AssassinBranchId, TEXT("Talent_Rogue_Assassin_Hemorrhage")));
	ClassAsset.ProgressionChoices.Add(MakeChoice(
		TEXT("Talent_Rogue_Assassin_Finisher"), TEXT("Finisseur"),
		TEXT("Disponible seulement si armure physique=0 et PV <=30 % PV maximum. Inflige 220 % dégâts de l'arme. Si la cible survit, aucun effet secondaire automatique."), 18, AssassinBranchId, TEXT("Talent_Rogue_Assassin_WeakPoint")));

	// Shadow progression.
	ClassAsset.ProgressionChoices.Add(MakeChoice(
		TEXT("Talent_Rogue_Shadow_Dodge"), TEXT("Esquive"),
		TEXT("Applique Status_Evasive 1 round : Esquive +4. la réapplication rafraîchit la durée, non cumulable."), 2, ShadowBranchId));
	ClassAsset.ProgressionChoices.Add(MakeChoice(
		TEXT("Talent_Rogue_Shadow_ShortVanish"), TEXT("Disparition courte"),
		TEXT("Applique Status_Hidden jusqu'au début du prochain tour du Voleur ou jusqu'à sa première action offensive. Les attaques ciblées hostiles ne peuvent pas le sélectionner ; AoE/DoT/surfaces continuent de l'affecter."), 6, ShadowBranchId, TEXT("Talent_Rogue_Shadow_Dodge")));
	ClassAsset.ProgressionChoices.Add(MakeChoice(
		TEXT("Talent_Rogue_Shadow_ShadowStep"), TEXT("Pas de l'ombre"),
		TEXT("Applique Status_ShadowReach jusqu'à la fin du tour : la prochaine attaque de mêlée avec arme légère peut être exécutée depuis le rang arrière et gagne +1 cellule de portée ; l'effet est consommé à l'attaque."), 10, ShadowBranchId, TEXT("Talent_Rogue_Shadow_ShortVanish")));

	FRPGClassProgressionChoiceDefinition Elusive = MakeChoice(
		TEXT("Talent_Rogue_Shadow_Elusive"), TEXT("Insaisissable"),
		TEXT("Après utilisation réussie d'Esquive, Disparition courte ou Pas de l'ombre, applique Status_Elusive 1 round : Esquive +2 et InitiativeModifier +4. Ne se cumule pas avec lui-même."),
		14, ShadowBranchId, TEXT("Talent_Rogue_Shadow_ShadowStep"));
	{
		FGridCombatReactionProfile Reaction;
		Reaction.ReactionId = TEXT("Reaction_Rogue_Elusive");
		Reaction.Trigger = EGridCombatReactionTrigger::ActionResolved;
		Reaction.Limit = EGridCombatReactionLimit::OncePerAction;
		Reaction.ActionIds = {
			TEXT("Action_Rogue_Dodge"),
			TEXT("Action_Rogue_ShortVanish"),
			TEXT("Action_Rogue_ShadowStep")
		};
		Reaction.ApplyOwnerStatusEffectId = TEXT("Status_Elusive");
		Reaction.ApplyOwnerStatusDurationOverride = 1;
		Elusive.CombatReactions.Add(Reaction);
	}
	ClassAsset.ProgressionChoices.Add(Elusive);
	ClassAsset.ProgressionChoices.Add(MakeChoice(
		TEXT("Talent_Rogue_Shadow_PerfectShadow"), TEXT("Ombre parfaite"),
		TEXT("Status_HiddenPerfect 2 rounds. La première action offensive bénéficie de +50 % dégâts et Précision +2 puis rompt l'effet ; recevoir des dégâts directs rompt aussi l'effet après résolution."), 18, ShadowBranchId, TEXT("Talent_Rogue_Shadow_Elusive")));

	// Saboteur progression.
	FRPGClassProgressionChoiceDefinition ExpertDisarm = MakeChoice(
		TEXT("Talent_Rogue_Saboteur_ExpertDisarm"), TEXT("Désamorçage expert"),
		TEXT("Jets de Pièges / désamorçage +2. 1 fois par piège, un échec de 1 ou 2 points devient un échec sûr : le piège reste armé mais ne se déclenche pas."),
		2, SaboteurBranchId);
	{
		FRPGSkillProgressionModifier Modifier;
		Modifier.SkillId = TEXT("Skill_Traps");
		Modifier.CheckModifier = 2;
		Modifier.SafeFailureMargin = 2;
		ExpertDisarm.SkillModifiers.Add(Modifier);
	}
	ClassAsset.ProgressionChoices.Add(ExpertDisarm);
	ClassAsset.ProgressionChoices.Add(MakeChoice(
		TEXT("Talent_Rogue_Saboteur_QuickTrap"), TEXT("Piège rapide"),
		TEXT("Pose Trap_Quick pour 3 rounds. Premier hostile entrant : dégâts Physical Piercing = 6 + DEX mod ; si armure physique=0 après dégâts, Status_Immobilized 1 round. Le piège est ensuite consommé."), 6, SaboteurBranchId, TEXT("Talent_Rogue_Saboteur_ExpertDisarm")));
	ClassAsset.ProgressionChoices.Add(MakeChoice(
		TEXT("Talent_Rogue_Saboteur_SmokeBomb"), TEXT("Bombe fumigène"),
		TEXT("Crée Surface_Smoke 2 rounds. Une ligne de tir traversant la fumée invalide les attaques/spells ciblés à distance ; un occupant de la fumée gagne Esquive +2 contre les attaques à distance."), 10, SaboteurBranchId, TEXT("Talent_Rogue_Saboteur_QuickTrap")));

	FRPGClassProgressionChoiceDefinition MasterLocksmith = MakeChoice(
		TEXT("Talent_Rogue_Saboteur_MasterLocksmith"), TEXT("Maître des serrures"),
		TEXT("Jets de Crochetage +2. Pour les prérequis de Crochetage, le rang effectif vaut rang +1, plafonné à 5. Un échec de 2 points ou moins ne bloque ni ne coince jamais la serrure."),
		14, SaboteurBranchId, TEXT("Talent_Rogue_Saboteur_SmokeBomb"));
	{
		FRPGSkillProgressionModifier Modifier;
		Modifier.SkillId = TEXT("Skill_Lockpicking");
		Modifier.CheckModifier = 2;
		Modifier.RequirementGrantRankModifier = 1;
		Modifier.SafeFailureMargin = 2;
		MasterLocksmith.SkillModifiers.Add(Modifier);
	}
	ClassAsset.ProgressionChoices.Add(MasterLocksmith);
	ClassAsset.ProgressionChoices.Add(MakeChoice(
		TEXT("Talent_Rogue_Saboteur_Sabotage"), TEXT("Sabotage"),
		TEXT("Sur une cible Mechanical ou Construct : effectue un test Intelligence + Mécanique contre sa difficulté. En cas de succès, applique Saboté pendant 2 rounds : Précision -2 et Initiative -4. Hors combat, un mécanisme explicitement sabotable reçoit son événement de sabotage."), 18, SaboteurBranchId, TEXT("Talent_Rogue_Saboteur_MasterLocksmith")));
}

bool FRPGRogueAuthoring::ConfigureStatus(UGridStatusEffectDefinitionAsset& StatusAsset, FName EffectId)
{
	using namespace RPGRogueAuthoring;

	StatusAsset.EffectId = EffectId;
	StatusAsset.StatusTags.Reset();
	StatusAsset.Icon.Reset();
	StatusAsset.DefaultPotency = 0;
	StatusAsset.StackPolicy = EGridStatusEffectStackPolicy::RefreshDuration;
	StatusAsset.MaxStacks = 1;
	StatusAsset.PeriodicDamage = FGridStatusEffectPeriodicDamageProfile();
	StatusAsset.InitiativeModifier = 0;
	StatusAsset.Control = FGridStatusEffectControlProfile();
	StatusAsset.CombatModifiers.Reset();
	StatusAsset.CombatReactions.Reset();
	StatusAsset.bExpireAtOwnerNextActivation = false;

	if (EffectId == TEXT("Status_Bleeding"))
	{
		StatusAsset.DisplayName = FText::FromString(TEXT("Saignement"));
		StatusAsset.Description = FText::FromString(TEXT("Subit 2 dégâts physiques à chaque tick pendant 3 tours."));
		StatusAsset.Disposition = EGridStatusEffectDisposition::Debuff;
		StatusAsset.DurationUnit = EGridStatusEffectDurationUnit::Turns;
		StatusAsset.DefaultDuration = 3;
		StatusAsset.PeriodicDamage.DamageType = EGridDamageType::Physical;
		StatusAsset.PeriodicDamage.DamagePerStack = 2;
		return true;
	}
	if (EffectId == TEXT("Status_ExposedPhysical"))
	{
		StatusAsset.DisplayName = FText::FromString(TEXT("Exposé physiquement"));
		StatusAsset.Description = FText::FromString(TEXT("Dégâts physiques reçus +20 %."));
		StatusAsset.Disposition = EGridStatusEffectDisposition::Debuff;
		StatusAsset.DurationUnit = EGridStatusEffectDurationUnit::Rounds;
		StatusAsset.DefaultDuration = 2;
		FGridCombatModifierProfile Modifier;
		Modifier.DamageTypes = { EGridDamageType::Physical };
		Modifier.IncomingDamagePercentModifier = 20;
		StatusAsset.CombatModifiers.Add(Modifier);
		return true;
	}
	if (EffectId == TEXT("Status_Evasive"))
	{
		StatusAsset.DisplayName = FText::FromString(TEXT("Esquive"));
		StatusAsset.Description = FText::FromString(TEXT("Evasion +4."));
		StatusAsset.Disposition = EGridStatusEffectDisposition::Buff;
		StatusAsset.DurationUnit = EGridStatusEffectDurationUnit::Rounds;
		StatusAsset.DefaultDuration = 1;
		FGridCombatModifierProfile Modifier;
		Modifier.EvasionModifier = 4;
		StatusAsset.CombatModifiers.Add(Modifier);
		return true;
	}
	if (EffectId == TEXT("Status_Hidden"))
	{
		StatusAsset.DisplayName = FText::FromString(TEXT("Caché"));
		StatusAsset.Description = FText::FromString(TEXT("Non ciblable directement jusqu'à la prochaine activation ou première action offensive."));
		StatusAsset.Disposition = EGridStatusEffectDisposition::Buff;
		StatusAsset.DurationUnit = EGridStatusEffectDurationUnit::Permanent;
		StatusAsset.DefaultDuration = 0;
		StatusAsset.bExpireAtOwnerNextActivation = true;
		StatusAsset.Control.bBlockDirectHostileTargeting = true;

		FGridCombatReactionProfile Break;
		Break.ReactionId = TEXT("Reaction_Rogue_HiddenBreak");
		Break.Trigger = EGridCombatReactionTrigger::ActionResolved;
		Break.Limit = EGridCombatReactionLimit::OncePerAction;
		Break.bRequireOffensiveAction = true;
		Break.bConsumeOwningStatus = true;
		StatusAsset.CombatReactions.Add(Break);
		return true;
	}
	if (EffectId == TEXT("Status_ShadowReach"))
	{
		StatusAsset.DisplayName = FText::FromString(TEXT("Allonge de l'ombre"));
		StatusAsset.Description = FText::FromString(TEXT("La prochaine attaque de mêlée avec arme légère gagne +1 cellule de portée."));
		StatusAsset.Disposition = EGridStatusEffectDisposition::Buff;
		StatusAsset.DurationUnit = EGridStatusEffectDurationUnit::Turns;
		StatusAsset.DefaultDuration = 1;

		FGridCombatModifierProfile Range;
		Range.RequiredSourceTags = { LightWeaponTag };
		Range.ActionTypes = { EGridCombatActionType::MeleeAttack };
		Range.RangeCellsModifier = 1;
		StatusAsset.CombatModifiers.Add(Range);

		FGridCombatReactionProfile Consume;
		Consume.ReactionId = TEXT("Reaction_Rogue_ShadowReachConsume");
		Consume.Trigger = EGridCombatReactionTrigger::ActionResolved;
		Consume.Limit = EGridCombatReactionLimit::OncePerAction;
		Consume.ActionTypes = { EGridCombatActionType::MeleeAttack };
		Consume.RequiredSourceTags = { LightWeaponTag };
		Consume.bRequireOffensiveAction = true;
		Consume.bConsumeOwningStatus = true;
		StatusAsset.CombatReactions.Add(Consume);
		return true;
	}
	if (EffectId == TEXT("Status_Elusive"))
	{
		StatusAsset.DisplayName = FText::FromString(TEXT("Insaisissable"));
		StatusAsset.Description = FText::FromString(TEXT("Evasion +2 et Initiative +4."));
		StatusAsset.Disposition = EGridStatusEffectDisposition::Buff;
		StatusAsset.DurationUnit = EGridStatusEffectDurationUnit::Rounds;
		StatusAsset.DefaultDuration = 1;
		StatusAsset.InitiativeModifier = 4;
		FGridCombatModifierProfile Modifier;
		Modifier.EvasionModifier = 2;
		StatusAsset.CombatModifiers.Add(Modifier);
		return true;
	}
	if (EffectId == TEXT("Status_HiddenPerfect"))
	{
		StatusAsset.DisplayName = FText::FromString(TEXT("Ombre parfaite"));
		StatusAsset.Description = FText::FromString(TEXT("Caché ; première offense : dégâts +50 % et Accuracy +2, puis rupture. Les dégâts directs rompent aussi l'effet."));
		StatusAsset.Disposition = EGridStatusEffectDisposition::Buff;
		StatusAsset.DurationUnit = EGridStatusEffectDurationUnit::Rounds;
		StatusAsset.DefaultDuration = 2;
		StatusAsset.Control.bBlockDirectHostileTargeting = true;

		FGridCombatModifierProfile Offense;
		Offense.ActionTypes = { EGridCombatActionType::MeleeAttack, EGridCombatActionType::RangedAttack };
		Offense.AccuracyModifier = 2;
		Offense.OutgoingDamagePercentModifier = 50;
		StatusAsset.CombatModifiers.Add(Offense);

		FGridCombatReactionProfile BreakOnOffense;
		BreakOnOffense.ReactionId = TEXT("Reaction_Rogue_HiddenPerfectOffenseBreak");
		BreakOnOffense.Trigger = EGridCombatReactionTrigger::ActionResolved;
		BreakOnOffense.Limit = EGridCombatReactionLimit::OncePerAction;
		BreakOnOffense.bRequireOffensiveAction = true;
		BreakOnOffense.bConsumeOwningStatus = true;
		StatusAsset.CombatReactions.Add(BreakOnOffense);

		FGridCombatReactionProfile BreakOnDamage;
		BreakOnDamage.ReactionId = TEXT("Reaction_Rogue_HiddenPerfectDamageBreak");
		BreakOnDamage.Trigger = EGridCombatReactionTrigger::DirectDamageReceived;
		BreakOnDamage.Limit = EGridCombatReactionLimit::OncePerAction;
		BreakOnDamage.bConsumeOwningStatus = true;
		StatusAsset.CombatReactions.Add(BreakOnDamage);
		return true;
	}
	if (EffectId == TEXT("Status_Immobilized"))
	{
		StatusAsset.DisplayName = FText::FromString(TEXT("Immobilisé"));
		StatusAsset.Description = FText::FromString(TEXT("La translation volontaire est bloquée pendant 1 round."));
		StatusAsset.StatusTags = { TEXT("Control.Physical"), TEXT("Control.Immobilize") };
		StatusAsset.Disposition = EGridStatusEffectDisposition::Debuff;
		StatusAsset.DurationUnit = EGridStatusEffectDurationUnit::Rounds;
		StatusAsset.DefaultDuration = 1;
		StatusAsset.Control.bBlockTranslation = true;
		return true;
	}
	if (EffectId == TEXT("Status_Sabotaged"))
	{
		StatusAsset.DisplayName = FText::FromString(TEXT("Saboté"));
		StatusAsset.Description = FText::FromString(TEXT("Accuracy -2 et Initiative -4."));
		StatusAsset.Disposition = EGridStatusEffectDisposition::Debuff;
		StatusAsset.DurationUnit = EGridStatusEffectDurationUnit::Rounds;
		StatusAsset.DefaultDuration = 2;
		StatusAsset.InitiativeModifier = -4;
		FGridCombatModifierProfile Modifier;
		Modifier.AccuracyModifier = -2;
		StatusAsset.CombatModifiers.Add(Modifier);
		return true;
	}
	return false;
}

bool FRPGRogueAuthoring::AuthorProductionAssets(FString& OutError)
{
	OutError.Reset();

	URPGClassAsset* Rogue = LoadObject<URPGClassAsset>(nullptr, RogueAssetPath());
	if (!IsValid(Rogue))
	{
		OutError = FString::Printf(TEXT("Production Rogue asset not found: %s"), RogueAssetPath());
		return false;
	}
	if (Rogue->ClassId != RPGRogueAuthoring::RogueClassId)
	{
		OutError = FString::Printf(TEXT("Unexpected Rogue ClassId '%s'."), *Rogue->ClassId.ToString());
		return false;
	}

	Rogue->Modify();
	ConfigureClass(*Rogue);
	if (!Rogue->IsValidDefinition())
	{
		OutError = TEXT("Authored DA_Class_Rogue is structurally invalid.");
		return false;
	}

	TArray<UGridStatusEffectDefinitionAsset*> StatusAssets;
	TArray<FName> StatusIds;
	GetRequiredStatusIds(StatusIds);
	for (const FName StatusId : StatusIds)
	{
		UGridStatusEffectDefinitionAsset* Status = RPGRogueAuthoring::FindOrCreateStatus(StatusId, OutError);
		if (!IsValid(Status))
		{
			return false;
		}
		Status->Modify();
		if (!ConfigureStatus(*Status, StatusId))
		{
			OutError = FString::Printf(TEXT("No Rogue status authoring definition for '%s'."), *StatusId.ToString());
			return false;
		}

		FString StatusError;
		if (!Status->ValidateDefinition(StatusError))
		{
			OutError = FString::Printf(TEXT("Authored status '%s' is invalid: %s"), *StatusId.ToString(), *StatusError);
			return false;
		}
		StatusAssets.Add(Status);
	}

	for (UGridStatusEffectDefinitionAsset* Status : StatusAssets)
	{
		if (!RPGRogueAuthoring::SaveAuthoredAsset(Status, OutError))
		{
			return false;
		}
	}
	if (!RPGRogueAuthoring::SaveAuthoredAsset(Rogue, OutError))
	{
		return false;
	}
	return true;
}
