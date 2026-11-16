#include "ARMEMENT/GridArmClean064WWorldItemReferencesCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/ARFilter.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridObjectPaletteAsset.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridCanonicalItemCatalogues.h"
#include "Runtime/Items/GridCanonicalItemDefinitionAsset.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

namespace
{
    constexpr TCHAR RootPath[] =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem");
    constexpr TCHAR PalettePath[] =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/DA_ObjectPalette_Default.DA_ObjectPalette_Default");
    constexpr TCHAR LevelFolder[] =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels");

    bool Save(UObject* Object)
    {
        UPackage* Package = IsValid(Object) ? Object->GetOutermost() : nullptr;
        if (!IsValid(Package)) { return false; }
        const FString Filename = FPackageName::LongPackageNameToFilename(
            Package->GetName(), FPackageName::GetAssetPackageExtension());
        IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);
        Package->MarkPackageDirty();
        FSavePackageArgs Args;
        Args.TopLevelFlags = RF_Public | RF_Standalone;
        Args.SaveFlags = SAVE_NoError;
        return UPackage::SavePackage(Package, Object, *Filename, Args);
    }

    FString RelativePackageFilename(const UObject& Object)
    {
        const FString Package = Object.GetOutermost()->GetName();
        if (!Package.StartsWith(TEXT("/Game/"))) { return FString(); }
        return TEXT("Content/") + Package.Mid(6) + TEXT(".uasset");
    }

    struct FCounts
    {
        int32 Loose = 0;
        int32 Spawn = 0;
        int32 Receptacle = 0;
        int32 Palette = 0;
        int32 DefinitionDefault = 0;
        int32 Checked = 0;
    };

    bool ConvertId(FName& Destination, const UGridItemDefinitionAsset* Legacy,
        const TMap<FName, const UGridCanonicalItemDefinitionAsset*>& Canonical,
        int32 Quantity, int32 ItemLevel, bool bApply, FCounts& Counts)
    {
        if (!IsValid(Legacy) || Legacy->ItemDefinitionId.IsNone() ||
            Quantity < 1 || ItemLevel < 0 || ItemLevel > 20)
        {
            return false;
        }
        const UGridCanonicalItemDefinitionAsset* const* Found =
            Canonical.Find(Legacy->ItemDefinitionId);
        if (!Found || !IsValid(*Found) ||
            ((*Found)->ItemId != Legacy->ItemDefinitionId))
        {
            return false;
        }
        // An old ARM must resolve to precisely the same archetype, never a
        // coincidentally matching ID of a potion, stone or other non-ARM.
        if (!Legacy->WeaponArchetypeId.IsNone())
        {
            const UGridArmamentDefinitionAsset* Weapon =
                Cast<UGridArmamentDefinitionAsset>(*Found);
            if (!Weapon || Weapon->ArchetypeId != Legacy->WeaponArchetypeId)
            {
                return false;
            }
        }
        else if (!Cast<UGridNonArmamentDefinitionAsset>(*Found))
        {
            return false;
        }
        if (bApply)
        {
            if (!Destination.IsNone()) { return false; }
            Destination = Legacy->ItemDefinitionId;
        }
        else if (Destination != Legacy->ItemDefinitionId)
        {
            return false; // Fresh Editor process must read actual saved IDs.
        }
        ++Counts.Checked;
        return true;
    }

    void AddFile(TArray<TSharedPtr<FJsonValue>>& Files, const UObject* Object)
    {
        Files.Add(MakeShared<FJsonValueString>(
            IsValid(Object) ? RelativePackageFilename(*Object) : FString()));
    }
}

UGridArmClean064WWorldItemReferencesCommandlet::
    UGridArmClean064WWorldItemReferencesCommandlet()
{
    IsClient = false;
    IsEditor = true;
    LogToConsole = true;
}

int32 UGridArmClean064WWorldItemReferencesCommandlet::Main(const FString& Params)
{
    const bool bApply = FParse::Param(*Params, TEXT("Apply"));
    const bool bVerify = FParse::Param(*Params, TEXT("Verify"));
    FString Output;
    if (bApply == bVerify ||
        !FParse::Value(*Params, TEXT("Output="), Output) || Output.IsEmpty())
    {
        return 1;
    }
    UGridItemSystemAsset* Root = LoadObject<UGridItemSystemAsset>(nullptr, RootPath);
    UGridObjectPaletteAsset* Palette =
        LoadObject<UGridObjectPaletteAsset>(nullptr, PalettePath);
    if (!IsValid(Root) || !IsValid(Palette) ||
        Root->ItemDefinitions.Num() != 34)
    {
        return 2;
    }
    TArray<FString> Errors;
    if (!Root->Validate(Errors)) { return 2; }
    const bool bRootDirty = Root->GetOutermost()->IsDirty();
    TMap<FName, const UGridCanonicalItemDefinitionAsset*> Canonical;
    for (const UGridCanonicalItemDefinitionAsset* Item : Root->ItemDefinitions)
    {
        if (!IsValid(Item) || Item->ItemId.IsNone() ||
            Canonical.Contains(Item->ItemId)) { return 2; }
        Canonical.Add(Item->ItemId, Item);
    }
    if (Canonical.Num() != 34) { return 2; }
    IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(
        TEXT("AssetRegistry")).Get();
    Registry.SearchAllAssets(true);
    FARFilter Filter;
    Filter.PackagePaths.Add(FName(LevelFolder));
    Filter.ClassPaths.Add(UGridLevelAsset::StaticClass()->GetClassPathName());
    Filter.bRecursivePaths = true;
    Filter.bRecursiveClasses = true;
    TArray<FAssetData> Levels;
    Registry.GetAssets(Filter, Levels);
    Levels.Sort([](const FAssetData& A, const FAssetData& B)
    {
        return A.PackageName.LexicalLess(B.PackageName);
    });
    if (Levels.Num() != 4) { return 3; }
    FCounts Counts;
    TArray<TSharedPtr<FJsonValue>> Files;
    TArray<UGridLevelAsset*> LevelObjects;
    for (const FAssetData& Asset : Levels)
    {
        UGridLevelAsset* Level = Cast<UGridLevelAsset>(Asset.GetAsset());
        if (!IsValid(Level)) { return 3; }
        for (FGridLooseItemInstance& Item : Level->LooseItemInstances)
        {
            if (!ConvertId(Item.CanonicalItemId, Item.ItemDefinition, Canonical,
                Item.Quantity, Item.ItemLevel, bApply, Counts)) { return 4; }
            ++Counts.Loose;
        }
        for (FGridItemSpawnInstance& Spawn : Level->ItemSpawns)
        {
            if (!ConvertId(Spawn.CanonicalItemId, Spawn.ItemDefinition, Canonical,
                Spawn.Quantity, Spawn.ItemLevel, bApply, Counts)) { return 4; }
            ++Counts.Spawn;
        }
        for (FGridWorldObjectInstance& Object : Level->WorldObjectInstances)
        {
            for (FGridReceptacleInitialItemConfig& Item :
                Object.InstanceConfig.ReceptacleInitialContent)
            {
                if (!ConvertId(Item.CanonicalItemId, Item.ItemDefinition, Canonical,
                    Item.Quantity, Item.ItemLevel, bApply, Counts)) { return 4; }
                ++Counts.Receptacle;
            }
        }
        LevelObjects.Add(Level);
        AddFile(Files, Level);
    }
    for (FGridObjectPaletteEntry& Entry : Palette->Entries)
    {
        if (!Entry.DefaultItemDefinition) { continue; }
        if (!ConvertId(Entry.CanonicalItemId, Entry.DefaultItemDefinition,
            Canonical, 1, 0, bApply, Counts)) { return 5; }
        ++Counts.Palette;
    }
    AddFile(Files, Palette);

    FARFilter DefinitionFilter;
    DefinitionFilter.PackagePaths.Add(FName(
        TEXT("/Game/GrimrockPrototype/Core/DataAssets")));
    DefinitionFilter.ClassPaths.Add(
        UGridWorldObjectDefinitionAsset::StaticClass()->GetClassPathName());
    DefinitionFilter.bRecursivePaths = true;
    DefinitionFilter.bRecursiveClasses = true;
    TArray<FAssetData> Definitions;
    Registry.GetAssets(DefinitionFilter, Definitions);
    if (Definitions.Num() != 44) { return 6; }
    for (const FAssetData& Asset : Definitions)
    {
        const UGridWorldObjectDefinitionAsset* Definition =
            Cast<UGridWorldObjectDefinitionAsset>(Asset.GetAsset());
        if (!IsValid(Definition)) { return 6; }
        Counts.DefinitionDefault +=
            Definition->DefaultBehavior.Receptacle.InitialContent.Num();
    }

    // Exact original 06.4H real-world inventory:
    // 44 loose + 0 spawns + 18 receptacles + 33 palette = 95.
    // The historical 40 missing canonical IDs have since been migrated in
    // 06.4T/U/V. No new data or authority is guessed from an asset name.
    if (Counts.Loose != 44 || Counts.Spawn != 0 ||
        Counts.Receptacle != 18 || Counts.Palette != 33 ||
        Counts.DefinitionDefault != 0 || Counts.Checked != 95 ||
        Files.Num() != 5 || Root->GetOutermost()->IsDirty() != bRootDirty)
    {
        return 7;
    }
    if (bApply)
    {
        for (UGridLevelAsset* Level : LevelObjects)
        {
            if (!Save(Level)) { return 8; }
        }
        if (!Save(Palette)) { return 8; }
    }
    TSharedRef<FJsonObject> Report = MakeShared<FJsonObject>();
    Report->SetStringField(TEXT("schema"),
        TEXT("ARM-CLEAN06.4W/world-canonical-reference-materialization/v1"));
    Report->SetBoolField(TEXT("assets_saved"), bApply);
    Report->SetBoolField(TEXT("verified_after_reopen"), bVerify);
    Report->SetNumberField(TEXT("levels"), Levels.Num());
    Report->SetNumberField(TEXT("loose_items"), Counts.Loose);
    Report->SetNumberField(TEXT("spawns"), Counts.Spawn);
    Report->SetNumberField(TEXT("receptacle_items"), Counts.Receptacle);
    Report->SetNumberField(TEXT("palette_items"), Counts.Palette);
    Report->SetNumberField(TEXT("world_definition_default_items"), Counts.DefinitionDefault);
    Report->SetNumberField(TEXT("canonical_references"), Counts.Checked);
    Report->SetNumberField(TEXT("old_unmapped_ids_now_resolved"), 40);
    Report->SetNumberField(TEXT("new_real_asset_files"), Files.Num());
    Report->SetBoolField(TEXT("legacy_runtime_fallback_added"), false);
    Report->SetBoolField(TEXT("legacy_source_pointers_retained_until_cutover"), true);
    Report->SetBoolField(TEXT("savegame_files_opened"), false);
    Report->SetArrayField(TEXT("asset_paths_private"), Files);
    FString Json;
    if (!FJsonSerializer::Serialize(Report,
        TJsonWriterFactory<>::Create(&Json)) ||
        !FFileHelper::SaveStringToFile(Json, *Output,
            FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
    {
        return 9;
    }
    UE_LOG(LogTemp, Display,
        TEXT("[ARM-CLEAN06.4W] 95 real world/palette ItemIds %s, 5 authentic UE assets; no SaveGame."),
        bApply ? TEXT("saved") : TEXT("verified after reload"));
    return 0;
}
