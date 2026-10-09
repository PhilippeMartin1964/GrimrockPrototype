#include "RPG/RPGRangerAuthoring.h"

#include "AssetRegistry/ARFilter.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "HAL/FileManager.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGClassProgressionAuthoring.h"
#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterCategoryAsset.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

namespace RPGRangerAuthoring
{
	const FName RangerClassId(TEXT("Ranger"));
	const FName MarksmanBranchId(TEXT("Marksman"));
	const FName HunterBranchId(TEXT("Hunter"));
	const FName ScoutBranchId(TEXT("Scout"));
	const FName MarkStatusId(TEXT("Status_MarkedByRanger"));
	const FName ImmobilizedStatusId(TEXT("Status_Immobilized"));
	const FName FavoredEnemyAlias(TEXT("Talent_Ranger_Hunter_FavoredEnemy"));
	const FName FavoredEnemyGroup(TEXT("TalentGroup_Ranger_Hunter_FavoredEnemy"));

	struct FProductionMonsterCategorySpec
	{
		FName CategoryId = NAME_None;
		const TCHAR* DisplayName = nullptr;
	};

	const FProductionMonsterCategorySpec ProductionCategorySpecs[] = {
		{ TEXT("Goblin"), TEXT("Gobelins") },
		{ TEXT("Vermin"), TEXT("Vermine") }
	};

	const FProductionMonsterCategorySpec* FindProductionCategorySpec(FName CategoryId)
	{
		for (const FProductionMonsterCategorySpec& Spec : ProductionCategorySpecs)
		{
			if (Spec.CategoryId == CategoryId)
			{
				return &Spec;
			}
		}
		return nullptr;
	}

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


	ERPGTalentPresentationType ResolveTalentPresentationType(FName ChoiceId)
	{
		const FString Id = ChoiceId.ToString();
		if (Id.StartsWith(TEXT("Talent_Ranger_Hunter_FavoredEnemy"))) return ERPGTalentPresentationType::Passive;
		if (Id == TEXT("Talent_Ranger_Hunter_AlphaHunter")) return ERPGTalentPresentationType::AutomaticReaction;
		if (Id == TEXT("Talent_Ranger_Marksman_EagleEye") || Id == TEXT("Talent_Ranger_Scout_Vigilance") ||
			Id == TEXT("Talent_Ranger_Scout_TerrainMaster") || Id == TEXT("Talent_Ranger_Scout_GroupGuide"))
			return ERPGTalentPresentationType::Passive;
		if (Id == TEXT("Talent_Ranger_Marksman_PreciseShot") || Id == TEXT("Talent_Ranger_Marksman_PiercingShot") ||
			Id == TEXT("Talent_Ranger_Marksman_RapidShot") || Id == TEXT("Talent_Ranger_Marksman_Volley") ||
			Id == TEXT("Talent_Ranger_Hunter_MarkPrey") || Id == TEXT("Talent_Ranger_Hunter_PinningShot") ||
			Id == TEXT("Talent_Ranger_Hunter_PredatorStrike") || Id == TEXT("Talent_Ranger_Scout_HuntingTrap") ||
			Id == TEXT("Talent_Ranger_Scout_TacticalRetreat")) return ERPGTalentPresentationType::Active;
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

	FString CategoryAssetName(FName CategoryId)
	{
		return FString::Printf(TEXT("DA_MONCAT_%s"), *SanitizeCategoryForChoiceId(CategoryId));
	}

	UGridMonsterCategoryAsset* FindOrCreateCategoryAsset(FName CategoryId, FString& OutError)
	{
		const FString AssetName = CategoryAssetName(CategoryId);
		const FString PackageName = FString::Printf(
			TEXT("%s/%s"), UGridMonsterCategoryAsset::ProductionFolder(), *AssetName);
		const FString ObjectPath = FString::Printf(TEXT("%s.%s"), *PackageName, *AssetName);

		if (UGridMonsterCategoryAsset* Existing = LoadObject<UGridMonsterCategoryAsset>(nullptr, *ObjectPath))
		{
			return Existing;
		}

		UPackage* Package = CreatePackage(*PackageName);
		if (!Package)
		{
			OutError = FString::Printf(TEXT("Failed to create monster category package '%s'."), *PackageName);
			return nullptr;
		}

		UGridMonsterCategoryAsset* Created = NewObject<UGridMonsterCategoryAsset>(
			Package, FName(*AssetName), RF_Public | RF_Standalone | RF_Transactional);
		if (!Created)
		{
			OutError = FString::Printf(TEXT("Failed to create monster category asset '%s'."), *ObjectPath);
			return nullptr;
		}
		FAssetRegistryModule::AssetCreated(Created);
		return Created;
	}

	bool CollectProductionMonsterDefinitions(TArray<UGridMonsterDefinitionAsset*>& OutDefinitions, FString& OutError)
	{
		OutDefinitions.Reset();

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
			UGridMonsterDefinitionAsset* Definition = Cast<UGridMonsterDefinitionAsset>(AssetData.GetAsset());
			if (IsValid(Definition) && !Definition->CategoryId.IsNone())
			{
				OutDefinitions.Add(Definition);
			}
		}
		OutDefinitions.Sort(
			[](const UGridMonsterDefinitionAsset& Left, const UGridMonsterDefinitionAsset& Right)
			{
				return Left.GetPathName() < Right.GetPathName();
			});

		if (OutDefinitions.IsEmpty())
		{
			OutError = TEXT("No production monster definition found for Favored Enemy authoring.");
			return false;
		}
		return true;
	}

	bool EnsureProductionMonsterCategoryAuthority(FString& OutError)
	{
		TArray<UGridMonsterDefinitionAsset*> Monsters;
		if (!CollectProductionMonsterDefinitions(Monsters, OutError))
		{
			return false;
		}

		TMap<FName, UGridMonsterCategoryAsset*> Categories;
		for (UGridMonsterDefinitionAsset* Monster : Monsters)
		{
			if (!IsValid(Monster))
			{
				continue;
			}

			const FProductionMonsterCategorySpec* Spec = FindProductionCategorySpec(Monster->CategoryId);
			if (!Spec || !Spec->DisplayName)
			{
				OutError = FString::Printf(
					TEXT("Production monster '%s' uses CategoryId '%s' without an authored bestiary DisplayName."),
					*Monster->GetPathName(), *Monster->CategoryId.ToString());
				return false;
			}

			UGridMonsterCategoryAsset*& Category = Categories.FindOrAdd(Monster->CategoryId);
			if (!IsValid(Category))
			{
				Category = FindOrCreateCategoryAsset(Monster->CategoryId, OutError);
				if (!IsValid(Category))
				{
					return false;
				}
				Category->Modify();
				Category->CategoryId = Monster->CategoryId;
				Category->DisplayName = FText::FromString(Spec->DisplayName);
				if (!Category->IsValidDefinition() || !SaveAuthoredAsset(Category, OutError))
				{
					if (OutError.IsEmpty())
					{
						OutError = FString::Printf(TEXT("Invalid monster category asset '%s'."), *Category->GetPathName());
					}
					return false;
				}
			}

			const FSoftObjectPath ExpectedPath(Category);
			if (Monster->CategoryDefinition.ToSoftObjectPath() != ExpectedPath)
			{
				Monster->Modify();
				Monster->CategoryDefinition = Category;
				if (!Monster->IsValidDefinition() || !SaveAuthoredAsset(Monster, OutError))
				{
					if (OutError.IsEmpty())
					{
						OutError = FString::Printf(TEXT("Failed to assign category authority to '%s'."), *Monster->GetPathName());
					}
					return false;
				}
			}
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

void FRPGRangerAuthoring::ConfigureClass(URPGClassAsset& ClassAsset, const TArray<FRPGRangerFavoredEnemyCategoryDefinition>& FavoredEnemyCategories)
{
	using namespace RPGRangerAuthoring;

	FRPGClassProgressionAuthoring::ConfigureCanonicalTalentGrants(ClassAsset);
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
		TEXT("Talent_Ranger_Marksman_PreciseShot"), TEXT("Tir précis"), TEXT("150 % des dégâts de l'arme et Précision +2. Requiert une arme à distance. La portée finale est limitée à 32 cellules."), 2, MarksmanBranchId);
	{
		FGridCombatModifierProfile Modifier;
		Modifier.ActionIds = { TEXT("Action_Ranger_PreciseShot") };
		Modifier.AccuracyModifier = 2;
		Precise.CombatModifiers.Add(Modifier);
	}
	ClassAsset.ProgressionChoices.Add(Precise);
	ClassAsset.ProgressionChoices.Add(MakeChoice(
		TEXT("Talent_Ranger_Marksman_PiercingShot"), TEXT("Tir perforant"), TEXT("110 % des dégâts de l'arme, de type physique perforant. Inflige en plus 50 % des dégâts bruts à l'armure physique uniquement, sans débordement vers les PV."),
		6, MarksmanBranchId, TEXT("Talent_Ranger_Marksman_PreciseShot")));
	ClassAsset.ProgressionChoices.Add(MakeChoice(
		TEXT("Talent_Ranger_Marksman_RapidShot"), TEXT("Tir rapide"), TEXT("Deux attaques successives indépendantes à 65 % dégâts de l'arme chacune. La seconde a Précision -1. Les deux peuvent critiquer ; la mort après le premier tir annule le second."),
		10, MarksmanBranchId, TEXT("Talent_Ranger_Marksman_PiercingShot")));
	ClassAsset.ProgressionChoices.Add(MakeChoice(
		TEXT("Talent_Ranger_Marksman_Volley"), TEXT("Volée"), TEXT("Chaque hostile de la zone reçoit une attaque indépendante à 80 % dégâts de l'arme. Aucun statut. Les obstacles/ligne de vue sont évalués vers la cellule cible."),
		14, MarksmanBranchId, TEXT("Talent_Ranger_Marksman_RapidShot")));
	FRPGClassProgressionChoiceDefinition EagleEye = MakeChoice(
		TEXT("Talent_Ranger_Marksman_EagleEye"), TEXT("Œil d'aigle"),
		TEXT("Actions à distance : portée +1 et Précision +1. Jets de Perception à distance +2. Les limites globales de portée restent applicables."),
		18, MarksmanBranchId, TEXT("Talent_Ranger_Marksman_Volley"));
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
		TEXT("Marque la cible pendant 3 rounds, quelle que soit son armure. Une seule marque peut être active par Rôdeur. Contre sa propre cible marquée : Précision +2 et dégâts +15 %."), 2, HunterBranchId);
	{
		FGridCombatModifierProfile Modifier;
		Modifier.RequiredTargetStatusEffectIds = { MarkStatusId };
		Modifier.bRequiredTargetStatusesFromOwner = true;
		Modifier.AccuracyModifier = 2;
		Modifier.OutgoingDamagePercentModifier = 15;
		MarkPrey.CombatModifiers.Add(Modifier);
	}
	ClassAsset.ProgressionChoices.Add(MarkPrey);

	TArray<FRPGRangerFavoredEnemyCategoryDefinition> Categories = FavoredEnemyCategories;
	Categories.RemoveAll([](const FRPGRangerFavoredEnemyCategoryDefinition& Category) { return !Category.IsValid(); });
	Categories.Sort(
		[](const FRPGRangerFavoredEnemyCategoryDefinition& A, const FRPGRangerFavoredEnemyCategoryDefinition& B)
		{
			return A.CategoryId.ToString() < B.CategoryId.ToString();
		});
	for (int32 Index = Categories.Num() - 1; Index > 0; --Index)
	{
		if (Categories[Index].CategoryId == Categories[Index - 1].CategoryId)
		{
			Categories.RemoveAt(Index);
		}
	}
	for (const FRPGRangerFavoredEnemyCategoryDefinition& Category : Categories)
	{
		const FName CategoryId = Category.CategoryId;
		const FString Suffix = SanitizeCategoryForChoiceId(CategoryId);
		const FName ChoiceId(*FString::Printf(TEXT("Talent_Ranger_Hunter_FavoredEnemy_%s"), *Suffix));
		const FString Display = FString::Printf(TEXT("Ennemi juré — %s"), *Category.DisplayName.ToString());
		FRPGClassProgressionChoiceDefinition Choice = MakeChoice(
			ChoiceId, *Display, TEXT("Dégâts +15 % et tests liés +2 contre cette catégorie."),
			6, HunterBranchId, TEXT("Talent_Ranger_Hunter_MarkPrey"), FavoredEnemyAlias);
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
		TEXT("100 % dégâts de l'arme. Si armure physique=0 après dégâts, applique Status_Immobilized 1 round (bBlockTranslation=true)."), 10, HunterBranchId);
	Pinning.PrerequisiteRequirementIds = { FavoredEnemyAlias };
	ClassAsset.ProgressionChoices.Add(Pinning);

	FRPGClassProgressionChoiceDefinition Predator = MakeChoice(
		TEXT("Talent_Ranger_Hunter_PredatorStrike"), TEXT("Frappe du prédateur"),
		TEXT("Requiert Status_MarkedByRanger provenant du lanceur. Inflige 170 % dégâts de l'arme avec Précision +1. La marque n'est pas consommée."),
		14, HunterBranchId, TEXT("Talent_Ranger_Hunter_PinningShot"));
	{
		FGridCombatModifierProfile Modifier;
		Modifier.ActionIds = { TEXT("Action_Ranger_PredatorStrike") };
		Modifier.AccuracyModifier = 1;
		Predator.CombatModifiers.Add(Modifier);
	}
	ClassAsset.ProgressionChoices.Add(Predator);

	FRPGClassProgressionChoiceDefinition Alpha = MakeChoice(
		TEXT("Talent_Ranger_Hunter_AlphaHunter"), TEXT("Chasseur alpha"),
		TEXT("1 fois/round, à la mort de la cible marquée, transfère automatiquement la marque vers l'hostile vivant le plus proche dans un rayon de 3 cellules ; nouvelle durée 2 rounds. Aucun PA."),
		18, HunterBranchId, TEXT("Talent_Ranger_Hunter_PredatorStrike"));
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
		TEXT("Le Rôdeur apporte +2 au meilleur jet de groupe de Perception et gagne Initiative +2 au premier round de chaque combat. Ce bonus ne se cumule pas entre plusieurs Rôdeurs."), 2, ScoutBranchId);
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
		TEXT("Pose un piège de chasse pendant 4 rounds. Le premier hostile qui entre subit 5 + modificateur de SAG dégâts physiques perforants ; si son armure physique est épuisée, il est immobilisé pendant 1 round. Le piège est ensuite consommé."), 6, ScoutBranchId, TEXT("Talent_Ranger_Scout_Vigilance")));
	ClassAsset.ProgressionChoices.Add(MakeChoice(
		TEXT("Talent_Ranger_Scout_TacticalRetreat"), TEXT("Repli tactique"),
		TEXT("Déplace tout le groupe d'une cellule en arrière si la translation est légale. Ne paie pas le coût personnel normal de translation, mais paie 1 PAM. Aucun franchissement d'obstacle."), 10, ScoutBranchId, TEXT("Talent_Ranger_Scout_HuntingTrap")));

	FRPGClassProgressionChoiceDefinition Terrain = MakeChoice(
		TEXT("Talent_Ranger_Scout_TerrainMaster"), TEXT("Maître du terrain"),
		TEXT("Si le groupe n'a effectué aucune translation depuis la précédente activation du Rôdeur : attaques à distance +10 % dégâts et Précision +1. Le bonus disparaît immédiatement après une translation."),
		14, ScoutBranchId, TEXT("Talent_Ranger_Scout_TacticalRetreat"));
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
		TEXT("Tant qu'un Rôdeur vivant possède ce talent : meilleurs jets de groupe Perception/Survie +2 et MaximumMobilityActionPoints +1 par round. Effet global non cumulable."),
		18, ScoutBranchId, TEXT("Talent_Ranger_Scout_TerrainMaster"));
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

bool FRPGRangerAuthoring::CollectProductionFavoredEnemyCategories(
	TArray<FRPGRangerFavoredEnemyCategoryDefinition>& OutCategories, FString& OutError)
{
	OutCategories.Reset();
	OutError.Reset();

	TArray<UGridMonsterDefinitionAsset*> Monsters;
	if (!RPGRangerAuthoring::CollectProductionMonsterDefinitions(Monsters, OutError))
	{
		return false;
	}

	TMap<FName, FText> DisplayNameByCategory;
	for (const UGridMonsterDefinitionAsset* Monster : Monsters)
	{
		if (!IsValid(Monster) || Monster->CategoryId.IsNone())
		{
			continue;
		}
		if (Monster->CategoryDefinition.IsNull())
		{
			OutError = FString::Printf(
				TEXT("Production monster '%s' has CategoryId '%s' but no CategoryDefinition presentation authority."),
				*Monster->GetPathName(), *Monster->CategoryId.ToString());
			return false;
		}

		const UGridMonsterCategoryAsset* Category = Monster->CategoryDefinition.LoadSynchronous();
		if (!IsValid(Category) || !Category->IsValidDefinition() || Category->CategoryId != Monster->CategoryId)
		{
			OutError = FString::Printf(
				TEXT("Production monster '%s' has an invalid or mismatched CategoryDefinition."),
				*Monster->GetPathName());
			return false;
		}

		if (const FText* Existing = DisplayNameByCategory.Find(Category->CategoryId))
		{
			if (!Existing->EqualTo(Category->DisplayName))
			{
				OutError = FString::Printf(
					TEXT("CategoryId '%s' resolves to conflicting bestiary DisplayName values."),
					*Category->CategoryId.ToString());
				return false;
			}
		}
		else
		{
			DisplayNameByCategory.Add(Category->CategoryId, Category->DisplayName);
		}
	}

	for (const TPair<FName, FText>& Pair : DisplayNameByCategory)
	{
		OutCategories.Emplace(Pair.Key, Pair.Value);
	}
	OutCategories.Sort(
		[](const FRPGRangerFavoredEnemyCategoryDefinition& A, const FRPGRangerFavoredEnemyCategoryDefinition& B)
		{
			return A.CategoryId.ToString() < B.CategoryId.ToString();
		});

	if (OutCategories.IsEmpty())
	{
		OutError = TEXT("No production monster category presentation found for Favored Enemy authoring.");
		return false;
	}
	return true;
}

bool FRPGRangerAuthoring::AuthorProductionAssets(FString& OutError)
{
	OutError.Reset();

	if (!RPGRangerAuthoring::EnsureProductionMonsterCategoryAuthority(OutError))
	{
		return false;
	}

	TArray<FRPGRangerFavoredEnemyCategoryDefinition> Categories;
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
