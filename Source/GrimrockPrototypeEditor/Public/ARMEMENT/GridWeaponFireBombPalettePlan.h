#pragma once

#include "Core/GridObjectPaletteAsset.h"
#include "Runtime/GridItemDefinitionAsset.h"

// ARMEMENT01.10.2.3: editor-only presentation categories. Gameplay ItemType and
// persisted palette entry identities are intentionally unchanged.
namespace ARMEMENT011023
{
    struct FSpec
    {
        const TCHAR* EntryId;
        const TCHAR* ObjectPath;
        const TCHAR* ItemDefinitionId;
        const TCHAR* Category;
    };

    static const FSpec Specs[] =
    {
        { TEXT("Weapon_Dagger_01"),
          TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_Dagger_01.DA_Weapon_Dagger_01"),
          TEXT("Weapon_Dagger_01"), TEXT("Weapon") },
        { TEXT("Weapon_ShortBow_01"),
          TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_ShortBow_01.DA_Weapon_ShortBow_01"),
          TEXT("Weapon_ShortBow_01"), TEXT("Weapon") },
        { TEXT("Weapon_RoundShield_01"),
          TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_RoundShield_01.DA_Weapon_RoundShield_01"),
          TEXT("Weapon_RoundShield_01"), TEXT("Weapon") },
        { TEXT("Weapon_StraightSword_01"),
          TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_StraightSword_01.DA_Weapon_StraightSword_01"),
          TEXT("Weapon_StraightSword_01"), TEXT("Weapon") },
        { TEXT("Item_Bomb_Fire"),
          TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Bomb_Fire.DA_Item_Bomb_Fire"),
          TEXT("Item_Bomb_Fire"), TEXT("Items") }
    };

    struct FResolved
    {
        const FSpec* Spec = nullptr;
        UGridItemDefinitionAsset* Item = nullptr;
    };

    // Pure proposal: no package dirtying, no mutation of Original and no saves.
    // The caller must separately run UGridObjectPaletteAsset::ValidatePalette
    // on the complete proposed entries before saving the production package.
    inline bool Propose(
        const TArray<FGridObjectPaletteEntry>& Original,
        const TArray<FResolved>& Targets,
        TArray<FGridObjectPaletteEntry>& OutEntries,
        int32& OutAdded,
        int32& OutMoved,
        FString& OutError)
    {
        OutError.Reset();
        OutAdded = 0;
        OutMoved = 0;
        OutEntries = Original;

        TSet<FName> RequestedIds;
        for (const FResolved& Target : Targets)
        {
            if (!Target.Spec || !IsValid(Target.Item) || !Target.Item->IsValidDefinition())
            {
                OutError = TEXT("Missing or invalid real ItemDefinition");
                return false;
            }
            const FName Id(Target.Spec->EntryId);
            const FName DesiredCategory(Target.Spec->Category);
            if (Id.IsNone() || DesiredCategory.IsNone() ||
                Target.Item->ItemDefinitionId != FName(Target.Spec->ItemDefinitionId) ||
                RequestedIds.Contains(Id))
            {
                OutError = FString::Printf(TEXT("Invalid/duplicate requested identity: %s"), Target.Spec->EntryId);
                return false;
            }
            RequestedIds.Add(Id);

            FGridObjectPaletteEntry* Existing = nullptr;
            int32 Count = 0;
            for (FGridObjectPaletteEntry& Entry : OutEntries)
            {
                if (Entry.EntryId == Id)
                {
                    Existing = &Entry;
                    ++Count;
                }
                // Refuse a second palette identity pointing at the same item.
                // Never remove or silently rewrite manually authored entries.
                else if (Entry.DefaultItemDefinition &&
                    Entry.DefaultItemDefinition->ItemDefinitionId == Target.Item->ItemDefinitionId)
                {
                    OutError = FString::Printf(TEXT("Item already exposed by another EntryId: %s"), *Entry.EntryId.ToString());
                    return false;
                }
            }
            if (Count > 1)
            {
                OutError = FString::Printf(TEXT("Duplicate existing EntryId: %s"), Target.Spec->EntryId);
                return false;
            }
            if (!Existing)
            {
                FGridObjectPaletteEntry& NewEntry = OutEntries.AddDefaulted_GetRef();
                NewEntry.EntryId = Id;
                NewEntry.PaletteCategory = DesiredCategory;
                NewEntry.DefaultItemDefinition = Target.Item;
                ++OutAdded;
                continue;
            }
            if (Existing->DefaultItemDefinition.Get() != Target.Item ||
                Existing->DefaultWorldObjectDefinition != nullptr ||
                Existing->Icon != nullptr ||
                Existing->DefaultMonsterDefinition != nullptr ||
                Existing->DefaultStoryCompanionDefinition != nullptr ||
                !Existing->IsValidEntry())
            {
                OutError = FString::Printf(TEXT("Conflicting direct collectible entry: %s"), Target.Spec->EntryId);
                return false;
            }

            if (Existing->PaletteCategory == DesiredCategory)
            {
                continue;
            }
            // Only the previous pilot grouping is allowed as a migration
            // source. An arbitrary manually chosen category is never overwritten.
            if (DesiredCategory == FName(TEXT("Weapon")) &&
                Existing->PaletteCategory == FName(TEXT("Items")))
            {
                Existing->PaletteCategory = DesiredCategory;
                ++OutMoved;
                continue;
            }
            OutError = FString::Printf(TEXT("Unexpected authored category for %s: %s"),
                Target.Spec->EntryId, *Existing->PaletteCategory.ToString());
            return false;
        }
        return true;
    }
}
