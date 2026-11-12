#include "ARMEMENT/GridWeaponCatalogueCompactCommandlet.h"

#include "HAL/FileManager.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Runtime/Combat/GridWeaponCatalogueAsset.h"
#include "UObject/SavePackage.h"

namespace
{
	const TCHAR* const CataloguePath =
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponCatalogue.DA_WeaponCatalogue");

	bool SameProfile(const FGridWeaponLevelProfile& Left, const FGridWeaponLevelProfile& Right)
	{
		return Left.ArchetypeId == Right.ArchetypeId &&
			Left.ItemLevel == Right.ItemLevel && Left.DisplayName.EqualTo(Right.DisplayName) &&
			Left.Family == Right.Family && Left.HandUsage == Right.HandUsage &&
			Left.DamageType == Right.DamageType &&
			Left.PhysicalSubtype == Right.PhysicalSubtype &&
			Left.ScalingAttribute == Right.ScalingAttribute &&
			Left.AccuracyBonus == Right.AccuracyBonus &&
			Left.DamageDice.bEnabled == Right.DamageDice.bEnabled &&
			Left.DamageDice.DiceCount == Right.DamageDice.DiceCount &&
			Left.DamageDice.DiceSides == Right.DamageDice.DiceSides &&
			Left.DamageDice.FlatBonus == Right.DamageDice.FlatBonus &&
			Left.PhysicalArmor == Right.PhysicalArmor &&
			Left.MagicalArmor == Right.MagicalArmor &&
			Left.ActionPointCost == Right.ActionPointCost &&
			Left.RangeCells == Right.RangeCells &&
			FMath::IsNearlyEqual(Left.WeightKg, Right.WeightKg, 0.0001f);
	}
}

UGridWeaponCatalogueCompactCommandlet::UGridWeaponCatalogueCompactCommandlet()
{
	IsClient = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UGridWeaponCatalogueCompactCommandlet::Main(const FString& Params)
{
	const bool bApply = FParse::Param(*Params, TEXT("Apply"));
	UGridWeaponCatalogueAsset* Catalogue =
		LoadObject<UGridWeaponCatalogueAsset>(nullptr, CataloguePath);
	if (!IsValid(Catalogue))
	{
		UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN03] Cannot load DA_WeaponCatalogue."));
		return 1;
	}
	if (Catalogue->Profiles.IsEmpty() && Catalogue->ArchetypeProgressions.Num() == 82)
	{
		if (!Catalogue->IsValidCatalogue())
		{
			UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN03] Already compact but invalid."));
			return 1;
		}
		UE_LOG(LogTemp, Display, TEXT("[ARM-CLEAN03] ALREADY_COMPACT; no mutation."));
		return 0;
	}
	if (!Catalogue->IsValidCatalogue())
	{
		UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN03] Full catalogue is not valid."));
		return 1;
	}

	TArray<FGridWeaponArchetypeProgression> Compact;
	int32 Exceptions = 0;
	if (!Catalogue->BuildCompactCandidate(Compact, Exceptions))
	{
		UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN03] Cannot build lossless compact progression."));
		return 1;
	}
	UGridWeaponCatalogueAsset* Preview = NewObject<UGridWeaponCatalogueAsset>(GetTransientPackage());
	Preview->ArchetypeProgressions = Compact;
	if (!Preview->IsValidCatalogue())
	{
		UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN03] Compact preview is invalid."));
		return 1;
	}
	int32 Compared = 0;
	for (const FGridWeaponLevelProfile& Original : Catalogue->Profiles)
	{
		const FGridWeaponLevelProfile* Candidate =
			Preview->FindProfile(Original.ArchetypeId, Original.ItemLevel);
		if (!Candidate || !SameProfile(Original, *Candidate))
		{
			UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN03] PARITY_FAIL %s N%d"),
				*Original.ArchetypeId.ToString(), Original.ItemLevel);
			return 1;
		}
		++Compared;
	}
	if (Compared != 1640)
	{
		UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN03] Expected 1640 profiles, got %d."), Compared);
		return 1;
	}
	UE_LOG(LogTemp, Display,
		TEXT("[ARM-CLEAN03] PARITY_OK Profiles=1640 Archetypes=82 Rules=410 Exceptions=%d Mode=%s"),
		Exceptions, bApply ? TEXT("APPLY") : TEXT("DRY_RUN"));
	if (!bApply)
	{
		return 0;
	}

	// Preserve the backup in memory; do not save unless the compact runtime
	// reproduces exactly the profile contract. XLSX remains authoritative.
	Catalogue->Modify();
	TArray<FGridWeaponLevelProfile> Backup = MoveTemp(Catalogue->Profiles);
	Catalogue->ArchetypeProgressions = MoveTemp(Compact);
	Catalogue->InvalidateProfileCache();
	if (!Catalogue->IsValidCatalogue())
	{
		Catalogue->Profiles = MoveTemp(Backup);
		Catalogue->ArchetypeProgressions.Reset();
		Catalogue->InvalidateProfileCache();
		UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN03] Compact validation failed; in-memory source restored."));
		return 1;
	}

	UPackage* Package = Catalogue->GetOutermost();
	Package->MarkPackageDirty();
	const FString PackageName = FPackageName::ObjectPathToPackageName(CataloguePath);
	const FString File = FPackageName::LongPackageNameToFilename(
		PackageName, FPackageName::GetAssetPackageExtension());
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(File), true);
	FSavePackageArgs Args;
	Args.TopLevelFlags = RF_Public | RF_Standalone;
	Args.SaveFlags = SAVE_NoError;
	if (!UPackage::SavePackage(Package, Catalogue, *File, Args))
	{
		UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN03] SavePackage failed; inspect the local asset, do not commit."));
		return 1;
	}
	UE_LOG(LogTemp, Display, TEXT("[ARM-CLEAN03] SAVED %s; validate tests before committing the uasset."), *File);
	return 0;
}
