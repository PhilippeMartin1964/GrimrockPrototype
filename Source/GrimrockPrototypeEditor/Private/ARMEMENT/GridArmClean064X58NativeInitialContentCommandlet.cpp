#include "ARMEMENT/GridArmClean064X58NativeInitialContentCommandlet.h"

#include "GridEditorNativeInitialContentPlan.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Engine/GameInstance.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Runtime/Items/GridCanonicalItemCatalogues.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/StrongObjectPtr.h"

namespace ARMCLEAN064X58
{
using FPlan = FGridEditorNativeInitialContentPlan;
const TCHAR* RootPath = TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem");

bool ExactSha(const FString& Sha)
{
    if (Sha.Len() != 40) return false;
    for (const TCHAR Ch : Sha)
        if (!FChar::IsDigit(Ch) && (Ch < TEXT('a') || Ch > TEXT('f'))) return false;
    return true;
}
FString Normalized(FString Path)
{
    Path = FPaths::ConvertRelativePathToFull(Path);
    FPaths::NormalizeFilename(Path);
    FPaths::CollapseRelativeDirectories(Path);
    return Path;
}
bool OutsideProject(const FString& Path)
{
    if (Path.IsEmpty() || FPaths::IsRelative(Path)) return false;
    const FString Full = Normalized(Path);
    FString Project = Normalized(FPaths::ProjectDir());
    FPaths::NormalizeDirectoryName(Project);
    return !Full.Equals(Project, ESearchCase::IgnoreCase) &&
        !Full.StartsWith(Project + TEXT("/"), ESearchCase::IgnoreCase);
}
bool ExactBackup(const FString& Backup, const FString& Live)
{
    TArray<uint8> Before, Snapshot;
    return OutsideProject(Backup) &&
        FFileHelper::LoadFileToArray(Before, *Live) &&
        FFileHelper::LoadFileToArray(Snapshot, *Backup) &&
        !Before.IsEmpty() && Before == Snapshot;
}
bool Clean(const TArray<UGridLevelAsset*>& Levels, const UGridItemSystemAsset* Root)
{
    if (!IsValid(Root) || Root->GetOutermost()->IsDirty()) return false;
    for (const UGridLevelAsset* Level : Levels)
        if (!IsValid(Level) || Level->GetOutermost()->IsDirty()) return false;
    return true;
}
TArray<FString> Manifests(const TArray<UGridLevelAsset*>& Levels, const TArray<FPlan::FLevel>& Plans)
{
    TArray<FString> Out;
    for (int32 Index = 0; Index < Levels.Num(); ++Index)
        Out.Add(FPlan::Manifest(*Levels[Index], Plans[Index]));
    return Out;
}

bool Report(const FString& Output, const FString& Mode, int32 Old, int32 Saved,
    const TArray<FString>& Manifest)
{
    if (!OutsideProject(Output) || Manifest.Num() != FPlan::LevelCount) return false;
    TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
    Json->SetStringField(TEXT("schema"), TEXT("ARM-CLEAN06.4X58/initial-two/v1"));
    Json->SetStringField(TEXT("mode"), Mode);
    Json->SetNumberField(TEXT("targets"), FPlan::TargetCount);
    Json->SetNumberField(TEXT("initial_rows"), FPlan::ExpectedRows);
    Json->SetNumberField(TEXT("legacy_pointers"), Old);
    Json->SetNumberField(TEXT("packages_saved"), Saved);
    Json->SetBoolField(TEXT("only_two_levels_allowed"), true);
    Json->SetBoolField(TEXT("savegames_touched"), false);
    TArray<TSharedPtr<FJsonValue>> Rows;
    for (int32 Index = 0; Index < Manifest.Num(); ++Index)
    {
        TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
        Row->SetStringField(TEXT("level"), FPlan::Path(Index));
        Row->SetNumberField(TEXT("initial_rows"), FPlan::OriginalRows(Index));
        Row->SetStringField(TEXT("normalized_authoring"), Manifest[Index]);
        Rows.Add(MakeShared<FJsonValueObject>(Row));
    }
    Json->SetArrayField(TEXT("levels"), Rows);
    FString Text;
    return FJsonSerializer::Serialize(Json, TJsonWriterFactory<>::Create(&Text)) &&
        FFileHelper::SaveStringToFile(Text, *Output, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}
}

UGridArmClean064X58NativeInitialContentCommandlet::UGridArmClean064X58NativeInitialContentCommandlet()
{
    IsClient = false;
    IsEditor = true;
    LogToConsole = true;
}

int32 UGridArmClean064X58NativeInitialContentCommandlet::Main(const FString& Params)
{
    using namespace ARMCLEAN064X58;
    const bool bPreflight = FParse::Param(*Params, TEXT("Preflight"));
    const bool bVerify = FParse::Param(*Params, TEXT("Verify"));
    const bool bApply = FParse::Param(*Params, TEXT("Apply"));
    FString Output;
    if (static_cast<int32>(bPreflight) + static_cast<int32>(bVerify) +
        static_cast<int32>(bApply) != 1 ||
        !FParse::Value(*Params, TEXT("Output="), Output) || !OutsideProject(Output))
        return 1;

    TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr, RootPath));
    if (!Root.IsValid() || Root->ItemDefinitions.Num() != 34) return 2;
    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>(GetTransientPackage()));
    TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
    TArray<FString> Errors;
    if (!Registry.IsValid() || !Registry->RegisterItemSystem(Root.Get(), Errors)) return 2;

    TArray<TStrongObjectPtr<UGridLevelAsset>> Owned;
    TArray<UGridLevelAsset*> Levels;
    for (int32 Index = 0; Index < FPlan::LevelCount; ++Index)
    {
        Owned.Emplace(LoadObject<UGridLevelAsset>(nullptr, FPlan::Path(Index)));
        Levels.Add(Owned.Last().Get());
    }
    TArray<FPlan::FLevel> Plans;
    FString Error;
    int32 Old = INDEX_NONE;
    if (!Clean(Levels, Root.Get()) ||
        !FPlan::TryBuild(Levels, Registry.Get(), Plans, Old, Error))
    {
        UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4X58] Read-only plan refused: %s"), *Error);
        return 3;
    }
    const TArray<FString> Before = Manifests(Levels, Plans);
    if (!Clean(Levels, Root.Get())) return 3;
    const FString Mode = bApply ? TEXT("Apply") : (bVerify ? TEXT("Verify") : TEXT("Preflight"));
    if (bPreflight)
        return Report(Output, Mode, Old, 0, Before) ? 0 : 9;
    if (bVerify)
    {
        if (!Report(Output, Mode, Old, 0, Before)) return 9;
        return Old == 0 ? 0 : 4;
    }
    if (Old == 0)
        return Report(Output, Mode, 0, 0, Before) ? 0 : 9;

    FString Head;
    if (Old != FPlan::ExpectedRows ||
        !FParse::Value(*Params, TEXT("ExpectedHead="), Head) || !ExactSha(Head))
        return 5;
    TSet<FString> Unique;
    TArray<FString> LiveFiles;
    for (int32 Index = 0; Index < FPlan::TargetCount; ++Index)
    {
        const FString Key = FString::Printf(TEXT("Backup%d="), Index);
        FString Backup;
        if (!FParse::Value(*Params, *Key, Backup) || !OutsideProject(Backup)) return 5;
        FString Path = Normalized(Backup);
        Path.ToLowerInline();
        if (Unique.Contains(Path) || Path.Equals(Normalized(Output), ESearchCase::IgnoreCase)) return 5;
        Unique.Add(Path);
        const FString Live = FPackageName::LongPackageNameToFilename(
            Levels[Index]->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
        if (!ExactBackup(Backup, Live))
        {
            UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4X58] Private backup %d is not byte-equal."), Index);
            return 5;
        }
        LiveFiles.Add(Live);
    }
    if (!Clean(Levels, Root.Get())) return 5;

    // Both receipts were verified BEFORE modifying either level. The only
    // native assignment is the historical pointer inside initial contents.
    for (int32 Index = 0; Index < FPlan::TargetCount; ++Index)
    {
        UGridLevelAsset* Level = Levels[Index];
        Level->Modify();
        for (FGridWorldObjectInstance& Object : Level->WorldObjectInstances)
            for (FGridReceptacleInitialItemConfig& Item : Object.InstanceConfig.ReceptacleInitialContent)
                Item.ItemDefinition = nullptr;
        if (!FPlan::SameObjects(Level->WorldObjectInstances, Plans[Index].WorldObjects)) return 7;
    }
    TArray<FPlan::FLevel> Rechecked;
    int32 Remaining = INDEX_NONE;
    if (!FPlan::TryBuild(Levels, Registry.Get(), Rechecked, Remaining, Error) ||
        Remaining != 0 || Manifests(Levels, Rechecked) != Before ||
        Root->GetOutermost()->IsDirty())
    {
        UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4X58] Projected level authoring changed: %s"), *Error);
        return 7;
    }
    for (int32 Index = FPlan::TargetCount; Index < FPlan::LevelCount; ++Index)
        if (Levels[Index]->GetOutermost()->IsDirty()) return 7;

    FSavePackageArgs Args;
    Args.TopLevelFlags = RF_Public | RF_Standalone;
    Args.SaveFlags = SAVE_NoError;
    int32 Saved = 0;
    for (int32 Index = 0; Index < FPlan::TargetCount; ++Index)
    {
        UGridLevelAsset* Level = Levels[Index];
        if (!UPackage::SavePackage(Level->GetOutermost(), Level, *LiveFiles[Index], Args))
        {
            UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4X58] SavePackage failed after %d writes; rollback both."), Saved);
            return 8;
        }
        ++Saved;
    }
    if (!Report(Output, Mode, 0, Saved, Before)) return 9;
    UE_LOG(LogTemp, Display, TEXT("[ARM-CLEAN06.4X58] Both levels saved; verify in a fresh UE process."));
    return 0;
}
