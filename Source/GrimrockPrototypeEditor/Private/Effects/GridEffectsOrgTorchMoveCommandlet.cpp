#include "Effects/GridEffectsOrgTorchMoveCommandlet.h"

#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "FileHelpers.h"
#include "Misc/AssetRegistryInterface.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Modules/ModuleManager.h"
#include "NiagaraSystem.h"
#include "UObject/ObjectRedirector.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"

DEFINE_LOG_CATEGORY_STATIC(LogEffectsOrgTorchMove, Log, All);

UGridEffectsOrgTorchMoveCommandlet::UGridEffectsOrgTorchMoveCommandlet()
{
    IsClient = false;
    IsServer = false;
    IsEditor = true;
    LogToConsole = true;
}

int32 UGridEffectsOrgTorchMoveCommandlet::Main(const FString& Params)
{
    static const FString OldDirectory(TEXT("/Game/GrimrockPrototype/Art/FX"));
    static const FString NewDirectory(TEXT("/Game/GrimrockPrototype/Effects/Fire/Torch"));
    static const FString OldPackage = OldDirectory + TEXT("/NS_Flame_8_Torch");
    static const FString NewPackage = NewDirectory + TEXT("/NS_Flame_8_Torch");
    static const FString OldObject = OldPackage + TEXT(".NS_Flame_8_Torch");
    static const FString NewObject = NewPackage + TEXT(".NS_Flame_8_Torch");
    static const FName ItemPackage(TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Torch"));
    static const FName FlameMaterial(TEXT("/Game/GrimrockPrototype/Art/Materials/Instances/Flame/MI_Flame_8"));
    static const FName DistortionMaterial(TEXT("/Game/GrimrockPrototype/Art/Materials/Instances/Flame/MI_Distortion_1"));

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
        UE_LOG(LogEffectsOrgTorchMove, Error, TEXT("Expected exactly one old torch Niagara and no target."));
        return 11;
    }

    TArray<FName> Referencers;
    if (!Registry.GetReferencers(FName(*OldPackage), Referencers,
        UE::AssetRegistry::EDependencyCategory::All, UE::AssetRegistry::FDependencyQuery()) ||
        Referencers.Num() != 1 || !Referencers.Contains(ItemPackage))
    {
        UE_LOG(LogEffectsOrgTorchMove, Error, TEXT("Torch Niagara referencers differ from the single item consumer."));
        return 12;
    }
    TArray<FName> Dependencies;
    if (!Registry.GetDependencies(FName(*OldPackage), Dependencies,
        UE::AssetRegistry::EDependencyCategory::All, UE::AssetRegistry::FDependencyQuery()) ||
        !Dependencies.Contains(FlameMaterial) ||
        !Dependencies.Contains(DistortionMaterial))
    {
        UE_LOG(LogEffectsOrgTorchMove, Error, TEXT("Torch Niagara material dependency contract changed."));
        return 13;
    }
    TArray<FName> ItemDependencies;
    if (!Registry.GetDependencies(ItemPackage, ItemDependencies,
        UE::AssetRegistry::EDependencyCategory::All, UE::AssetRegistry::FDependencyQuery()) ||
        !ItemDependencies.Contains(FName(*OldPackage)))
    {
        UE_LOG(LogEffectsOrgTorchMove, Error, TEXT("Torch item does not reference source Niagara."));
        return 14;
    }

    UNiagaraSystem* Niagara = LoadObject<UNiagaraSystem>(nullptr, *OldObject);
    UObject* TorchItem = LoadObject<UObject>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Torch.DA_Item_Torch"));
    if (!IsValid(Niagara) || !IsValid(TorchItem))
    {
        UE_LOG(LogEffectsOrgTorchMove, Error, TEXT("Torch or item Unreal packages could not be loaded."));
        return 15;
    }
    if (!bApply)
    {
        UE_LOG(LogEffectsOrgTorchMove, Display,
            TEXT("[EFFECTS-ORG01.2D] Dry-run OK: source, referencer and materials verified."));
        return 0;
    }

    FAssetToolsModule& Tools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
    TArray<FAssetRenameData> Renames;
    Renames.Emplace(TWeakObjectPtr<UObject>(Niagara), NewDirectory, TEXT("NS_Flame_8_Torch"));
    if (!Tools.Get().RenameAssets(Renames))
    {
        UE_LOG(LogEffectsOrgTorchMove, Error, TEXT("AssetTools failed to relocate torch Niagara."));
        return 20;
    }

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
        if (UObjectRedirector* Loaded = FindObject<UObjectRedirector>(nullptr, *OldObject))
        {
            Redirectors.Add(Loaded);
        }
    }
    if (!Redirectors.IsEmpty())
    {
        Tools.Get().FixupReferencers(
            Redirectors, false, ERedirectFixupMode::DeleteFixedUpRedirectors);
    }

    TArray<UPackage*> PackagesToSave;
    PackagesToSave.Add(Niagara->GetOutermost());
    PackagesToSave.Add(TorchItem->GetOutermost());
    if (!UEditorLoadingAndSavingUtils::SavePackages(PackagesToSave, false))
    {
        UE_LOG(LogEffectsOrgTorchMove, Error,
            TEXT("Saving relocated torch Niagara and item consumer failed."));
        return 21;
    }

    if (!FPackageName::DoesPackageExist(NewPackage) ||
        FPackageName::DoesPackageExist(OldPackage) ||
        !IsValid(LoadObject<UNiagaraSystem>(nullptr, *NewObject)))
    {
        UE_LOG(LogEffectsOrgTorchMove, Error,
            TEXT("Torch postflight found old path or missing relocated Niagara."));
        return 22;
    }

    UE_LOG(LogEffectsOrgTorchMove, Display,
        TEXT("[EFFECTS-ORG01.2D] Torch Niagara moved; item reference saved; no redirector remains."));
    return 0;
}
