#pragma once

#include "Core/GridLevelPlacementTypes.h"
#include "Runtime/Items/GridItemSubsystem.h"

/** One-shot materialization of the authored physical generator identity.
 * The generator's stable GUID also identifies the resulting pickup.
 */
struct FGridItemSpawnReadModel
{
    static bool TryMaterialize(const FGridItemSpawnInstance& Source,
        const UGridItemSubsystem* Registry, FGridLooseItemInstance& Out)
    {
        Out = FGridLooseItemInstance();
        if (!Source.SpawnId.IsValid() || !Source.bSpawnAtStart ||
            !IsValid(Registry) || !Registry->IsConfigured() ||
            Source.ItemId.IsNone() || Source.Quantity < 1)
            return false;

        FGridResolvedItem Physical;
        if (!Registry->ResolveItem(Source.ItemId, Source.ItemLevel, Physical) ||
            !Physical.IsValid() || Physical.ItemId != Source.ItemId ||
            Physical.ItemLevel != Source.ItemLevel)
            return false;

        Out.InstanceId = Source.SpawnId;
        Out.ItemId = Source.ItemId;
        Out.ItemLevel = Source.ItemLevel;
        Out.Quantity = Source.Quantity;
        Out.CellX = Source.CellX;
        Out.CellY = Source.CellY;
        Out.SurfaceSide = EGridEdge::None;
        return true;
    }
};
