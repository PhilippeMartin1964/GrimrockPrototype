#include "ARMEMENT/GridArmClean065G71PuzzleCategoryCommandlet.h"
#include "GridEditorPuzzleCategoryMigrationPlan.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/SavePackage.h"

namespace Plan = GridEditorPuzzleCategoryMigration;

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

    bool Report(const FString& Output, int32 Tagged, int32 Saved, const FString& Fingerprint)
    {
        const TSharedRef<FJsonObject> R = MakeShared<FJsonObject>();
        R->SetStringField(TEXT("schema"), TEXT("ARM-CLEAN06.5G71/puzzle/v1"));
        R->SetNumberField(TEXT("tagged"), Tagged);
        R->SetNumberField(TEXT("packages_saved"), Saved);
        R->SetStringField(TEXT("normalized_manifest"), Fingerprint);
        R->SetBoolField(TEXT("savegames_touched"), false);
        FString Json;
        return FJsonSerializer::Serialize(R, TJsonWriterFactory<>::Create(&Json)) &&
            FFileHelper::SaveStringToFile(Json, *Output, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    }
}

UGridArmClean065G71PuzzleCategoryCommandlet::UGridArmClean065G71PuzzleCategoryCommandlet()
{
    IsClient = false;
    IsEditor = true;
    LogToConsole = true;
}

int32 UGridArmClean065G71PuzzleCategoryCommandlet::Main(const FString& Params)
{
    const bool bAudit = FParse::Param(*Params, TEXT("Audit"));
    const bool bApply = FParse::Param(*Params, TEXT("Apply"));
    const bool bVerify = FParse::Param(*Params, TEXT("Verify"));
    FString Output;
    if (int32(bAudit) + int32(bApply) + int32(bVerify) != 1 ||
        !FParse::Value(*Params, TEXT("Output="), Output) || !OutsideProject(Output) || FPaths::FileExists(Output)) return 1;
    TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr, GridEditorRetirementAudit::RootPath()));
    TStrongObjectPtr<UGridItemSystemAsset> Preview(NewObject<UGridItemSystemAsset>());
    FString Error;
    if (!Root.IsValid() || !Plan::BuildPreview(*Root.Get(), *Preview.Get(), Error))
    {
        UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.5G71] Plan refused: %s"), *Error);
        return 2;
    }
    const FString Manifest = Plan::Fingerprint(*Root.Get());
    if (Manifest.IsEmpty() || Manifest != Plan::Fingerprint(*Preview.Get())) return 3;
    int32 Tagged = 0;
    for (const auto& Def : Root->ItemDefinitions)
        Tagged += GridCanonicalPuzzleCategoryTags::CountReserved(Def->Tags);
    if (bAudit) return Report(Output, Tagged, 0, Manifest) ? 0 : 9;
    if (bVerify) return Report(Output, Tagged, 0, Manifest) ? (Tagged == 26 ? 0 : 4) : 9;
    if (Tagged != 0) return 5;
    FString Head, BackupDir;
    FString Project = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
    FPaths::NormalizeDirectoryName(Project);
    if (!Project.EndsWith(TEXT("/GrimrockPrototype_UEValidation"), ESearchCase::IgnoreCase) ||
        !FParse::Value(*Params, TEXT("ExpectedHead="), Head) || Head.Len() != 40 ||
        !FParse::Value(*Params, TEXT("BackupDir="), BackupDir) || !OutsideProject(BackupDir)) return 5;
    for (TCHAR Ch : Head)
        if (!FChar::IsDigit(Ch) && (Ch < TEXT('a') || Ch > TEXT('f'))) return 5;

    struct FWrite
    {
        UGridCanonicalItemDefinitionAsset* Def;
        TArray<FName> Tags;
        FString Filename;
    };
    TArray<FWrite> Writes;
    for (int32 I = 0; I < Root->ItemDefinitions.Num(); ++I)
    {
        auto* Def = Root->ItemDefinitions[I].Get();
        if (!Plan::Manifest().Contains(Def->ItemId)) continue;
        const FString Filename = FPackageName::LongPackageNameToFilename(
            Def->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
        const FString Backup = FPaths::Combine(BackupDir, FPaths::GetCleanFilename(Filename) + TEXT(".before"));
        TArray<uint8> Original, BackupBytes;
        if (!OutsideProject(Backup) || !FFileHelper::LoadFileToArray(Original, *Filename) ||
            !FFileHelper::LoadFileToArray(BackupBytes, *Backup) || Original.IsEmpty() || Original != BackupBytes)
            return 5;
        Writes.Add({Def, Preview->ItemDefinitions[I]->Tags, Filename});
    }
    if (Writes.Num() != 26) return 5;
    // Verify every source before the first package write.
    for (auto& Entry : Writes) Entry.Def->Tags = Entry.Tags;
    if (Plan::Fingerprint(*Root.Get()) != Manifest) return 6;
    TArray<FString> Errors;
    if (!Root->Validate(Errors)) return 6;
    FSavePackageArgs Args;
    Args.TopLevelFlags = RF_Public | RF_Standalone;
    Args.SaveFlags = SAVE_NoError;
    int32 Saved = 0;
    for (auto& Entry : Writes)
    {
        UPackage* Package = Entry.Def->GetOutermost();
        Package->MarkPackageDirty();
        if (!UPackage::SavePackage(Package, Entry.Def, *Entry.Filename, Args)) return 7;
        ++Saved;
    }
    return Report(Output, 26, Saved, Manifest) ? 0 : 9;
}
