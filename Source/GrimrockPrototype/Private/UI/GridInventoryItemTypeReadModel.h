#pragma once

#include "Runtime/GridItemDefinitionAsset.h"
#include "CoreGlobals.h"
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
    static FGridInventoryItemTypeReadView Resolve(
        const UGridItemSubsystem* Registry, const UGridItemDefinitionAsset* Legacy,
        const FGridItemInstance& Item)
    {
        FGridInventoryItemTypeReadView View;
        if (Item.ItemDefinitionId.IsNone()) return View;

        if (Registry && Registry->ContainsItem(Item.ItemDefinitionId))
        {
            // Recognize membership before IsValid(): that method rejects
            // N21 itself, but an invalid known ID must NEVER borrow a
            // legacy category, even in diagnostic/tooltip projections.
            View.bRegisteredIdentity = true;
            if (!Item.IsValid()) return View;
            FGridResolvedItem Resolved;
            if (!Registry->ResolveItem(Item.ItemDefinitionId, Item.ItemLevel, Resolved) ||
                !Resolved.IsValid() || Resolved.ItemId != Item.ItemDefinitionId ||
                Resolved.ItemLevel != Item.ItemLevel ||
                (Resolved.bIsArmament ? Item.ItemLevel < 1 : Item.ItemLevel != 0))
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
                // Presentation only: retain the explicitly authored category
                // without inferring combat/equip/use permissions from it.
                using namespace GridInventoryCategoryTags;
                if (Count(Resolved.Tags) > 1)
                {
                    View.bResolved = false;
                    return View;
                }
                View.ItemType = Resolved.Tags.Contains(Weapon()) ? EGridItemType::Weapon :
                    Resolved.Tags.Contains(Food()) ? EGridItemType::Food :
                    Resolved.Tags.Contains(Component()) ? EGridItemType::Component :
                    Resolved.Tags.Contains(Potion()) ? EGridItemType::Potion : EGridItemType::Misc;
            }
            return View;
        }

        if (!Item.IsValid()) return View;

        // A diagnostic unknown ID has no category. Historical fixtures only.
#if WITH_DEV_AUTOMATION_TESTS
        if (!Registry && GIsAutomationTesting && IsValid(Legacy) && Legacy->ItemDefinitionId == Item.ItemDefinitionId)
        {
            View.bResolved = true;
            View.ItemType = Legacy->ItemType;
        }
#endif
        return View;
    }
};
