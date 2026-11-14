#pragma once

#include "../RPG/GridStartingKitStaticMetadataReadModel.h"
#include "Runtime/GridItemDefinitionAsset.h"

/**
 * Read-only frontend projection of a starting equipment entry. The authored
 * level 0 is a default-level REQUEST for ARM, but a real level 0 for ammo.
 * No ItemInstance, UPackage or mutable starting kit is touched.
 */
struct FGridStartingKitDisplayReadView
{
    bool bCanonicalIdentity = false;
    bool bResolved = false;
    int32 FixedItemLevel = 0;
    FText DisplayName;
};

struct FGridStartingKitDisplayReadModel
{
    static FGridStartingKitDisplayReadView Resolve(
        const UGridItemSubsystem* Registry,
        const UGridItemDefinitionAsset* LegacyDefinition,
        int32 AuthoredItemLevel)
    {
        FGridStartingKitDisplayReadView View;
        if (!IsValid(LegacyDefinition)) return View;
        const FName ItemId = LegacyDefinition->ItemDefinitionId;
        View.bCanonicalIdentity = Registry && Registry->HasCanonicalItem(ItemId);
        if (ItemId.IsNone() || AuthoredItemLevel < 0 || AuthoredItemLevel > 20)
        {
            return View;
        }

        // The canonical type, not an obsolete bUseWeaponCatalogue copy, decides
        // whether a persisted N0 denotes an ARM default or unlevelled arrows.
        bool bArmament = LegacyDefinition->bUseWeaponCatalogue;
        if (View.bCanonicalIdentity)
        {
            FGridResolvedItem Probe;
            if (!Registry->ResolveItem(ItemId, 1, Probe))
            {
                return View; // Registered but invalid: never legacy fallback.
            }
            bArmament = Probe.bIsArmament;
        }
        const int32 FixedLevel = bArmament && AuthoredItemLevel == 0
            ? LegacyDefinition->DefaultWeaponItemLevel : AuthoredItemLevel;
        if ((bArmament && (FixedLevel < 1 || FixedLevel > 20)) ||
            (!bArmament && FixedLevel != 0))
        {
            return View;
        }

        const FGridStartingKitStaticMetadataReadView Metadata =
            FGridStartingKitStaticMetadataReadModel::Resolve(
                Registry, ItemId, FixedLevel,
                LegacyDefinition->DisplayName, LegacyDefinition->Weight);
        if (!Metadata.bResolved) return View;
        View.bResolved = true;
        View.FixedItemLevel = FixedLevel;
        View.DisplayName = Metadata.DisplayName;
        return View;
    }
};
