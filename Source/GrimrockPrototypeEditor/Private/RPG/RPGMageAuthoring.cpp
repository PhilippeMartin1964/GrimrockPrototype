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
	const FName ElementalOverloadActionId(TEXT("Action_Mage_ElementalOverload"));
	const FName ElementalOverloadStatusId(TEXT("Status_ElementalOverload"));

	struct FAffinityVariant
	{
		const TCHAR* Suffix;
		const TCHAR* DisplayName;
		const TCHAR* SchoolTag;
	};

	const FAffinityVariant AffinityVariants[] = {
		{ TEXT("Fire"), TEXT("Affinité élémentaire — Feu"), TEXT("Spell.School.Fire") },
		{ TEXT("Frost"), TEXT("Affinité élémentaire — Glace"), TEXT("Spell.School.Frost") },
		{ TEXT("Air"), TEXT("Affinité élémentaire — Air"), TEXT("Spell.School.Air") },
		{ TEXT("Earth"), TEXT("Affinité élémentaire — Terre"), TEXT("Spell.School.Earth") }
	};

	FName MakeAffinityChoiceId(const TCHAR* Suffix)
	{
		return FName(*FString::Printf(TEXT("Talent_Mage_Evoker_ElementalAffinity_%s"), Suffix));
	}

	FRPGClassProgressionChoiceDefinition MakeChoice(FName ChoiceId, const TCHAR* DisplayName, const TCHAR* Description, int32 MinimumLevel)
	{
		FRPGClassProgressionChoiceDefinition Choice;
		Choice.ChoiceId = ChoiceId;
		Choice.DisplayName = FText::FromString(DisplayName);
		Choice.Description = FText::FromString(Description);
		Choice.MinimumLevel = MinimumLevel;
		Choice.PointCost = 1;
		return Choice;
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

	UGridStatusEffectDefinitionAsset* FindOrCreateElementalOverloadStatus(FString& OutError)
	{
		if (UGridStatusEffectDefinitionAsset* Existing =
				LoadObject<UGridStatusEffectDefinitionAsset>(nullptr, FRPGMageAuthoring::ElementalOverloadStatusPath()))
		{
			return Existing;
		}

		const FString AssetName(TEXT("DA_Status_ElementalOverload"));
		const FString PackageName(TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/StatusEffects/DA_Status_ElementalOverload"));
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
			OutError = TEXT("Failed to create DA_Status_ElementalOverload.");
			return nullptr;
		}
		FAssetRegistryModule::AssetCreated(Created);
		return Created;
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

void FRPGMageAuthoring::ConfigureClass(URPGClassAsset& ClassAsset)
{
	using namespace RPGMageAuthoring;

	ClassAsset.CombatActions.Reset();
	ClassAsset.ProgressionChoices.Reset();

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
	OverloadAction.StatusApplications.Add(MakeStatusApplication(ElementalOverloadStatusId, 1));
	ClassAsset.CombatActions.Add(OverloadAction);

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
}

bool FRPGMageAuthoring::ConfigureElementalOverloadStatus(UGridStatusEffectDefinitionAsset& StatusAsset)
{
	using namespace RPGMageAuthoring;

	StatusAsset.EffectId = ElementalOverloadStatusId;
	StatusAsset.DisplayName = FText::FromString(TEXT("Surcharge élémentaire"));
	StatusAsset.Description =
		FText::FromString(TEXT("Le prochain sort correspondant à l'affinité élémentaire inflige +35 % de dégâts puis consomme cet effet."));
	StatusAsset.StatusTags.Reset();
	StatusAsset.Icon.Reset();
	StatusAsset.Disposition = EGridStatusEffectDisposition::Buff;
	StatusAsset.DurationUnit = EGridStatusEffectDurationUnit::Turns;
	StatusAsset.DefaultDuration = 1;
	StatusAsset.bExpireAtOwnerNextActivation = false;
	StatusAsset.DefaultPotency = 0;
	StatusAsset.StackPolicy = EGridStatusEffectStackPolicy::NoStack;
	StatusAsset.MaxStacks = 1;
	StatusAsset.bDistinctPerSource = false;
	StatusAsset.bUniquePerSourceAcrossMonsters = false;
	StatusAsset.PeriodicDamage = FGridStatusEffectPeriodicDamageProfile();
	StatusAsset.InitiativeModifier = 0;
	StatusAsset.Control = FGridStatusEffectControlProfile();
	StatusAsset.CombatModifiers.Reset();
	StatusAsset.CombatReactions.Reset();

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

bool FRPGMageAuthoring::AuthorProductionAssets(FString& OutError)
{
	OutError.Reset();

	URPGClassAsset* Mage = LoadObject<URPGClassAsset>(nullptr, MageAssetPath());
	if (!IsValid(Mage))
	{
		OutError = FString::Printf(TEXT("Production Mage asset not found: %s"), MageAssetPath());
		return false;
	}
	if (Mage->ClassId != RPGMageAuthoring::MageClassId)
	{
		OutError = FString::Printf(TEXT("Unexpected Mage ClassId '%s'."), *Mage->ClassId.ToString());
		return false;
	}

	UGridStatusEffectDefinitionAsset* Overload = RPGMageAuthoring::FindOrCreateElementalOverloadStatus(OutError);
	if (!IsValid(Overload))
	{
		return false;
	}
	Overload->Modify();
	if (!ConfigureElementalOverloadStatus(*Overload))
	{
		OutError = TEXT("Authored Status_ElementalOverload is structurally invalid.");
		return false;
	}

	Mage->Modify();
	ConfigureClass(*Mage);
	if (!Mage->IsValidDefinition())
	{
		OutError = TEXT("Authored DA_Class_Mage is structurally invalid.");
		return false;
	}

	if (!RPGMageAuthoring::SaveAuthoredAsset(Overload, OutError) || !RPGMageAuthoring::SaveAuthoredAsset(Mage, OutError))
	{
		return false;
	}
	return true;
}
