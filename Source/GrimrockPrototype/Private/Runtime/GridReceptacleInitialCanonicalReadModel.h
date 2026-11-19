#pragma once

#include "Runtime/GridReceptacleActor.h"
#include "Runtime/Items/GridItemSubsystem.h"

/** Initial content is an exact physical ItemId, quantity and level.
 * Historical sources are not admitted after native conversion.
 */
struct FGridReceptacleInitialCanonicalReadModel
{
    static bool TryResolve(const UGridItemSubsystem* Registry,
        const FGridReceptacleInitialItemConfig& Source,
        FGridInitialReceptacleItem& Out)
    {
        Out = FGridInitialReceptacleItem();
        if (!IsValid(Registry) || !Registry->IsConfigured() ||
            Source.ItemDefinition || Source.CanonicalItemId.IsNone() || Source.Quantity < 1)
            return false;

        FGridResolvedItem Physical;
        if (!Registry->ResolveItem(Source.CanonicalItemId, Source.ItemLevel, Physical) ||
            !Physical.IsValid() || Physical.ItemId != Source.CanonicalItemId ||
            Physical.ItemLevel != Source.ItemLevel)
            return false;

        Out.bHasPersistedCanonicalIdentity = true;
        Out.ItemDefinitionId = Source.CanonicalItemId;
        Out.ItemLevel = Source.ItemLevel;
        Out.Quantity = Source.Quantity;
        return true;
    }
};
