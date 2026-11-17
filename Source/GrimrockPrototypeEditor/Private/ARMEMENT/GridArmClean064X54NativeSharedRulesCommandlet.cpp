#include "ARMEMENT/GridArmClean064X54NativeSharedRulesCommandlet.h"

#include "Core/GridWorldObjectDefinitionAsset.h"
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

namespace ARMCLEAN064X54
{
    struct FTarget
    {
        const TCHAR* ObjectPath;
        const TCHAR* DefinitionId;
        const TCHAR* AcceptedId;
        bool bLock;
    };

    // Deliberately closed: these four and no other .uasset may be saved.
    const FTarget Targets[] = {
        {TEXT("/Game/GrimrockPrototype/Core/DataAssets/"
              "DA_Receptacle_TorchHolder.DA_Receptacle_TorchHolder"),
         TEXT("Receptacle_TorchHolder"), TEXT("Item_Torch"), false},
        {TEXT("/Game/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/"
              "DA_Guardian.DA_Guardian"),
         TEXT("Guardian"), TEXT("Gem_Blue"), false},
        {TEXT("/Game/GrimrockPrototype/Core/DataAssets/"
              "DA_Lock_CopperWall.DA_Lock_CopperWall"),
         TEXT("Lock_CopperWallLock"), TEXT("Key_Copper"), true},
        {TEXT("/Game/GrimrockPrototype/Core/DataAssets/"
              "DA_Lock_IronWall.DA_Lock_IronWall"),
         TEXT("Lock_IronWallLock"), TEXT("Key_Iron"), true}
    };
    constexpr int32 TargetCount = UE_ARRAY_COUNT(Targets);
    const TCHAR* RootPath = TEXT("/Game/GrimrockPrototype/Core/DataAssets/"
                                 "CanonicalStage/DA_ItemSystem.DA_ItemSystem");

    bool IsExactSha40(const FString& Value)
    {
        if (Value.Len() != 40) return false;
        for (const TCHAR Ch : Value)
        {
            if (!FChar::IsDigit(Ch) && (Ch < TEXT('a') || Ch > TEXT('f')))
                return false;
        }
        return true;
    }

    bool OutsideProject(FString Path)
    {
        if (Path.IsEmpty() || FPaths::IsRelative(Path)) return false;
        Path = FPaths::ConvertRelativePathToFull(Path);
        FString Project = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
        FPaths::NormalizeFilename(Path);
        FPaths::NormalizeDirectoryName(Project);
        return !Path.Equals(Project, ESearchCase::IgnoreCase) &&
            !Path.StartsWith(Project + TEXT("/"), ESearchCase::IgnoreCase);
    }

    bool BackupIsExact(const FString& BackupPath, const FString& LiveFile)
    {
        if (!OutsideProject(BackupPath) || !FPaths::FileExists(BackupPath) ||
            !FPaths::FileExists(LiveFile)) return false;
        TArray<uint8> Before;
        TArray<uint8> Snapshot;
        return FFileHelper::LoadFileToArray(Before, *LiveFile) &&
            FFileHelper::LoadFileToArray(Snapshot, *BackupPath) &&
            !Before.IsEmpty() && Before == Snapshot;
    }

    // Valid in two and ONLY two complete native states: 4 old pointers, or 0.
    bool Validate(const TArray<UGridWorldObjectDefinitionAsset*>& Assets,
        const UGridItemSubsystem& Registry, int32& OutPointers, FString& OutError)
    {
        OutPointers = 0;
        if (Assets.Num() != TargetCount)
        {
            OutError = TEXT("Exactly four native shared definitions required.");
            return false;
        }
        for (int32 Index = 0; Index < TargetCount; ++Index)
        {
            const FTarget& Expected = Targets[Index];
            const UGridWorldObjectDefinitionAsset* Asset = Assets[Index];
            const FName ItemId(Expected.AcceptedId);
            if (!IsValid(Asset) || Asset->GetPathName() != FString(Expected.ObjectPath) ||
                Asset->DefinitionId != FName(Expected.DefinitionId) ||
                !Registry.HasCanonicalItem(ItemId))
            {
                OutError = TEXT("An exact rule package, definition or registered ID diverged.");
                return false;
            }

            if (Expected.bLock)
            {
                const FGridLockBehaviorParams& Lock = Asset->DefaultBehavior.Lock;
                if (Lock.AcceptedKeyIds.Num() != 1 || Lock.AcceptedKeyIds[0] != ItemId ||
                    Lock.AcceptedKeyItems.Num() > 1)
                {
                    OutError = TEXT("Lock key list is not the exact approved single ID.");
                    return false;
                }
                if (!Lock.AcceptedKeyItems.IsEmpty())
                {
                    const UGridItemDefinitionAsset* Old = Lock.AcceptedKeyItems[0].Get();
                    if (!IsValid(Old) || Old->ItemDefinitionId != ItemId)
                    {
                        OutError = TEXT("Historical lock pointer disagrees with canonical ID.");
                        return false;
                    }
                    ++OutPointers;
                }
            }
            else
            {
                const FGridReceptacleBehaviorParams& Receptacle =
                    Asset->DefaultBehavior.Receptacle;
                if (Receptacle.AcceptedItems.Num() != 1 ||
                    Receptacle.AcceptedItems[0].CanonicalItemId != ItemId)
                {
                    OutError = TEXT("Receptacle acceptance list or ItemId diverged.");
                    return false;
                }
                const UGridItemDefinitionAsset* Old =
                    Receptacle.AcceptedItems[0].ItemDefinition.Get();
                if (Old)
                {
                    if (!IsValid(Old) || Old->ItemDefinitionId != ItemId)
                    {
                        OutError = TEXT("Historical receptacle pointer mismatches ItemId.");
                        return false;
                    }
                    ++OutPointers;
                }
            }
        }
        if (OutPointers != 0 && OutPointers != TargetCount)
        {
            OutError = TEXT("Partially converted native transaction refused.");
            return false;
        }
        return true;
    }

    bool Report(const FString& Path, const FString& Mode,
        int32 RemainingPointers, int32 SavedPackages)
    {
        if (!OutsideProject(Path)) return false;
        TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
        Root->SetStringField(TEXT("schema"), TEXT("ARM-CLEAN06.4X54/shared-four/v1"));
        Root->SetStringField(TEXT("mode"), Mode);
        Root->SetNumberField(TEXT("targets"), TargetCount);
        Root->SetNumberField(TEXT("legacy_pointers"), RemainingPointers);
        Root->SetNumberField(TEXT("packages_saved"), SavedPackages);
        Root->SetBoolField(TEXT("only_four_packages_allowed"), true);
        Root->SetBoolField(TEXT("savegames_touched"), false);
        FString Json;
        return FJsonSerializer::Serialize(Root, TJsonWriterFactory<>::Create(&Json)) &&
            FFileHelper::SaveStringToFile(Json, *Path,
                FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    }
}

UGridArmClean064X54NativeSharedRulesCommandlet::
    UGridArmClean064X54NativeSharedRulesCommandlet()
{
    IsClient = false;
    IsEditor = true;
    LogToConsole = true;
}

int32 UGridArmClean064X54NativeSharedRulesCommandlet::Main(const FString& Params)
{
    const bool bPreflight = FParse::Param(*Params, TEXT("Preflight"));
    const bool bVerify = FParse::Param(*Params, TEXT("Verify"));
    const bool bApply = FParse::Param(*Params, TEXT("Apply"));
    FString Output;
    if (static_cast<int32>(bPreflight) + static_cast<int32>(bVerify) +
            static_cast<int32>(bApply) != 1 ||
        !FParse::Value(*Params, TEXT("Output="), Output) ||
        !ARMCLEAN064X54::OutsideProject(Output))
        return 1;

    TStrongObjectPtr<UGridItemSystemAsset> ItemSystem(
        LoadObject<UGridItemSystemAsset>(nullptr, ARMCLEAN064X54::RootPath));
    if (!IsValid(ItemSystem.Get()) || ItemSystem->ItemDefinitions.Num() != 34)
        return 2;
    const bool bItemSystemDirty = ItemSystem->GetOutermost()->IsDirty();
    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>(GetTransientPackage()));
    TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
    TArray<FString> Errors;
    if (!IsValid(Registry.Get()) ||
        !Registry->RegisterItemSystem(ItemSystem.Get(), Errors)) return 2;

    TArray<UGridWorldObjectDefinitionAsset*> Assets;
    for (const ARMCLEAN064X54::FTarget& Target : ARMCLEAN064X54::Targets)
    {
        UGridWorldObjectDefinitionAsset* Definition =
            LoadObject<UGridWorldObjectDefinitionAsset>(nullptr, Target.ObjectPath);
        if (!IsValid(Definition) || Definition->GetOutermost()->IsDirty())
        {
            UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4X54] Missing or dirty native package."));
            return 3;
        }
        Assets.Add(Definition);
    }
    if (ItemSystem->GetOutermost()->IsDirty() != bItemSystemDirty) return 3;

    int32 OldCount = INDEX_NONE;
    FString Error;
    if (!ARMCLEAN064X54::Validate(Assets, *Registry.Get(), OldCount, Error))
    {
        UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4X54] %s"), *Error);
        return 3;
    }
    const FString Mode = bApply ? TEXT("Apply") :
        (bVerify ? TEXT("Verify") : TEXT("Preflight"));
    if (bPreflight)
        return ARMCLEAN064X54::Report(Output, Mode, OldCount, 0) ? 0 : 9;
    if (bVerify)
    {
        if (!ARMCLEAN064X54::Report(Output, Mode, OldCount, 0)) return 9;
        return OldCount == 0 ? 0 : 4;
    }
    if (OldCount == 0)
    {
        UE_LOG(LogTemp, Display, TEXT("[ARM-CLEAN06.4X54] Already pointerless; no-op."));
        return ARMCLEAN064X54::Report(Output, Mode, 0, 0) ? 0 : 9;
    }

    FString Head;
    if (!FParse::Value(*Params, TEXT("ExpectedHead="), Head) ||
        !ARMCLEAN064X54::IsExactSha40(Head)) return 5;

    TSet<FString> UniqueBackups;
    for (int32 Index = 0; Index < ARMCLEAN064X54::TargetCount; ++Index)
    {
        const FString Key = FString::Printf(TEXT("Backup%d="), Index);
        FString BackupPath;
        if (!FParse::Value(*Params, *Key, BackupPath) ||
            !ARMCLEAN064X54::OutsideProject(BackupPath)) return 5;
        FString Canonical = FPaths::ConvertRelativePathToFull(BackupPath);
        FPaths::NormalizeFilename(Canonical);
        Canonical.ToLowerInline();
        if (UniqueBackups.Contains(Canonical)) return 5;
        UniqueBackups.Add(Canonical);

        UPackage* Package = Assets[Index]->GetOutermost();
        if (!IsValid(Package) || Package->IsDirty()) return 5;
        const FString Live = FPackageName::LongPackageNameToFilename(
            Package->GetName(), FPackageName::GetAssetPackageExtension());
        if (!ARMCLEAN064X54::BackupIsExact(BackupPath, Live))
        {
            UE_LOG(LogTemp, Error,
                TEXT("[ARM-CLEAN06.4X54] Private backup %d not byte-equal."), Index);
            return 5;
        }
    }

    // All four receipts are validated BEFORE the first object modification.
    for (int32 Index = 0; Index < ARMCLEAN064X54::TargetCount; ++Index)
    {
        UGridWorldObjectDefinitionAsset* Asset = Assets[Index];
        Asset->Modify();
        if (ARMCLEAN064X54::Targets[Index].bLock)
            Asset->DefaultBehavior.Lock.AcceptedKeyItems.Reset();
        else
            Asset->DefaultBehavior.Receptacle.AcceptedItems[0].ItemDefinition = nullptr;
    }
    int32 Remaining = INDEX_NONE;
    if (!ARMCLEAN064X54::Validate(Assets, *Registry.Get(), Remaining, Error) ||
        Remaining != 0)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[ARM-CLEAN06.4X54] Staged native projection refused: %s"), *Error);
        return 7; // No SavePackage: outer runner restores any on-disk differences.
    }

    FSavePackageArgs Args;
    Args.TopLevelFlags = RF_Public | RF_Standalone;
    Args.SaveFlags = SAVE_NoError;
    int32 Saved = 0;
    for (UGridWorldObjectDefinitionAsset* Asset : Assets)
    {
        UPackage* Package = Asset->GetOutermost();
        const FString Live = FPackageName::LongPackageNameToFilename(
            Package->GetName(), FPackageName::GetAssetPackageExtension());
        if (!UPackage::SavePackage(Package, Asset, *Live, Args))
        {
            UE_LOG(LogTemp, Error,
                TEXT("[ARM-CLEAN06.4X54] SavePackage failed after %d writes; runner MUST rollback all four."),
                Saved);
            return 8;
        }
        ++Saved;
    }
    if (!ARMCLEAN064X54::Report(Output, Mode, 0, Saved)) return 9;
    UE_LOG(LogTemp, Display,
        TEXT("[ARM-CLEAN06.4X54] Four rule assets saved; verify in a fresh UE process."));
    return 0;
}
