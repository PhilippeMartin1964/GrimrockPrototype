#pragma once

#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"

/** ARM-CLEAN06.3AU: transient inventory type projection; no persisted data. */
struct FGridCanonicalInventoryItemTypeReadView
{
    bool bCanonicalIdentity = false;
    bool bResolved = false;
    EGridItemType ItemType = EGridItemType::None;
};

struct FGridCanonicalInventoryItemTypeReadModel
{
    static FGridCanonicalInventoryItemTypeReadView Resolve(
        const UGridItemSubsystem* Registry, const UGridItemDefinitionAsset* Legacy,
        const FGridItemInstance& Item)
    {
        FGridCanonicalInventoryItemTypeReadView View;
        if (Item.ItemDefinitionId.IsNone()) return View;

        if (Registry && Registry->HasCanonicalItem(Item.ItemDefinitionId))
        {
            // Recognize membership before IsValid(): that method rejects
            // N21 itself, but an invalid known ID must NEVER borrow a
            // legacy category, even in diagnostic/tooltip projections.
            View.bCanonicalIdentity = true;
            if (!Item.IsValid()) return View;
            FGridResolvedItem Resolved;
            if (!Registry->ResolveItem(Item.ItemDefinitionId, Item.ItemLevel, Resolved))
            {
                return View; // Never read an old ItemType for an invalid ARM N0.
            }
            View.bResolved = true;
            if (Resolved.bIsArmament)
            {
                // Passive shields have their own category. Focus and
                // offensive weapons occupy the existing Weapon equipment type.
                View.ItemType = Resolved.Weapon.Kind == EGridCanonicalArmamentKind::Shield
                    ? EGridItemType::Shield : EGridItemType::Weapon;
            }
            else
            {
                // EGridItemType has no Ammo; canonical N0 arrows and other
                // non-ARM identities conservatively belong to Misc until
                // an explicitly authored non-ARM UI taxonomy exists.
                View.ItemType = EGridItemType::Misc;
            }
            return View;
        }

        if (!Item.IsValid()) return View;

        // Transitional lookup only for an ID absent from the sole registry.
        if (IsValid(Legacy) && Legacy->ItemDefinitionId == Item.ItemDefinitionId)
        {
            View.bResolved = true;
            View.ItemType = Legacy->ItemType;
        }
        return View;
    }
};
