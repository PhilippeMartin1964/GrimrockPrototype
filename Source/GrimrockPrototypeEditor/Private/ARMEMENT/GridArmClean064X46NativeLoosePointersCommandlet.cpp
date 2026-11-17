#include "ARMEMENT/GridArmClean064X46NativeLoosePointersCommandlet.h"

#include "Core/GridLevelAsset.h"
#include "Dom/JsonObject.h"
#include "Engine/GameInstance.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridCanonicalItemCatalogues.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/StrongObjectPtr.h"

namespace ARMCLEAN064X46
{
    constexpr TCHAR LevelPath[] =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/"
             "DA_GridLevel_00.DA_GridLevel_00");
    constexpr TCHAR RootPath[] =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/"
             "DA_ItemSystem.DA_ItemSystem");

    bool IsSha40(const FString& Hash)
    {
        if (Hash.Len() != 40) { return false; }
        for (const TCHAR Ch : Hash)
        {
            if (!FChar::IsDigit(Ch) && (Ch < TEXT('a') || Ch > TEXT('f')))
            {
                return false;
            }
        }
        return true;
    }

    bool BackupExactlyMatchesCurrentFile(const FString& BackupPath,
        const FString& LivePath, const FString& ExpectedHead)
    {
        if (!IsSha40(ExpectedHead) || BackupPath.IsEmpty() ||
            FPaths::IsRelative(BackupPath) || !FPaths::FileExists(BackupPath) ||
            !FPaths::FileExists(LivePath))
        {
            return false;
        }
        FString Backup = FPaths::ConvertRelativePathToFull(BackupPath);
        FString Project = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
        FPaths::NormalizeFilename(Backup);
        FPaths::NormalizeDirectoryName(Project);
        if (Backup.Equals(Project, ESearchCase::IgnoreCase) ||
            Backup.StartsWith(Project + TEXT("/"), ESearchCase::IgnoreCase))
        {
            return false; // No snapshots under tracked/untracked project files.
        }
        TArray<uint8> Before;
        TArray<uint8> Snapshot;
        return FFileHelper::LoadFileToArray(Before, *LivePath) &&
            FFileHelper::LoadFileToArray(Snapshot, *Backup) &&
            !Before.IsEmpty() && Before == Snapshot;
    }

    bool ValidateReal44(const UGridLevelAsset& Level, const UGridItemSubsystem& Registry,
        int32& OutOldPointers, FString& OutError)
    {
        OutOldPointers = 0;
        OutError.Reset();
        if (Level.LooseItemInstances.Num() != 44 || !Level.ItemSpawns.IsEmpty())
        {
            OutError = TEXT("Refused: target must contain 44 loose items and no item spawns.");
            return false;
        }
        TSet<FGuid> Guids;
        for (const FGridLooseItemInstance& Item : Level.LooseItemInstances)
        {
            if (!Item.InstanceId.IsValid() || Guids.Contains(Item.InstanceId) ||
                !Level.IsValidCoord(Item.CellX, Item.CellY) ||
                Item.CanonicalItemId.IsNone() || Item.Quantity < 1 ||
                Item.ItemLevel < 0 || Item.ItemLevel > 20 ||
                Item.LocalOffset.ContainsNaN() || !FMath::IsFinite(Item.LocalYaw))
            {
                OutError = TEXT("Refused: invalid GUID, location, quantity or level.");
                return false;
            }
            Guids.Add(Item.InstanceId);

            FGridResolvedItem Resolved;
            if (!Registry.ResolveItem(Item.CanonicalItemId, Item.ItemLevel, Resolved) ||
                !Resolved.IsValid() || Resolved.ItemId != Item.CanonicalItemId ||
                Resolved.ItemLevel != Item.ItemLevel ||
                (!Resolved.bIsArmament && Item.ItemLevel != 0) ||
                Item.Quantity > FMath::Max(1, Resolved.MaxStack))
            {
                OutError = TEXT("Refused: failed registered physical identity check.");
                return false;
            }
            if (Item.ItemDefinition)
            {
                const UGridItemDefinitionAsset* Legacy = Item.ItemDefinition.Get();
                if (!IsValid(Legacy) || Legacy->ItemDefinitionId != Item.CanonicalItemId ||
                    (Resolved.bIsArmament && Legacy->WeaponArchetypeId != Resolved.ArchetypeId) ||
                    (!Resolved.bIsArmament && !Legacy->WeaponArchetypeId.IsNone()))
                {
                    OutError = TEXT("Refused: legacy source diverged from registered identity.");
                    return false;
                }
                ++OutOldPointers;
            }
        }
        if (OutOldPointers != 0 && OutOldPointers != 44)
        {
            OutError = TEXT("Refused: partially converted target, expected 0 or 44 pointers.");
            return false;
        }
        return true;
    }

    bool PreservedAuthoring(const FGridLooseItemInstance& Before,
        const FGridLooseItemInstance& After)
    {
        return Before.InstanceId == After.InstanceId &&
            Before.CanonicalItemId == After.CanonicalItemId &&
            Before.ItemLevel == After.ItemLevel &&
            Before.Quantity == After.Quantity &&
            Before.CellX == After.CellX && Before.CellY == After.CellY &&
            Before.SurfaceSide == After.SurfaceSide &&
            Before.LocalOffset.Equals(After.LocalOffset) &&
            FMath::IsNearlyEqual(Before.LocalYaw, After.LocalYaw) &&
            Before.LogicId == After.LogicId &&
            Before.ReadableContentAsset == After.ReadableContentAsset &&
            Before.ReadableContentId == After.ReadableContentId &&
            Before.ReadTitleOverride.EqualTo(After.ReadTitleOverride) &&
            Before.ReadTextOverride.EqualTo(After.ReadTextOverride) &&
            Before.Tag == After.Tag &&
            Before.Notes == After.Notes &&
            Before.PaletteEntryId == After.PaletteEntryId &&
            After.ItemDefinition == nullptr;
    }

    bool SaveReport(const FString& Path, const FString& Mode,
        int32 OldPointers, int32 PackagesSaved)
    {
        if (Path.IsEmpty()) { return false; }
        const FString ReportPath = FPaths::ConvertRelativePathToFull(Path);
        FString Project = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
        FPaths::NormalizeDirectoryName(Project);
        if (ReportPath.StartsWith(Project + TEXT("/"), ESearchCase::IgnoreCase))
        {
            return false; // Reports must be private, not untracked in the repo.
        }
        TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
        Root->SetStringField(TEXT("schema"), TEXT("ARM-CLEAN06.4X46/native-loose44/v1"));
        Root->SetStringField(TEXT("mode"), Mode);
        Root->SetNumberField(TEXT("physical_items"), 44);
        Root->SetNumberField(TEXT("legacy_pointers"), OldPointers);
        Root->SetNumberField(TEXT("packages_saved"), PackagesSaved);
        Root->SetStringField(TEXT("target"), TEXT("DA_GridLevel_00"));
        Root->SetBoolField(TEXT("other_world_objects_modified"), false);
        Root->SetBoolField(TEXT("palette_or_other_levels_modified"), false);
        Root->SetBoolField(TEXT("savegames_touched"), false);
        FString Json;
        return FJsonSerializer::Serialize(Root,
            TJsonWriterFactory<>::Create(&Json)) &&
            FFileHelper::SaveStringToFile(Json, *ReportPath,
                FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    }
}

UGridArmClean064X46NativeLoosePointersCommandlet::
    UGridArmClean064X46NativeLoosePointersCommandlet()
{
    IsClient = false;
    IsEditor = true;
    LogToConsole = true;
}

int32 UGridArmClean064X46NativeLoosePointersCommandlet::Main(const FString& Params)
{
    const bool bPreflight = FParse::Param(*Params, TEXT("Preflight"));
    const bool bVerify = FParse::Param(*Params, TEXT("Verify"));
    const bool bApply = FParse::Param(*Params, TEXT("Apply"));
    if (static_cast<int32>(bPreflight) + static_cast<int32>(bVerify) +
        static_cast<int32>(bApply) != 1)
    {
        UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4X46] Exactly one mode required."));
        return 1;
    }
    FString Output;
    if (!FParse::Value(*Params, TEXT("Output="), Output) || Output.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4X46] Private -Output required."));
        return 1;
    }
    TStrongObjectPtr<UGridLevelAsset> Level(
        LoadObject<UGridLevelAsset>(nullptr, ARMCLEAN064X46::LevelPath));
    TStrongObjectPtr<UGridItemSystemAsset> Root(
        LoadObject<UGridItemSystemAsset>(nullptr, ARMCLEAN064X46::RootPath));
    if (!IsValid(Level.Get()) || !IsValid(Root.Get()))
    {
        UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4X46] Native target/root missing."));
        return 2;
    }
    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>(GetTransientPackage()));
    TStrongObjectPtr<UGridItemSubsystem> Registry(
        NewObject<UGridItemSubsystem>(GI.Get()));
    TArray<FString> Errors;
    if (!Registry->RegisterItemSystem(Root.Get(), Errors))
    {
        UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4X46] Registry failed."));
        return 2;
    }

    int32 OldCount = INDEX_NONE;
    FString Error;
    if (!ARMCLEAN064X46::ValidateReal44(*Level.Get(), *Registry.Get(), OldCount, Error))
    {
        UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4X46] %s"), *Error);
        return 3;
    }
    const FString Mode = bApply ? TEXT("Apply") :
        (bVerify ? TEXT("Verify") : TEXT("Preflight"));
    if (bPreflight)
    {
        return ARMCLEAN064X46::SaveReport(Output, Mode, OldCount, 0) ? 0 : 9;
    }
    if (bVerify)
    {
        if (!ARMCLEAN064X46::SaveReport(Output, Mode, OldCount, 0)) { return 9; }
        return OldCount == 0 ? 0 : 4; // Exact mismatch code used by runner.
    }
    if (OldCount == 0)
    {
        UE_LOG(LogTemp, Display, TEXT("[ARM-CLEAN06.4X46] Already canonical; verified no-op."));
        return ARMCLEAN064X46::SaveReport(Output, Mode, 0, 0) ? 0 : 9;
    }

    FString ExpectedHead;
    FString BackupFile;
    if (!FParse::Value(*Params, TEXT("ExpectedHead="), ExpectedHead) ||
        !FParse::Value(*Params, TEXT("BackupFile="), BackupFile))
    {
        UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4X46] No native backup receipt."));
        return 5;
    }
    UPackage* TargetPackage = Level->GetOutermost();
    if (!IsValid(TargetPackage) || TargetPackage->IsDirty())
    {
        UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4X46] Target already dirty."));
        return 5;
    }
    const FString LiveFile = FPackageName::LongPackageNameToFilename(
        TargetPackage->GetName(), FPackageName::GetAssetPackageExtension());
    if (!ARMCLEAN064X46::BackupExactlyMatchesCurrentFile(
        BackupFile, LiveFile, ExpectedHead))
    {
        UE_LOG(LogTemp, Error,
            TEXT("[ARM-CLEAN06.4X46] Refused: private backup is missing or not byte-equal."));
        return 5;
    }

    // Exact single-field operation: no other UObject/config is touched.
    const TArray<FGridLooseItemInstance> Before = Level->LooseItemInstances;
    Level->Modify();
    for (FGridLooseItemInstance& Item : Level->LooseItemInstances)
    {
        Item.ItemDefinition = nullptr;
    }
    int32 Remaining = INDEX_NONE;
    if (!ARMCLEAN064X46::ValidateReal44(*Level.Get(), *Registry.Get(),
            Remaining, Error) || Remaining != 0 ||
        Before.Num() != Level->LooseItemInstances.Num())
    {
        UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4X46] Staged projection rejected."));
        return 7; // No SavePackage.
    }
    for (int32 Index = 0; Index < Before.Num(); ++Index)
    {
        if (!ARMCLEAN064X46::PreservedAuthoring(
            Before[Index], Level->LooseItemInstances[Index]))
        {
            UE_LOG(LogTemp, Error,
                TEXT("[ARM-CLEAN06.4X46] Authored data was changed beyond the pointer."));
            return 7;
        }
    }

    FSavePackageArgs Args;
    Args.TopLevelFlags = RF_Public | RF_Standalone;
    Args.SaveFlags = SAVE_NoError;
    if (!UPackage::SavePackage(TargetPackage, Level.Get(), *LiveFile, Args))
    {
        UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4X46] SavePackage failed: restore backup."));
        return 8;
    }
    if (!ARMCLEAN064X46::SaveReport(Output, Mode, 0, 1))
    {
        return 9; // Caller MUST restore exact private backup.
    }
    UE_LOG(LogTemp, Display,
        TEXT("[ARM-CLEAN06.4X46] Saved ONE native level: 44 pointers removed. Verify using a second Unreal process."));
    return 0;
}
