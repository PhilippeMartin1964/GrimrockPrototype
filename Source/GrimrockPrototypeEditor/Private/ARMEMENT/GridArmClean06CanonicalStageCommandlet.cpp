#include "ARMEMENT/GridArmClean06CanonicalStageCommandlet.h"

#include "ARMEMENT/GridArmClean06CompleteProductionPreview.h"
#include "ARMEMENT/GridWeaponPipelineManifest.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "HAL/FileManager.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	constexpr const TCHAR* BaseFolder =
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage");

	struct FPlannedAsset
	{
		FString PackagePath;
		UObject* Candidate = nullptr;
		UObject* Staged = nullptr;
	};

	bool IsSafeItemId(FName Id)
	{
		const FString Value = Id.ToString();
		if (Id.IsNone() || Value.IsEmpty()) { return false; }
		for (TCHAR Ch : Value)
		{
			if (!FChar::IsAlnum(Ch) && Ch != TEXT('_')) { return false; }
		}
		return true;
	}

	bool SaveNewPackage(UObject* Asset)
	{
		if (!IsValid(Asset)) { return false; }
		UPackage* Package = Asset->GetOutermost();
		if (!Package) { return false; }
		const FString Filename = FPackageName::LongPackageNameToFilename(
			Package->GetName(), FPackageName::GetAssetPackageExtension());
		if (FPaths::FileExists(Filename)) { return false; }
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		Args.SaveFlags = SAVE_NoError;
		FAssetRegistryModule::AssetCreated(Asset);
		Asset->MarkPackageDirty();
		return UPackage::SavePackage(Package, Asset, *Filename, Args);
	}
}

UGridArmClean06CanonicalStageCommandlet::UGridArmClean06CanonicalStageCommandlet()
{
	IsClient = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UGridArmClean06CanonicalStageCommandlet::Main(const FString& Params)
{
	// -Apply is explicit. Default invocation only verifies the real 22-arm
	// contents, stages nothing, and never creates a package or file.
	const bool bApply = FParse::Param(*Params, TEXT("Apply"));
	TArray<FGridWeaponPipelineRow> Manifest;
	FString Error;
	if (!FGridWeaponPipelineManifest::Load(
		FGridWeaponPipelineManifest::DefaultFile(), Manifest, Error) ||
		Manifest.Num() != 22)
	{
		UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.2AM] Invalid production manifest: %s"), *Error);
		return 1;
	}

	const UGridWeaponCatalogueAsset* Numerics = LoadObject<UGridWeaponCatalogueAsset>(
		nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponCatalogue.DA_WeaponCatalogue"));
	const UGridWeaponVisualCatalogueAsset* Visuals = LoadObject<UGridWeaponVisualCatalogueAsset>(
		nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponVisualCatalogue.DA_WeaponVisualCatalogue"));
	const UGridEquippedWeaponActionIconPalette* Icons = LoadObject<UGridEquippedWeaponActionIconPalette>(
		nullptr, TEXT("/Game/GrimrockPrototype/UI/Icons/Combat/DA_EquippedWeaponActionIcons.DA_EquippedWeaponActionIcons"));
	if (!IsValid(Numerics) || !IsValid(Visuals) || !IsValid(Icons))
	{
		UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.2AM] Source catalogues missing."));
		return 2;
	}
	const bool bNumericWasDirty = Numerics->GetOutermost()->IsDirty();
	const bool bVisualWasDirty = Visuals->GetOutermost()->IsDirty();
	const bool bIconsWereDirty = Icons->GetOutermost()->IsDirty();

	TStrongObjectPtr<UGridItemSystemAsset> Candidate(
		NewObject<UGridItemSystemAsset>(GetTransientPackage()));
	GridArmClean06CompleteProductionPreview::FSummary Summary;
	TArray<FString> ValidationErrors;
	if (!GridArmClean06CompleteProductionPreview::Build(
			Manifest, *Numerics, *Visuals, *Icons, *Candidate, Summary) ||
		!Summary.ReadyForBinaryCutover() ||
		!Candidate->Validate(ValidationErrors) ||
		Summary.Core.Actions.BoundActions != 19 ||
		Summary.Core.Actions.NeutralDefaultBindings != 15 ||
		Candidate->ItemDefinitions.Num() != 23 ||
		Numerics->GetOutermost()->IsDirty() != bNumericWasDirty ||
		Visuals->GetOutermost()->IsDirty() != bVisualWasDirty ||
		Icons->GetOutermost()->IsDirty() != bIconsWereDirty)
	{
		UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.2AM] Canonical source parity gate refused."));
		return 3;
	}

	TArray<FPlannedAsset> Plan;
	auto Queue = [&Plan](const FString& PackagePath, UObject* Source)
	{
		FPlannedAsset& Row = Plan.AddDefaulted_GetRef();
		Row.PackagePath = PackagePath;
		Row.Candidate = Source;
	};
	Queue(FString(BaseFolder) + TEXT("/DA_CanonicalWeaponCatalogue"),
		Candidate->WeaponCatalogue);
	Queue(FString(BaseFolder) + TEXT("/DA_CanonicalWeaponVisualCatalogue"),
		Candidate->WeaponVisualCatalogue);
	Queue(FString(BaseFolder) + TEXT("/DA_ActionCatalogue"),
		Candidate->ActionCatalogue);
	Queue(FString(BaseFolder) + TEXT("/DA_ActionPresentationCatalogue"),
		Candidate->ActionPresentationCatalogue);
	for (UGridCanonicalItemDefinitionAsset* Item : Candidate->ItemDefinitions)
	{
		if (!IsValid(Item) || !IsSafeItemId(Item->ItemId))
		{
			UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.2AM] Invalid item identity."));
			return 4;
		}
		Queue(FString(BaseFolder) + TEXT("/Definitions/DA_Canonical_") +
			Item->ItemId.ToString(), Item);
	}
	const FString RootPath = FString(BaseFolder) + TEXT("/DA_ItemSystem");
	if (Plan.Num() != 27) { return 4; }
	TSet<FString> UniquePaths;
	UniquePaths.Add(RootPath);
	for (const FPlannedAsset& Entry : Plan)
	{
		const FString Filename = FPackageName::LongPackageNameToFilename(
			Entry.PackagePath, FPackageName::GetAssetPackageExtension());
		if (!IsValid(Entry.Candidate) || UniquePaths.Contains(Entry.PackagePath) ||
			FPaths::FileExists(Filename) || FindPackage(nullptr, *Entry.PackagePath))
		{
			UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.2AM] Destination occupied or invalid: %s"), *Entry.PackagePath);
			return 5;
		}
		UniquePaths.Add(Entry.PackagePath);
	}
	const FString RootFilename = FPackageName::LongPackageNameToFilename(
		RootPath, FPackageName::GetAssetPackageExtension());
	if (FPaths::FileExists(RootFilename) || FindPackage(nullptr, *RootPath) ||
		UniquePaths.Num() != 28)
	{
		UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.2AM] Root package already exists."));
		return 5;
	}
	if (!bApply)
	{
		UE_LOG(LogTemp, Display, TEXT("[ARM-CLEAN06.2AM] DRY_RUN_OK: 28 new assets; 22 ARM + 1 arrows; 19 actions; 5 presentations; no files written."));
		return 0;
	}

	// Only NEW stage paths may be written. Never overwrite original arms,
	// levels, SaveGame packages, palette, audio or unrelated user content.
	for (FPlannedAsset& Entry : Plan)
	{
		UPackage* Package = CreatePackage(*Entry.PackagePath);
		const FString AssetName = FPackageName::GetLongPackageAssetName(Entry.PackagePath);
		Entry.Staged = Package ? StaticDuplicateObject(
			Entry.Candidate, Package, FName(*AssetName)) : nullptr;
		if (!IsValid(Entry.Staged) ||
			Entry.Staged->GetClass() != Entry.Candidate->GetClass())
		{
			UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.2AM] Staging failed; no files saved."));
			return 6;
		}
		Entry.Staged->SetFlags(RF_Public | RF_Standalone);
	}

	UPackage* RootPackage = CreatePackage(*RootPath);
	UGridItemSystemAsset* PersistedRoot = RootPackage ?
		NewObject<UGridItemSystemAsset>(RootPackage, FName(TEXT("DA_ItemSystem")),
			RF_Public | RF_Standalone) : nullptr;
	if (!PersistedRoot) { return 6; }
	PersistedRoot->WeaponCatalogue = Cast<UGridCanonicalWeaponCatalogueAsset>(Plan[0].Staged);
	PersistedRoot->WeaponVisualCatalogue = Cast<UGridCanonicalWeaponVisualCatalogueAsset>(Plan[1].Staged);
	PersistedRoot->ActionCatalogue = Cast<UGridActionCatalogueAsset>(Plan[2].Staged);
	PersistedRoot->ActionPresentationCatalogue = Cast<UGridActionPresentationCatalogueAsset>(Plan[3].Staged);
	for (int32 I = 4; I < Plan.Num(); ++I)
	{
		UGridCanonicalItemDefinitionAsset* Item =
			Cast<UGridCanonicalItemDefinitionAsset>(Plan[I].Staged);
		if (!Item) { return 6; }
		PersistedRoot->ItemDefinitions.Add(Item);
	}
	ValidationErrors.Reset();
	if (!PersistedRoot->Validate(ValidationErrors))
	{
		UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.2AM] Cloned registry invalid; no files saved."));
		return 7;
	}

	for (const FPlannedAsset& Entry : Plan)
	{
		if (!SaveNewPackage(Entry.Staged))
		{
			UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.2AM] Partial stage write failed. Inspect ONLY CanonicalStage before retry."));
			return 8;
		}
	}
	if (!SaveNewPackage(PersistedRoot))
	{
		UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.2AM] Root package write failed. Inspect CanonicalStage."));
		return 8;
	}
	UE_LOG(LogTemp, Display, TEXT("[ARM-CLEAN06.2AM] SAVED 28 canonical stage packages. Original game remains unchanged."));
	return 0;
}
