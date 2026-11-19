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

        // Unknown identities preserve the old receptacle paths during 06.3.
        // Without any old definition an ID-only item retains its fixed level.
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
            View.DisplayName = LegacyDefinition->DisplayName;
        }
        View.bResolved = true;
        return View;
    }
};
