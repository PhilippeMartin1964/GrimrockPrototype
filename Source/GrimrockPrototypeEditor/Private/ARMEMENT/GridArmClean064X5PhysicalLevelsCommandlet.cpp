#include "ARMEMENT/GridArmClean064X5PhysicalLevelsCommandlet.h"

#include "ARMEMENT/GridArmClean064X3AuthoredLevelMaterializer.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridObjectPaletteAsset.h"
#include "Dom/JsonObject.h"
#include "Engine/GameInstance.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridCanonicalItemCatalogues.h"
#include "Runtime/Items/GridCanonicalItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
constexpr TCHAR RegistryPath[] = TEXT(
    "/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem");
constexpr TCHAR PalettePath[] = TEXT(
    "/Game/GrimrockPrototype/Core/DataAssets/DA_ObjectPalette_Default.DA_ObjectPalette_Default");
const TCHAR* const LevelPaths[] = {
    TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/"
         "DA_GridLevel_00.DA_GridLevel_00"),
    TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/"
         "DA_GridLevel_01.DA_GridLevel_01"),
    TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/"
         "DA_GridLevel_New_Level_03.DA_GridLevel_New_Level_03"),
    TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/"
         "DA_GridLevel_Pillars_of_Light.DA_GridLevel_Pillars_of_Light")
};

bool SaveNativePackage(UObject* Asset)
{
    if (!IsValid(Asset)) { return false; }
    UPackage* Package = Asset->GetOutermost();
    if (!IsValid(Package)) { return false; }
    const FString Filename = FPackageName::LongPackageNameToFilename(
        Package->GetName(), FPackageName::GetAssetPackageExtension());
    FSavePackageArgs Args;
    Args.TopLevelFlags = RF_Public | RF_Standalone;
    Args.SaveFlags = SAVE_NoError;
    Package->MarkPackageDirty();
    return UPackage::SavePackage(Package, Asset, *Filename, Args);
}

struct FCounts
{
    int32 Loose = 0;
    int32 Spawn = 0;
    int32 Receptacle = 0;
    int32 Palette = 0;
    int32 Checked = 0;
    int32 Armament = 0;
    int32 InheritedWorldArmaments = 0;
    int32 NonArm = 0;
    int32 WorldLevelsChanged = 0;
    int32 PaletteLevelsChanged = 0;
};

bool CheckAndMaterialize(FName Id, UGridItemDefinitionAsset* Source,
    const TMap<FName, const UGridCanonicalItemDefinitionAsset*>& Index,
    UGridItemSubsystem* Resolver, int32& InOutLevel, int32 Quantity,
    bool bApply, bool bPalette, FCounts& Counts)
{
    if (!IsValid(Source) || Id.IsNone() ||
        Source->ItemDefinitionId != Id || Quantity < 1)
    {
        return false;
    }
    const UGridCanonicalItemDefinitionAsset* const* Found = Index.Find(Id);
    if (!Found || !IsValid(*Found)) { return false; }

    // Before application, W's real authored placements are strictly N0.
    // For an existing palette entry the canonical default was not yet stored.
    // After application we require EXACTLY the level inferred from its source.
    if (bApply && InOutLevel != 0) { return false; }

    int32 Effective = INDEX_NONE;
    bool bInherited = false;
    if (!FGridArmClean064X3AuthoredLevelMaterializer::TryResolve(
        *Found, 0, Source->DefaultWeaponItemLevel,
        Effective, bInherited))
    {
        return false;
    }
    const UGridArmamentDefinitionAsset* Arm =
        Cast<UGridArmamentDefinitionAsset>(*Found);
    if (Arm)
    {
        if (Arm->ArchetypeId.IsNone() ||
            Source->WeaponArchetypeId != Arm->ArchetypeId ||
            Effective < 1 || Effective > 20 || !bInherited)
        {
            return false;
        }
        ++Counts.Armament;
        if (!bPalette) { ++Counts.InheritedWorldArmaments; }
    }
    else
    {
        if (!Cast<UGridNonArmamentDefinitionAsset>(*Found) ||
            !Source->WeaponArchetypeId.IsNone() ||
            Effective != 0 || bInherited)
        {
            return false;
        }
        ++Counts.NonArm;
    }

    FGridResolvedItem Resolved;
    if (!IsValid(Resolver) ||
        !Resolver->ResolveItem(Id, Effective, Resolved) ||
        !Resolved.IsValid() || Resolved.ItemId != Id ||
        Resolved.ItemLevel != Effective ||
        Resolved.bIsArmament != (Arm != nullptr) ||
        (Arm && Resolved.ArchetypeId != Arm->ArchetypeId))
    {
        return false;
    }

    if (bApply)
    {
        if (Effective != InOutLevel)
        {
            InOutLevel = Effective;
            if (bPalette) { ++Counts.PaletteLevelsChanged; }
            else { ++Counts.WorldLevelsChanged; }
        }
    }
    else if (InOutLevel != Effective)
    {
        // No old source-pointer or guessed N1 fallback. Exact persisted
        // physical N-level is mandatory once the final binaries exist.
        return false;
    }
    ++Counts.Checked;
    return true;
}
}

UGridArmClean064X5PhysicalLevelsCommandlet::
    UGridArmClean064X5PhysicalLevelsCommandlet()
{
    IsClient = false;
    IsEditor = true;
    LogToConsole = true;
}

int32 UGridArmClean064X5PhysicalLevelsCommandlet::Main(
    const FString& Params)
{
    const bool bApply = FParse::Param(*Params, TEXT("Apply"));
    const bool bVerify = FParse::Param(*Params, TEXT("Verify"));
    FString Output;
    if (bApply == bVerify ||
        !FParse::Value(*Params, TEXT("Output="), Output) ||
        Output.IsEmpty())
    {
        return 1;
    }

    TStrongObjectPtr<UGridItemSystemAsset> Root(
        LoadObject<UGridItemSystemAsset>(nullptr, RegistryPath));
    TStrongObjectPtr<UGridObjectPaletteAsset> Palette(
        LoadObject<UGridObjectPaletteAsset>(nullptr, PalettePath));
    if (!IsValid(Root.Get()) || !IsValid(Palette.Get()) ||
        Root->ItemDefinitions.Num() != 34)
    {
        return 2;
    }

    const bool bRootWasDirty = Root->GetOutermost()->IsDirty();
    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
    TStrongObjectPtr<UGridItemSubsystem> Resolver(
        NewObject<UGridItemSubsystem>(GI.Get()));
    TArray<FString> Errors;
    if (!IsValid(Resolver.Get()) ||
        !Resolver->RegisterItemSystem(Root.Get(), Errors))
    {
        return 2;
    }
    TMap<FName, const UGridCanonicalItemDefinitionAsset*> Index;
    for (const UGridCanonicalItemDefinitionAsset* Canonical :
        Root->ItemDefinitions)
    {
        if (!IsValid(Canonical) || Canonical->ItemId.IsNone() ||
            Index.Contains(Canonical->ItemId)) { return 2; }
        Index.Add(Canonical->ItemId, Canonical);
    }
    if (Index.Num() != 34) { return 2; }

    FCounts Counts;
    TArray<TStrongObjectPtr<UGridLevelAsset>> Levels;
    TSet<FGuid> LooseGuids;
    for (const TCHAR* Path : LevelPaths)
    {
        TStrongObjectPtr<UGridLevelAsset> Level(
            LoadObject<UGridLevelAsset>(nullptr, Path));
        if (!IsValid(Level.Get())) { return 3; }
        for (FGridLooseItemInstance& Item : Level->LooseItemInstances)
        {
            if (!Item.InstanceId.IsValid() ||
                LooseGuids.Contains(Item.InstanceId) ||
                !CheckAndMaterialize(Item.CanonicalItemId,
                    Item.ItemDefinition.Get(), Index, Resolver.Get(),
                    Item.ItemLevel, Item.Quantity, bApply, false, Counts))
            {
                return 4;
            }
            LooseGuids.Add(Item.InstanceId);
            ++Counts.Loose;
        }
        for (FGridItemSpawnInstance& Spawn : Level->ItemSpawns)
        {
            if (!Spawn.SpawnId.IsValid() ||
                !CheckAndMaterialize(Spawn.CanonicalItemId,
                    Spawn.ItemDefinition.Get(), Index, Resolver.Get(),
                    Spawn.ItemLevel, Spawn.Quantity, bApply, false, Counts))
            {
                return 4;
            }
            ++Counts.Spawn;
        }
        for (FGridWorldObjectInstance& World :
            Level->WorldObjectInstances)
        {
            for (FGridReceptacleInitialItemConfig& Initial :
                World.InstanceConfig.ReceptacleInitialContent)
            {
                if (!CheckAndMaterialize(Initial.CanonicalItemId,
                    Initial.ItemDefinition.Get(), Index, Resolver.Get(),
                    Initial.ItemLevel, Initial.Quantity, bApply, false, Counts))
                {
                    return 4;
                }
                ++Counts.Receptacle;
            }
        }
        Levels.Emplace(MoveTemp(Level));
    }

    for (FGridObjectPaletteEntry& Entry : Palette->Entries)
    {
        if (!Entry.DefaultItemDefinition) { continue; }
        if (!CheckAndMaterialize(Entry.CanonicalItemId,
            Entry.DefaultItemDefinition.Get(), Index, Resolver.Get(),
            Entry.CanonicalDefaultItemLevel, 1, bApply, true, Counts))
        {
            return 5;
        }
        ++Counts.Palette;
    }

    // W's entire real scope, with a typed inheritance source for every item.
    // X.5 cannot change identities, archetypes, source pointers or counts.
    if (Counts.Loose != 44 || Counts.Spawn != 0 ||
        Counts.Receptacle != 18 || Counts.Palette != 33 ||
        Counts.Checked != 95 || Counts.Armament + Counts.NonArm != 95 ||
        Counts.InheritedWorldArmaments <= 0 ||
        (bApply && Counts.WorldLevelsChanged !=
            Counts.InheritedWorldArmaments) ||
        (bVerify && (Counts.WorldLevelsChanged != 0 ||
            Counts.PaletteLevelsChanged != 0)) ||
        Root->GetOutermost()->IsDirty() != bRootWasDirty)
    {
        return 6;
    }

    if (bApply)
    {
        for (TStrongObjectPtr<UGridLevelAsset>& Level : Levels)
        {
            if (!SaveNativePackage(Level.Get())) { return 7; }
        }
        if (!SaveNativePackage(Palette.Get())) { return 7; }
    }

    TSharedRef<FJsonObject> Report = MakeShared<FJsonObject>();
    Report->SetStringField(TEXT("schema"),
        TEXT("ARM-CLEAN06.4X5/real-authored-physical-levels/v1"));
    Report->SetBoolField(TEXT("assets_saved"), bApply);
    Report->SetBoolField(TEXT("verified_after_reopen"), bVerify);
    Report->SetNumberField(TEXT("levels"), Levels.Num());
    Report->SetNumberField(TEXT("loose"), Counts.Loose);
    Report->SetNumberField(TEXT("spawns"), Counts.Spawn);
    Report->SetNumberField(TEXT("receptacle"), Counts.Receptacle);
    Report->SetNumberField(TEXT("palette"), Counts.Palette);
    Report->SetNumberField(TEXT("total"), Counts.Checked);
    Report->SetNumberField(TEXT("armament"), Counts.Armament);
    Report->SetNumberField(TEXT("inherited_world_armament"),
        Counts.InheritedWorldArmaments);
    Report->SetNumberField(TEXT("nonarm"), Counts.NonArm);
    Report->SetNumberField(TEXT("world_levels_written"),
        Counts.WorldLevelsChanged);
    Report->SetNumberField(TEXT("palette_levels_written"),
        Counts.PaletteLevelsChanged);
    Report->SetBoolField(TEXT("legacy_source_pointers_retained"), true);
    Report->SetBoolField(TEXT("savegames_opened_or_written"), false);

    FString Json;
    if (!FJsonSerializer::Serialize(Report,
        TJsonWriterFactory<>::Create(&Json)) ||
        !FFileHelper::SaveStringToFile(Json, *Output,
            FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
    {
        return 8;
    }
    UE_LOG(LogTemp, Display,
        TEXT("[ARM-CLEAN06.4X5] %s 95 real authored references, "
             "ARM levels %d, nonarm %d, %d physical packages; no .sav."),
        bApply ? TEXT("materialized") : TEXT("verified"),
        Counts.Armament, Counts.NonArm, 5);
    return 0;
}
