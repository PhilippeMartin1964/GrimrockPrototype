#include "RPG/RPGWarriorAuthoring.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "HAL/FileManager.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGClassProgressionAuthoring.h"
#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

namespace RPGWarriorAuthoring
{
	const FName WarriorClassId(TEXT("Warrior"));
	const FName GuardianBranchId(TEXT("Guardian"));
	const FName BreakerBranchId(TEXT("Breaker"));
	const FName WeaponMasterBranchId(TEXT("WeaponMaster"));
	const FName ShieldRequirement(TEXT("Equipment.Shield"));
	const FName HeavyWeaponTag(TEXT("Weapon.Heavy"));
	const FName MartialRequirement(TEXT("Talent_Warrior_WeaponMaster_MartialSpecialization"));
	const FName MartialGroup(TEXT("TalentGroup_Warrior_WeaponMaster_MartialSpecialization"));

	FGridCombatStatusApplicationProfile MakeStatusApplication(FName StatusId, EGridCombatStatusApplicationTrigger Trigger,
		EGridCombatStatusArmorGate Gate = EGridCombatStatusArmorGate::None, int32 DurationOverride = INDEX_NONE,
		EGridCombatResolvedTargetScope Scope = EGridCombatResolvedTargetScope::AllResolvedTargets)
	{
		FGridCombatStatusApplicationProfile Profile;
		Profile.StatusEffectId = StatusId;
		Profile.Trigger = Trigger;
		Profile.ArmorGate = Gate;
		Profile.DurationOverride = DurationOverride;
		Profile.TargetScope = Scope;
		return Profile;
	}

	FGridCombatArmorEffectProfile MakeArmorEffect(EGridCombatArmorEffectOperation Operation,
		EGridCombatArmorEffectMagnitude Magnitude, int32 Amount, EGridCombatArmorEffectTrigger Trigger)
	{
		FGridCombatArmorEffectProfile Profile;
		Profile.Pool = EGridCombatArmorPool::Physical;
		Profile.Operation = Operation;
		Profile.Magnitude = Magnitude;
		Profile.Trigger = Trigger;
		Profile.Amount = Amount;
		return Profile;
	}

	FGridCombatModifierProfile MakeSubtypeModifier(EGridPhysicalDamageSubtype Subtype, int32 Accuracy, int32 DamagePercent)
	{
		FGridCombatModifierProfile Profile;
		Profile.SourcePolicies = { EGridCombatActionSourcePolicy::Equipment, EGridCombatActionSourcePolicy::Ability };
		Profile.ActionTypes = { EGridCombatActionType::MeleeAttack, EGridCombatActionType::RangedAttack };
		Profile.PhysicalSubtypes = { Subtype };
		Profile.AccuracyModifier = Accuracy;
		Profile.OutgoingDamagePercentModifier = DamagePercent;
		return Profile;
	}


	ERPGTalentPresentationType ResolveTalentPresentationType(FName ChoiceId)
	{
		const FString Id = ChoiceId.ToString();
		if (Id.StartsWith(TEXT("Talent_Warrior_WeaponMaster_MartialSpecialization"))) return ERPGTalentPresentationType::Passive;
		if (Id == TEXT("Talent_Warrior_Guardian_Interception") || Id == TEXT("Talent_Warrior_WeaponMaster_Riposte"))
			return ERPGTalentPresentationType::AutomaticReaction;
		if (Id == TEXT("Talent_Warrior_Guardian_Bulwark") || Id == TEXT("Talent_Warrior_WeaponMaster_CriticalMastery"))
			return ERPGTalentPresentationType::Passive;
		if (Id == TEXT("Talent_Warrior_Guardian_DefensiveStance") || Id == TEXT("Talent_Warrior_Guardian_ShieldBash") ||
			Id == TEXT("Talent_Warrior_Guardian_Fortress") || Id == TEXT("Talent_Warrior_Breaker_PowerStrike") ||
			Id == TEXT("Talent_Warrior_Breaker_ArmorBreak") || Id == TEXT("Talent_Warrior_Breaker_Sweep") ||
			Id == TEXT("Talent_Warrior_Breaker_Execution") || Id == TEXT("Talent_Warrior_Breaker_Devastation") ||
			Id == TEXT("Talent_Warrior_WeaponMaster_SecondWind") || Id == TEXT("Talent_Warrior_WeaponMaster_Warlord"))
			return ERPGTalentPresentationType::Active;
		return ERPGTalentPresentationType::None;
	}

	FRPGClassProgressionChoiceDefinition MakeChoice(FName ChoiceId,
		int32 MinimumLevel, FName TalentBranchId, FName PrerequisiteChoiceId = NAME_None, FName TalentNodeId = NAME_None)
	{
		FRPGClassProgressionChoiceDefinition Choice;
		Choice.ChoiceId = ChoiceId;
		Choice.TalentBranchId = TalentBranchId;
		Choice.TalentNodeId = TalentNodeId.IsNone() ? ChoiceId : TalentNodeId;
		Choice.MinimumLevel = MinimumLevel;
		Choice.PointCost = 1;
		Choice.PresentationType = ResolveTalentPresentationType(ChoiceId);
		if (!PrerequisiteChoiceId.IsNone())
		{
			Choice.PrerequisiteChoiceIds.Add(PrerequisiteChoiceId);
		}
		return Choice;
	}

	FGridCombatActionDefinition MakeEffectAction(FName ActionId,
		FName RequirementId, int32 ActionPointCost, EGridCombatTargetingPolicy TargetingPolicy, int32 CooldownRounds)
	{
		FGridCombatActionDefinition Action;
		Action.ActionId = ActionId;
		Action.ActionType = EGridCombatActionType::Ability;
		Action.SourcePolicy = EGridCombatActionSourcePolicy::Ability;
		Action.TargetingPolicy = TargetingPolicy;
		Action.ResolutionProfile = EGridCombatActionResolutionProfile::Effect;
		Action.ActionPointCost = ActionPointCost;
		Action.CooldownRounds = CooldownRounds;
		Action.Requirements = { RequirementId };
		return Action;
	}

	FGridCombatActionDefinition MakeWeaponAction(FName ActionId,
		FName RequirementId, int32 ActionPointCost, EGridCombatTargetingPolicy TargetingPolicy,
		int32 RangeCells, int32 CooldownRounds, int32 WeaponDamagePercent)
	{
		FGridCombatActionDefinition Action;
		Action.ActionId = ActionId;
		Action.ActionType = EGridCombatActionType::MeleeAttack;
		Action.SourcePolicy = EGridCombatActionSourcePolicy::Ability;
		Action.TargetingPolicy = TargetingPolicy;
		Action.ResolutionProfile = EGridCombatActionResolutionProfile::Attack;
		Action.ActionPointCost = ActionPointCost;
		Action.RangeCells = RangeCells;
		Action.CooldownRounds = CooldownRounds;
		Action.Requirements = { RequirementId };
		Action.WeaponAttackProfile.bUseEquippedWeapon = true;
		Action.WeaponAttackProfile.WeaponDamagePercent = WeaponDamagePercent;
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
		const FString ObjectPath = FRPGWarriorAuthoring::GetStatusObjectPath(EffectId);
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

const TCHAR* FRPGWarriorAuthoring::WarriorAssetPath()
{
	return TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Warrior.DA_Class_Warrior");
}

void FRPGWarriorAuthoring::GetRequiredStatusIds(TArray<FName>& OutStatusIds)
{
	OutStatusIds = {
		TEXT("Status_Guarded"),
		TEXT("Status_Stunned"),
		TEXT("Status_Fortified"),
		TEXT("Status_KnockedDown"),
		TEXT("Status_Warlord")
	};
}

FString FRPGWarriorAuthoring::GetStatusObjectPath(FName EffectId)
{
	return FString::Printf(TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/StatusEffects/DA_%s.DA_%s"),
		*EffectId.ToString(), *EffectId.ToString());
}

void FRPGWarriorAuthoring::ConfigureClass(URPGClassAsset& ClassAsset)
{
	FRPGClassProgressionAuthoring::RematerializePreservingChoiceAndActionText(ClassAsset, [&]()
	{
		using namespace RPGWarriorAuthoring;

		FRPGClassProgressionAuthoring::ConfigureCanonicalTalentGrants(ClassAsset);
		ClassAsset.CombatActions.Reset();
		ClassAsset.ProgressionChoices.Reset();

		// Guardian
		{
			FGridCombatActionDefinition Action = MakeEffectAction(
				TEXT("Action_Warrior_DefensiveStance"), TEXT("Talent_Warrior_Guardian_DefensiveStance"),
				1, EGridCombatTargetingPolicy::Self, 3);
			Action.StatusApplications.Add(MakeStatusApplication(
				TEXT("Status_Guarded"), EGridCombatStatusApplicationTrigger::AfterResolution, EGridCombatStatusArmorGate::None, 2));
			ClassAsset.CombatActions.Add(Action);
		}
		{
			FGridCombatActionDefinition Action = MakeWeaponAction(
				TEXT("Action_Warrior_ShieldBash"),
				TEXT("Talent_Warrior_Guardian_ShieldBash"), 2, EGridCombatTargetingPolicy::FirstAxialTarget, 1, 2, 80);
			Action.Requirements.Add(ShieldRequirement);
			Action.WeaponAttackProfile.bOverrideDamageDescriptor = true;
			Action.WeaponAttackProfile.OverrideDamageType = EGridDamageType::Physical;
			Action.WeaponAttackProfile.OverridePhysicalSubtype = EGridPhysicalDamageSubtype::Bludgeoning;
			Action.StatusApplications.Add(MakeStatusApplication(
				TEXT("Status_Stunned"), EGridCombatStatusApplicationTrigger::AfterSuccessfulHit,
				EGridCombatStatusArmorGate::PhysicalArmorDepleted, 1));
			ClassAsset.CombatActions.Add(Action);
		}
		{
			FGridCombatActionDefinition Action = MakeEffectAction(
				TEXT("Action_Warrior_Fortress"),
				TEXT("Talent_Warrior_Guardian_Fortress"), 3, EGridCombatTargetingPolicy::FrontRowParty, 5);
			Action.ArmorEffects.Add(MakeArmorEffect(
				EGridCombatArmorEffectOperation::Restore, EGridCombatArmorEffectMagnitude::ReferencePercent, 40,
				EGridCombatArmorEffectTrigger::AfterResolution));
			Action.StatusApplications.Add(MakeStatusApplication(
				TEXT("Status_Fortified"), EGridCombatStatusApplicationTrigger::AfterResolution, EGridCombatStatusArmorGate::None, 2));
			ClassAsset.CombatActions.Add(Action);
		}

		// Breaker
		{
			FGridCombatActionDefinition Action = MakeWeaponAction(
				TEXT("Action_Warrior_PowerStrike"),
				TEXT("Talent_Warrior_Breaker_PowerStrike"), 3, EGridCombatTargetingPolicy::FirstAxialTarget, 1, 1, 150);
			Action.WeaponAttackProfile.RequiredItemTags = { HeavyWeaponTag };
			ClassAsset.CombatActions.Add(Action);
		}
		{
			FGridCombatActionDefinition Action = MakeWeaponAction(
				TEXT("Action_Warrior_ArmorBreak"),
				TEXT("Talent_Warrior_Breaker_ArmorBreak"), 2, EGridCombatTargetingPolicy::FirstAxialTarget, 1, 2, 100);
			Action.ArmorEffects.Add(MakeArmorEffect(
				EGridCombatArmorEffectOperation::Damage, EGridCombatArmorEffectMagnitude::RawDamagePercent, 50,
				EGridCombatArmorEffectTrigger::AfterSuccessfulHit));
			ClassAsset.CombatActions.Add(Action);
		}
		{
			FGridCombatActionDefinition Action = MakeWeaponAction(
				TEXT("Action_Warrior_Sweep"),
				TEXT("Talent_Warrior_Breaker_Sweep"), 3, EGridCombatTargetingPolicy::Area, 1, 2, 85);
			Action.AreaRadiusCells = 1;
			Action.bAffectsAlliesInArea = false;
			ClassAsset.CombatActions.Add(Action);
		}
		{
			FGridCombatActionDefinition Action = MakeWeaponAction(
				TEXT("Action_Warrior_Execution"),
				TEXT("Talent_Warrior_Breaker_Execution"), 2, EGridCombatTargetingPolicy::FirstAxialTarget, 1, 2, 200);
			Action.TargetFilter.bRequirePhysicalArmorDepleted = true;
			Action.TargetFilter.MaximumHealthPercent = 35;
			ClassAsset.CombatActions.Add(Action);
		}
		{
			FGridCombatActionDefinition Action = MakeWeaponAction(
				TEXT("Action_Warrior_Devastation"),
				TEXT("Talent_Warrior_Breaker_Devastation"), 4, EGridCombatTargetingPolicy::Area, 1, 5, 140);
			Action.AreaRadiusCells = 1;
			Action.bAffectsAlliesInArea = false;
			Action.StatusApplications.Add(MakeStatusApplication(
				TEXT("Status_KnockedDown"), EGridCombatStatusApplicationTrigger::AfterSuccessfulHit,
				EGridCombatStatusArmorGate::PhysicalArmorDepleted, 1, EGridCombatResolvedTargetScope::PrimaryTargetOnly));
			ClassAsset.CombatActions.Add(Action);
		}

		// Weapon Master
		{
			FGridCombatActionDefinition Action = MakeEffectAction(
				TEXT("Action_Warrior_SecondWind"), TEXT("Talent_Warrior_WeaponMaster_SecondWind"),
				1, EGridCombatTargetingPolicy::Self, 4);
			Action.EffectProfile.RestoreHealthMaximumPercent = 20;
			ClassAsset.CombatActions.Add(Action);
		}
		{
			FGridCombatActionDefinition Action = MakeEffectAction(
				TEXT("Action_Warrior_Warlord"),
				TEXT("Talent_Warrior_WeaponMaster_Warlord"), 2, EGridCombatTargetingPolicy::Party, 4);
			Action.StatusApplications.Add(MakeStatusApplication(
				TEXT("Status_Warlord"), EGridCombatStatusApplicationTrigger::AfterResolution, EGridCombatStatusArmorGate::None, 2));
			ClassAsset.CombatActions.Add(Action);
		}

		ClassAsset.ProgressionChoices.Add(MakeChoice(
			TEXT("Talent_Warrior_Guardian_DefensiveStance"), 2, GuardianBranchId));
		ClassAsset.ProgressionChoices.Add(MakeChoice(
			TEXT("Talent_Warrior_Guardian_ShieldBash"), 6, GuardianBranchId, TEXT("Talent_Warrior_Guardian_DefensiveStance")));

		FRPGClassProgressionChoiceDefinition Interception = MakeChoice(
			TEXT("Talent_Warrior_Guardian_Interception"),
			10, GuardianBranchId, TEXT("Talent_Warrior_Guardian_ShieldBash"));
		{
			FGridCombatReactionProfile Reaction;
			Reaction.ReactionId = TEXT("Reaction_Warrior_Interception");
			Reaction.Trigger = EGridCombatReactionTrigger::IncomingAttackHit;
			Reaction.Limit = EGridCombatReactionLimit::OncePerRound;
			Reaction.ActionTypes = { EGridCombatActionType::MeleeAttack, EGridCombatActionType::RangedAttack };
			Reaction.DamageTypes = { EGridDamageType::Physical };
			Reaction.InterceptFinalDamagePercent = 50;
			Reaction.bRequireOwnerFrontRow = true;
			Reaction.bRequireEventTargetFrontRow = true;
			Interception.CombatReactions.Add(Reaction);
		}
		ClassAsset.ProgressionChoices.Add(Interception);

		FRPGClassProgressionChoiceDefinition Bulwark = MakeChoice(
			TEXT("Talent_Warrior_Guardian_Bulwark"),
			14, GuardianBranchId, TEXT("Talent_Warrior_Guardian_Interception"));
		{
			FGridCombatModifierProfile Modifier;
			Modifier.PhysicalArmorReferencePercentModifier = 25;
			Bulwark.CombatModifiers.Add(Modifier);
		}
		ClassAsset.ProgressionChoices.Add(Bulwark);
		ClassAsset.ProgressionChoices.Add(MakeChoice(
			TEXT("Talent_Warrior_Guardian_Fortress"), 18, GuardianBranchId, TEXT("Talent_Warrior_Guardian_Bulwark")));

		FRPGClassProgressionChoiceDefinition PowerStrike = MakeChoice(
			TEXT("Talent_Warrior_Breaker_PowerStrike"), 2, BreakerBranchId);
		{
			FGridCombatModifierProfile Modifier;
			Modifier.ActionIds = { TEXT("Action_Warrior_PowerStrike") };
			Modifier.AccuracyModifier = -2;
			PowerStrike.CombatModifiers.Add(Modifier);
		}
		ClassAsset.ProgressionChoices.Add(PowerStrike);
		ClassAsset.ProgressionChoices.Add(MakeChoice(
			TEXT("Talent_Warrior_Breaker_ArmorBreak"), 6, BreakerBranchId, TEXT("Talent_Warrior_Breaker_PowerStrike")));
		ClassAsset.ProgressionChoices.Add(MakeChoice(
			TEXT("Talent_Warrior_Breaker_Sweep"), 10, BreakerBranchId, TEXT("Talent_Warrior_Breaker_ArmorBreak")));
		ClassAsset.ProgressionChoices.Add(MakeChoice(
			TEXT("Talent_Warrior_Breaker_Execution"), 14, BreakerBranchId, TEXT("Talent_Warrior_Breaker_Sweep")));
		ClassAsset.ProgressionChoices.Add(MakeChoice(
			TEXT("Talent_Warrior_Breaker_Devastation"), 18, BreakerBranchId, TEXT("Talent_Warrior_Breaker_Execution")));

		const struct
		{
			EGridPhysicalDamageSubtype Subtype;
			const TCHAR* Suffix;
		} Specializations[] = {
			{ EGridPhysicalDamageSubtype::Slashing, TEXT("Slashing") },
			{ EGridPhysicalDamageSubtype::Piercing, TEXT("Piercing") },
			{ EGridPhysicalDamageSubtype::Bludgeoning, TEXT("Bludgeoning") }
		};
		for (const auto& Spec : Specializations)
		{
			const FName ChoiceId(*FString::Printf(TEXT("Talent_Warrior_WeaponMaster_MartialSpecialization_%s"), Spec.Suffix));
			FRPGClassProgressionChoiceDefinition Choice = MakeChoice(
				ChoiceId, 2, WeaponMasterBranchId, NAME_None, MartialRequirement);
			Choice.ExclusiveChoiceGroupId = MartialGroup;
			Choice.GrantedRequirementIds = { MartialRequirement };
			Choice.CombatModifiers.Add(MakeSubtypeModifier(Spec.Subtype, 1, 10));
			ClassAsset.ProgressionChoices.Add(Choice);
		}

		FRPGClassProgressionChoiceDefinition Riposte = MakeChoice(
			TEXT("Talent_Warrior_WeaponMaster_Riposte"),
			6, WeaponMasterBranchId);
		Riposte.PrerequisiteRequirementIds = { MartialRequirement };
		{
			FGridCombatReactionProfile Reaction;
			Reaction.ReactionId = TEXT("Reaction_Warrior_Riposte");
			Reaction.Trigger = EGridCombatReactionTrigger::AttackMiss;
			Reaction.Limit = EGridCombatReactionLimit::OncePerRound;
			Reaction.ActionTypes = { EGridCombatActionType::MeleeAttack };
			Reaction.CounterAttackActionId = TEXT("Action_Warrior_Riposte");
			Reaction.CounterAttackRangeCells = 1;
			Reaction.CounterAttackWeaponProfile.bUseEquippedWeapon = true;
			Reaction.CounterAttackWeaponProfile.WeaponDamagePercent = 75;
			Reaction.CounterAttackWeaponProfile.bAllowUnarmed = true;
			Riposte.CombatReactions.Add(Reaction);
		}
		ClassAsset.ProgressionChoices.Add(Riposte);

		ClassAsset.ProgressionChoices.Add(MakeChoice(
			TEXT("Talent_Warrior_WeaponMaster_SecondWind"), 10, WeaponMasterBranchId, TEXT("Talent_Warrior_WeaponMaster_Riposte")));

		FRPGClassProgressionChoiceDefinition CriticalMastery = MakeChoice(
			TEXT("Talent_Warrior_WeaponMaster_CriticalMastery"),
			14, WeaponMasterBranchId, TEXT("Talent_Warrior_WeaponMaster_SecondWind"));
		for (const auto& Spec : Specializations)
		{
			FGridCombatModifierProfile Modifier = MakeSubtypeModifier(Spec.Subtype, 0, 0);
			Modifier.CriticalChancePercentModifier = 10;
			Modifier.CriticalDamagePercentModifier = 25;
			Modifier.RequiredOwnerRequirementIds = {
				FName(*FString::Printf(TEXT("Talent_Warrior_WeaponMaster_MartialSpecialization_%s"), Spec.Suffix))
			};
			CriticalMastery.CombatModifiers.Add(Modifier);
		}
		ClassAsset.ProgressionChoices.Add(CriticalMastery);

		ClassAsset.ProgressionChoices.Add(MakeChoice(
			TEXT("Talent_Warrior_WeaponMaster_Warlord"), 18, WeaponMasterBranchId, TEXT("Talent_Warrior_WeaponMaster_CriticalMastery")));
	});
}

bool FRPGWarriorAuthoring::ConfigureStatus(UGridStatusEffectDefinitionAsset& StatusAsset, FName EffectId)
{
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

	if (EffectId == TEXT("Status_Guarded"))
	{
		StatusAsset.DisplayName = FText::FromString(TEXT("Garde"));
		StatusAsset.Description = FText::FromString(TEXT("Dégâts physiques reçus -20 %, Esquive +2, dégâts d'arme infligés -10 %."));
		StatusAsset.Disposition = EGridStatusEffectDisposition::Buff;
		StatusAsset.DurationUnit = EGridStatusEffectDurationUnit::Rounds;
		StatusAsset.DefaultDuration = 2;

		FGridCombatModifierProfile Incoming;
		Incoming.DamageTypes = { EGridDamageType::Physical };
		Incoming.IncomingDamagePercentModifier = -20;
		StatusAsset.CombatModifiers.Add(Incoming);

		FGridCombatModifierProfile Evasion;
		Evasion.EvasionModifier = 2;
		StatusAsset.CombatModifiers.Add(Evasion);

		FGridCombatModifierProfile Outgoing;
		Outgoing.SourcePolicies = { EGridCombatActionSourcePolicy::Equipment, EGridCombatActionSourcePolicy::Ability };
		Outgoing.ActionTypes = { EGridCombatActionType::MeleeAttack, EGridCombatActionType::RangedAttack };
		Outgoing.OutgoingDamagePercentModifier = -10;
		StatusAsset.CombatModifiers.Add(Outgoing);
		return true;
	}
	if (EffectId == TEXT("Status_Stunned"))
	{
		StatusAsset.DisplayName = FText::FromString(TEXT("Étourdi"));
		StatusAsset.Description = FText::FromString(TEXT("La prochaine activation est ignorée."));
		StatusAsset.StatusTags = { TEXT("Control.Physical"), TEXT("Control.Stun") };
		StatusAsset.Disposition = EGridStatusEffectDisposition::Debuff;
		StatusAsset.DurationUnit = EGridStatusEffectDurationUnit::Turns;
		StatusAsset.DefaultDuration = 1;
		StatusAsset.Control.bSkipActivation = true;
		return true;
	}
	if (EffectId == TEXT("Status_Fortified"))
	{
		StatusAsset.DisplayName = FText::FromString(TEXT("Fortifié"));
		StatusAsset.Description = FText::FromString(TEXT("Dégâts physiques reçus -25 %."));
		StatusAsset.Disposition = EGridStatusEffectDisposition::Buff;
		StatusAsset.DurationUnit = EGridStatusEffectDurationUnit::Rounds;
		StatusAsset.DefaultDuration = 2;

		FGridCombatModifierProfile Incoming;
		Incoming.DamageTypes = { EGridDamageType::Physical };
		Incoming.IncomingDamagePercentModifier = -25;
		StatusAsset.CombatModifiers.Add(Incoming);
		return true;
	}
	if (EffectId == TEXT("Status_KnockedDown"))
	{
		StatusAsset.DisplayName = FText::FromString(TEXT("À terre"));
		StatusAsset.Description = FText::FromString(TEXT("La prochaine activation est ignorée."));
		StatusAsset.StatusTags = { TEXT("Control.Physical"), TEXT("Control.Knockdown") };
		StatusAsset.Disposition = EGridStatusEffectDisposition::Debuff;
		StatusAsset.DurationUnit = EGridStatusEffectDurationUnit::Turns;
		StatusAsset.DefaultDuration = 1;
		StatusAsset.Control.bSkipActivation = true;
		return true;
	}
	if (EffectId == TEXT("Status_Warlord"))
	{
		StatusAsset.DisplayName = FText::FromString(TEXT("Seigneur de guerre"));
		StatusAsset.Description = FText::FromString(TEXT("Précision +2 et Initiative +4."));
		StatusAsset.Disposition = EGridStatusEffectDisposition::Buff;
		StatusAsset.DurationUnit = EGridStatusEffectDurationUnit::Rounds;
		StatusAsset.DefaultDuration = 2;
		StatusAsset.InitiativeModifier = 4;

		FGridCombatModifierProfile Accuracy;
		Accuracy.AccuracyModifier = 2;
		StatusAsset.CombatModifiers.Add(Accuracy);
		return true;
	}
	return false;
}

bool FRPGWarriorAuthoring::AuthorProductionAssets(FString& OutError)
{
	OutError.Reset();

	URPGClassAsset* Warrior = LoadObject<URPGClassAsset>(nullptr, WarriorAssetPath());
	if (!IsValid(Warrior))
	{
		OutError = FString::Printf(TEXT("Production Warrior asset not found: %s"), WarriorAssetPath());
		return false;
	}
	if (Warrior->ClassId != RPGWarriorAuthoring::WarriorClassId)
	{
		OutError = FString::Printf(TEXT("Unexpected Warrior ClassId '%s'."), *Warrior->ClassId.ToString());
		return false;
	}

	Warrior->Modify();
	ConfigureClass(*Warrior);
	if (!Warrior->IsValidDefinition())
	{
		OutError = TEXT("Authored DA_Class_Warrior is structurally invalid.");
		return false;
	}

	TArray<UGridStatusEffectDefinitionAsset*> StatusAssets;
	TArray<FName> StatusIds;
	GetRequiredStatusIds(StatusIds);
	for (const FName StatusId : StatusIds)
	{
		UGridStatusEffectDefinitionAsset* Status = RPGWarriorAuthoring::FindOrCreateStatus(StatusId, OutError);
		if (!IsValid(Status))
		{
			return false;
		}

		Status->Modify();
		if (!ConfigureStatus(*Status, StatusId))
		{
			OutError = FString::Printf(TEXT("No Warrior status authoring definition for '%s'."), *StatusId.ToString());
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
		if (!RPGWarriorAuthoring::SaveAuthoredAsset(Status, OutError))
		{
			return false;
		}
	}
	if (!RPGWarriorAuthoring::SaveAuthoredAsset(Warrior, OutError))
	{
		return false;
	}

	return true;
}
