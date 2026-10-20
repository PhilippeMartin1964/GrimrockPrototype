#include "ARMEMENT/GridWeaponCatalogueImportCommandlet.h"

#include "Runtime/Combat/GridWeaponCatalogueAsset.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "HAL/FileManager.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Misc/LexFromString.h"
#include "Modules/ModuleManager.h"
#include "UObject/SavePackage.h"

namespace
{
	constexpr int32 ExpectedColumns = 19;
	const TCHAR* const ColumnNames[ExpectedColumns] =
	{
		TEXT("ArchetypeId"), TEXT("ItemLevel"), TEXT("DisplayName"), TEXT("Family"), TEXT("HandUsage"),
		TEXT("AccuracyBonus"), TEXT("DamageType"), TEXT("PhysicalSubtype"), TEXT("ScalingAttribute"),
		TEXT("DiceCount"), TEXT("DiceSides"), TEXT("FlatBonus"), TEXT("PhysicalArmor"), TEXT("MagicalArmor"),
		TEXT("ActionPointCost"), TEXT("RangeCells"), TEXT("WeightKg"), TEXT("MinDamage"), TEXT("MaxDamage")
	};

	bool ReadInt(const TArray<FString>& Fields, int32 Index, int32& OutValue)
	{
		return Fields.IsValidIndex(Index) && LexTryParseString(OutValue, *Fields[Index]);
	}

	bool ReadFloat(const TArray<FString>& Fields, int32 Index, float& OutValue)
	{
		return Fields.IsValidIndex(Index) && LexTryParseString(OutValue, *Fields[Index]) &&
			FMath::IsFinite(OutValue);
	}

	template <typename EnumType>
	bool ReadEnum(const TArray<FString>& Fields, int32 Index, EnumType& OutValue)
	{
		if (!Fields.IsValidIndex(Index) || !StaticEnum<EnumType>())
		{
			return false;
		}
		const int64 Value = StaticEnum<EnumType>()->GetValueByNameString(Fields[Index]);
		if (Value == INDEX_NONE)
		{
			return false;
		}
		OutValue = static_cast<EnumType>(Value);
		return true;
	}


	/** Parse normalized RFC-4180 CSV without optional Unreal CSV parser headers. */
	bool ParseNormalizedCsvRows(const FString& Csv, TArray<TArray<FString>>& OutRows, FString& OutError)
	{
		OutRows.Reset();
		TArray<FString> Row;
		FString Field;
		bool bQuoted = false;
		bool bFieldStarted = false;
		bool bClosedQuote = false;

		for (int32 Index = 0; Index < Csv.Len(); ++Index)
		{
			const TCHAR Character = Csv[Index];
			if (Index == 0 && Character == static_cast<TCHAR>(0xFEFF))
			{
				continue;
			}

			if (bQuoted)
			{
				if (Character == TEXT('"'))
				{
					if (Index + 1 < Csv.Len() && Csv[Index + 1] == TEXT('"'))
					{
						Field.AppendChar(TEXT('"'));
						++Index;
					}
					else
					{
						bQuoted = false;
						bClosedQuote = true;
					}
				}
				else
				{
					Field.AppendChar(Character);
				}
				continue;
			}

			if (Character == TEXT('"'))
			{
				if (bFieldStarted || bClosedQuote)
				{
					OutError = FString::Printf(TEXT("Unexpected quote at CSV character %d."), Index);
					return false;
				}
				bQuoted = true;
				bFieldStarted = true;
				continue;
			}

			if (bClosedQuote && Character != TEXT(',') &&
				Character != TEXT('\r') && Character != TEXT('\n'))
			{
				OutError = FString::Printf(TEXT("Unexpected text after quoted CSV field at character %d."), Index);
				return false;
			}

			if (Character == TEXT(',') || Character == TEXT('\r') || Character == TEXT('\n'))
			{
				Row.Add(MoveTemp(Field));
				Field.Reset();
				bFieldStarted = false;
				bClosedQuote = false;

				if (Character == TEXT('\r') || Character == TEXT('\n'))
				{
					OutRows.Add(MoveTemp(Row));
					Row.Reset();
					if (Character == TEXT('\r') && Index + 1 < Csv.Len() && Csv[Index + 1] == TEXT('\n'))
					{
						++Index;
					}
				}
				continue;
			}

			Field.AppendChar(Character);
			bFieldStarted = true;
		}

		if (bQuoted)
		{
			OutError = TEXT("Unterminated quoted CSV field.");
			return false;
		}

		if (bFieldStarted || bClosedQuote || !Field.IsEmpty() || !Row.IsEmpty())
		{
			Row.Add(MoveTemp(Field));
			OutRows.Add(MoveTemp(Row));
		}
		return true;
	}

	bool ParseCsv(const FString& Csv, TArray<FGridWeaponLevelProfile>& OutProfiles, FString& OutError)
	{
		OutProfiles.Reset();
		TArray<TArray<FString>> Rows;
		if (!ParseNormalizedCsvRows(Csv, Rows, OutError))
		{
			return false;
		}
		if (Rows.Num() != 1641)
		{
			OutError = FString::Printf(TEXT("Expected a header + 1640 rows, received %d rows."), Rows.Num());
			return false;
		}
		if (Rows[0].Num() != ExpectedColumns)
		{
			OutError = TEXT("Unexpected normalized CSV header width.");
			return false;
		}
		for (int32 Column = 0; Column < ExpectedColumns; ++Column)
		{
			if (FString(Rows[0][Column]) != ColumnNames[Column])
			{
				OutError = FString::Printf(TEXT("Header column %d must be '%s'."), Column, ColumnNames[Column]);
				return false;
			}
		}

		OutProfiles.Reserve(1640);
		for (int32 RowIndex = 1; RowIndex < Rows.Num(); ++RowIndex)
		{
			const TArray<FString>& V = Rows[RowIndex];
			if (V.Num() != ExpectedColumns)
			{
				OutError = FString::Printf(TEXT("Row %d has %d columns, expected %d."),
					RowIndex + 1, V.Num(), ExpectedColumns);
				return false;
			}
			FGridWeaponLevelProfile P;
			P.ArchetypeId = FName(*V[0]);
			P.DisplayName = FText::FromString(V[2]);
			P.Family = V[3];
			P.HandUsage = V[4];
			if (!ReadInt(V, 1, P.ItemLevel) ||
				!ReadInt(V, 5, P.AccuracyBonus) ||
				!ReadEnum(V, 6, P.DamageType) ||
				!ReadEnum(V, 7, P.PhysicalSubtype) ||
				!ReadEnum(V, 8, P.ScalingAttribute) ||
				!ReadInt(V, 9, P.DamageDice.DiceCount) ||
				!ReadInt(V, 10, P.DamageDice.DiceSides) ||
				!ReadInt(V, 11, P.DamageDice.FlatBonus) ||
				!ReadInt(V, 12, P.PhysicalArmor) ||
				!ReadInt(V, 13, P.MagicalArmor) ||
				!ReadInt(V, 14, P.ActionPointCost) ||
				!ReadInt(V, 15, P.RangeCells) ||
				!ReadFloat(V, 16, P.WeightKg))
			{
				OutError = FString::Printf(TEXT("Row %d has invalid numeric or enum fields."), RowIndex + 1);
				return false;
			}
			int32 MinDamage = 0;
			int32 MaxDamage = 0;
			if (!ReadInt(V, 17, MinDamage) || !ReadInt(V, 18, MaxDamage))
			{
				OutError = FString::Printf(TEXT("Row %d has invalid damage bounds."), RowIndex + 1);
				return false;
			}

			P.DamageDice.bEnabled = P.DamageDice.DiceCount != 0;
			if (!P.IsValidProfile())
			{
				OutError = FString::Printf(TEXT("Row %d: invalid profile %s level %d."),
					RowIndex + 1, *P.ArchetypeId.ToString(), P.ItemLevel);
				return false;
			}
			const int32 DerivedMin = P.DamageDice.bEnabled ? P.DamageDice.GetMinimumDamage() : 0;
			const int32 DerivedMax = P.DamageDice.bEnabled ? P.DamageDice.GetMaximumDamage() : 0;
			if (MinDamage != DerivedMin || MaxDamage != DerivedMax)
			{
				OutError = FString::Printf(TEXT("Row %d: dice bounds %d..%d do not match XLSX %d..%d."),
					RowIndex + 1, DerivedMin, DerivedMax, MinDamage, MaxDamage);
				return false;
			}
			OutProfiles.Add(MoveTemp(P));
		}
		return true;
	}
}

UGridWeaponCatalogueImportCommandlet::UGridWeaponCatalogueImportCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UGridWeaponCatalogueImportCommandlet::Main(const FString& Params)
{
	FString FilePath;
	if (!FParse::Value(*Params, TEXT("SourceCSV="), FilePath) || FilePath.IsEmpty())
	{
		FilePath = FPaths::Combine(FPaths::ProjectSavedDir(),
			TEXT("ARMEMENT01/ARMEMENT01_4_Profiles.csv"));
	}
	if (FPaths::IsRelative(FilePath))
	{
		FilePath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir(), FilePath);
	}
	FString Csv;
	if (!FFileHelper::LoadFileToString(Csv, *FilePath))
	{
		UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.4] Cannot read normalized CSV: %s"), *FilePath);
		return 1;
	}

	TArray<FGridWeaponLevelProfile> ParsedProfiles;
	FString Error;
	if (!ParseCsv(Csv, ParsedProfiles, Error))
	{
		UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.4] Import refused: %s"), *Error);
		return 1;
	}

	// Only save after the entire catalogue has passed strict validation.
	const FString PackageName(TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponCatalogue"));
	const FString AssetName(TEXT("DA_WeaponCatalogue"));
	const FString ObjectPath = PackageName + TEXT(".") + AssetName;
	UGridWeaponCatalogueAsset* Asset = LoadObject<UGridWeaponCatalogueAsset>(nullptr, *ObjectPath);
	if (!Asset)
	{
		UPackage* Package = CreatePackage(*PackageName);
		if (!Package)
		{
			UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.4] Failed creating package %s"), *PackageName);
			return 1;
		}
		Asset = NewObject<UGridWeaponCatalogueAsset>(Package, FName(*AssetName),
			RF_Public | RF_Standalone | RF_Transactional);
		if (!Asset)
		{
			return 1;
		}
		FAssetRegistryModule::AssetCreated(Asset);
	}

	// Validate imported values without changing an existing serialized asset.
	UGridWeaponCatalogueAsset* Preview = NewObject<UGridWeaponCatalogueAsset>(GetTransientPackage());
	Preview->Profiles = MoveTemp(ParsedProfiles);
	if (!Preview->IsValidCatalogue())
	{
		UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.4] Import refused: catalogue is incomplete or inconsistent."));
		return 1;
	}
	Asset->Modify();
	Asset->Profiles = MoveTemp(Preview->Profiles);
	UPackage* Package = Asset->GetOutermost();
	Package->MarkPackageDirty();

	const FString PackageFile = FPackageName::LongPackageNameToFilename(
		PackageName, FPackageName::GetAssetPackageExtension());
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(PackageFile), true);
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.SaveFlags = SAVE_NoError;
	if (!UPackage::SavePackage(Package, Asset, *PackageFile, SaveArgs))
	{
		UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.4] SavePackage failed: %s"), *PackageFile);
		return 1;
	}
	UE_LOG(LogTemp, Display,
		TEXT("[ARMEMENT01.4] Imported 82 archetypes x 20 immutable levels (1640 records) into %s."),
		*ObjectPath);
	return 0;
}
