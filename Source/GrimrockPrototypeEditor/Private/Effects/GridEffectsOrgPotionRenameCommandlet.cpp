#include "Effects/GridEffectsOrgPotionRenameCommandlet.h"

#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "EditorLoadingAndSavingUtils.h"
#include "Materials/MaterialInterface.h"
#include "Misc/AssetRegistryInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Modules/ModuleManager.h"
#include "NiagaraSystem.h"
#include "UObject/ObjectRedirector.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"

DEFINE_LOG_CATEGORY_STATIC(LogEffectsOrgPotionRename, Log, All);

UGridEffectsOrgPotionRenameCommandlet::UGridEffectsOrgPotionRenameCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UGridEffectsOrgPotionRenameCommandlet::Main(const FString& Params)
{
	static const FString Directory(TEXT("/Game/GrimrockPrototype/Effects/Alchimie"));
	static const FString OldPackage = Directory + TEXT("/M_Teleporter_Spark_02");
	static const FString NewPackage = Directory + TEXT("/M_Potion_Spark");
	static const FString OldObject = OldPackage + TEXT(".M_Teleporter_Spark_02");
	static const FString NewObject = NewPackage + TEXT(".M_Potion_Spark");
	static const FName NiagaraPackage(TEXT("/Game/GrimrockPrototype/Effects/Alchimie/NS_LiquideSparkle"));
	static const FName PotionPackage(TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Potion_Healing_01"));

	const bool bApply = FParse::Param(*Params, TEXT("Apply"));
	IAssetRegistry& Registry =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	Registry.SearchAllAssets(true);

	TArray<FAssetData> OldAssets;
	TArray<FAssetData> NewAssets;
	Registry.GetAssetsByPackageName(FName(*OldPackage), OldAssets, true, false);
	Registry.GetAssetsByPackageName(FName(*NewPackage), NewAssets, true, false);
	if (OldAssets.Num() != 1 || !NewAssets.IsEmpty())
	{
		UE_LOG(LogEffectsOrgPotionRename, Error, TEXT("Exactly one old potion material and no new package are required."));
		return 11;
	}

	TArray<FName> Referencers;
	if (!Registry.GetReferencers(FName(*OldPackage), Referencers,
		UE::AssetRegistry::EDependencyCategory::All, UE::AssetRegistry::FDependencyQuery()) ||
		Referencers.Num() != 1 || !Referencers.Contains(NiagaraPackage))
	{
		UE_LOG(LogEffectsOrgPotionRename, Error,
			TEXT("Potion material referencers differ from the UE-validated single Niagara consumer."));
		return 12;
	}
	TArray<FName> NiagaraDependencies;
	if (!Registry.GetDependencies(NiagaraPackage, NiagaraDependencies,
		UE::AssetRegistry::EDependencyCategory::All, UE::AssetRegistry::FDependencyQuery()) ||
		!NiagaraDependencies.Contains(FName(*OldPackage)))
	{
		UE_LOG(LogEffectsOrgPotionRename, Error, TEXT("NS_LiquideSparkle no longer depends on the source material."));
		return 13;
	}
	TArray<FName> PotionDependencies;
	if (!Registry.GetDependencies(PotionPackage, PotionDependencies,
		UE::AssetRegistry::EDependencyCategory::All, UE::AssetRegistry::FDependencyQuery()) ||
		!PotionDependencies.Contains(NiagaraPackage))
	{
		UE_LOG(LogEffectsOrgPotionRename, Error, TEXT("Healing potion no longer references NS_LiquideSparkle."));
		return 14;
	}

	UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, *OldObject);
	UNiagaraSystem* System = LoadObject<UNiagaraSystem>(nullptr,
		TEXT("/Game/GrimrockPrototype/Effects/Alchimie/NS_LiquideSparkle.NS_LiquideSparkle"));
	if (!IsValid(Material) || !IsValid(System))
	{
		UE_LOG(LogEffectsOrgPotionRename, Error, TEXT("Required Unreal packages could not be loaded."));
		return 15;
	}

	if (!bApply)
	{
		UE_LOG(LogEffectsOrgPotionRename, Display,
			TEXT("[EFFECTS-ORG01.2B] Dry-run OK: one potion material, one Niagara referencer, item link preserved."));
		return 0;
	}

	// The AssetTools operation updates serialized references (a git mv does not).
	FAssetToolsModule& AssetToolsModule =
		FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
	TArray<FAssetRenameData> Renames;
	Renames.Emplace(TWeakObjectPtr<UObject>(Material), Directory, TEXT("M_Potion_Spark"));
	if (!AssetToolsModule.Get().RenameAssets(Renames))
	{
		UE_LOG(LogEffectsOrgPotionRename, Error, TEXT("AssetTools failed to rename the potion material."));
		return 20;
	}

	// Fix only redirectors created by THIS rename. Never mass-fix the project.
	TArray<FAssetData> PotentialRedirectors;
	Registry.GetAssetsByPackageName(FName(*OldPackage), PotentialRedirectors, true, false);
	TArray<UObjectRedirector*> Redirectors;
	for (const FAssetData& Asset : PotentialRedirectors)
	{
		if (UObjectRedirector* Redirector = Cast<UObjectRedirector>(Asset.GetAsset()))
		{
			Redirectors.Add(Redirector);
		}
	}
	if (Redirectors.IsEmpty())
	{
		// Some editor sessions have a loaded redirector before the registry
		// indexes it. Find the exact old object, never scan other packages.
		if (UObjectRedirector* Loaded = FindObject<UObjectRedirector>(nullptr, *OldObject))
		{
			Redirectors.Add(Loaded);
		}
	}
	if (!Redirectors.IsEmpty())
	{
		AssetToolsModule.Get().FixupReferencers(
			Redirectors, false, ERedirectFixupMode::DeleteFixedUpRedirectors);
	}

	// Write the new material and its direct Niagara consumer. The potion item
	// still references that same Niagara asset, so no item-definition migration.
	TArray<UPackage*> PackagesToSave;
	PackagesToSave.Add(Material->GetOutermost());
	PackagesToSave.Add(System->GetOutermost());
	if (!UEditorLoadingAndSavingUtils::SavePackages(PackagesToSave, false))
	{
		UE_LOG(LogEffectsOrgPotionRename, Error, TEXT("Saving the renamed material and Niagara consumer failed."));
		return 21;
	}

	if (!FPackageName::DoesPackageExist(NewPackage) ||
		FPackageName::DoesPackageExist(OldPackage) ||
		!IsValid(LoadObject<UMaterialInterface>(nullptr, *NewObject)))
	{
		UE_LOG(LogEffectsOrgPotionRename, Error,
			TEXT("Postflight failed: expected only M_Potion_Spark, without old package or redirector."));
		return 22;
	}
	UE_LOG(LogEffectsOrgPotionRename, Display,
		TEXT("[EFFECTS-ORG01.2B] Renamed M_Potion_Spark and updated Niagara references; no compatibility redirector remains."));
	return 0;
}
