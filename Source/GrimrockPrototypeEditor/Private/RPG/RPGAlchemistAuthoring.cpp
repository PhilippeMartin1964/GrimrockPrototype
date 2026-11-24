#include "RPG/RPGAlchemistAuthoring.h"

#include "RPG/GridEditorAssetSave.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGClassProgressionAuthoring.h"
#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "Engine/GameInstance.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/Package.h"

namespace RPGAlchemistAuthoring
{
	using GridEditorAssetSave::SaveAuthoredAsset;
	const FName AlchemistClassId(TEXT("Alchemist"));
	const FName GrenadierBranchId(TEXT("Grenadier"));
	const FName ApothecaryBranchId(TEXT("Apothecary"));
	const FName TransmuterBranchId(TEXT("Transmuter"));
	const FName BurningStatus(TEXT("Status_Burning"));

	const FName FireBombTalent(TEXT("Talent_Alchemist_Grenadier_FireBomb"));
	const FName ToxicBombTalent(TEXT("Talent_Alchemist_Grenadier_ToxicBomb"));
	const FName PreciseChargeTalent(TEXT("Talent_Alchemist_Grenadier_PreciseCharge"));
	const FName ChainReactionTalent(TEXT("Talent_Alchemist_Grenadier_ChainReaction"));
	const FName MasterGrenadierTalent(TEXT("Talent_Alchemist_Grenadier_MasterGrenadier"));

	const FName EnhancedPotionTalent(TEXT("Talent_Alchemist_Apothecary_EnhancedPotion"));
	const FName AntidoteTalent(TEXT("Talent_Alchemist_Apothecary_Antidote"));
	const FName DefensiveElixirTalent(TEXT("Talent_Alchemist_Apothecary_DefensiveElixir"));
	const FName DiffusionTalent(TEXT("Talent_Alchemist_Apothecary_Diffusion"));
	const FName PanaceaTalent(TEXT("Talent_Alchemist_Apothecary_Panacea"));

	const FName OilSlickTalent(TEXT("Talent_Alchemist_Transmuter_OilSlick"));
	const FName AcidFlaskTalent(TEXT("Talent_Alchemist_Transmuter_AcidFlask"));
	const FName CorrosiveCloudTalent(TEXT("Talent_Alchemist_Transmuter_CorrosiveCloud"));
	const FName CatalystTalent(TEXT("Talent_Alchemist_Transmuter_Catalyst"));
	const FName MajorTransmutationTalent(TEXT("Talent_Alchemist_Transmuter_MajorTransmutation"));

	const FName FireBombItem(TEXT("Item_Bomb_Fire"));
	const FName ToxicBombItem(TEXT("Item_Bomb_Toxic"));
	const FName AntidoteItem(TEXT("Item_Antidote"));
	const FName DefensiveFireItem(TEXT("Item_DefensiveElixir_Fire"));
	const FName DefensiveIceItem(TEXT("Item_DefensiveElixir_Ice"));
	const FName DefensiveLightningItem(TEXT("Item_DefensiveElixir_Lightning"));
	const FName DefensivePoisonItem(TEXT("Item_DefensiveElixir_Poison"));
	const FName PanaceaItem(TEXT("Item_Panacea"));
	const FName OilSlickItem(TEXT("Item_Flask_Oil"));
	const FName AcidFlaskItem(TEXT("Item_Flask_Acid"));
	const FName CorrosiveCloudItem(TEXT("Item_Flask_CorrosiveCloud"));

	const FName CatalystAction(TEXT("Action_Alchemist_Catalyst"));
	const FName MajorTransmutationAction(TEXT("Action_Alchemist_MajorTransmutation"));

	const FName PoisonStatus(TEXT("Status_Poison"));
	const FName DefensiveFireStatus(TEXT("Status_DefensiveElixir_Fire"));
	const FName DefensiveIceStatus(TEXT("Status_DefensiveElixir_Ice"));
	const FName DefensiveLightningStatus(TEXT("Status_DefensiveElixir_Lightning"));
	const FName DefensivePoisonStatus(TEXT("Status_DefensiveElixir_Poison"));
	const FName CorrodedStatus(TEXT("Status_Corroded"));

	ERPGTalentPresentationType ResolveTalentPresentationType(FName ChoiceId)
	{
		const FString Id = ChoiceId.ToString();
		if (Id == TEXT("Talent_Alchemist_Grenadier_ChainReaction")) return ERPGTalentPresentationType::AutomaticReaction;
		if (Id == TEXT("Talent_Alchemist_Grenadier_PreciseCharge") || Id == TEXT("Talent_Alchemist_Grenadier_MasterGrenadier") ||
			Id == TEXT("Talent_Alchemist_Apothecary_EnhancedPotion") || Id == TEXT("Talent_Alchemist_Apothecary_Diffusion"))
			return ERPGTalentPresentationType::Passive;
		if (Id == TEXT("Talent_Alchemist_Transmuter_Catalyst")) return ERPGTalentPresentationType::Active;
		if (Id == TEXT("Talent_Alchemist_Transmuter_MajorTransmutation")) return ERPGTalentPresentationType::RecipeActive;
		if (Id == TEXT("Talent_Alchemist_Grenadier_FireBomb") || Id == TEXT("Talent_Alchemist_Grenadier_ToxicBomb") ||
			Id == TEXT("Talent_Alchemist_Apothecary_Antidote") || Id == TEXT("Talent_Alchemist_Apothecary_DefensiveElixir") ||
			Id == TEXT("Talent_Alchemist_Apothecary_Panacea") || Id == TEXT("Talent_Alchemist_Transmuter_OilSlick") ||
			Id == TEXT("Talent_Alchemist_Transmuter_AcidFlask") || Id == TEXT("Talent_Alchemist_Transmuter_CorrosiveCloud"))
			return ERPGTalentPresentationType::RecipeQuickItem;
		return ERPGTalentPresentationType::None;
	}

	FRPGClassProgressionChoiceDefinition MakeChoice(
		FName ChoiceId, int32 Level, FName TalentBranchId, FName Prerequisite = NAME_None, FName TalentNodeId = NAME_None)
	{
		FRPGClassProgressionChoiceDefinition Choice;
		Choice.ChoiceId = ChoiceId;
		Choice.TalentBranchId = TalentBranchId;
		Choice.TalentNodeId = TalentNodeId.IsNone() ? ChoiceId : TalentNodeId;
		Choice.MinimumLevel = Level;
		Choice.PointCost = 1;
		Choice.PresentationType = ResolveTalentPresentationType(ChoiceId);
		if (!Prerequisite.IsNone())
		{
			Choice.PrerequisiteChoiceIds.Add(Prerequisite);
		}
		return Choice;
	}

	void SetBootstrapText(FName StableId, FText& DisplayName, FText& Description)
	{
		DisplayName = FText::FromName(StableId);
		Description = FText::FromString(
			FString::Printf(TEXT("AUTHOR_TEXT_REQUIRED:%s"), *StableId.ToString()));
	}

	FGridCombatActionDefinition MakeQuickItemAction(
		FName ActionId, EGridCombatTargetingPolicy Targeting, EGridCombatActionResolutionProfile Resolution,
		int32 ActionPointCost, int32 RangeCells, int32 CooldownRounds = 0)
	{
		FGridCombatActionDefinition Action;
		Action.ActionId = ActionId;
		SetBootstrapText(ActionId, Action.DisplayName, Action.Description);
		Action.ActionType = EGridCombatActionType::Ability;
		Action.SourcePolicy = EGridCombatActionSourcePolicy::QuickItem;
		Action.TargetingPolicy = Targeting;
		Action.ResolutionProfile = Resolution;
		Action.ActionPointCost = ActionPointCost;
		Action.RangeCells = RangeCells;
		Action.CooldownRounds = CooldownRounds;
		Action.ResourceCosts.SourceItemQuantityCost = 1;
		Action.bRequiresLineOfSight =
			Targeting == EGridCombatTargetingPolicy::Hostile ||
			Targeting == EGridCombatTargetingPolicy::Cell ||
			Targeting == EGridCombatTargetingPolicy::Area;
		return Action;
	}


	UGridStatusEffectDefinitionAsset* FindOrCreateStatus(FName EffectId, FString& OutError)
	{
		if (EffectId.IsNone())
		{
			OutError = TEXT("Cannot author an empty Alchemist status id.");
			return nullptr;
		}
		const FString AssetName = FString::Printf(TEXT("DA_%s"), *EffectId.ToString());
		const FString PackageName =
			FString::Printf(TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/StatusEffects/%s"), *AssetName);
		if (FPackageName::DoesPackageExist(PackageName))
		{
			const FString ObjectPath = FRPGAlchemistAuthoring::GetStatusObjectPath(EffectId);
			if (UGridStatusEffectDefinitionAsset* Existing =
					LoadObject<UGridStatusEffectDefinitionAsset>(nullptr, *ObjectPath))
			{
				return Existing;
			}
			OutError = FString::Printf(TEXT("Existing Alchemist status package could not load expected asset: %s"), *ObjectPath);
			return nullptr;
		}

		UPackage* Package = CreatePackage(*PackageName);
		UGridStatusEffectDefinitionAsset* Created = Package
			? NewObject<UGridStatusEffectDefinitionAsset>(
				Package, *AssetName, RF_Public | RF_Standalone | RF_Transactional)
			: nullptr;
		if (!Created)
		{
			OutError = FString::Printf(TEXT("Failed to create Alchemist status '%s'."), *AssetName);
			return nullptr;
		}
		FAssetRegistryModule::AssetCreated(Created);
		return Created;
	}

	bool ValidateSharedStatus(FName EffectId, FString& OutError)
	{
		const FString Path = FRPGAlchemistAuthoring::GetStatusObjectPath(EffectId);
		const UGridStatusEffectDefinitionAsset* Status =
			LoadObject<UGridStatusEffectDefinitionAsset>(nullptr, *Path);
		if (!IsValid(Status) || Status->EffectId != EffectId || !Status->IsValidDefinition())
		{
			OutError = FString::Printf(TEXT("Required shared status is missing or invalid: %s"), *Path);
			return false;
		}
		return true;
	}
}

const TCHAR* FRPGAlchemistAuthoring::AlchemistAssetPath()
{
	return TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Alchemist.DA_Class_Alchemist");
}

FString FRPGAlchemistAuthoring::GetItemObjectPath(FName ItemDefinitionId)
{
	return FString::Printf(TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_%s.DA_%s"),
		*ItemDefinitionId.ToString(), *ItemDefinitionId.ToString());
}

FString FRPGAlchemistAuthoring::GetStatusObjectPath(FName EffectId)
{
	return FString::Printf(TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/StatusEffects/DA_%s.DA_%s"),
		*EffectId.ToString(), *EffectId.ToString());
}

void FRPGAlchemistAuthoring::ConfigureClass(URPGClassAsset& ClassAsset)
{
	FRPGClassProgressionAuthoring::RematerializePreservingChoiceAndActionText(ClassAsset, [&]()
	{
		using namespace RPGAlchemistAuthoring;

		FRPGClassProgressionAuthoring::ConfigureCanonicalTalentGrants(ClassAsset);
		ClassAsset.CombatActions.Reset();
		ClassAsset.ProgressionChoices.Reset();

		// Grenadier
		{
			FRPGClassProgressionChoiceDefinition Choice = MakeChoice(
				FireBombTalent, 2, GrenadierBranchId);
			Choice.GrantedRequirementIds = { TEXT("Recipe_Bomb_Fire") };
			ClassAsset.ProgressionChoices.Add(Choice);
		}
		{
			FRPGClassProgressionChoiceDefinition Choice = MakeChoice(
				ToxicBombTalent, 6, GrenadierBranchId, FireBombTalent);
			Choice.GrantedRequirementIds = { TEXT("Recipe_Bomb_Toxic") };
			ClassAsset.ProgressionChoices.Add(Choice);
		}
		{
			FRPGClassProgressionChoiceDefinition Choice = MakeChoice(
				PreciseChargeTalent, 10, GrenadierBranchId, ToxicBombTalent);
			FGridCombatModifierProfile Modifier;
			Modifier.SourcePolicies = { EGridCombatActionSourcePolicy::QuickItem };
			Modifier.RequiredSourceTags = { TEXT("QuickItem.Bomb") };
			Modifier.RangeCellsModifier = 1;
			Modifier.FriendlyDirectDamagePercentModifier = -50;
			Choice.CombatModifiers.Add(Modifier);
			ClassAsset.ProgressionChoices.Add(Choice);
		}
		{
			FRPGClassProgressionChoiceDefinition Choice = MakeChoice(
				ChainReactionTalent, 14, GrenadierBranchId,
				PreciseChargeTalent);
			FGridCombatReactionProfile Reaction;
			Reaction.ReactionId = TEXT("Reaction_Alchemist_ChainReaction");
			Reaction.Trigger = EGridCombatReactionTrigger::SurfaceReaction;
			Reaction.Limit = EGridCombatReactionLimit::OncePerAction;
			Reaction.SourcePolicies = { EGridCombatActionSourcePolicy::QuickItem };
			Reaction.RequiredSourceTags = { TEXT("QuickItem.Bomb") };
			Reaction.SurfaceReactionDamagePercentModifier = 25;
			Reaction.SurfaceReactionAreaRadiusModifier = 1;
			Choice.CombatReactions.Add(Reaction);
			ClassAsset.ProgressionChoices.Add(Choice);
		}
		{
			FRPGClassProgressionChoiceDefinition Choice = MakeChoice(
				MasterGrenadierTalent, 18, GrenadierBranchId, ChainReactionTalent);
			FGridCombatModifierProfile Modifier;
			Modifier.SourcePolicies = { EGridCombatActionSourcePolicy::QuickItem };
			Modifier.RequiredSourceTags = { TEXT("QuickItem.Bomb") };
			Modifier.ActionPointCostModifier = -1;
			Modifier.OutgoingDamagePercentModifier = 20;
			Choice.CombatModifiers.Add(Modifier);
			ClassAsset.ProgressionChoices.Add(Choice);
		}

		// Apothecary
		{
			FRPGClassProgressionChoiceDefinition Choice = MakeChoice(
				EnhancedPotionTalent, 2, ApothecaryBranchId);
			FGridCombatModifierProfile Modifier;
			Modifier.SourcePolicies = { EGridCombatActionSourcePolicy::QuickItem };
			Modifier.RequiredSourceTags = { TEXT("QuickItem.Potion.Positive") };
			Modifier.PositiveEffectPercentModifier = 25;
			Choice.CombatModifiers.Add(Modifier);
			ClassAsset.ProgressionChoices.Add(Choice);
		}
		{
			FRPGClassProgressionChoiceDefinition Choice = MakeChoice(
				AntidoteTalent, 6, ApothecaryBranchId, EnhancedPotionTalent);
			Choice.GrantedRequirementIds = { TEXT("Recipe_Antidote") };
			ClassAsset.ProgressionChoices.Add(Choice);
		}
		{
			FRPGClassProgressionChoiceDefinition Choice = MakeChoice(
				DefensiveElixirTalent, 10, ApothecaryBranchId, AntidoteTalent);
			Choice.GrantedRequirementIds = {
				TEXT("Recipe_DefensiveElixir_Fire"), TEXT("Recipe_DefensiveElixir_Ice"),
				TEXT("Recipe_DefensiveElixir_Lightning"), TEXT("Recipe_DefensiveElixir_Poison")
			};
			ClassAsset.ProgressionChoices.Add(Choice);
		}
		{
			FRPGClassProgressionChoiceDefinition Choice = MakeChoice(
				DiffusionTalent, 14, ApothecaryBranchId,
				DefensiveElixirTalent);
			FGridCombatModifierProfile Modifier;
			Modifier.SourcePolicies = { EGridCombatActionSourcePolicy::QuickItem };
			Modifier.RequiredSourceTags = { TEXT("QuickItem.Potion.Positive") };
			Modifier.QuickItemSecondaryTargetCount = 1;
			Modifier.QuickItemSecondaryMagnitudePercent = 50;
			Modifier.QuickItemSecondaryDurationPercent = 50;
			Choice.CombatModifiers.Add(Modifier);
			ClassAsset.ProgressionChoices.Add(Choice);
		}
		{
			FRPGClassProgressionChoiceDefinition Choice = MakeChoice(
				PanaceaTalent, 18, ApothecaryBranchId, DiffusionTalent);
			Choice.GrantedRequirementIds = { TEXT("Recipe_Panacea") };
			ClassAsset.ProgressionChoices.Add(Choice);
		}

		// Transmuter
		{
			FRPGClassProgressionChoiceDefinition Choice = MakeChoice(
				OilSlickTalent, 2, TransmuterBranchId);
			Choice.GrantedRequirementIds = { TEXT("Recipe_Flask_Oil") };
			ClassAsset.ProgressionChoices.Add(Choice);
		}
		{
			FRPGClassProgressionChoiceDefinition Choice = MakeChoice(
				AcidFlaskTalent, 6, TransmuterBranchId, OilSlickTalent);
			Choice.GrantedRequirementIds = { TEXT("Recipe_Flask_Acid") };
			ClassAsset.ProgressionChoices.Add(Choice);
		}
		{
			FRPGClassProgressionChoiceDefinition Choice = MakeChoice(
				CorrosiveCloudTalent, 10, TransmuterBranchId, AcidFlaskTalent);
			Choice.GrantedRequirementIds = { TEXT("Recipe_Flask_CorrosiveCloud") };
			ClassAsset.ProgressionChoices.Add(Choice);
		}
		{
			FRPGClassProgressionChoiceDefinition Choice = MakeChoice(
				CatalystTalent, 14, TransmuterBranchId, CorrosiveCloudTalent);
			ClassAsset.ProgressionChoices.Add(Choice);

			FGridCombatActionDefinition Action;
			Action.ActionId = CatalystAction;
			Action.ActionType = EGridCombatActionType::Ability;
			Action.SourcePolicy = EGridCombatActionSourcePolicy::Ability;
			Action.TargetingPolicy = EGridCombatTargetingPolicy::Cell;
			Action.ResolutionProfile = EGridCombatActionResolutionProfile::Effect;
			Action.ActionPointCost = 1;
			Action.RangeCells = 4;
			Action.bRequiresLineOfSight = true;
			Action.CooldownRounds = 2;
			Action.Requirements = { CatalystTalent };
			Action.SurfaceInteraction = EGridCombatSurfaceInteraction::AnyCanonical;
			ClassAsset.CombatActions.Add(Action);
		}
		{
			FRPGClassProgressionChoiceDefinition Choice = MakeChoice(
				MajorTransmutationTalent,
				18, TransmuterBranchId, CatalystTalent);
			Choice.GrantedRequirementIds = {
				TEXT("Recipe_MajorTransmutation_Fire"), TEXT("Recipe_MajorTransmutation_Ice"),
				TEXT("Recipe_MajorTransmutation_Poison"), TEXT("Recipe_MajorTransmutation_Oil")
			};
			FGridCombatModifierProfile ReactionBonus;
			ReactionBonus.ActionIds = { MajorTransmutationAction };
			ReactionBonus.SourcePolicies = { EGridCombatActionSourcePolicy::QuickItem };
			ReactionBonus.SurfaceReactionDamagePercentModifier = 50;
			Choice.CombatModifiers.Add(ReactionBonus);
			ClassAsset.ProgressionChoices.Add(Choice);
		}
	});
}

bool FRPGAlchemistAuthoring::ConfigureStatus(UGridStatusEffectDefinitionAsset& StatusAsset, FName EffectId)
{
	const FName CanonicalEffectId = EffectId;
	const bool bPreserveSerializedText =
		!CanonicalEffectId.IsNone() && StatusAsset.EffectId == CanonicalEffectId;
	const FText ExistingDisplayName = StatusAsset.DisplayName;
	const FText ExistingDescription = StatusAsset.Description;

	const bool bConfigured = [&]()
	{
	using namespace RPGAlchemistAuthoring;
	StatusAsset.EffectId = EffectId;
	StatusAsset.DisplayName = FText::FromName(CanonicalEffectId);
	StatusAsset.Description = FText::FromString(
		FString::Printf(TEXT("AUTHOR_TEXT_REQUIRED:%s"), *CanonicalEffectId.ToString()));
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

	if (EffectId == PoisonStatus)
	{
		StatusAsset.StatusTags = { TEXT("Purifiable"), TEXT("Dispel.Magical"), TEXT("Toxin"), TEXT("Elemental.Poison") };
		StatusAsset.Disposition = EGridStatusEffectDisposition::Debuff;
		StatusAsset.DurationUnit = EGridStatusEffectDurationUnit::Turns;
		StatusAsset.DefaultDuration = 3;
		StatusAsset.PeriodicDamage.DamageType = EGridDamageType::Poison;
		StatusAsset.PeriodicDamage.DamagePerStack = 2;
		return StatusAsset.IsValidDefinition();
	}

	FGridDamageResistanceSet Resistance;
	bool bDefensiveStatus = true;
	if (EffectId == DefensiveFireStatus) { Resistance.FireResistance = 25; }
	else if (EffectId == DefensiveIceStatus) { Resistance.IceResistance = 25; }
	else if (EffectId == DefensiveLightningStatus) { Resistance.LightningResistance = 25; }
	else if (EffectId == DefensivePoisonStatus) { Resistance.PoisonResistance = 25; }
	else { bDefensiveStatus = false; }
	if (bDefensiveStatus)
	{
		StatusAsset.StatusTags = { TEXT("Dispel.Magical") };
		StatusAsset.Disposition = EGridStatusEffectDisposition::Buff;
		StatusAsset.DurationUnit = EGridStatusEffectDurationUnit::Rounds;
		StatusAsset.DefaultDuration = 3;
		FGridCombatModifierProfile Modifier;
		Modifier.ResistanceModifiers = Resistance;
		StatusAsset.CombatModifiers.Add(Modifier);
		return StatusAsset.IsValidDefinition();
	}
	if (EffectId == CorrodedStatus)
	{
		StatusAsset.StatusTags = { TEXT("Purifiable"), TEXT("Dispel.Magical"), TEXT("Corrosion") };
		StatusAsset.Disposition = EGridStatusEffectDisposition::Debuff;
		StatusAsset.DurationUnit = EGridStatusEffectDurationUnit::Rounds;
		StatusAsset.DefaultDuration = 2;
		FGridCombatModifierProfile Modifier;
		Modifier.PhysicalArmorRestorationPercentModifier = -20;
		StatusAsset.CombatModifiers.Add(Modifier);
		return StatusAsset.IsValidDefinition();
	}
	return false;

	}();

	if (!bConfigured)
	{
		return false;
	}

	if (bPreserveSerializedText)
	{
		StatusAsset.DisplayName = ExistingDisplayName;
		StatusAsset.Description = ExistingDescription;
	}

	return StatusAsset.IsValidDefinition();
}

void FRPGAlchemistAuthoring::GetB1ItemIds(TArray<FName>& OutItemIds)
{
	using namespace RPGAlchemistAuthoring;
	OutItemIds = {
		FireBombItem, ToxicBombItem, AntidoteItem,
		DefensiveFireItem, DefensiveIceItem, DefensiveLightningItem, DefensivePoisonItem, PanaceaItem
	};
}

void FRPGAlchemistAuthoring::GetB1StatusIds(TArray<FName>& OutStatusIds)
{
	using namespace RPGAlchemistAuthoring;
	OutStatusIds = {
		PoisonStatus, DefensiveFireStatus, DefensiveIceStatus, DefensiveLightningStatus, DefensivePoisonStatus
	};
}

void FRPGAlchemistAuthoring::GetB2ItemIds(TArray<FName>& OutItemIds)
{
	using namespace RPGAlchemistAuthoring;
	OutItemIds = { OilSlickItem, AcidFlaskItem, CorrosiveCloudItem };
}

void FRPGAlchemistAuthoring::GetB2StatusIds(TArray<FName>& OutStatusIds)
{
	using namespace RPGAlchemistAuthoring;
	OutStatusIds = { CorrodedStatus };
}

bool FRPGAlchemistAuthoring::BuildMajorTransmutationRecipeAction(
	EGridCombatSurfaceType OutputSurfaceType, FGridCombatActionDefinition& OutAction)
{
	using namespace RPGAlchemistAuthoring;
	OutAction = FGridCombatActionDefinition();
	if (OutputSurfaceType != EGridCombatSurfaceType::Fire &&
		OutputSurfaceType != EGridCombatSurfaceType::Ice &&
		OutputSurfaceType != EGridCombatSurfaceType::Poison &&
		OutputSurfaceType != EGridCombatSurfaceType::Oil)
	{
		return false;
	}

	OutAction = MakeQuickItemAction(
		MajorTransmutationAction, EGridCombatTargetingPolicy::Area,
		EGridCombatActionResolutionProfile::Effect, 4, 4, 5);
	OutAction.AreaRadiusCells = 2;
	OutAction.Requirements = { MajorTransmutationTalent };
	OutAction.SourceTags = { TEXT("QuickItem.Alchemy"), TEXT("QuickItem.Catalyst") };

	FGridCombatSurfaceConversionProfile Conversion;
	Conversion.bAllowEmptyCell = true;
	Conversion.InputSurfaceTypes = {
		EGridCombatSurfaceType::Fire, EGridCombatSurfaceType::Water, EGridCombatSurfaceType::Ice,
		EGridCombatSurfaceType::Poison, EGridCombatSurfaceType::Oil, EGridCombatSurfaceType::ElectrifiedWater,
		EGridCombatSurfaceType::Smoke, EGridCombatSurfaceType::PoisonCloud, EGridCombatSurfaceType::Blood
	};
	Conversion.OutputSurfaceType = OutputSurfaceType;
	Conversion.EmptyCellDurationRounds = 4;
	Conversion.bUseFixedFinalDuration = true;
	Conversion.FixedFinalDurationRounds = 4;
	Conversion.bTriggerReactionBeforeConversion =
		OutputSurfaceType == EGridCombatSurfaceType::Fire || OutputSurfaceType == EGridCombatSurfaceType::Ice;
	Conversion.OutputTraversalCostModifier = OutputSurfaceType == EGridCombatSurfaceType::Oil ? 1 : 0;
	OutAction.SurfaceConversions.Add(Conversion);
	return OutAction.IsValid();
}

bool FRPGAlchemistAuthoring::PlanUnregisteredItems(const UGridItemSubsystem& Registry,
    TArray<FName>& OutItemIds, FString& OutError)
{
    OutItemIds.Reset();
    OutError.Reset();
    if (!Registry.IsConfigured() || !Registry.ContainsItem(TEXT("Item_Flask_Oil")))
    {
        OutError = TEXT("Canonical Oil/root must be available before legacy alchemy authoring.");
        return false;
    }
    TArray<FName> All, B2, Pending;
    GetB1ItemIds(All);
    GetB2ItemIds(B2);
    All.Append(B2);
    TSet<FName> Seen;
    for (FName Id : All)
    {
        if (Id.IsNone() || Seen.Contains(Id))
        {
            OutError = TEXT("Invalid or duplicate alchemy source identity.");
            return false;
        }
        Seen.Add(Id);
        if (!Registry.ContainsItem(Id))
        {
            Pending.Add(Id);
            continue;
        }
        FGridResolvedItem Item;
        FGridCombatActionDefinition Action;
        if (!Registry.ResolveItem(Id, 0, Item) || !Item.IsValid() ||
            Item.bIsArmament || Item.AvailableActions.Num() != 1 ||
            !Registry.ResolveDetailedNonArmAction(Id, 0, Item.AvailableActions[0].ActionId, Action) ||
            Action.SourcePolicy != EGridCombatActionSourcePolicy::QuickItem)
        {
            OutError = TEXT("Invalid canonical alchemy owner: ") + Id.ToString();
            return false;
        }
    }
    if (All.Num() != 11 || (Pending.Num() != 10 && Pending.Num() != 0))
    {
        OutError = TEXT("Unexpected partial alchemy migration; authoring refused before writes.");
        return false;
    }
    OutItemIds = MoveTemp(Pending);
    return true;
}

bool FRPGAlchemistAuthoring::AuthorProductionAssets(FString& OutError)
{
	using namespace RPGAlchemistAuthoring;
	OutError.Reset();

	URPGClassAsset* Alchemist = LoadObject<URPGClassAsset>(nullptr, AlchemistAssetPath());
	if (!IsValid(Alchemist))
	{
		OutError = FString::Printf(TEXT("Production Alchemist asset not found: %s"), AlchemistAssetPath());
		return false;
	}
	if (Alchemist->ClassId != AlchemistClassId)
	{
		OutError = FString::Printf(TEXT("Unexpected Alchemist ClassId '%s'."), *Alchemist->ClassId.ToString());
		return false;
	}
	if (!ValidateSharedStatus(BurningStatus, OutError))
	{
		return false;
	}

    // The configured canonical system is the authority. Do not recreate or
    // mutate old item packages for registered IDs (Oil already now, all
    // eleven after cutover). Class and status authoring are independent.
    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
    TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
    UGridItemSystemAsset* Root = Registry->LoadConfiguredItemSystemForPreview();
    TArray<FString> RegistryErrors;
    TArray<FName> ItemIds;
    if (!Registry->RegisterItemSystem(Root, RegistryErrors))
    {
        OutError = TEXT("Canonical authoring guard: ") + FString::Join(RegistryErrors, TEXT("; "));
        return false;
    }
    if (!PlanUnregisteredItems(*Registry.Get(), ItemIds, OutError)) return false;

	// All eleven Alchemist items now belong to DA_ItemSystem. A registry
	// missing any of them is incomplete, not permission to rebuild DA_Item_*
	// source assets. No legacy package may be created or modified here.
	if (!ItemIds.IsEmpty())
	{
		OutError = TEXT("The canonical Alchemist ItemSystem is incomplete; legacy item authoring is retired.");
		return false;
	}

	TArray<FName> StatusIds;
	TArray<FName> B2StatusIds;
	GetB1StatusIds(StatusIds);
	GetB2StatusIds(B2StatusIds);
	StatusIds.Append(B2StatusIds);

	TArray<UGridStatusEffectDefinitionAsset*> Statuses;
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
			OutError = FString::Printf(TEXT("No valid Alchemist status authoring definition for '%s'."), *StatusId.ToString());
			return false;
		}
		Statuses.Add(Status);
	}

	Alchemist->Modify();
	ConfigureClass(*Alchemist);
	if (!Alchemist->IsValidDefinition())
	{
		OutError = TEXT("Authored DA_Class_Alchemist is structurally invalid.");
		return false;
	}

	for (UGridStatusEffectDefinitionAsset* Status : Statuses)
	{
		if (!SaveAuthoredAsset(Status, OutError))
		{
			return false;
		}
	}
	return SaveAuthoredAsset(Alchemist, OutError);
}
