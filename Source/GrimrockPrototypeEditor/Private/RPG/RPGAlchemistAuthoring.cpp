#include "RPG/RPGAlchemistAuthoring.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "HAL/FileManager.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGClassProgressionAuthoring.h"
#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

namespace RPGAlchemistAuthoring
{
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
	const FName RareCatalystItem(TEXT("Item_Catalyst_Rare"));

	const FName FireBombAction(TEXT("Action_Alchemist_FireBomb"));
	const FName ToxicBombAction(TEXT("Action_Alchemist_ToxicBomb"));
	const FName AntidoteAction(TEXT("Action_Alchemist_Antidote"));
	const FName DefensiveElixirAction(TEXT("Action_Alchemist_DefensiveElixir"));
	const FName PanaceaAction(TEXT("Action_Alchemist_Panacea"));
	const FName OilSlickAction(TEXT("Action_Alchemist_OilSlick"));
	const FName AcidFlaskAction(TEXT("Action_Alchemist_AcidFlask"));
	const FName CorrosiveCloudAction(TEXT("Action_Alchemist_CorrosiveCloud"));
	const FName CatalystAction(TEXT("Action_Alchemist_Catalyst"));
	const FName MajorTransmutationAction(TEXT("Action_Alchemist_MajorTransmutation"));

	const FName PoisonStatus(TEXT("Status_Poison"));
	const FName DefensiveFireStatus(TEXT("Status_DefensiveElixir_Fire"));
	const FName DefensiveIceStatus(TEXT("Status_DefensiveElixir_Ice"));
	const FName DefensiveLightningStatus(TEXT("Status_DefensiveElixir_Lightning"));
	const FName DefensivePoisonStatus(TEXT("Status_DefensiveElixir_Poison"));
	const FName CorrodedStatus(TEXT("Status_Corroded"));

	FRPGClassProgressionChoiceDefinition MakeChoice(
		FName ChoiceId, const TCHAR* DisplayName, const TCHAR* Description, int32 Level, FName TalentBranchId, FName Prerequisite = NAME_None, FName TalentNodeId = NAME_None)
	{
		FRPGClassProgressionChoiceDefinition Choice;
		Choice.ChoiceId = ChoiceId;
		Choice.TalentBranchId = TalentBranchId;
		Choice.TalentNodeId = TalentNodeId.IsNone() ? ChoiceId : TalentNodeId;
		Choice.DisplayName = FText::FromString(DisplayName);
		Choice.Description = FText::FromString(Description);
		Choice.MinimumLevel = Level;
		Choice.PointCost = 1;
		if (!Prerequisite.IsNone())
		{
			Choice.PrerequisiteChoiceIds.Add(Prerequisite);
		}
		return Choice;
	}

	FGridCombatActionDefinition MakeQuickItemAction(
		FName ActionId, EGridCombatTargetingPolicy Targeting, EGridCombatActionResolutionProfile Resolution,
		int32 ActionPointCost, int32 RangeCells, int32 CooldownRounds = 0)
	{
		FGridCombatActionDefinition Action;
		Action.ActionId = ActionId;
		Action.DisplayName = FText::FromName(ActionId);
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

	FGridCombatActionDefinition MakeBomb(
		FName ActionId, EGridDamageType DamageType, FName StatusId, int32 StatusDuration,
		EGridCombatSurfaceType SurfaceType, int32 SurfaceDuration)
	{
		FGridCombatActionDefinition Action = MakeQuickItemAction(
			ActionId, EGridCombatTargetingPolicy::Area, EGridCombatActionResolutionProfile::Attack, 2, 4);
		Action.AreaRadiusCells = 1;
		Action.bAffectsAlliesInArea = true;
		Action.OffensiveProfile.AttackId = ActionId;
		Action.OffensiveProfile.AttackDefinition.DamageType = DamageType;
		Action.OffensiveProfile.AttackDefinition.MinDamage = 6;
		Action.OffensiveProfile.AttackDefinition.MaxDamage = 6;
		Action.OffensiveProfile.AttackDefinition.bAlwaysHits = true;
		Action.OffensiveProfile.AttackDefinition.bCanCriticalHit = false;
		Action.OffensiveProfile.RangeCells = 4;
		Action.QuickItemScaling.ScalingSkillId = TEXT("Skill_Alchemy");
		Action.QuickItemScaling.DirectDamageSkillRankScale = 1;

		FGridCombatStatusApplicationProfile Status;
		Status.StatusEffectId = StatusId;
		Status.Trigger = EGridCombatStatusApplicationTrigger::AfterSuccessfulHit;
		Status.ArmorGate = EGridCombatStatusArmorGate::MagicalArmorDepleted;
		Status.DurationOverride = StatusDuration;
		Action.StatusApplications.Add(Status);

		FGridCombatSurfaceEffectProfile Surface;
		Surface.SurfaceType = SurfaceType;
		Surface.DurationRounds = SurfaceDuration;
		Action.SurfaceEffects.Add(Surface);
		return Action;
	}

	FGridCombatArmorEffectProfile MakeMagicalArmorRestore(int32 Amount)
	{
		FGridCombatArmorEffectProfile Profile;
		Profile.Pool = EGridCombatArmorPool::Magical;
		Profile.Operation = EGridCombatArmorEffectOperation::Restore;
		Profile.Magnitude = EGridCombatArmorEffectMagnitude::Flat;
		Profile.Trigger = EGridCombatArmorEffectTrigger::AfterResolution;
		Profile.Amount = Amount;
		return Profile;
	}

	FGridCombatStatusApplicationProfile MakeStatus(FName StatusId, int32 Duration)
	{
		FGridCombatStatusApplicationProfile Profile;
		Profile.StatusEffectId = StatusId;
		Profile.Trigger = EGridCombatStatusApplicationTrigger::AfterResolution;
		Profile.ArmorGate = EGridCombatStatusArmorGate::None;
		Profile.DurationOverride = Duration;
		return Profile;
	}

	FGridCombatArmorEffectProfile MakePhysicalArmorDamage(int32 Amount, int32 AlchemyScale)
	{
		FGridCombatArmorEffectProfile Profile;
		Profile.Pool = EGridCombatArmorPool::Physical;
		Profile.Operation = EGridCombatArmorEffectOperation::Damage;
		Profile.Magnitude = EGridCombatArmorEffectMagnitude::Flat;
		Profile.Trigger = EGridCombatArmorEffectTrigger::AfterResolution;
		Profile.Amount = Amount;
		if (AlchemyScale > 0)
		{
			Profile.ScalingSkillId = TEXT("Skill_Alchemy");
			Profile.SkillRankScale = AlchemyScale;
		}
		return Profile;
	}

	void ConfigureItemBase(UGridItemDefinitionAsset& Item, FName ItemId, const TCHAR* DisplayName, EGridItemType Type, FName ActionId)
	{
		Item.ItemDefinitionId = ItemId;
		Item.DisplayName = FText::FromString(DisplayName);
		Item.ItemType = Type;
		Item.HandUsage = EGridItemHandUsage::NotHandHeld;
		Item.bProvidesQuickItemCombatAction = true;
		Item.QuickItemActionIdOverride = ActionId;
		Item.ItemTags = { TEXT("QuickItem.Alchemy") };
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

	UGridItemDefinitionAsset* FindOrCreateItem(FName ItemDefinitionId, FString& OutError)
	{
		if (ItemDefinitionId.IsNone())
		{
			OutError = TEXT("Cannot author an empty Alchemist item id.");
			return nullptr;
		}
		const FString AssetName = FString::Printf(TEXT("DA_%s"), *ItemDefinitionId.ToString());
		const FString PackageName =
			FString::Printf(TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/%s"), *AssetName);
		if (FPackageName::DoesPackageExist(PackageName))
		{
			const FString ObjectPath = FRPGAlchemistAuthoring::GetItemObjectPath(ItemDefinitionId);
			if (UGridItemDefinitionAsset* Existing = LoadObject<UGridItemDefinitionAsset>(nullptr, *ObjectPath))
			{
				return Existing;
			}
			OutError = FString::Printf(TEXT("Existing Alchemist item package could not load expected asset: %s"), *ObjectPath);
			return nullptr;
		}

		UPackage* Package = CreatePackage(*PackageName);
		UGridItemDefinitionAsset* Created = Package
			? NewObject<UGridItemDefinitionAsset>(Package, *AssetName, RF_Public | RF_Standalone | RF_Transactional)
			: nullptr;
		if (!Created)
		{
			OutError = FString::Printf(TEXT("Failed to create Alchemist item '%s'."), *AssetName);
			return nullptr;
		}
		FAssetRegistryModule::AssetCreated(Created);
		return Created;
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
	using namespace RPGAlchemistAuthoring;

	FRPGClassProgressionAuthoring::ConfigureCanonicalTalentGrants(ClassAsset);
	ClassAsset.CombatActions.Reset();
	ClassAsset.ProgressionChoices.Reset();

	// Grenadier
	{
		FRPGClassProgressionChoiceDefinition Choice = MakeChoice(
			FireBombTalent, TEXT("Bombe incendiaire"), TEXT("Débloque la recette de Bombe incendiaire."), 2, GrenadierBranchId);
		Choice.GrantedRequirementIds = { TEXT("Recipe_Bomb_Fire") };
		ClassAsset.ProgressionChoices.Add(Choice);
	}
	{
		FRPGClassProgressionChoiceDefinition Choice = MakeChoice(
			ToxicBombTalent, TEXT("Bombe toxique"), TEXT("Débloque la recette de Bombe toxique."), 6, GrenadierBranchId, FireBombTalent);
		Choice.GrantedRequirementIds = { TEXT("Recipe_Bomb_Toxic") };
		ClassAsset.ProgressionChoices.Add(Choice);
	}
	{
		FRPGClassProgressionChoiceDefinition Choice = MakeChoice(
			PreciseChargeTalent, TEXT("Charge précise"), TEXT("Bombes : +1 portée et -50 % dégâts directs aux alliés."), 10, GrenadierBranchId, ToxicBombTalent);
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
			ChainReactionTalent, TEXT("Réaction en chaîne"), TEXT("Une réaction de surface de bombe par action reçoit +25 % dégâts et +1 rayon."), 14, GrenadierBranchId,
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
			MasterGrenadierTalent, TEXT("Maître grenadier"), TEXT("Bombes : -1 PA (minimum 1) et +20 % dégâts directs."), 18, GrenadierBranchId, ChainReactionTalent);
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
			EnhancedPotionTalent, TEXT("Potion renforcée"), TEXT("Potions positives : +25 % Health/Mana/Armor, durée inchangée."), 2, ApothecaryBranchId);
		FGridCombatModifierProfile Modifier;
		Modifier.SourcePolicies = { EGridCombatActionSourcePolicy::QuickItem };
		Modifier.RequiredSourceTags = { TEXT("QuickItem.Potion.Positive") };
		Modifier.PositiveEffectPercentModifier = 25;
		Choice.CombatModifiers.Add(Modifier);
		ClassAsset.ProgressionChoices.Add(Choice);
	}
	{
		FRPGClassProgressionChoiceDefinition Choice = MakeChoice(
			AntidoteTalent, TEXT("Antidote"), TEXT("Débloque la recette d'Antidote."), 6, ApothecaryBranchId, EnhancedPotionTalent);
		Choice.GrantedRequirementIds = { TEXT("Recipe_Antidote") };
		ClassAsset.ProgressionChoices.Add(Choice);
	}
	{
		FRPGClassProgressionChoiceDefinition Choice = MakeChoice(
			DefensiveElixirTalent, TEXT("Élixir défensif"), TEXT("Débloque quatre variantes élémentaires d'Élixir défensif."), 10, ApothecaryBranchId, AntidoteTalent);
		Choice.GrantedRequirementIds = {
			TEXT("Recipe_DefensiveElixir_Fire"), TEXT("Recipe_DefensiveElixir_Ice"),
			TEXT("Recipe_DefensiveElixir_Lightning"), TEXT("Recipe_DefensiveElixir_Poison")
		};
		ClassAsset.ProgressionChoices.Add(Choice);
	}
	{
		FRPGClassProgressionChoiceDefinition Choice = MakeChoice(
			DiffusionTalent, TEXT("Diffusion"), TEXT("Une potion positive peut diffuser 50 % de sa magnitude et durée à un second allié."), 14, ApothecaryBranchId,
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
			PanaceaTalent, TEXT("Panacée"), TEXT("Débloque la recette de Panacée."), 18, ApothecaryBranchId, DiffusionTalent);
		Choice.GrantedRequirementIds = { TEXT("Recipe_Panacea") };
		ClassAsset.ProgressionChoices.Add(Choice);
	}

	// Transmuter
	{
		FRPGClassProgressionChoiceDefinition Choice = MakeChoice(
			OilSlickTalent, TEXT("Huile glissante"), TEXT("Débloque la recette de Flasque d'huile."), 2, TransmuterBranchId);
		Choice.GrantedRequirementIds = { TEXT("Recipe_Flask_Oil") };
		ClassAsset.ProgressionChoices.Add(Choice);
	}
	{
		FRPGClassProgressionChoiceDefinition Choice = MakeChoice(
			AcidFlaskTalent, TEXT("Flasque acide"), TEXT("Débloque la recette de Flasque acide."), 6, TransmuterBranchId, OilSlickTalent);
		Choice.GrantedRequirementIds = { TEXT("Recipe_Flask_Acid") };
		ClassAsset.ProgressionChoices.Add(Choice);
	}
	{
		FRPGClassProgressionChoiceDefinition Choice = MakeChoice(
			CorrosiveCloudTalent, TEXT("Nuage corrosif"), TEXT("Débloque la recette de Flasque de nuage corrosif."), 10, TransmuterBranchId, AcidFlaskTalent);
		Choice.GrantedRequirementIds = { TEXT("Recipe_Flask_CorrosiveCloud") };
		ClassAsset.ProgressionChoices.Add(Choice);
	}
	{
		FRPGClassProgressionChoiceDefinition Choice = MakeChoice(
			CatalystTalent, TEXT("Catalyseur"), TEXT("Débloque Catalyseur."), 14, TransmuterBranchId, CorrosiveCloudTalent);
		ClassAsset.ProgressionChoices.Add(Choice);

		FGridCombatActionDefinition Action;
		Action.ActionId = CatalystAction;
		Action.DisplayName = FText::FromString(TEXT("Catalyseur"));
		Action.Description = FText::FromString(TEXT("Déclenche immédiatement une réaction canonique sur la surface ciblée."));
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
			MajorTransmutationTalent, TEXT("Transmutation majeure"),
			TEXT("Débloque les recettes de Transmutation majeure Fire/Ice/Poison/Oil utilisant un Catalyseur rare."),
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
}

bool FRPGAlchemistAuthoring::ConfigureItem(UGridItemDefinitionAsset& Item, FName ItemDefinitionId)
{
	using namespace RPGAlchemistAuthoring;

	// UObject instances are never value-assigned. Reset only gameplay authoring owned by
	// this helper so re-authoring preserves presentation/mesh/icon fields.
	Item.ItemDefinitionId = NAME_None;
	Item.DisplayName = FText::GetEmpty();
	Item.Description = FText::GetEmpty();
	Item.ItemType = EGridItemType::None;
	Item.HandUsage = EGridItemHandUsage::NotHandHeld;
	Item.CompatibleEquipmentSlots.Reset();
	Item.CombatActions.Reset();
	Item.bProvidesQuickItemCombatAction = false;
	Item.QuickItemCombatAction = FGridCombatActionDefinition();
	Item.QuickItemActionIdOverride = NAME_None;
	Item.bCombatThrowWeapon = false;
	Item.ItemTags.Reset();

	if (ItemDefinitionId == FireBombItem)
	{
		ConfigureItemBase(Item, FireBombItem, TEXT("Bombe incendiaire"), EGridItemType::Misc, FireBombAction);
		Item.ItemTags.Add(TEXT("QuickItem.Bomb"));
		Item.QuickItemCombatAction = MakeBomb(FireBombAction, EGridDamageType::Fire, TEXT("Status_Burning"), 2, EGridCombatSurfaceType::Fire, 2);
		return Item.IsValidDefinition();
	}
	if (ItemDefinitionId == ToxicBombItem)
	{
		ConfigureItemBase(Item, ToxicBombItem, TEXT("Bombe toxique"), EGridItemType::Misc, ToxicBombAction);
		Item.ItemTags.Add(TEXT("QuickItem.Bomb"));
		Item.QuickItemCombatAction = MakeBomb(ToxicBombAction, EGridDamageType::Poison, PoisonStatus, 3, EGridCombatSurfaceType::Poison, 3);
		return Item.IsValidDefinition();
	}
	if (ItemDefinitionId == AntidoteItem)
	{
		ConfigureItemBase(Item, AntidoteItem, TEXT("Antidote"), EGridItemType::Potion, AntidoteAction);
		Item.ItemTags.Append({ TEXT("QuickItem.Potion"), TEXT("QuickItem.Potion.Positive") });
		FGridCombatActionDefinition Action = MakeQuickItemAction(
			AntidoteAction, EGridCombatTargetingPolicy::Ally, EGridCombatActionResolutionProfile::Effect, 1, 1);
		FGridCombatStatusRemovalProfile Poison;
		Poison.EffectIds = { TEXT("Status_Poison") };
		Poison.AllowedDispositions = { EGridStatusEffectDisposition::Debuff };
		Poison.TargetSide = EGridCombatStatusRemovalTargetSide::Party;
		Poison.MaximumRemovals = 1;
		Action.StatusRemovals.Add(Poison);
		FGridCombatStatusRemovalProfile Toxin;
		Toxin.AnyStatusTags = { TEXT("Toxin") };
		Toxin.AllowedDispositions = { EGridStatusEffectDisposition::Debuff };
		Toxin.TargetSide = EGridCombatStatusRemovalTargetSide::Party;
		Toxin.MaximumRemovals = 1;
		Action.StatusRemovals.Add(Toxin);
		Item.QuickItemCombatAction = Action;
		return Item.IsValidDefinition();
	}

	FName DefensiveStatus = NAME_None;
	EGridDamageType DefensiveType = EGridDamageType::Physical;
	const TCHAR* DefensiveName = nullptr;
	if (ItemDefinitionId == DefensiveFireItem)
	{
		DefensiveStatus = DefensiveFireStatus; DefensiveType = EGridDamageType::Fire; DefensiveName = TEXT("Élixir défensif — Feu");
	}
	else if (ItemDefinitionId == DefensiveIceItem)
	{
		DefensiveStatus = DefensiveIceStatus; DefensiveType = EGridDamageType::Ice; DefensiveName = TEXT("Élixir défensif — Glace");
	}
	else if (ItemDefinitionId == DefensiveLightningItem)
	{
		DefensiveStatus = DefensiveLightningStatus; DefensiveType = EGridDamageType::Lightning; DefensiveName = TEXT("Élixir défensif — Foudre");
	}
	else if (ItemDefinitionId == DefensivePoisonItem)
	{
		DefensiveStatus = DefensivePoisonStatus; DefensiveType = EGridDamageType::Poison; DefensiveName = TEXT("Élixir défensif — Poison");
	}
	if (!DefensiveStatus.IsNone())
	{
		ConfigureItemBase(Item, ItemDefinitionId, DefensiveName, EGridItemType::Potion, DefensiveElixirAction);
		Item.ItemTags.Append({ TEXT("QuickItem.Potion"), TEXT("QuickItem.Potion.Positive") });
		FGridCombatActionDefinition Action = MakeQuickItemAction(
			DefensiveElixirAction, EGridCombatTargetingPolicy::Ally, EGridCombatActionResolutionProfile::Effect, 1, 1);
		Action.ArmorEffects.Add(MakeMagicalArmorRestore(4));
		Action.StatusApplications.Add(MakeStatus(DefensiveStatus, 3));
		Item.QuickItemCombatAction = Action;
		(void)DefensiveType;
		return Item.IsValidDefinition();
	}

	if (ItemDefinitionId == PanaceaItem)
	{
		ConfigureItemBase(Item, PanaceaItem, TEXT("Panacée"), EGridItemType::Potion, PanaceaAction);
		Item.ItemTags.Append({ TEXT("QuickItem.Potion"), TEXT("QuickItem.Potion.Positive") });
		FGridCombatActionDefinition Action = MakeQuickItemAction(
			PanaceaAction, EGridCombatTargetingPolicy::Ally, EGridCombatActionResolutionProfile::Effect, 2, 1, 3);
		Action.EffectProfile.RestoreHealth = 10;
		Action.QuickItemScaling.ScalingSkillId = TEXT("Skill_Alchemy");
		Action.QuickItemScaling.RestoreHealthSkillRankScale = 2;
		Action.ArmorEffects.Add(MakeMagicalArmorRestore(8));
		FGridCombatStatusRemovalProfile Removal;
		Removal.EffectIds = {
			TEXT("Status_Poison"), TEXT("Status_Burning"), TEXT("Status_Bleeding"),
			TEXT("Status_Slow"), TEXT("Status_Silence"), TEXT("Status_Immobilized")
		};
		Removal.AnyStatusTags = { TEXT("Toxin"), TEXT("Purifiable") };
		Removal.AllowedDispositions = { EGridStatusEffectDisposition::Debuff };
		Removal.TargetSide = EGridCombatStatusRemovalTargetSide::Party;
		Removal.MaximumRemovals = 3;
		Action.StatusRemovals.Add(Removal);
		Item.QuickItemCombatAction = Action;
		return Item.IsValidDefinition();
	}
	if (ItemDefinitionId == OilSlickItem)
	{
		ConfigureItemBase(Item, OilSlickItem, TEXT("Flasque d'huile"), EGridItemType::Misc, OilSlickAction);
		Item.ItemTags.Add(TEXT("QuickItem.Flask"));
		FGridCombatActionDefinition Action = MakeQuickItemAction(
			OilSlickAction, EGridCombatTargetingPolicy::Area, EGridCombatActionResolutionProfile::Effect, 2, 4);
		Action.AreaRadiusCells = 1;
		FGridCombatSurfaceEffectProfile Oil;
		Oil.SurfaceType = EGridCombatSurfaceType::Oil;
		Oil.DurationRounds = 4;
		Oil.TraversalCostModifier = 1;
		Action.SurfaceEffects.Add(Oil);
		Item.QuickItemCombatAction = Action;
		return Item.IsValidDefinition();
	}
	if (ItemDefinitionId == AcidFlaskItem)
	{
		ConfigureItemBase(Item, AcidFlaskItem, TEXT("Flasque acide"), EGridItemType::Misc, AcidFlaskAction);
		Item.ItemTags.Add(TEXT("QuickItem.Flask"));
		FGridCombatActionDefinition Action = MakeQuickItemAction(
			AcidFlaskAction, EGridCombatTargetingPolicy::Hostile, EGridCombatActionResolutionProfile::Effect, 2, 4, 1);
		Action.ArmorEffects.Add(MakePhysicalArmorDamage(6, 2));
		Action.StatusApplications.Add(MakeStatus(CorrodedStatus, 2));
		Item.QuickItemCombatAction = Action;
		return Item.IsValidDefinition();
	}
	if (ItemDefinitionId == CorrosiveCloudItem)
	{
		ConfigureItemBase(Item, CorrosiveCloudItem, TEXT("Flasque de nuage corrosif"), EGridItemType::Misc, CorrosiveCloudAction);
		Item.ItemTags.Add(TEXT("QuickItem.Flask"));
		FGridCombatActionDefinition Action = MakeQuickItemAction(
			CorrosiveCloudAction, EGridCombatTargetingPolicy::Area, EGridCombatActionResolutionProfile::Attack, 3, 4, 2);
		Action.AreaRadiusCells = 1;
		Action.OffensiveProfile.AttackId = CorrosiveCloudAction;
		Action.OffensiveProfile.AttackDefinition.DamageType = EGridDamageType::Poison;
		Action.OffensiveProfile.AttackDefinition.MinDamage = 4;
		Action.OffensiveProfile.AttackDefinition.MaxDamage = 4;
		Action.OffensiveProfile.AttackDefinition.bAlwaysHits = true;
		Action.OffensiveProfile.AttackDefinition.bCanCriticalHit = false;
		Action.OffensiveProfile.RangeCells = 4;
		Action.QuickItemScaling.ScalingSkillId = TEXT("Skill_Alchemy");
		Action.QuickItemScaling.DirectDamageSkillRankScale = 1;
		FGridCombatSurfaceEffectProfile Cloud;
		Cloud.SurfaceType = EGridCombatSurfaceType::PoisonCloud;
		Cloud.DurationRounds = 3;
		Cloud.PeriodicDamageType = EGridDamageType::Poison;
		Cloud.PeriodicDamagePerRound = 2;
		FGridCombatStatusApplicationProfile Poison;
		Poison.StatusEffectId = PoisonStatus;
		Poison.Trigger = EGridCombatStatusApplicationTrigger::AfterResolution;
		Poison.ArmorGate = EGridCombatStatusArmorGate::MagicalArmorDepleted;
		Poison.DurationOverride = 2;
		Cloud.PeriodicStatusApplications.Add(Poison);
		Action.SurfaceEffects.Add(Cloud);
		Item.QuickItemCombatAction = Action;
		return Item.IsValidDefinition();
	}

	return false;
}

bool FRPGAlchemistAuthoring::ConfigureStatus(UGridStatusEffectDefinitionAsset& StatusAsset, FName EffectId)
{
	using namespace RPGAlchemistAuthoring;
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

	if (EffectId == PoisonStatus)
	{
		StatusAsset.DisplayName = FText::FromString(TEXT("Poison"));
		StatusAsset.Description = FText::FromString(TEXT("Subit 2 dégâts de Poison à chaque tick pendant 3 tours."));
		StatusAsset.StatusTags = { TEXT("Purifiable"), TEXT("Dispel.Magical"), TEXT("Toxin"), TEXT("Elemental.Poison") };
		StatusAsset.Disposition = EGridStatusEffectDisposition::Debuff;
		StatusAsset.DurationUnit = EGridStatusEffectDurationUnit::Turns;
		StatusAsset.DefaultDuration = 3;
		StatusAsset.PeriodicDamage.DamageType = EGridDamageType::Poison;
		StatusAsset.PeriodicDamage.DamagePerStack = 2;
		return StatusAsset.IsValidDefinition();
	}

	FGridDamageResistanceSet Resistance;
	const TCHAR* Name = nullptr;
	if (EffectId == DefensiveFireStatus) { Resistance.FireResistance = 25; Name = TEXT("Élixir défensif — Feu"); }
	else if (EffectId == DefensiveIceStatus) { Resistance.IceResistance = 25; Name = TEXT("Élixir défensif — Glace"); }
	else if (EffectId == DefensiveLightningStatus) { Resistance.LightningResistance = 25; Name = TEXT("Élixir défensif — Foudre"); }
	else if (EffectId == DefensivePoisonStatus) { Resistance.PoisonResistance = 25; Name = TEXT("Élixir défensif — Poison"); }
	if (Name)
	{
		StatusAsset.DisplayName = FText::FromString(Name);
		StatusAsset.Description = FText::FromString(TEXT("+25 % de résistance élémentaire pendant 3 rounds."));
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
		StatusAsset.DisplayName = FText::FromString(TEXT("Corrodé"));
		StatusAsset.Description = FText::FromString(TEXT("Les restaurations d'armure physique reçues sont réduites de 20 % pendant 2 rounds."));
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
	OutAction.DisplayName = FText::FromString(TEXT("Transmutation majeure"));
	OutAction.Description = FText::FromString(TEXT("Convertit une zone de rayon 2 vers la sortie choisie par la recette."));
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
	Conversion.OutputTraversalCostModifier = OutputSurfaceType == EGridCombatSurfaceType::Oil ? 1 : 0;
	OutAction.SurfaceConversions.Add(Conversion);
	return OutAction.IsValid();
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

	TArray<FName> ItemIds;
	TArray<FName> B2ItemIds;
	GetB1ItemIds(ItemIds);
	GetB2ItemIds(B2ItemIds);
	ItemIds.Append(B2ItemIds);

	TArray<UGridItemDefinitionAsset*> Items;
	for (const FName ItemId : ItemIds)
	{
		UGridItemDefinitionAsset* Item = FindOrCreateItem(ItemId, OutError);
		if (!IsValid(Item))
		{
			return false;
		}
		Item->Modify();
		if (!ConfigureItem(*Item, ItemId))
		{
			OutError = FString::Printf(TEXT("No valid Alchemist item authoring definition for '%s'."), *ItemId.ToString());
			return false;
		}
		Items.Add(Item);
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

	for (UGridItemDefinitionAsset* Item : Items)
	{
		if (!SaveAuthoredAsset(Item, OutError))
		{
			return false;
		}
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
