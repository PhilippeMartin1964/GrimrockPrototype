#include "ARMEMENT/GridWeaponVisualPilotImportCommandlet.h"

#include "Runtime/Combat/GridWeaponVisualCatalogueAsset.h"
#include "AssetImportTask.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Factories/FbxFactory.h"
#include "Factories/FbxImportUI.h"
#include "Factories/TextureFactory.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "HAL/FileManager.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "UObject/SavePackage.h"

namespace
{
	const TCHAR* const TextureDest = TEXT("/Game/GrimrockPrototype/UI/Icons/Weapons");
	const TCHAR* const MeshDest = TEXT("/Game/GrimrockPrototype/Meshes/Weapons");
	const TCHAR* const CataloguePackage = TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponVisualCatalogue");

	struct FPilotVisual
	{
		const TCHAR* Archetype;
		const TCHAR* VisualId;
		const TCHAR* Family;
		const TCHAR* IconName;
		const TCHAR* MeshName;
	};

	const FPilotVisual Pilots[] = {
		{TEXT("ARM001"),TEXT("VIS_Dagger_A"),TEXT("Light"),TEXT("T_Weapon_Dagger_A"),TEXT("SM_Weapon_Dagger_A")},
		{TEXT("ARM045"),TEXT("VIS_ShortBow_A"),TEXT("Ranged"),TEXT("T_Weapon_ShortBow_A"),TEXT("SM_Weapon_ShortBow_A")},
		{TEXT("ARM075"),TEXT("VIS_RoundShield_A"),TEXT("Shield"),TEXT("T_Weapon_RoundShield_A"),TEXT("SM_Weapon_RoundShield_A")}
	};

	FString ObjectPath(const TCHAR* Folder, const TCHAR* Name)
	{
		return FString::Printf(TEXT("%s/%s.%s"), Folder, Name, Name);
	}

	bool ImportAsset(
		IAssetTools& AssetTools,
		const FString& Filename,
		const TCHAR* Folder,
		const TCHAR* Name,
		bool bMesh)
	{
		if (!FPaths::FileExists(Filename))
		{
			UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.6.3] Missing source file %s"), *Filename);
			return false;
		}
		UAssetImportTask* Task = NewObject<UAssetImportTask>(GetTransientPackage());
		Task->Filename = Filename;
		Task->DestinationPath = Folder;
		Task->DestinationName = Name;
		Task->bAutomated = true;
		Task->bReplaceExisting = false;
		Task->bSave = true;
		Task->bAsync = false;

		if (bMesh)
		{
			UFbxFactory* Factory = NewObject<UFbxFactory>(Task);
			UFbxImportUI* Settings = NewObject<UFbxImportUI>(Factory);
			Settings->bAutomatedImportShouldDetectType = false;
			Settings->MeshTypeToImport = FBXIT_StaticMesh;
			Settings->bImportAsSkeletal = false;
			Settings->bImportMesh = true;
			Settings->bImportMaterials = true; // material slots belong to the imported mesh
			Settings->bImportTextures = false;
			Settings->bImportAnimations = false;
			Factory->ImportUI = Settings;
			Task->Factory = Factory;
			Task->Options = Settings;
		}
		else
		{
			Task->Factory = NewObject<UTextureFactory>(Task);
		}

		TArray<UAssetImportTask*> Tasks;
		Tasks.Add(Task);
		AssetTools.ImportAssetTasks(Tasks);

		if (Task->ImportedObjectPaths.IsEmpty())
		{
			UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.6.3] No asset imported for %s"), *Filename);
			return false;
		}
		UE_LOG(LogTemp, Display, TEXT("[ARMEMENT01.6.3] Imported %s -> %s"), *Filename, *ObjectPath(Folder, Name));
		return true;
	}
}

UGridWeaponVisualPilotImportCommandlet::UGridWeaponVisualPilotImportCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UGridWeaponVisualPilotImportCommandlet::Main(const FString& Params)
{
	FString SourceRoot;
	FString IconsRoot;
	if (!FParse::Value(*Params, TEXT("SourceRoot="), SourceRoot) || SourceRoot.IsEmpty())
	{
		SourceRoot = FPaths::Combine(FPaths::ProjectDir(), TEXT("ArtSource/ARMEMENT01_6_3/Meshes"));
	}
	if (!FParse::Value(*Params, TEXT("IconsRoot="), IconsRoot) || IconsRoot.IsEmpty())
	{
		IconsRoot = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("ARMEMENT01/ARMEMENT01_6_3/Icons"));
	}
	SourceRoot = FPaths::ConvertRelativePathToFull(SourceRoot);
	IconsRoot = FPaths::ConvertRelativePathToFull(IconsRoot);

	// Refuse accidental overwriting of a reviewed visual package.
	const FString CatalogueFile = FPackageName::LongPackageNameToFilename(
		CataloguePackage, FPackageName::GetAssetPackageExtension());
	if (FPaths::FileExists(CatalogueFile))
	{
		UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.6.3] Pilot catalogue already exists: %s"), *CatalogueFile);
		return 1;
	}

	// Validate every input file before writing a single .uasset.
	for (const FPilotVisual& Pilot : Pilots)
	{
		const FString ObjFile = FPaths::Combine(SourceRoot, FString(Pilot.MeshName) + TEXT(".obj"));
		const FString MtlFile = FPaths::Combine(SourceRoot, FString(Pilot.MeshName) + TEXT(".mtl"));
		const FString PngFile = FPaths::Combine(IconsRoot, FString(Pilot.IconName) + TEXT(".png"));
		if (!FPaths::FileExists(ObjFile) || !FPaths::FileExists(MtlFile) || !FPaths::FileExists(PngFile))
		{
			UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.6.3] Missing pilot sources for %s"), Pilot.Archetype);
			return 1;
		}
	}

	FAssetToolsModule& Module = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
	IAssetTools& AssetTools = Module.Get();

	for (const FPilotVisual& Pilot : Pilots)
	{
		if (!ImportAsset(AssetTools,
				FPaths::Combine(IconsRoot, FString(Pilot.IconName) + TEXT(".png")), TextureDest, Pilot.IconName, false) ||
			!ImportAsset(AssetTools,
				FPaths::Combine(SourceRoot, FString(Pilot.MeshName) + TEXT(".obj")), MeshDest, Pilot.MeshName, true))
		{
			return 1;
		}
	}
	UGridWeaponVisualCatalogueAsset* Preview = NewObject<UGridWeaponVisualCatalogueAsset>(GetTransientPackage());
	for (const FPilotVisual& Pilot : Pilots)
	{
		FGridWeaponVisualArchetypeBinding Binding;
		Binding.ArchetypeId = FName(Pilot.Archetype);
		Binding.VisualId = FName(Pilot.VisualId);
		Preview->ArchetypeBindings.Add(Binding);

		FGridWeaponVisualProfile Profile;
		Profile.VisualId = FName(Pilot.VisualId);
		Profile.FamilyTag = FName(Pilot.Family);
		const FString IconPath = ObjectPath(TextureDest, Pilot.IconName);
		const FString MeshPath = ObjectPath(MeshDest, Pilot.MeshName);
		if (!IsValid(LoadObject<UTexture2D>(nullptr, *IconPath)) ||
			!IsValid(LoadObject<UStaticMesh>(nullptr, *MeshPath)))
		{
			UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.6.3] Wrong import class or missing package for %s"), Pilot.Archetype);
			return 1;
		}
		Profile.BaseIcon = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(IconPath));
		Profile.WorldMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(MeshPath));
		// EquippedMesh defaults to the world silhouette in ARMEMENT01.6.2.
		Preview->VisualProfiles.Add(Profile);
	}

	if (!Preview->IsValidCatalogue())
	{
		UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.6.3] Visual catalogue authoring rejected."));
		return 1;
	}
	UPackage* Package = CreatePackage(CataloguePackage);
	UGridWeaponVisualCatalogueAsset* Catalogue = NewObject<UGridWeaponVisualCatalogueAsset>(
		Package, TEXT("DA_WeaponVisualCatalogue"), RF_Public | RF_Standalone | RF_Transactional);
	Catalogue->ArchetypeBindings = MoveTemp(Preview->ArchetypeBindings);
	Catalogue->VisualProfiles = MoveTemp(Preview->VisualProfiles);
	FAssetRegistryModule::AssetCreated(Catalogue);
	Package->MarkPackageDirty();
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(CatalogueFile), true);
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.SaveFlags = SAVE_NoError;
	if (!UPackage::SavePackage(Package, Catalogue, *CatalogueFile, SaveArgs))
	{
		UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.6.3] Unable to save visual catalogue: %s"), *CatalogueFile);
		return 1;
	}
	UE_LOG(LogTemp, Display,
		TEXT("[ARMEMENT01.6.3] Three verified silhouette profiles and ARM001/ARM045/ARM075 bindings saved: %s"),
		CataloguePackage);
	return 0;
}
