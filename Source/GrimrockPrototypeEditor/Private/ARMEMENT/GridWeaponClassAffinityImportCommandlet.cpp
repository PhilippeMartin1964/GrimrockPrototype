#include "ARMEMENT/GridWeaponClassAffinityImportCommandlet.h"

#include "Runtime/Combat/GridWeaponCatalogueAsset.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/PackageName.h"
#include "Serialization/Csv/CsvParser.h"
#include "UObject/SavePackage.h"

namespace WeaponAffinityImport
{
    FString ResolvePath(const FString& Params, const TCHAR* Parameter, const TCHAR* DefaultRelative)
    {
        FString Path;
        if (!FParse::Value(*Params, Parameter, Path) || Path.IsEmpty())
        {
            Path = FPaths::Combine(FPaths::ProjectSavedDir(), DefaultRelative);
        }
        return FPaths::IsRelative(Path)
            ? FPaths::ConvertRelativePathToFull(FPaths::ProjectDir(), Path) : Path;
    }

    bool LoadRows(const FString& Path, const TArray<FString>& Header,
        TArray<TArray<FString>>& OutRows, FString& OutError)
    {
        FString Input;
        if (!FFileHelper::LoadFileToString(Input, *Path))
        {
            OutError = FString::Printf(TEXT("Cannot read generated CSV %s"), *Path);
            return false;
        }
        FCsvParser Parser(Input);
        const auto& Rows = Parser.GetRows();
        if (Rows.IsEmpty() || Rows[0].Num() != Header.Num())
        {
            OutError = FString::Printf(TEXT("Unexpected CSV header in %s"), *Path);
            return false;
        }
        for (int32 Column = 0; Column < Header.Num(); ++Column)
        {
            FString Value(Rows[0][Column] ? Rows[0][Column] : TEXT(""));
            if (Column == 0) Value.RemoveFromStart(FString(1, TCHAR(0xFEFF)));
            if (Value != Header[Column])
            {
                OutError = FString::Printf(TEXT("Unexpected column %d in %s"), Column, *Path);
                return false;
            }
        }
        OutRows.Reset();
        for (int32 Index = 1; Index < Rows.Num(); ++Index)
        {
            const TArray<const TCHAR*>& Source = Rows[Index];
            if (Source.IsEmpty() || !Source[0] || !*Source[0]) continue;
            if (Source.Num() != Header.Num())
            {
                OutError = FString::Printf(TEXT("Invalid row %d in %s"), Index + 1, *Path);
                return false;
            }
            TArray<FString>& Values = OutRows.AddDefaulted_GetRef();
            for (const TCHAR* Field : Source)
            {
                Values.Add(Field ? FString(Field) : FString());
            }
        }
        return true;
    }

    bool ParseAffinities(const TArray<TArray<FString>>& Rows,
        TArray<FGridWeaponArchetypeClassAffinity>& Out)
    {
        if (Rows.Num() != 82) return false;
        Out.Reset();
        for (const TArray<FString>& Values : Rows)
        {
            FGridWeaponArchetypeClassAffinity& Row = Out.AddDefaulted_GetRef();
            Row.ArchetypeId = FName(*Values[0]);
            Row.PrimaryClassId = FName(*Values[1]);
            Row.SecondarySourceLabel = Values[3];
            TArray<FString> Split;
            Values[2].ParseIntoArray(Split, TEXT(";"), true);
            for (const FString& Value : Split)
            {
                Row.SecondaryClassIds.Add(FName(*Value));
            }
        }
        return true;
    }

    bool ParsePreferences(const TArray<TArray<FString>>& Rows,
        TArray<FGridWeaponClassPreference>& Out)
    {
        if (Rows.Num() != 6) return false;
        Out.Reset();
        for (const TArray<FString>& Values : Rows)
        {
            FGridWeaponClassPreference& Row = Out.AddDefaulted_GetRef();
            Row.ClassId = FName(*Values[0]);
            Row.CanonicalBranches = Values[1];
            Row.PriorityWeapons = Values[2];
            Row.EmblematicExamples = Values[3];
            Row.Alternatives = Values[4];
            Row.DominantAttributes = Values[5];
            Row.Synergies = Values[6];
        }
        return true;
    }

    bool IsSameProjection(const UGridWeaponCatalogueAsset& A, const UGridWeaponCatalogueAsset& B)
    {
        if (A.ClassAffinities.Num() != B.ClassAffinities.Num() ||
            A.ClassPreferences.Num() != B.ClassPreferences.Num()) return false;
        for (int32 I = 0; I < A.ClassAffinities.Num(); ++I)
        {
            const auto& X = A.ClassAffinities[I];
            const auto& Y = B.ClassAffinities[I];
            if (X.ArchetypeId != Y.ArchetypeId || X.PrimaryClassId != Y.PrimaryClassId ||
                X.SecondaryClassIds != Y.SecondaryClassIds ||
                X.SecondarySourceLabel != Y.SecondarySourceLabel) return false;
        }
        for (int32 I = 0; I < A.ClassPreferences.Num(); ++I)
        {
            const auto& X = A.ClassPreferences[I];
            const auto& Y = B.ClassPreferences[I];
            if (X.ClassId != Y.ClassId || X.CanonicalBranches != Y.CanonicalBranches ||
                X.PriorityWeapons != Y.PriorityWeapons ||
                X.EmblematicExamples != Y.EmblematicExamples ||
                X.Alternatives != Y.Alternatives ||
                X.DominantAttributes != Y.DominantAttributes || X.Synergies != Y.Synergies)
            {
                return false;
            }
        }
        return true;
    }
}

UGridWeaponClassAffinityImportCommandlet::UGridWeaponClassAffinityImportCommandlet()
{
    IsEditor = true;
    IsClient = false;
    IsServer = false;
    LogToConsole = true;
}

int32 UGridWeaponClassAffinityImportCommandlet::Main(const FString& Params)
{
    using namespace WeaponAffinityImport;
    const FString AffinityFile = ResolvePath(Params, TEXT("AffinityCSV="),
        TEXT("ARMEMENT01/ARMEMENT01_9_5_2_Affinities.csv"));
    const FString ClassFile = ResolvePath(Params, TEXT("ClassesCSV="),
        TEXT("ARMEMENT01/ARMEMENT01_9_5_2_ClassPreferences.csv"));
    TArray<TArray<FString>> AffinityRows, PreferenceRows;
    FString Error;
    if (!LoadRows(AffinityFile,
        {TEXT("ArchetypeId"), TEXT("PrimaryClassId"), TEXT("SecondaryClassIds"), TEXT("SecondarySourceLabel")},
        AffinityRows, Error) ||
        !LoadRows(ClassFile,
        {TEXT("ClassId"), TEXT("Branches"), TEXT("PriorityWeapons"), TEXT("Examples"),
         TEXT("Alternatives"), TEXT("DominantAttributes"), TEXT("Synergies")},
        PreferenceRows, Error))
    {
        UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.9.5.2] %s"), *Error);
        return 1;
    }
    UGridWeaponCatalogueAsset* Preview = NewObject<UGridWeaponCatalogueAsset>(GetTransientPackage());
    if (!ParseAffinities(AffinityRows, Preview->ClassAffinities) ||
        !ParsePreferences(PreferenceRows, Preview->ClassPreferences) ||
        !Preview->IsValidClassAffinityData())
    {
        UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.9.5.2] Invalid workbook-derived affinity projection."));
        return 1;
    }
    const TCHAR* AssetPath =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponCatalogue.DA_WeaponCatalogue");
    UGridWeaponCatalogueAsset* Asset = LoadObject<UGridWeaponCatalogueAsset>(nullptr, AssetPath);
    if (!Asset || !Asset->IsValidCatalogue())
    {
        UE_LOG(LogTemp, Error,
            TEXT("[ARMEMENT01.9.5.2] Existing valid 1640-profile catalogue is required; none created."));
        return 1;
    }
    if (IsSameProjection(*Asset, *Preview))
    {
        UE_LOG(LogTemp, Display, TEXT("[ARMEMENT01.9.5.2] Changed=false (82 affinities / 6 classes)."));
        return 0;
    }
    // Strictly touch metadata arrays. Never replace the validated 1640 profiles.
    Asset->Modify();
    Asset->ClassAffinities = MoveTemp(Preview->ClassAffinities);
    Asset->ClassPreferences = MoveTemp(Preview->ClassPreferences);
    UPackage* Package = Asset->GetOutermost();
    Package->MarkPackageDirty();
    const FString PackageFile = FPackageName::LongPackageNameToFilename(
        Package->GetName(), FPackageName::GetAssetPackageExtension());
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    SaveArgs.SaveFlags = SAVE_NoError;
    if (!UPackage::SavePackage(Package, Asset, *PackageFile, SaveArgs))
    {
        UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.9.5.2] Cannot save %s"), *PackageFile);
        return 1;
    }
    UE_LOG(LogTemp, Display, TEXT("[ARMEMENT01.9.5.2] Changed=true. 82 weapon affinities / 6 class preferences. Profiles unchanged."));
    return 0;
}
