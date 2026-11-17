#include "ARMEMENT/GridArmClean064X37NativeArmWorldProfilesCommandlet.h"

#include "GridArmClean064X37ArmWorldProfileProjection.h"
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

namespace
{
    constexpr TCHAR RootObjectPath[] =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem");

    bool CheckPrivateBackupManifest(const FString& Path, const FString& ExpectedHead)
    {
        if (Path.IsEmpty() || ExpectedHead.Len() != 40 ||
            !FPaths::FileExists(Path))
        {
            return false;
        }
        const FString AbsoluteBackup = FPaths::ConvertRelativePathToFull(Path);
        const FString Project = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
        if (AbsoluteBackup.StartsWith(Project, ESearchCase::IgnoreCase))
        {
            return false; // Never keep the verified backups inside the repo.
        }

        FString Text;
        TSharedPtr<FJsonObject> Manifest;
        if (!FFileHelper::LoadFileToString(Text, *Path) ||
            !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Manifest) ||
            !Manifest.IsValid())
        {
            return false;
        }
        FString Schema, Head;
        bool bVerified = false;
        int32 SourceCount = 0, OutputCount = 0;
        const TArray<TSharedPtr<FJsonValue>>* Files = nullptr;
        return Manifest->TryGetStringField(TEXT("schema"), Schema) &&
            Schema == TEXT("ARM-CLEAN06.4X.37/native-backup/v1") &&
            Manifest->TryGetStringField(TEXT("head"), Head) &&
            Head == ExpectedHead &&
            Manifest->TryGetBoolField(TEXT("verified"), bVerified) && bVerified &&
            Manifest->TryGetNumberField(TEXT("source_count"), SourceCount) &&
            SourceCount == 22 &&
            Manifest->TryGetNumberField(TEXT("output_count"), OutputCount) &&
            OutputCount == 2 &&
            Manifest->TryGetArrayField(TEXT("files"), Files) &&
            Files && Files->Num() == 24;
    }

    bool SaveNativeCatalogue(UObject* Asset)
    {
        UPackage* Package = IsValid(Asset) ? Asset->GetOutermost() : nullptr;
        if (!IsValid(Package)) { return false; }
        const FString Filename = FPackageName::LongPackageNameToFilename(
            Package->GetName(), FPackageName::GetAssetPackageExtension());
        Package->MarkPackageDirty();
        FSavePackageArgs Args;
        Args.TopLevelFlags = RF_Public | RF_Standalone;
        Args.SaveFlags = SAVE_NoError;
        return UPackage::SavePackage(Package, Asset, *Filename, Args);
    }
}

UGridArmClean064X37NativeArmWorldProfilesCommandlet::
    UGridArmClean064X37NativeArmWorldProfilesCommandlet()
{
    IsClient = false;
    IsEditor = true;
    LogToConsole = true;
}

int32 UGridArmClean064X37NativeArmWorldProfilesCommandlet::Main(const FString& Params)
{
    const bool bPreflight = FParse::Param(*Params, TEXT("Preflight"));
    const bool bApply = FParse::Param(*Params, TEXT("Apply"));
    const bool bVerify = FParse::Param(*Params, TEXT("Verify"));
    if (static_cast<int32>(bPreflight) + static_cast<int32>(bApply) +
        static_cast<int32>(bVerify) != 1)
    {
        UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4X.37] Exactly one of -Preflight, -Apply, -Verify required."));
        return 1;
    }

    UGridItemSystemAsset* Root = LoadObject<UGridItemSystemAsset>(nullptr, RootObjectPath);
    if (!IsValid(Root) || !IsValid(Root->WeaponCatalogue) ||
        !IsValid(Root->WeaponVisualCatalogue))
    {
        UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4X.37] Canonical root catalogues missing."));
        return 2;
    }

    ARMCLEAN064X37::FPlan Plan;
    FString Error;
    if (!ARMCLEAN064X37::BuildPlan(Root, Plan, Error))
    {
        UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4X.37] Preflight refused: %s"), *Error);
        return 3;
    }

    if (bVerify)
    {
        const bool bValid = ARMCLEAN064X37::VerifyDiskProfiles(Root, Error);
        if (bValid)
        {
            UE_LOG(LogTemp, Display, TEXT("[ARM-CLEAN06.4X.37] Reloaded native parity: SUCCESS (22 ARM, two catalogues)."));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4X.37] Reloaded native parity: FAILED (%s)."), *Error);
        }
        return bValid ? 0 : 4;
    }
    if (bPreflight)
    {
        UE_LOG(LogTemp, Display,
            TEXT("[ARM-CLEAN06.4X.37] Read-only preflight: sources=%d mismatching_profiles=%d; allowed binaries=2; no packages saved."),
            Plan.SourceCount, Plan.ChangedProfiles);
        return 0;
    }

    FString BackupManifest, ExpectedHead;
    if (!FParse::Value(*Params, TEXT("BackupManifest="), BackupManifest) ||
        !FParse::Value(*Params, TEXT("ExpectedHead="), ExpectedHead) ||
        !CheckPrivateBackupManifest(BackupManifest, ExpectedHead))
    {
        UE_LOG(LogTemp, Error,
            TEXT("[ARM-CLEAN06.4X.37] Apply refused: verified private 24-file backup receipt required."));
        return 5;
    }

    if (Plan.ChangedProfiles == 0)
    {
        UE_LOG(LogTemp, Display,
            TEXT("[ARM-CLEAN06.4X.37] All 22 ARM already match; no binary write."));
        return 0;
    }

    // Two arrays are staged and checked before touching the real catalogues.
    const bool bVisualWasDirty =
        Root->WeaponVisualCatalogue->GetOutermost()->IsDirty();
    const bool bWeaponWasDirty =
        Root->WeaponCatalogue->GetOutermost()->IsDirty();
    if (bVisualWasDirty || bWeaponWasDirty)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[ARM-CLEAN06.4X.37] Apply refused: loaded catalogue already modified."));
        return 6;
    }

    UGridCanonicalWeaponVisualCatalogueAsset* Visual = Root->WeaponVisualCatalogue.Get();
    UGridCanonicalWeaponCatalogueAsset* Weapon = Root->WeaponCatalogue.Get();
    Visual->Modify();
    Weapon->Modify();
    Visual->VisualProfiles = MoveTemp(Plan.Visuals);
    Weapon->Archetypes = MoveTemp(Plan.Archetypes);

    TArray<FString> ValidationErrors;
    if (!Visual->Validate(ValidationErrors) ||
        !Weapon->Validate(ValidationErrors) ||
        !Root->Validate(ValidationErrors) ||
        !ARMCLEAN064X37::VerifyDiskProfiles(Root, Error))
    {
        UE_LOG(LogTemp, Error,
            TEXT("[ARM-CLEAN06.4X.37] Staged catalogue validation failed; no SavePackage performed."));
        return 7;
    }

    // Only two existing UAssets may be written. If one save fails, the
    // Windows runner restores BOTH from the verified private snapshot.
    if (!SaveNativeCatalogue(Visual) || !SaveNativeCatalogue(Weapon))
    {
        UE_LOG(LogTemp, Error,
            TEXT("[ARM-CLEAN06.4X.37] Native SavePackage failed: restore BOTH catalogues from backup."));
        return 8;
    }
    UE_LOG(LogTemp, Display,
        TEXT("[ARM-CLEAN06.4X.37] Native SavePackage completed for 22 ARM / 2 catalogues. Run -Verify in a separate UE process before commit."));
    return 0;
}
