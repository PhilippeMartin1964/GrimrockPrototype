#pragma once

#include "Runtime/Items/GridItemSubsystem.h"

/** Read-only identity, fixed level and static metadata for contained items. */
struct FGridReceptacleItemMetadataReadView
{
    bool bRegisteredIdentity = false;
    bool bResolved = false;
    int32 FixedItemLevel = 0;
    float UnitWeightKg = 0.0f;
    FText DisplayName;
};

struct FGridReceptacleItemMetadataReadModel
{
    static FGridReceptacleItemMetadataReadView Resolve(
        const UGridItemSubsystem* Registry,
        FName ItemId, int32 ItemLevel)
    {
        FGridReceptacleItemMetadataReadView View;
        View.bRegisteredIdentity = Registry && Registry->ContainsItem(ItemId);
        if (ItemId.IsNone() || ItemLevel < 0 || ItemLevel > 20)
        {
            return View;
        }

        if (View.bRegisteredIdentity)
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

        return View;
    }
};
