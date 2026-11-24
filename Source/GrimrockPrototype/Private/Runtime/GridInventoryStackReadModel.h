#pragma once

#include "Runtime/Items/GridItemSubsystem.h"

/**
 * Transient source of stack capacity for pickup, merge, split and cursor
 * transfers. Once ItemId belongs to DA_ItemSystem, MaxStack is authoritative,
 * even when an obsolete DA_Item* contradicts it or does not exist.
 */
struct FGridInventoryStackReadView
{
    bool bRegisteredIdentity = false;
    bool bResolved = false;
    int32 MaxStackSize = 1;

    bool IsStackable() const { return bResolved && MaxStackSize > 1; }
};

struct FGridInventoryStackReadModel
{
    static FGridInventoryStackReadView Resolve(
        const UGridItemSubsystem* Registry,
        const FGridItemInstance& Item)
    {
        FGridInventoryStackReadView View;
        if (Item.ItemDefinitionId.IsNone() || Item.ItemLevel < 0 || Item.ItemLevel > 20)
        {
            return View;
        }
        if (Registry && Registry->ContainsItem(Item.ItemDefinitionId))
        {
            View.bRegisteredIdentity = true;
            FGridResolvedItem Resolved;
            if (!Registry->ResolveItem(Item.ItemDefinitionId, Item.ItemLevel, Resolved) ||
                (!Resolved.bIsArmament && Item.ItemLevel != 0))
            {
                return View; // Registered invalid level must never use old MaxStackSize.
            }
            return Project(&Resolved, true);
        }
        return View;
    }

    static FGridInventoryStackReadView Project(const FGridResolvedItem* Item, bool bRegisteredIdentity)
    {
        FGridInventoryStackReadView View;
        View.bRegisteredIdentity = bRegisteredIdentity;
        if (Item)
        {
            View.bResolved = true;
            View.MaxStackSize = FMath::Max(1, Item->MaxStack);
        }
        return View;
    }
};
