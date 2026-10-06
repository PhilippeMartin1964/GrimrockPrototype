#include "RPG/RPGRangerAuthoring.h"

#include "AssetRegistry/ARFilter.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "HAL/FileManager.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

namespace RPGRangerAuthoring
{
	const FName RangerClassId(TEXT("Ranger"));
	const FName MarkStatusId(TEXT("Status_MarkedByRanger"));
	const FName ImmobilizedStatusId(TEXT("Status_Immobilized"));
	const FName FavoredEnemyAlias(TEXT("Talent_Ranger_Hunter_FavoredEnemy"));
	const FName FavoredEnemyGroup(TEXT("TalentGroup_Ranger_Hunter_FavoredEnemy"));

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

	FGridCombatArmorEffectProfile MakeArmorDamageRawPercent(int32 Amount)
	{
		FGridCombatArmorEffectProfile Profile;
		Profile.Pool = EGridCombatArmorPool::Physical;
		Profile.Operation = EGridCombatArmorEffectOperation::Damage;
		Profile.Magnitude = EGridCombatArmorEffectMagnitude::RawDamagePercent;
		Profile.Trigger = EGridCombatArmorEffectTrigger::AfterSuccessfulHit;
		Profile.Amount = Amount;
		return Profile;
	}

	FRPGClassProgressionChoiceDefinition MakeChoice(FName ChoiceId, const TCHAR* DisplayName, const TCHAR* Description,
		int32 MinimumLevel, FName PrerequisiteChoiceId = NAME_None)
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

	FGridCombatActionDefinition MakeRangedWeaponAction(FName ActionId, const TCHAR* DisplayName, const TCHAR* Description,
		FName RequirementId, int32 ActionPointCost, EGridCombatTargetingPolicy TargetingPolicy, int32 RangeCells, int32 CooldownRounds,
		int32 WeaponDamagePercent, bool bUseWeaponRange = true, int32 WeaponRangeModifier = 0)
	{
		FGridCombatActionDefinition Action;
		Action.ActionId = ActionId;
		Action.DisplayName = FText::FromString(DisplayName);
		Action.Description = FText::FromString(Description);
		Action.ActionType = EGridCombatActionType::RangedAttack;
		Action.SourcePolicy = EGridCombatActionSourcePolicy::Ability;
		Action.TargetingPolicy = TargetingPolicy;
		Action.ResolutionProfile = EGridCombatActionResolutionProfile::Attack;
		Action.ActionPointCost = ActionPointCost;
		Action.RangeCells = RangeCells;
		Action.CooldownRounds = CooldownRounds;
		Action.Requirements = { RequirementId };
		Action.WeaponAttackProfile.bUseEquippedWeapon = true;
		Action.WeaponAttackProfile.bRequireRangedWeapon = true;
		Action.WeaponAttackProfile.bUseWeaponRange = bUseWeaponRange;
		Action.WeaponAttackProfile.WeaponRangeModifier = bUseWeaponRange ? WeaponRangeModifier : 0;
		Action.WeaponAttackProfile.WeaponDamagePercent = WeaponDamagePercent;
		return Action;
	}

	FString SanitizeCategoryForChoiceId(FName CategoryId)
	{
		const FString Source = CategoryId.ToString();
		FString Result;
		Result.Reserve(Source.Len());
		for (const TCHAR Character : Source)
		{
			Result.AppendChar(FChar::IsAlnum(Character) ? Character : TEXT('_'));
		}
		return Result.IsEmpty() ? TEXT("Category") : Result;
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

	UGridStatusEffectDefinitionAsset* FindOrCreateMarkedStatus(FString& OutError)
	{
		if (UGridStatusEffectDefinitionAsset* Existing =
				LoadObject<UGridStatusEffectDefinitionAsset>(nullptr, FRPGRangerAuthoring::MarkedStatusPath()))
		{
			return Existing;
		}

		const FString AssetName(TEXT("DA_Status_MarkedByRanger"));
		const FString PackageName(TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/StatusEffects/DA_Status_MarkedByRanger"));
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
			OutError = TEXT("Failed to create DA_Status_MarkedByRanger.");
			return nullptr;
		}
		FAssetRegistryModule::AssetCreated(Created);
		return Created;
	}
}

const TCHAR* FRPGRangerAuthoring::RangerAssetPath()
{
	return TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Ranger.DA_Class_Ranger");
}

const TCHAR* FRPGRangerAuthoring::MarkedStatusPath()
{
	return TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/StatusEffects/DA_Status_MarkedByRanger.DA_Status_MarkedByRanger");
}

void FRPGRangerAuthoring::ConfigureClass(URPGClassAsset& ClassAsset, const TArray<FName>& FavoredEnemyCategoryIds)
{
	using namespace RPGRangerAuthoring;

	ClassAsset.CombatActions.Reset();
	ClassAsset.ProgressionChoices.Reset();

	// Marksman.
	{
		FGridCombatActionDefinition Action = MakeRangedWeaponAction(
			TEXT("Action_Ranger_PreciseShot"), TEXT("Tir précis"),
			TEXT("150 % WD, Accuracy +2, portée de l'arme +2."),
			TEXT("Talent_Ranger_Marksman_PreciseShot"), 3, EGridCombatTargetingPolicy::FirstAxialTarget, 1, 1, 150, true, 2);
		ClassAsset.CombatActions.Add(Action);
	}
	{
		FGridCombatActionDefinition Action = MakeRangedWeaponAction(
			TEXT("Action_Ranger_PiercingShot"), TEXT("Tir perforant"),
			TEXT("110 % WD perforant et 50 % du RawDamage supplémentaires contre l'armure physique uniquement."),
			TEXT("Talent_Ranger_Marksman_PiercingShot"), 2, EGridCombatTargetingPolicy::FirstAxialTarget, 1, 2, 110);
		Action.WeaponAttackProfile.bOverrideDamageDescriptor = true;
		Action.WeaponAttackProfile.OverrideDamageType = EGridDamageType::Physical;
		Action.WeaponAttackProfile.OverridePhysicalSubtype = EGridPhysicalDamageSubtype::Piercing;
		Action.ArmorEffects.Add(MakeArmorDamageRawPercent(50));
		ClassAsset.CombatActions.Add(Action);
	}
	{
		FGridCombatActionDefinition Action = MakeRangedWeaponAction(
			TEXT("Action_Ranger_RapidShot"), TEXT("Tir rapide"),
			TEXT("Deux attaques indépendantes à 65 % WD ; la seconde subit Accuracy -1."),
			TEXT("Talent_Ranger_Marksman_RapidShot"), 2, EGridCombatTargetingPolicy::FirstAxialTarget, 1, 2, 65);
		Action.ResolutionCount = 2;
		Action.SubsequentResolutionAccuracyModifier = -1;
		ClassAsset.CombatActions.Add(Action);
	}
	{
		FGridCombatActionDefinition Action = MakeRangedWeaponAction(
			TEXT("Action_Ranger_Volley"), TEXT("Volée"),
			TEXT("Chaque hostile de la zone reçoit une attaque indépendante à 80 % WD."),
			TEXT("Talent_Ranger_Marksman_Volley"), 3, EGridCombatTargetingPolicy::Area, 5, 3, 80, false);
		Action.AreaRadiusCells = 1;
		Action.bAffectsAlliesInArea = false;
		Action.bRequiresLineOfSight = true;
		ClassAsset.CombatActions.Add(Action);
	}

	// Hunter.
	{
		FGridCombatActionDefinition Action = MakeEffectAction(
			TEXT("Action_Ranger_MarkPrey"), TEXT("Marque de la proie"),
			TEXT("Marque une cible à portée 5 pendant 3 rounds."),
			TEXT("Talent_Ranger_Hunter_MarkPrey"), 1, EGridCombatTargetingPolicy::Hostile, 5, 1);
		Action.StatusApplications.Add(MakeStatusApplication(
			MarkStatusId, EGridCombatStatusApplicationTrigger::AfterResolution, EGridCombatStatusArmorGate::None, 3));
		ClassAsset.CombatActions.Add(Action);
	}
	{
		FGridCombatActionDefinition Action = MakeRangedWeaponAction(
			TEXT("Action_Ranger_PinningShot"), TEXT("Tir immobilisant"),
			TEXT("100 % WD ; si l'armure physique est épuisée après dégâts, immobilise 1 round."),
			TEXT("Talent_Ranger_Hunter_PinningShot"), 2, EGridCombatTargetingPolicy::FirstAxialTarget, 1, 2, 100);
		Action.StatusApplications.Add(MakeStatusApplication(
			ImmobilizedStatusId, EGridCombatStatusApplicationTrigger::AfterSuccessfulHit,
			EGridCombatStatusArmorGate::PhysicalArmorDepleted, 1));
		ClassAsset.CombatActions.Add(Action);
	}
	{
		FGridCombatActionDefinition Action = MakeRangedWeaponAction(
			TEXT("Action_Ranger_PredatorStrike"), TEXT("Frappe du prédateur"),
			TEXT("170 % WD, Accuracy +1, uniquement contre la propre cible marquée du Rôdeur."),
			TEXT("Talent_Ranger_Hunter_PredatorStrike"), 3, EGridCombatTargetingPolicy::FirstAxialTarget, 1, 2, 170);
		Action.TargetFilter.RequiredStatusEffectIds = { MarkStatusId };
		Action.TargetFilter.bRequiredStatusesFromSource = true;
		ClassAsset.CombatActions.Add(Action);
	}

	// Scout.
	{
		FGridCombatActionDefinition Action = MakeEffectAction(
			TEXT("Action_Ranger_HuntingTrap"), TEXT("Piège de chasse"),
			TEXT("Pose un piège de chasse temporaire sur une cellule adjacente."),
			TEXT("Talent_Ranger_Scout_HuntingTrap"), 2, EGridCombatTargetingPolicy::Cell, 1, 2);
		Action.TrapEffect.bPlaceTrap = true;
		Action.TrapEffect.TrapId = TEXT("Trap_Hunting");
		Action.TrapEffect.DurationRounds = 4;
		Action.TrapEffect.BaseDamage = 5;
		Action.TrapEffect.DamageType = EGridDamageType::Physical;
		Action.TrapEffect.PhysicalSubtype = EGridPhysicalDamageSubtype::Piercing;
		Action.TrapEffect.DamageScalingAttribute = EGridAttackScalingAttribute::Wisdom;
		Action.TrapEffect.bConsumeOnTrigger = true;
		Action.TrapEffect.StatusApplications.Add(MakeStatusApplication(
			ImmobilizedStatusId, EGridCombatStatusApplicationTrigger::AfterSuccessfulHit,
			EGridCombatStatusArmorGate::PhysicalArmorDepleted, 1));
		ClassAsset.CombatActions.Add(Action);
	}
	{
		FGridCombatActionDefinition Action = MakeEffectAction(
			TEXT("Action_Ranger_TacticalRetreat"), TEXT("Repli tactique"),
			TEXT("Déplace le groupe d'une cellule en arrière pour 1 PAM."),
			TEXT("Talent_Ranger_Scout_TacticalRetreat"), 1, EGridCombatTargetingPolicy::Self, 0, 3);
		FGridCombatMovementEffectProfile Movement;
		Movement.Subject = EGridCombatMovementSubject::PartyGroup;
		Movement.Direction = EGridCombatMovementDirection::BackwardFromFacing;
		Movement.DistanceCells = 1;
		Movement.MobilityActionPointCost = 1;
		Movement.bForced = false;
		Action.MovementEffects.Add(Movement);
		ClassAsset.CombatActions.Add(Action);
	}

	// Marksman progression.
	FRPGClassProgressionChoiceDefinition Precise = MakeChoice(
		TEXT("Talent_Ranger_Marksman_PreciseShot"), TEXT("Tir précis"), TEXT("Débloque Tir précis."), 2);
	{
		FGridCombatModifierProfile Modifier;
		Modifier.ActionIds = { TEXT("Action_Ranger_PreciseShot") };
		Modifier.AccuracyModifier = 2;
		Precise.CombatModifiers.Add(Modifier);
	}
	ClassAsset.ProgressionChoices.Add(Precise);
	ClassAsset.ProgressionChoices.Add(MakeChoice(
		TEXT("Talent_Ranger_Marksman_PiercingShot"), TEXT("Tir perforant"), TEXT("Débloque Tir perforant."),
		6, TEXT("Talent_Ranger_Marksman_PreciseShot")));
	ClassAsset.ProgressionChoices.Add(MakeChoice(
		TEXT("Talent_Ranger_Marksman_RapidShot"), TEXT("Tir rapide"), TEXT("Débloque Tir rapide."),
		10, TEXT("Talent_Ranger_Marksman_PiercingShot")));
	ClassAsset.ProgressionChoices.Add(MakeChoice(
		TEXT("Talent_Ranger_Marksman_Volley"), TEXT("Volée"), TEXT("Débloque Volée."),
		14, TEXT("Talent_Ranger_Marksman_RapidShot")));
	FRPGClassProgressionChoiceDefinition EagleEye = MakeChoice(
		TEXT("Talent_Ranger_Marksman_EagleEye"), TEXT("Œil d'aigle"),
		TEXT("Actions Ranged : portée +1, Accuracy +1 ; Perception en contexte distant +2."),
		18, TEXT("Talent_Ranger_Marksman_Volley"));
	{
		FGridCombatModifierProfile Modifier;
		Modifier.ActionTypes = { EGridCombatActionType::RangedAttack };
		Modifier.AccuracyModifier = 1;
		Modifier.RangeCellsModifier = 1;
		EagleEye.CombatModifiers.Add(Modifier);

		FRPGSkillProgressionModifier Skill;
		Skill.SkillId = TEXT("Skill_Perception");
		Skill.CheckModifier = 2;
		Skill.bRequireRangedContext = true;
		EagleEye.SkillModifiers.Add(Skill);
	}
	ClassAsset.ProgressionChoices.Add(EagleEye);

	// Hunter progression.
	FRPGClassProgressionChoiceDefinition MarkPrey = MakeChoice(
		TEXT("Talent_Ranger_Hunter_MarkPrey"), TEXT("Marque de la proie"),
		TEXT("Débloque Marque de la proie ; contre sa propre marque : Accuracy +2 et dégâts +15 %."), 2);
	{
		FGridCombatModifierProfile Modifier;
		Modifier.RequiredTargetStatusEffectIds = { MarkStatusId };
		Modifier.bRequiredTargetStatusesFromOwner = true;
		Modifier.AccuracyModifier = 2;
		Modifier.OutgoingDamagePercentModifier = 15;
		MarkPrey.CombatModifiers.Add(Modifier);
	}
	ClassAsset.ProgressionChoices.Add(MarkPrey);

	TArray<FName> Categories = FavoredEnemyCategoryIds;
	Categories.RemoveAll([](const FName Id) { return Id.IsNone(); });
	Categories.Sort([](const FName A, const FName B) { return A.ToString() < B.ToString(); });
	for (int32 Index = Categories.Num() - 1; Index > 0; --Index)
	{
		if (Categories[Index] == Categories[Index - 1])
		{
			Categories.RemoveAt(Index);
		}
	}
	for (const FName CategoryId : Categories)
	{
		const FString Suffix = SanitizeCategoryForChoiceId(CategoryId);
		const FName ChoiceId(*FString::Printf(TEXT("Talent_Ranger_Hunter_FavoredEnemy_%s"), *Suffix));
		const FString Display = FString::Printf(TEXT("Ennemi juré — %s"), *CategoryId.ToString());
		FRPGClassProgressionChoiceDefinition Choice = MakeChoice(
			ChoiceId, *Display, TEXT("Dégâts +15 % et tests liés +2 contre cette catégorie."),
			6, TEXT("Talent_Ranger_Hunter_MarkPrey"));
		Choice.ExclusiveChoiceGroupId = FavoredEnemyGroup;
		Choice.GrantedRequirementIds = { FavoredEnemyAlias };

		FGridCombatModifierProfile Damage;
		Damage.AllowedTargetMonsterCategoryIds = { CategoryId };
		Damage.OutgoingDamagePercentModifier = 15;
		Choice.CombatModifiers.Add(Damage);

		for (const FName SkillId : {
			FName(TEXT("Skill_Perception")), FName(TEXT("Skill_Nature")),
			FName(TEXT("Skill_History")), FName(TEXT("Skill_Religion"))
		})
		{
			FRPGSkillProgressionModifier Skill;
			Skill.SkillId = SkillId;
			Skill.CheckModifier = 2;
			Skill.RelatedMonsterCategoryIds = { CategoryId };
			Choice.SkillModifiers.Add(Skill);
		}
		ClassAsset.ProgressionChoices.Add(Choice);
	}

	FRPGClassProgressionChoiceDefinition Pinning = MakeChoice(
		TEXT("Talent_Ranger_Hunter_PinningShot"), TEXT("Tir immobilisant"),
		TEXT("Débloque Tir immobilisant."), 10);
	Pinning.PrerequisiteRequirementIds = { FavoredEnemyAlias };
	ClassAsset.ProgressionChoices.Add(Pinning);

	FRPGClassProgressionChoiceDefinition Predator = MakeChoice(
		TEXT("Talent_Ranger_Hunter_PredatorStrike"), TEXT("Frappe du prédateur"),
		TEXT("Débloque Frappe du prédateur et lui donne Accuracy +1."),
		14, TEXT("Talent_Ranger_Hunter_PinningShot"));
	{
		FGridCombatModifierProfile Modifier;
		Modifier.ActionIds = { TEXT("Action_Ranger_PredatorStrike") };
		Modifier.AccuracyModifier = 1;
		Predator.CombatModifiers.Add(Modifier);
	}
	ClassAsset.ProgressionChoices.Add(Predator);

	FRPGClassProgressionChoiceDefinition Alpha = MakeChoice(
		TEXT("Talent_Ranger_Hunter_AlphaHunter"), TEXT("Chasseur alpha"),
		TEXT("Une fois par round, transfère la propre marque du Rôdeur lorsqu'une cible marquée meurt."),
		18, TEXT("Talent_Ranger_Hunter_PredatorStrike"));
	{
		FGridCombatReactionProfile Reaction;
		Reaction.ReactionId = TEXT("Reaction_Ranger_AlphaHunter");
		Reaction.Trigger = EGridCombatReactionTrigger::OwnedStatusTargetDefeated;
		Reaction.Limit = EGridCombatReactionLimit::OncePerRound;
		Reaction.RequiredTargetStatusEffectIdsFromOwner = { MarkStatusId };
		Reaction.TransferOwnedTargetStatusEffectId = MarkStatusId;
		Reaction.TransferTargetRangeCells = 3;
		Reaction.TransferStatusDurationOverride = 2;
		Alpha.CombatReactions.Add(Reaction);
	}
	ClassAsset.ProgressionChoices.Add(Alpha);

	// Scout progression.
	FRPGClassProgressionChoiceDefinition Vigilance = MakeChoice(
		TEXT("Talent_Ranger_Scout_Vigilance"), TEXT("Vigilance"),
		TEXT("Meilleur jet de groupe de Perception +2 ; Initiative +2 au premier round."), 2);
	{
		FRPGPartyProgressionModifier Party;
		Party.StackingGroupId = TEXT("Party.Ranger.Vigilance");
		Party.GroupSkillIds = { TEXT("Skill_Perception") };
		Party.GroupSkillCheckModifier = 2;
		Vigilance.PartyModifiers.Add(Party);
		Vigilance.FirstRoundInitiativeModifier = 2;
	}
	ClassAsset.ProgressionChoices.Add(Vigilance);
	ClassAsset.ProgressionChoices.Add(MakeChoice(
		TEXT("Talent_Ranger_Scout_HuntingTrap"), TEXT("Piège de chasse"),
		TEXT("Débloque Piège de chasse."), 6, TEXT("Talent_Ranger_Scout_Vigilance")));
	ClassAsset.ProgressionChoices.Add(MakeChoice(
		TEXT("Talent_Ranger_Scout_TacticalRetreat"), TEXT("Repli tactique"),
		TEXT("Débloque Repli tactique."), 10, TEXT("Talent_Ranger_Scout_HuntingTrap")));

	FRPGClassProgressionChoiceDefinition Terrain = MakeChoice(
		TEXT("Talent_Ranger_Scout_TerrainMaster"), TEXT("Maître du terrain"),
		TEXT("Si le groupe n'a pas traduit depuis la précédente activation : Ranged dégâts +10 %, Accuracy +1."),
		14, TEXT("Talent_Ranger_Scout_TacticalRetreat"));
	{
		FGridCombatModifierProfile Modifier;
		Modifier.ActionTypes = { EGridCombatActionType::RangedAttack };
		Modifier.bRequirePartyStationarySincePreviousActivation = true;
		Modifier.AccuracyModifier = 1;
		Modifier.OutgoingDamagePercentModifier = 10;
		Terrain.CombatModifiers.Add(Modifier);
	}
	ClassAsset.ProgressionChoices.Add(Terrain);

	FRPGClassProgressionChoiceDefinition Guide = MakeChoice(
		TEXT("Talent_Ranger_Scout_GroupGuide"), TEXT("Guide du groupe"),
		TEXT("Meilleurs jets de groupe Perception/Survie +2 et PAM maximum +1 ; non cumulable."),
		18, TEXT("Talent_Ranger_Scout_TerrainMaster"));
	{
		FRPGPartyProgressionModifier Party;
		Party.StackingGroupId = TEXT("Party.Ranger.GroupGuide");
		Party.GroupSkillIds = { TEXT("Skill_Perception"), TEXT("Skill_Survival") };
		Party.GroupSkillCheckModifier = 2;
		Party.MaximumMobilityActionPointsModifier = 1;
		Guide.PartyModifiers.Add(Party);
	}
	ClassAsset.ProgressionChoices.Add(Guide);
}

bool FRPGRangerAuthoring::ConfigureMarkedStatus(UGridStatusEffectDefinitionAsset& StatusAsset)
{
	StatusAsset.EffectId = RPGRangerAuthoring::MarkStatusId;
	StatusAsset.DisplayName = FText::FromString(TEXT("Marqué par un Rôdeur"));
	StatusAsset.Description = FText::FromString(TEXT("Marque personnelle d'un Rôdeur ; plusieurs sources peuvent coexister sur une même cible."));
	StatusAsset.StatusTags = { TEXT("Marked") };
	StatusAsset.Icon.Reset();
	StatusAsset.Disposition = EGridStatusEffectDisposition::Debuff;
	StatusAsset.DurationUnit = EGridStatusEffectDurationUnit::Rounds;
	StatusAsset.DefaultDuration = 3;
	StatusAsset.bExpireAtOwnerNextActivation = false;
	StatusAsset.DefaultPotency = 0;
	StatusAsset.StackPolicy = EGridStatusEffectStackPolicy::RefreshDuration;
	StatusAsset.MaxStacks = 1;
	StatusAsset.bDistinctPerSource = true;
	StatusAsset.bUniquePerSourceAcrossMonsters = true;
	StatusAsset.PeriodicDamage = FGridStatusEffectPeriodicDamageProfile();
	StatusAsset.InitiativeModifier = 0;
	StatusAsset.Control = FGridStatusEffectControlProfile();
	StatusAsset.CombatModifiers.Reset();
	StatusAsset.CombatReactions.Reset();
	return StatusAsset.IsValidDefinition();
}

bool FRPGRangerAuthoring::CollectProductionFavoredEnemyCategories(TArray<FName>& OutCategoryIds, FString& OutError)
{
	OutCategoryIds.Reset();
	OutError.Reset();

	FAssetRegistryModule& RegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	FARFilter Filter;
	Filter.PackagePaths.Add(FName(TEXT("/Game/GrimrockPrototype/Monsters")));
	Filter.ClassPaths.Add(UGridMonsterDefinitionAsset::StaticClass()->GetClassPathName());
	Filter.bRecursivePaths = true;
	Filter.bRecursiveClasses = true;

	TArray<FAssetData> Assets;
	RegistryModule.Get().GetAssets(Filter, Assets);
	for (const FAssetData& AssetData : Assets)
	{
		const UGridMonsterDefinitionAsset* Definition = Cast<UGridMonsterDefinitionAsset>(AssetData.GetAsset());
		if (IsValid(Definition) && !Definition->CategoryId.IsNone())
		{
			OutCategoryIds.AddUnique(Definition->CategoryId);
		}
	}
	OutCategoryIds.Sort([](const FName A, const FName B) { return A.ToString() < B.ToString(); });
	if (OutCategoryIds.IsEmpty())
	{
		OutError = TEXT("No production monster CategoryId found for Favored Enemy authoring.");
		return false;
	}
	return true;
}

bool FRPGRangerAuthoring::AuthorProductionAssets(FString& OutError)
{
	OutError.Reset();

	TArray<FName> Categories;
	if (!CollectProductionFavoredEnemyCategories(Categories, OutError))
	{
		return false;
	}

	URPGClassAsset* Ranger = LoadObject<URPGClassAsset>(nullptr, RangerAssetPath());
	if (!IsValid(Ranger))
	{
		OutError = FString::Printf(TEXT("Production Ranger asset not found: %s"), RangerAssetPath());
		return false;
	}
	if (Ranger->ClassId != RPGRangerAuthoring::RangerClassId)
	{
		OutError = FString::Printf(TEXT("Unexpected Ranger ClassId '%s'."), *Ranger->ClassId.ToString());
		return false;
	}

	UGridStatusEffectDefinitionAsset* Immobilized = LoadObject<UGridStatusEffectDefinitionAsset>(
		nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/StatusEffects/DA_Status_Immobilized.DA_Status_Immobilized"));
	if (!IsValid(Immobilized) || !Immobilized->IsValidDefinition())
	{
		OutError = TEXT("Required production Status_Immobilized is unavailable or invalid.");
		return false;
	}

	UGridStatusEffectDefinitionAsset* Marked = RPGRangerAuthoring::FindOrCreateMarkedStatus(OutError);
	if (!IsValid(Marked))
	{
		return false;
	}
	Marked->Modify();
	if (!ConfigureMarkedStatus(*Marked))
	{
		OutError = TEXT("Authored Status_MarkedByRanger is structurally invalid.");
		return false;
	}

	Ranger->Modify();
	ConfigureClass(*Ranger, Categories);
	if (!Ranger->IsValidDefinition())
	{
		OutError = TEXT("Authored DA_Class_Ranger is structurally invalid.");
		return false;
	}

	if (!RPGRangerAuthoring::SaveAuthoredAsset(Marked, OutError) ||
		!RPGRangerAuthoring::SaveAuthoredAsset(Ranger, OutError))
	{
		return false;
	}
	return true;
}
