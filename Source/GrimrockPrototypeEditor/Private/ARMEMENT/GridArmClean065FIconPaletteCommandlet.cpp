#include "ARMEMENT/GridArmClean065FIconPaletteCommandlet.h"
#include "GridEditorArmPaletteDetachPlan.h"
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/SavePackage.h"

namespace Plan = GridEditorArmPaletteDetachPlan;
namespace
{
    bool OutsideProject(const FString& Path)
    {
        if (Path.IsEmpty() || FPaths::IsRelative(Path)) return false;
        FString Full = FPaths::ConvertRelativePathToFull(Path);
        FString Project = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
        FPaths::NormalizeFilename(Full); FPaths::CollapseRelativeDirectories(Full);
        FPaths::NormalizeDirectoryName(Project); FPaths::CollapseRelativeDirectories(Project);
        return !Full.Equals(Project, ESearchCase::IgnoreCase) && !Full.StartsWith(Project + TEXT("/"), ESearchCase::IgnoreCase);
    }

    bool Report(const FString& Output, const TCHAR* Mode, const Plan::FPlan& P, bool bAssigned, int32 Saved)
    {
        TSharedRef<FJsonObject> R = MakeShared<FJsonObject>();
        R->SetStringField(TEXT("schema"), TEXT("ARM-CLEAN06.5F/icon-palette/v1"));
        R->SetStringField(TEXT("mode"), Mode);
        R->SetNumberField(TEXT("icons"), P.IconCount);
        R->SetBoolField(TEXT("assigned"), bAssigned);
        R->SetBoolField(TEXT("disk_reference_observed_before_operation"), P.bDiskReference);
        R->SetNumberField(TEXT("packages_saved"), Saved);
        R->SetStringField(TEXT("normalized_manifest"), P.Manifest);
        R->SetBoolField(TEXT("savegames_touched"), false);
        FString Json;
        return FJsonSerializer::Serialize(R, TJsonWriterFactory<>::Create(&Json)) &&
            FFileHelper::SaveStringToFile(Json, *Output, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    }
}

UGridArmClean065FIconPaletteCommandlet::UGridArmClean065FIconPaletteCommandlet()
{
    IsClient = false;
    IsEditor = true;
    LogToConsole = true;
}

int32 UGridArmClean065FIconPaletteCommandlet::Main(const FString& Params)
{
    const bool bAudit = FParse::Param(*Params, TEXT("Audit"));
    const bool bApply = FParse::Param(*Params, TEXT("Apply"));
    const bool bVerify = FParse::Param(*Params, TEXT("Verify"));
    FString Output;
    if (int32(bAudit) + int32(bApply) + int32(bVerify) != 1 ||
        !FParse::Value(*Params, TEXT("Output="), Output) || !OutsideProject(Output) || FPaths::FileExists(Output)) return 1;
    Plan::FPlan P;
    FString Error;
    if (!Plan::Build(P, Error))
    {
        UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.5F] Plan refused: %s"), *Error);
        return 2;
    }
    if (bAudit) return Report(Output, TEXT("Audit"), P, P.bAssigned, 0) ? 0 : 9;
    if (bVerify) return Report(Output, TEXT("Verify"), P, P.bAssigned, 0) ? (P.bAssigned ? 4 : 0) : 9;
    if (!P.bAssigned) return 5;
    FString Head, Backup;
    if (!FParse::Value(*Params, TEXT("ExpectedHead="), Head) || Head.Len() != 40 ||
        !FParse::Value(*Params, TEXT("Backup0="), Backup) || !OutsideProject(Backup)) return 5;
    for (TCHAR Ch : Head) if (!FChar::IsDigit(Ch) && (Ch < TEXT('a') || Ch > TEXT('f'))) return 5;
    UPackage* Package = P.Blueprint->GetOutermost();
    const FString Filename = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
    TArray<uint8> Original, BackupBytes;
    if (!FFileHelper::LoadFileToArray(Original, *Filename) || !FFileHelper::LoadFileToArray(BackupBytes, *Backup) ||
        Original.IsEmpty() || Original != BackupBytes) return 5;

    // Only change one generated-class default. Do not rebuild the hierarchy.
    P.Defaults->EquippedWeaponActionIconPalette = nullptr;
    FString After;
    if (!Plan::Fingerprint(P.Blueprint, P.Defaults, After, Error) || After != P.Manifest) return 6;
    FSavePackageArgs Args;
    Args.TopLevelFlags = RF_Public | RF_Standalone;
    Args.SaveFlags = SAVE_NoError;
    Package->MarkPackageDirty();
    if (!UPackage::SavePackage(Package, P.Blueprint, *Filename, Args)) return 7;
    // A fresh Verify process proves the actual saved CDO and dependency.
    return Report(Output, TEXT("Apply"), P, false, 1) ? 0 : 9;
}
