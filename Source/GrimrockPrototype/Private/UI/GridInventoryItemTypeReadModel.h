#pragma once

#include "Runtime/GridItemCategory.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "Runtime/Items/GridInventoryCategoryTags.h"

/** ARM-CLEAN06.3AU: transient inventory type projection; no persisted data. */
struct FGridInventoryItemTypeReadView
{
    bool bRegisteredIdentity = false;
    bool bResolved = false;
    EGridItemType ItemType = EGridItemType::None;
};

struct FGridInventoryItemTypeReadModel
{
    static FGridInventoryItemTypeReadView Project(
        const FGridResolvedItem* Item, bool bRegisteredIdentity)
    {
        FGridInventoryItemTypeReadView View;
        View.bRegisteredIdentity = bRegisteredIdentity;
        if (!Item)
        {
            return View;
        }
        View.bResolved = true;
        if (Item->bIsArmament)
        {
            // Passive shields have their own category. Focus and
            // offensive weapons occupy the existing Weapon equipment type.
            View.ItemType = Item->Weapon.Kind == EGridArmamentKind::Shield
                ? EGridItemType::Shield : EGridItemType::Weapon;
        }
        else
        {
            // Presentation only: retain the explicitly authored category
            // without inferring combat/equip/use permissions from it.
            using namespace GridInventoryCategoryTags;
            if (Count(Item->Tags) > 1)
            {
                View.bResolved = false;
                return View;
            }
            View.ItemType = Item->Tags.Contains(Weapon()) ? EGridItemType::Weapon :
                Item->Tags.Contains(Food()) ? EGridItemType::Food :
                Item->Tags.Contains(Component()) ? EGridItemType::Component :
                Item->Tags.Contains(Potion()) ? EGridItemType::Potion :
                Item->Tags.Contains(Torch()) ? EGridItemType::Torch :
                Item->Tags.Contains(Book()) ? EGridItemType::Book :
                Item->Tags.Contains(Jewelry()) ? EGridItemType::Jewelry : EGridItemType::Misc;
        }
        return View;
    }

    static FGridInventoryItemTypeReadView Resolve(
        const UGridItemSubsystem* Registry, const FGridItemInstance& Item)
    {
        FGridResolvedItem Resolved;
        const bool bResolved = Registry && Registry->ResolvePhysicalItem(Item, Resolved);
        return Project(bResolved ? &Resolved : nullptr,
            Registry && Registry->ContainsItem(Item.ItemDefinitionId));
    }
};
