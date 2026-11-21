#pragma once

#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"

/** Read-only identity, fixed level and static metadata for contained items. */
struct FGridReceptacleItemMetadataReadView
{
    bool bCanonicalIdentity = false;
    bool bResolved = false;
    int32 FixedItemLevel = 0;
    float UnitWeightKg = 0.0f;
    FText DisplayName;
};

struct FGridReceptacleItemMetadataReadModel
{
    static FGridReceptacleItemMetadataReadView Resolve(
        const UGridItemSubsystem* Registry,
        const UGridItemDefinitionAsset* LegacyDefinition,
        FName ItemId, int32 ItemLevel, bool /*bAllowAuthoredDefault*/)
    {
        FGridReceptacleItemMetadataReadView View;
        View.bCanonicalIdentity = Registry && Registry->HasCanonicalItem(ItemId);
        if (ItemId.IsNone() || ItemLevel < 0 || ItemLevel > 20 ||
            (!View.bCanonicalIdentity && IsValid(LegacyDefinition) && LegacyDefinition->ItemDefinitionId != ItemId))
        {
            return View;
        }

        if (View.bCanonicalIdentity)
        {
            FGridResolvedItem Canonical;
            if (!Registry->ResolveItem(ItemId, ItemLevel, Canonical) ||
                (!Canonical.bIsArmament && ItemLevel != 0) ||
                !FMath::IsFinite(Canonical.WeightKg) || Canonical.WeightKg < 0.0f)
            {
                return View; // All registered placements now carry their physical level.
            }
            View.bResolved = true;
            View.FixedItemLevel = ItemLevel;
            View.UnitWeightKg = Canonical.WeightKg;
            View.DisplayName = Canonical.DisplayName.IsEmpty()
                ? FText::FromName(ItemId) : Canonical.DisplayName;
            return View;
        }

        // The only remaining compatibility path is for the isolated
        // registry-less Automation fixtures that exercise old level data.
#if WITH_DEV_AUTOMATION_TESTS
        if (Registry || !GIsAutomationTesting) return View;
        View.FixedItemLevel = IsValid(LegacyDefinition) &&
            !LegacyDefinition->bUseWeaponCatalogue ? 0 : ItemLevel;
        if (IsValid(LegacyDefinition))
        {
            const FGridWeaponLevelProfile* Profile = nullptr;
            if (LegacyDefinition->bUseWeaponCatalogue)
            {
                View.FixedItemLevel = ItemLevel == 0
                    ? LegacyDefinition->DefaultWeaponItemLevel : ItemLevel;
                if (!LegacyDefinition->TryGetWeaponCatalogueProfile(
                    View.FixedItemLevel, Profile))
                {
                    return View;
                }
            }
            View.UnitWeightKg = Profile ? Profile->WeightKg : LegacyDefinition->Weight;
            if (!FMath::IsFinite(View.UnitWeightKg) || View.UnitWeightKg < 0.0f)
                return FGridReceptacleItemMetadataReadView();
            View.DisplayName = LegacyDefinition->DisplayName;
        }
        View.bResolved = true;
#endif // WITH_DEV_AUTOMATION_TESTS
        return View;
    }
};
