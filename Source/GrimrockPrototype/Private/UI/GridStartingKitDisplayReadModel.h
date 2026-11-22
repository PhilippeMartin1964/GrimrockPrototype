#pragma once

#include "../RPG/GridStartingKitStaticMetadataReadModel.h"
#include "Runtime/GridItemDefinitionAsset.h"

/**
 * Read-only frontend projection of a starting equipment entry. Registered
 * entries require an explicit canonical ID and physical N1–N20 / N0.
 * No ItemInstance, UPackage or mutable starting kit is touched.
 */
struct FGridStartingKitDisplayReadView
{
    bool bRegisteredIdentity = false;
    bool bResolved = false;
    int32 FixedItemLevel = 0;
    FText DisplayName;
};

struct FGridStartingKitDisplayReadModel
{
    static FGridStartingKitDisplayReadView Resolve(
        const UGridItemSubsystem* Registry,
        const UGridItemDefinitionAsset* LegacyDefinition,
        int32 AuthoredItemLevel, FName ExplicitItemId = NAME_None)
    {
        FGridStartingKitDisplayReadView View;
        if (!ExplicitItemId.IsNone())
        {
            // The explicit item ID is an assertion of canonical authority,
            // not a request to infer an ARM default or load an old asset.
            View.bRegisteredIdentity = true;
            FGridResolvedItem Metadata;
            if (!Registry || !Registry->ContainsItem(ExplicitItemId) ||
                (IsValid(LegacyDefinition) && LegacyDefinition->ItemDefinitionId != ExplicitItemId) ||
                !Registry->ResolveItem(ExplicitItemId, AuthoredItemLevel, Metadata) ||
                !Metadata.IsValid() || (!Metadata.bIsArmament && AuthoredItemLevel != 0))
                return View;
            View.bResolved = true;
            View.FixedItemLevel = AuthoredItemLevel;
            View.DisplayName = Metadata.DisplayName;
            return View;
        }
        if (!IsValid(LegacyDefinition)) return View;
        const FName ItemId = LegacyDefinition->ItemDefinitionId;
        View.bRegisteredIdentity = Registry && Registry->ContainsItem(ItemId);
        if (ItemId.IsNone() || AuthoredItemLevel < 0 || AuthoredItemLevel > 20)
        {
            return View;
        }

        // Match the 5G.4 kit builder: an old pointer alone cannot author
        // or preview a registered starting item, even with a valid level.
        if (Registry) return View; // No unknown item may borrow a legacy preview in a configured world.
#if WITH_DEV_AUTOMATION_TESTS
        if (!GIsAutomationTesting) return View;
        const bool bArmament = LegacyDefinition->bUseWeaponCatalogue;
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
#endif
        return View;
    }
};
