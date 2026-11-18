#include "ARMEMENT/GridArmClean064X57NativePaletteCommandlet.h"

#include "GridEditorNativePalettePlan.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "RPG/RPGStoryCompanionAsset.h"
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
#include "UObject/UnrealType.h"

namespace ARMCLEAN064X57
{
    const TCHAR* PalettePath = TEXT("/Game/GrimrockPrototype/Core/DataAssets/"
                                    "DA_ObjectPalette_Default.DA_ObjectPalette_Default");
    const TCHAR* RootPath = TEXT("/Game/GrimrockPrototype/Core/DataAssets/"
                                 "CanonicalStage/DA_ItemSystem.DA_ItemSystem");

    bool ExactSha(const FString& Sha)
    {
        if (Sha.Len() != 40) return false;
        for (const TCHAR Ch : Sha)
            if (!FChar::IsDigit(Ch) && (Ch < TEXT('a') || Ch > TEXT('f'))) return false;
        return true;
    }

    FString NormalizedPath(const FString& Path)
    {
        FString Full = FPaths::ConvertRelativePathToFull(Path);
        FPaths::NormalizeFilename(Full);
        FPaths::CollapseRelativeDirectories(Full);
        return Full;
    }

    bool OutsideProject(const FString& Path)
    {
        if (Path.IsEmpty() || FPaths::IsRelative(Path)) return false;
        const FString Full = NormalizedPath(Path);
        FString Project = NormalizedPath(FPaths::ProjectDir());
        FPaths::NormalizeDirectoryName(Project);
        return !Full.Equals(Project, ESearchCase::IgnoreCase) &&
            !Full.StartsWith(Project + TEXT("/"), ESearchCase::IgnoreCase);
    }

    bool BackupIsExact(const FString& Backup, const FString& Live)
    {
        TArray<uint8> Original, Snapshot;
        return OutsideProject(Backup) && FPaths::FileExists(Backup) &&
            FPaths::FileExists(Live) && FFileHelper::LoadFileToArray(Original, *Live) &&
            FFileHelper::LoadFileToArray(Snapshot, *Backup) &&
            !Original.IsEmpty() && Original == Snapshot;
    }

    bool SameEntries(const TArray<FGridObjectPaletteEntry>& A,
        const TArray<FGridObjectPaletteEntry>& B)
    {
        if (A.Num() != B.Num()) return false;
        for (int32 Index = 0; Index < A.Num(); ++Index)
            for (TFieldIterator<FProperty> Field(FGridObjectPaletteEntry::StaticStruct());
                Field; ++Field)
                if (!Field->Identical_InContainer(&A[Index], &B[Index])) return false;
        return true;
    }

    bool Report(const FString& Path, const FString& Mode,
        const UGridObjectPaletteAsset& Palette, int32 Old, int32 Saved)
    {
        if (!OutsideProject(Path)) return false;
        TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
        Root->SetStringField(TEXT("schema"), TEXT("ARM-CLEAN06.4X57/palette-one/v1"));
        Root->SetStringField(TEXT("mode"), Mode);
        Root->SetNumberField(TEXT("targets"), 1);
        Root->SetNumberField(TEXT("direct_entries"), FGridEditorNativePalettePlan::ExpectedDirectEntries);
        Root->SetNumberField(TEXT("legacy_pointers"), Old);
        Root->SetNumberField(TEXT("packages_saved"), Saved);
        Root->SetBoolField(TEXT("only_palette_package_allowed"), true);
        Root->SetBoolField(TEXT("savegames_touched"), false);
        Root->SetNumberField(TEXT("entry_count"), Palette.Entries.Num());

        // Ordered, persisted-field manifest excluding ONLY DefaultItemDefinition.
        // The runner compares this across the original and a fresh reopened UE process.
        TArray<TSharedPtr<FJsonValue>> Rows;
        for (const FGridObjectPaletteEntry& Entry : Palette.Entries)
        {
            TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
            Row->SetStringField(TEXT("entry_id"), Entry.EntryId.ToString());
            Row->SetStringField(TEXT("category"), Entry.PaletteCategory.ToString());
            Row->SetStringField(TEXT("item_id"), Entry.CanonicalItemId.ToString());
            Row->SetNumberField(TEXT("item_level"), Entry.CanonicalDefaultItemLevel);
            Row->SetStringField(TEXT("icon"), GetPathNameSafe(Entry.Icon.Get()));
            Row->SetStringField(TEXT("world_definition"), GetPathNameSafe(Entry.DefaultWorldObjectDefinition.Get()));
            Row->SetStringField(TEXT("monster_definition"), GetPathNameSafe(Entry.DefaultMonsterDefinition.Get()));
            Row->SetStringField(TEXT("companion_definition"), GetPathNameSafe(Entry.DefaultStoryCompanionDefinition.Get()));
            Row->SetBoolField(TEXT("direct_item"), Entry.IsDirectItemEntry());
            Rows.Add(MakeShared<FJsonValueObject>(Row));
        }
        Root->SetArrayField(TEXT("entries"), Rows);
        FString Json;
        return FJsonSerializer::Serialize(Root, TJsonWriterFactory<>::Create(&Json)) &&
            FFileHelper::SaveStringToFile(Json, *Path,
                FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    }
}

UGridArmClean064X57NativePaletteCommandlet::UGridArmClean064X57NativePaletteCommandlet()
{
    IsClient = false;
    IsEditor = true;
    LogToConsole = true;
}

int32 UGridArmClean064X57NativePaletteCommandlet::Main(const FString& Params)
{
    using namespace ARMCLEAN064X57;
    const bool bPreflight = FParse::Param(*Params, TEXT("Preflight"));
    const bool bVerify = FParse::Param(*Params, TEXT("Verify"));
    const bool bApply = FParse::Param(*Params, TEXT("Apply"));
    FString Output;
    if (static_cast<int32>(bPreflight) + static_cast<int32>(bVerify) +
            static_cast<int32>(bApply) != 1 ||
        !FParse::Value(*Params, TEXT("Output="), Output) || !OutsideProject(Output))
        return 1;

    TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr, RootPath));
    TStrongObjectPtr<UGridObjectPaletteAsset> Palette(
        LoadObject<UGridObjectPaletteAsset>(nullptr, PalettePath));
    if (!Root.IsValid() || Root->ItemDefinitions.Num() != 34 || !Palette.IsValid() ||
        Palette->GetPathName() != FString(PalettePath) ||
        Root->GetOutermost()->IsDirty() || Palette->GetOutermost()->IsDirty())
        return 2;

    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>(GetTransientPackage()));
    TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
    TArray<FString> Errors;
    if (!Registry.IsValid() || !Registry->RegisterItemSystem(Root.Get(), Errors)) return 2;

    TArray<FGridObjectPaletteEntry> Plan;
    int32 Old = INDEX_NONE;
    FString Error;
    if (!FGridEditorNativePalettePlan::TryBuild(*Palette.Get(), Registry.Get(), Plan, Old, Error) ||
        Root->GetOutermost()->IsDirty() || Palette->GetOutermost()->IsDirty())
    {
        UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4X57] Read-only plan refused: %s"), *Error);
        return 3;
    }

    const FString Mode = bApply ? TEXT("Apply") : (bVerify ? TEXT("Verify") : TEXT("Preflight"));
    if (bPreflight)
        return Report(Output, Mode, *Palette.Get(), Old, 0) ? 0 : 9;
    if (bVerify)
    {
        if (!Report(Output, Mode, *Palette.Get(), Old, 0)) return 9;
        return Old == 0 ? 0 : 4;
    }
    if (Old == 0)
        return Report(Output, Mode, *Palette.Get(), 0, 0) ? 0 : 9;

    FString Head, Backup;
    if (Old != FGridEditorNativePalettePlan::ExpectedDirectEntries ||
        !FParse::Value(*Params, TEXT("ExpectedHead="), Head) || !ExactSha(Head) ||
        !FParse::Value(*Params, TEXT("Backup="), Backup) ||
        !OutsideProject(Backup) ||
        NormalizedPath(Backup).Equals(NormalizedPath(Output), ESearchCase::IgnoreCase))
        return 5;

    UPackage* Package = Palette->GetOutermost();
    if (!IsValid(Package) || Package->IsDirty()) return 5;
    const FString Live = FPackageName::LongPackageNameToFilename(
        Package->GetName(), FPackageName::GetAssetPackageExtension());
    if (!BackupIsExact(Backup, Live))
    {
        UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4X57] Private backup is not byte-equal."));
        return 5;
    }

    Palette->Modify();
    for (FGridObjectPaletteEntry& Entry : Palette->Entries)
        if (Entry.IsDirectItemEntry()) Entry.DefaultItemDefinition = nullptr;

    int32 Remaining = INDEX_NONE;
    TArray<FGridObjectPaletteEntry> Rechecked;
    if (!SameEntries(Palette->Entries, Plan) ||
        !FGridEditorNativePalettePlan::TryBuild(*Palette.Get(), Registry.Get(), Rechecked, Remaining, Error) ||
        Remaining != 0 || !SameEntries(Palette->Entries, Rechecked) ||
        Root->GetOutermost()->IsDirty())
    {
        UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4X57] Modified projection refused: %s"), *Error);
        return 7;
    }
    FSavePackageArgs Args;
    Args.TopLevelFlags = RF_Public | RF_Standalone;
    Args.SaveFlags = SAVE_NoError;
    if (!UPackage::SavePackage(Package, Palette.Get(), *Live, Args))
    {
        UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4X57] SavePackage failed; runner must restore private backup."));
        return 8;
    }
    if (!Report(Output, Mode, *Palette.Get(), 0, 1)) return 9;
    UE_LOG(LogTemp, Display, TEXT("[ARM-CLEAN06.4X57] Native palette saved; verify in a fresh UE process."));
    return 0;
}
