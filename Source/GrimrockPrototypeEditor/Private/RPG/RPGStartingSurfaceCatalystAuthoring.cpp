#include "RPG/RPGStartingSurfaceCatalystAuthoring.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "HAL/FileManager.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/UObjectGlobals.h"

namespace ARMEMENT01172
{
	const TCHAR* ItemName = TEXT("DA_Item_SurfaceCatalyst_01");
	const TCHAR* ItemId = TEXT("Item_SurfaceCatalyst_01");
	const TCHAR* ActionId = TEXT("Action_Mage_SurfaceCatalyst_01");
	const TCHAR* PackageName = TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_SurfaceCatalyst_01");
	const TCHAR* ObjectPath = TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_SurfaceCatalyst_01.DA_Item_SurfaceCatalyst_01");
	const TCHAR* FlaskOilPath = TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Flask_Oil.DA_Item_Flask_Oil");
	const TCHAR* FireBombPath = TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Bomb_Fire.DA_Item_Bomb_Fire");

	void CopyExistingPresentation(UGridItemDefinitionAsset& Item)
	{
		// Temporary N1 art: existing flask mesh and icon, never a fictional asset.
		// A later art pass may replace each reference without changing gameplay.
		const UGridItemDefinitionAsset* Flask = LoadObject<UGridItemDefinitionAsset>(nullptr, FlaskOilPath);
		const UGridItemDefinitionAsset* Bomb = LoadObject<UGridItemDefinitionAsset>(nullptr, FireBombPath);
		if (IsValid(Flask))
		{
			Item.WorldMesh = Flask->WorldMesh;
			Item.Icon = Flask->Icon;
		}
		if (IsValid(Bomb))
		{
			if (Item.Icon.IsNull()) Item.Icon = Bomb->Icon;
			if (Item.WorldMesh.IsNull()) Item.WorldMesh = Bomb->WorldMesh;
		}
	}
}

void FRPGStartingSurfaceCatalystAuthoring::Configure(UGridItemDefinitionAsset& Item)
{
	Item.ItemDefinitionId = ARMEMENT01172::ItemId;
	Item.DisplayName = NSLOCTEXT("ARMEMENT01172", "SurfaceCatalystName", "Catalyseur de surface");
	Item.Description = NSLOCTEXT("ARMEMENT01172", "SurfaceCatalystDescription",
		"Réactif à usage unique. Une impulsion agit sur les surfaces d'une zone, même sur un sol neutre.");
	Item.ItemType = EGridItemType::Misc;
	Item.Weight = 0.2f;
	Item.bStackable = true;
	Item.MaxStackSize = 10;
	Item.HandUsage = EGridItemHandUsage::NotHandHeld;
	Item.ItemTags = { TEXT("QuickItem.Flask"), TEXT("QuickItem.SurfaceCatalyst") };
	ARMEMENT01172::CopyExistingPresentation(Item);
	Item.bProvidesQuickItemCombatAction = true;
	Item.QuickItemActionIdOverride = ARMEMENT01172::ActionId;

	FGridCombatActionDefinition Action;
	Action.ActionId = ARMEMENT01172::ActionId;
	Action.DisplayName = Item.DisplayName;
	Action.Description = Item.Description;
	Action.ActionType = EGridCombatActionType::Ability;
	Action.SourcePolicy = EGridCombatActionSourcePolicy::QuickItem;
	Action.TargetingPolicy = EGridCombatTargetingPolicy::Area;
	Action.AreaRadiusCells = 1;
	Action.AreaShape = EGridCombatAreaShape::Square;
	Action.bAllowNoSurfaceReaction = true;
	Action.bUsableDuringExploration = true;
	Action.ResolutionProfile = EGridCombatActionResolutionProfile::Effect;
	Action.ActionPointCost = ActionPoints;
	Action.RangeCells = Range;
	Action.bRequiresLineOfSight = true;
	Action.ResourceCosts.SourceItemQuantityCost = 1;
	Action.SurfaceInteraction = EGridCombatSurfaceInteraction::Fire;
	Item.QuickItemCombatAction = Action;
}

bool FRPGStartingSurfaceCatalystAuthoring::Matches(const UGridItemDefinitionAsset& Item)
{
	FGridCombatActionDefinition A;
	if (!Item.IsValidDefinition() || !Item.BuildQuickItemCombatActionDefinition(A) ||
		Item.ItemDefinitionId != FName(ARMEMENT01172::ItemId) ||
		Item.DisplayName.IsEmpty() || Item.Description.IsEmpty() ||
		Item.ItemType != EGridItemType::Misc ||
		!FMath::IsNearlyEqual(Item.Weight, 0.2f) ||
		!Item.bStackable || Item.MaxStackSize != 10 ||
		Item.HandUsage != EGridItemHandUsage::NotHandHeld ||
		Item.ItemTags.Num() != 2 ||
		!Item.ItemTags.Contains(TEXT("QuickItem.Flask")) ||
		!Item.ItemTags.Contains(TEXT("QuickItem.SurfaceCatalyst")) ||
		!Item.bProvidesQuickItemCombatAction || Item.bCombatThrowWeapon ||
		Item.bUseWeaponCatalogue || !Item.CombatActions.IsEmpty() ||
		Item.Icon.IsNull() || Item.WorldMesh.IsNull() ||
		Item.QuickItemActionIdOverride != FName(ARMEMENT01172::ActionId))
	{
		return false;
	}
	return A.ActionId == FName(ARMEMENT01172::ActionId) &&
		A.SourcePolicy == EGridCombatActionSourcePolicy::QuickItem &&
		A.TargetingPolicy == EGridCombatTargetingPolicy::Area &&
		A.AreaRadiusCells == 1 && A.AreaShape == EGridCombatAreaShape::Square &&
		A.bAllowNoSurfaceReaction && A.bUsableDuringExploration &&
		A.ResolutionProfile == EGridCombatActionResolutionProfile::Effect &&
		A.ActionPointCost == ActionPoints && A.RangeCells == Range &&
		A.bRequiresLineOfSight && A.ResourceCosts.ManaCost == 0 &&
		A.ResourceCosts.SourceItemQuantityCost == 1 &&
		A.SurfaceInteraction == EGridCombatSurfaceInteraction::Fire &&
		A.SurfaceEffects.IsEmpty() && A.SurfaceConversions.IsEmpty() &&
		A.StatusApplications.IsEmpty() && A.StatusRemovals.IsEmpty() &&
		A.ArmorEffects.IsEmpty() && A.MovementEffects.IsEmpty() &&
		!A.TrapEffect.bPlaceTrap && !A.bRelocatePartyToTargetCell;
}

bool FRPGStartingSurfaceCatalystAuthoring::Run(bool bApply, FString& OutError)
{
	OutError.Reset();
	UGridItemDefinitionAsset* Candidate = NewObject<UGridItemDefinitionAsset>(GetTransientPackage());
	if (!IsValid(Candidate))
	{
		OutError = TEXT("Could not create transient catalyst.");
		return false;
	}
	Configure(*Candidate);
	if (!Matches(*Candidate))
	{
		OutError = TEXT("Catalyst contract/visual-source preflight failed.");
		return false;
	}
	if (FPackageName::DoesPackageExist(ARMEMENT01172::PackageName))
	{
		UGridItemDefinitionAsset* Existing = LoadObject<UGridItemDefinitionAsset>(
			nullptr, ARMEMENT01172::ObjectPath);
		if (!IsValid(Existing) || !Matches(*Existing))
		{
			OutError = TEXT("Catalyst asset already exists but differs; overwrite refused.");
			return false;
		}
		UE_LOG(LogTemp, Display, TEXT("[ARMEMENT01.17.2] PRESENT_SAME"));
		return true;
	}
	if (!bApply)
	{
		UE_LOG(LogTemp, Display, TEXT("[ARMEMENT01.17.2] DRY_RUN READY"));
		return true;
	}
	UPackage* Package = CreatePackage(ARMEMENT01172::PackageName);
	UGridItemDefinitionAsset* Asset = Package
		? NewObject<UGridItemDefinitionAsset>(Package, FName(ARMEMENT01172::ItemName),
			RF_Public | RF_Standalone | RF_Transactional) : nullptr;
	if (!IsValid(Asset))
	{
		OutError = TEXT("Could not create catalyst package.");
		return false;
	}
	Configure(*Asset);
	if (!Matches(*Asset))
	{
		OutError = TEXT("Catalyst contract failed before SavePackage.");
		return false;
	}
	FAssetRegistryModule::AssetCreated(Asset);
	Asset->MarkPackageDirty();
	const FString FileName = FPackageName::LongPackageNameToFilename(
		ARMEMENT01172::PackageName, FPackageName::GetAssetPackageExtension());
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(FileName), true);
	FSavePackageArgs Args;
	Args.TopLevelFlags = RF_Public | RF_Standalone;
	Args.SaveFlags = SAVE_NoError;
	if (!UPackage::SavePackage(Package, Asset, *FileName, Args))
	{
		OutError = TEXT("Unreal SavePackage failed for catalyst.");
		return false;
	}
	UE_LOG(LogTemp, Display, TEXT("[ARMEMENT01.17.2] SAVED %s"), ARMEMENT01172::ObjectPath);
	return true;
}

bool FRPGStartingSurfaceCatalystAuthoring::MatchesSerializedProduction(const UGridItemDefinitionAsset& Item)
{
	// Consult raw native fields, NOT the runtime projection bridge.
	if (!Matches(Item) ||
		Item.WorldMesh.ToSoftObjectPath().ToString() !=
			TEXT("/Game/GrimrockPrototype/Meshes/Items/SM_Catalyst_01.SM_Catalyst_01") ||
		Item.Icon.ToSoftObjectPath().ToString() !=
			TEXT("/Game/GrimrockPrototype/UI/Icons/Items/T_Catalyst_Icon.T_Catalyst_Icon"))
	{
		return false;
	}
	const FGridCombatActionDefinition& A = Item.QuickItemCombatAction;
	const FSoftObjectPath SparklePath(
		TEXT("/Game/GrimrockPrototype/Effects/Alchimie/NS_LiquideSparkle.NS_LiquideSparkle"));
	const bool bForbiddenSparkle = Item.VisualEffects.Effects.ContainsByPredicate(
		[&SparklePath](const FGridItemNiagaraEffectConfig& Effect)
		{
			return Effect.EffectId == FName(TEXT("LiquidSparkle")) ||
				Effect.NiagaraSystem.ToSoftObjectPath() == SparklePath;
		});
	return A.TargetingPolicy == EGridCombatTargetingPolicy::Area &&
		A.AreaRadiusCells == 1 && A.AreaShape == EGridCombatAreaShape::Square &&
		A.bAllowNoSurfaceReaction && A.bUsableDuringExploration &&
		!bForbiddenSparkle;
}

bool FRPGStartingSurfaceCatalystAuthoring::UpgradeExistingProduction(bool bApply, FString& OutError)
{
	OutError.Reset();
	// Never reconfigure the whole item: artist presentation must survive.
	UGridItemDefinitionAsset* Item = LoadObject<UGridItemDefinitionAsset>(
		nullptr, ARMEMENT01172::ObjectPath);
	if (!IsValid(Item) || Item->ItemDefinitionId != FName(ARMEMENT01172::ItemId) ||
		Item->QuickItemActionIdOverride != FName(ARMEMENT01172::ActionId) ||
		!Matches(*Item))
	{
		OutError = TEXT("Existing catalyst failed exact gameplay/identity preflight.");
		return false;
	}
	if (Item->WorldMesh.ToSoftObjectPath().ToString() !=
			TEXT("/Game/GrimrockPrototype/Meshes/Items/SM_Catalyst_01.SM_Catalyst_01") ||
		Item->Icon.ToSoftObjectPath().ToString() !=
			TEXT("/Game/GrimrockPrototype/UI/Icons/Items/T_Catalyst_Icon.T_Catalyst_Icon"))
	{
		OutError = TEXT("Approved artist mesh/icon mismatch; refusing mutation.");
		return false;
	}
	if (MatchesSerializedProduction(*Item))
	{
		UE_LOG(LogTemp, Display, TEXT("[ARMEMENT01.17.3D] PRESENT_SAME"));
		return true;
	}
	if (!bApply)
	{
		UE_LOG(LogTemp, Display, TEXT("[ARMEMENT01.17.3D] DRY_RUN READY"));
		return true;
	}

	FGridCombatActionDefinition& Action = Item->QuickItemCombatAction;
	Action.TargetingPolicy = EGridCombatTargetingPolicy::Area;
	Action.AreaRadiusCells = 1;
	Action.AreaShape = EGridCombatAreaShape::Square;
	Action.bAllowNoSurfaceReaction = true;
	Action.bUsableDuringExploration = true;

	// ITEM-VFX01.4A reserves LiquidSparkle for the healing potion only.
	// Do not touch any of the other artist-authored Niagara effects.
	const FSoftObjectPath SparklePath(
		TEXT("/Game/GrimrockPrototype/Effects/Alchimie/NS_LiquideSparkle.NS_LiquideSparkle"));
	Item->VisualEffects.Effects.RemoveAll(
		[&SparklePath](const FGridItemNiagaraEffectConfig& Effect)
		{
			return Effect.EffectId == FName(TEXT("LiquidSparkle")) ||
				Effect.NiagaraSystem.ToSoftObjectPath() == SparklePath;
		});
	if (!MatchesSerializedProduction(*Item))
	{
		OutError = TEXT("Native catalyst contract invalid after targeted migration.");
		return false;
	}
	UPackage* Package = Item->GetOutermost();
	if (!IsValid(Package) || Package->GetName() != ARMEMENT01172::PackageName)
	{
		OutError = TEXT("Unexpected package; refusing SavePackage.");
		return false;
	}
	const FString FileName = FPackageName::LongPackageNameToFilename(
		ARMEMENT01172::PackageName, FPackageName::GetAssetPackageExtension());
	if (!FPaths::FileExists(FileName))
	{
		OutError = TEXT("Original artist package not found on disk.");
		return false;
	}
	FSavePackageArgs Args;
	Args.TopLevelFlags = RF_Public | RF_Standalone;
	Args.SaveFlags = SAVE_NoError;
	Item->MarkPackageDirty();
	if (!UPackage::SavePackage(Package, Item, *FileName, Args))
	{
		OutError = TEXT("Existing catalyst SavePackage failed.");
		return false;
	}
	UE_LOG(LogTemp, Display, TEXT("[ARMEMENT01.17.3D] SERIALIZED_DATA_SAVED"));
	return true;
}
