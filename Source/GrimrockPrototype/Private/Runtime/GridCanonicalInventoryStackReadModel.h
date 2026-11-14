#pragma once

#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"

/**
 * Transient source of stack capacity for pickup, merge, split and cursor
 * transfers. Once ItemId belongs to DA_ItemSystem, MaxStack is authoritative,
 * even when an obsolete DA_Item* contradicts it or does not exist.
 */
struct FGridCanonicalInventoryStackReadView
{
    bool bCanonicalIdentity = false;
    bool bResolved = false;
    int32 MaxStackSize = 1;

    bool IsStackable() const { return bResolved && MaxStackSize > 1; }
};

struct FGridCanonicalInventoryStackReadModel
{
    static FGridCanonicalInventoryStackReadView Resolve(
        const UGridItemSubsystem* Registry, const UGridItemDefinitionAsset* Legacy,
        const FGridItemInstance& Item)
    {
        FGridCanonicalInventoryStackReadView View;
        if (Item.ItemDefinitionId.IsNone() || Item.ItemLevel < 0 || Item.ItemLevel > 20)
        {
            return View;
        }
        if (Registry && Registry->HasCanonicalItem(Item.ItemDefinitionId))
        {
            View.bCanonicalIdentity = true;
            FGridResolvedItem Resolved;
            if (!Registry->ResolveItem(Item.ItemDefinitionId, Item.ItemLevel, Resolved))
            {
                return View; // Registered invalid level must never use old MaxStackSize.
            }
            View.bResolved = true;
            View.MaxStackSize = FMath::Max(1, Resolved.MaxStack);
            return View;
        }
        if (IsValid(Legacy) && Legacy->ItemDefinitionId == Item.ItemDefinitionId)
        {
            View.bResolved = true;
            View.MaxStackSize = Legacy->bStackable
                ? FMath::Max(1, Legacy->MaxStackSize) : 1;
        }
        return View;
    }
};
