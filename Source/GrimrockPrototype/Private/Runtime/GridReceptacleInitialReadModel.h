#pragma once

#include "Runtime/GridReceptacleActor.h"
#include "Runtime/Items/GridItemSubsystem.h"

/** Initial content is an exact physical ItemId, quantity and level.
 * The serialized config contains no historical source.
 */
struct FGridReceptacleInitialReadModel
{
    static bool TryResolve(const UGridItemSubsystem* Registry,
        const FGridReceptacleInitialItemConfig& Source,
        FGridInitialReceptacleItem& Out)
    {
        Out = FGridInitialReceptacleItem();
        if (!IsValid(Registry) || !Registry->IsConfigured() ||
            Source.ItemId.IsNone() || Source.Quantity < 1)
            return false;

        FGridResolvedItem Physical;
        if (!Registry->ResolveItem(Source.ItemId, Source.ItemLevel, Physical) ||
            !Physical.IsValid() || Physical.ItemId != Source.ItemId ||
            Physical.ItemLevel != Source.ItemLevel)
            return false;

        Out.ItemDefinitionId = Source.ItemId;
        Out.ItemLevel = Source.ItemLevel;
        Out.Quantity = Source.Quantity;
        return true;
    }
};
