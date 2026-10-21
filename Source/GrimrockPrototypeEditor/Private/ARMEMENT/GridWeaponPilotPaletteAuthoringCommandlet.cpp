#include "ARMEMENT/GridWeaponPilotPaletteAuthoringCommandlet.h"

#include "Core/GridObjectPaletteAsset.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "UObject/SavePackage.h"
#include "UObject/UObjectGlobals.h"

namespace ARMEMENT0171
{
    static constexpr const TCHAR* PalettePath =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/DA_ObjectPalette_Default.DA_ObjectPalette_Default");
    static constexpr const TCHAR* WeaponFolder =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons");

    struct FPilot
    {
        const TCHAR* EntryId;
        const TCHAR* AssetName;
        const TCHAR* ItemId;
    };

    static const FPilot Pilots[] =
    {
        {TEXT("Weapon_Dagger_01"), TEXT("DA_Weapon_Dagger_01"), TEXT("Weapon_Dagger_01")},
        {TEXT("Weapon_ShortBow_01"), TEXT("DA_Weapon_ShortBow_01"), TEXT("Weapon_ShortBow_01")},
        {TEXT("Weapon_RoundShield_01"), TEXT("DA_Weapon_RoundShield_01"), TEXT("Weapon_RoundShield_01")}
    };

    static bool Matches(const FGridObjectPaletteEntry& Entry, const FPilot& Pilot,
        const UGridItemDefinitionAsset* Item)
    {
        return Entry.EntryId == FName(Pilot.EntryId)
            && Entry.PaletteCategory == FName(TEXT("Items"))
            && Entry.DefaultItemDefinition.Get() == Item
            && Entry.DefaultWorldObjectDefinition == nullptr
            && Entry.Icon == nullptr
            && Entry.IsValidEntry();
    }
}

UGridWeaponPilotPaletteAuthoringCommandlet::UGridWeaponPilotPaletteAuthoringCommandlet()
{
    IsEditor = true;
    IsClient = false;
    IsServer = false;
    LogToConsole = true;
}

int32 UGridWeaponPilotPaletteAuthoringCommandlet::Main(const FString& Params)
{
    const bool bApply = FParse::Param(*Params, TEXT("Apply"));
    // Explicit opt-in: no production asset repair in the ordinary authoring mode.
    const bool bRepairIcons = FParse::Param(*Params, TEXT("RepairIcons"));
    UGridObjectPaletteAsset* Palette = LoadObject<UGridObjectPaletteAsset>(nullptr, ARMEMENT0171::PalettePath);
    if (!IsValid(Palette))
    {
        UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.7.1] Production palette cannot load"));
        return 1;
    }

    // Preflight all three source assets before touching the production palette.
    UGridItemDefinitionAsset* Items[UE_ARRAY_COUNT(ARMEMENT0171::Pilots)] = {};
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(ARMEMENT0171::Pilots); ++Index)
    {
        const ARMEMENT0171::FPilot& Pilot = ARMEMENT0171::Pilots[Index];
        const FString Path = FString::Printf(TEXT("%s/%s.%s"),
            ARMEMENT0171::WeaponFolder, Pilot.AssetName, Pilot.AssetName);
        Items[Index] = LoadObject<UGridItemDefinitionAsset>(nullptr, *Path);
        if (!IsValid(Items[Index]) || !Items[Index]->IsValidDefinition() ||
            Items[Index]->ItemDefinitionId != FName(Pilot.ItemId))
        {
            UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.7.1] Missing/invalid item: %s"), *Path);
            return 1;
        }
    }

    // Make a complete candidate copy: no in-memory mutation of the production asset in dry-run.
    TArray<FGridObjectPaletteEntry> Proposed = Palette->Entries;
    int32 Added = 0;
    int32 Repaired = 0;
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(ARMEMENT0171::Pilots); ++Index)
    {
        const ARMEMENT0171::FPilot& Pilot = ARMEMENT0171::Pilots[Index];
        const FName EntryId(Pilot.EntryId);
        FGridObjectPaletteEntry* Existing = Proposed.FindByPredicate(
            [EntryId](const FGridObjectPaletteEntry& Entry) { return Entry.EntryId == EntryId; });
        if (Existing)
        {
            // Repair only the three known pilot direct collectibles. Preserve
            // their item, category, identity and every unrelated palette entry.
            if (bRepairIcons && Existing->Icon &&
                Existing->PaletteCategory == FName(TEXT("Items")) &&
                Existing->DefaultItemDefinition.Get() == Items[Index] &&
                Existing->DefaultWorldObjectDefinition == nullptr)
            {
                Existing->Icon = nullptr;
                ++Repaired;
                UE_LOG(LogTemp, Display, TEXT("[ARMEMENT01.7.1] READY ICON REPAIR %s"), Pilot.EntryId);
            }
            if (!ARMEMENT0171::Matches(*Existing, Pilot, Items[Index]))
            {
                UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.7.1] Conflicting entry; no overwrite: %s"), Pilot.EntryId);
                return 1;
            }
            UE_LOG(LogTemp, Display, TEXT("[ARMEMENT01.7.1] ALREADY PRESENT %s"), Pilot.EntryId);
            continue;
        }
        FGridObjectPaletteEntry& Entry = Proposed.AddDefaulted_GetRef();
        Entry.EntryId = EntryId;
        Entry.PaletteCategory = FName(TEXT("Items"));
        Entry.DefaultItemDefinition = Items[Index];
        // Direct collectible contract: no icon duplication and no WorldObjectDefinition.
        if (!ARMEMENT0171::Matches(Entry, Pilot, Items[Index]))
        {
            UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.7.1] Invalid candidate: %s"), Pilot.EntryId);
            return 1;
        }
        ++Added;
        UE_LOG(LogTemp, Display, TEXT("[ARMEMENT01.7.1] READY %s -> %s"), Pilot.EntryId, Pilot.AssetName);
    }

    UGridObjectPaletteAsset* Candidate = NewObject<UGridObjectPaletteAsset>(GetTransientPackage());
    Candidate->Entries = Proposed;
    TArray<FGridWorldObjectDefinitionValidationMessage> Errors;
    if (!Candidate->ValidatePalette(Errors))
    {
        UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.7.1] Candidate palette invalid (%d messages); no writes"), Errors.Num());
        for (const FGridWorldObjectDefinitionValidationMessage& Message : Errors)
        {
            UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.7.1] %s"), *Message.Message);
        }
        return 1;
    }
    if (!bApply)
    {
        UE_LOG(LogTemp, Display,
            TEXT("[ARMEMENT01.7.1] DRY RUN OK: +%d entries, %d icon repairs, zero writes"), Added, Repaired);
        return 0;
    }
    if (Added == 0 && Repaired == 0)
    {
        UE_LOG(LogTemp, Display, TEXT("[ARMEMENT01.7.1] Already complete: no writes"));
        return 0;
    }

    UPackage* Package = Palette->GetOutermost();
    if (!IsValid(Package))
    {
        UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.7.1] Production package unavailable"));
        return 1;
    }
    Palette->Modify();
    Palette->Entries = MoveTemp(Proposed);
    Palette->MarkPackageDirty();

    const FString Filename = FPackageName::LongPackageNameToFilename(
        Package->GetName(), FPackageName::GetAssetPackageExtension());
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    SaveArgs.SaveFlags = SAVE_NoError;
    if (!UPackage::SavePackage(Package, Palette, *Filename, SaveArgs))
    {
        UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.7.1] Save failed: %s"), *Filename);
        return 1;
    }
    UE_LOG(LogTemp, Display, TEXT("[ARMEMENT01.7.1] SAVED palette: +%d entries, %d icon repairs; other entries preserved"), Added, Repaired);
    return 0;
}
