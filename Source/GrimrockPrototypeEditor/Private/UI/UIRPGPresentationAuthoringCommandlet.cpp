#include "UI/UIRPGPresentationAuthoringCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "HAL/FileManager.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UI/RPGTalentPresentationAsset.h"
#include "UI/UIRPGTalentPresentationAuthoring.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

UUIRPGPresentationAuthoringCommandlet::UUIRPGPresentationAuthoringCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UUIRPGPresentationAuthoringCommandlet::Main(const FString& Params)
{
	(void)Params;

	URPGTalentPresentationAsset* Catalog =
		LoadObject<URPGTalentPresentationAsset>(nullptr, FUIRPGTalentPresentationAuthoring::ObjectPath());

	UPackage* Package = nullptr;
	if (Catalog)
	{
		Package = Catalog->GetOutermost();
	}
	else
	{
		Package = CreatePackage(FUIRPGTalentPresentationAuthoring::PackageName());
		if (!Package)
		{
			UE_LOG(LogTemp, Error, TEXT("[UI-RPG02.2] Failed to create presentation package."));
			return 1;
		}

		Catalog = NewObject<URPGTalentPresentationAsset>(
			Package,
			FName(FUIRPGTalentPresentationAuthoring::AssetName()),
			RF_Public | RF_Standalone);
		if (!Catalog)
		{
			UE_LOG(LogTemp, Error, TEXT("[UI-RPG02.2] Failed to create DA_RPGTalentPresentation."));
			return 1;
		}
		FAssetRegistryModule::AssetCreated(Catalog);
	}

	Catalog->Modify();
	FUIRPGTalentPresentationAuthoring::ConfigureCatalog(*Catalog);

	if (!Catalog->IsValidDefinition())
	{
		UE_LOG(LogTemp, Error, TEXT("[UI-RPG02.2] Authored presentation catalog is invalid."));
		return 1;
	}

	Package->MarkPackageDirty();
	const FString Filename = FPackageName::LongPackageNameToFilename(
		FUIRPGTalentPresentationAuthoring::PackageName(),
		FPackageName::GetAssetPackageExtension());
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.SaveFlags = SAVE_NoError;
	if (!UPackage::SavePackage(Package, Catalog, *Filename, SaveArgs))
	{
		UE_LOG(LogTemp, Error, TEXT("[UI-RPG02.2] Failed to save '%s'."), *Filename);
		return 1;
	}

	UE_LOG(LogTemp, Display,
		TEXT("[UI-RPG02.2] Materialized DA_RPGTalentPresentation with 6 classes and 18 ordered branches."));
	return 0;
}
