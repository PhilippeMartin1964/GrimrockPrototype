#pragma once

#include "Core/GridLevelPlacementTypes.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "GridWorldItemLevelReadModel.h"

/**
 * ARM-CLEAN06.4X.10: one-shot materialization of typed ItemSpawns.
 * Persisted CanonicalItemId is authoritative. No legacy fallback for a
 * known canonical source and no ARM N0 -> arbitrary N1 promotion.
 * Unmigrated synthetic editor fixtures keep their prior source semantics.
 */
struct FGridCanonicalItemSpawnReadModel
{
    static bool TryMaterialize(const FGridItemSpawnInstance& Source,
        const UGridItemSubsystem* Registry, FGridLooseItemInstance& Out)
    {
        Out = FGridLooseItemInstance();
        if (!Source.SpawnId.IsValid() || !Source.bSpawnAtStart)
        {
            return false;
        }

        const UGridItemDefinitionAsset* Legacy = Source.ItemDefinition.Get();
        const bool bPersistedCanonical = !Source.CanonicalItemId.IsNone();
        if (bPersistedCanonical)
        {
            if (!Registry || !Registry->HasCanonicalItem(Source.CanonicalItemId) ||
                Source.Quantity < 1 ||
                (IsValid(Legacy) && Legacy->ItemDefinitionId != Source.CanonicalItemId))
            {
                return false;
            }
            // Use the physical reader shared by world pickups and initial
            // receptacle contents; registered identities require exact levels.
            const FGridReceptacleItemMetadataReadView Physical =
                FGridWorldItemLevelReadModel::ResolvePersistedInstance(
                    Registry, nullptr, Source.CanonicalItemId, Source.ItemLevel);
            if (!Physical.bCanonicalIdentity || !Physical.bResolved ||
                Physical.FixedItemLevel != Source.ItemLevel)
            {
                return false;
            }
            FGridResolvedItem Projected;
            if (!Registry->ResolveItem(Source.CanonicalItemId, Physical.FixedItemLevel, Projected) ||
                !Projected.IsValid() || Projected.ItemId != Source.CanonicalItemId ||
                Projected.ItemLevel != Source.ItemLevel)
            {
                return false;
            }
        }
        else if (!IsValid(Legacy) || Legacy->ItemDefinitionId.IsNone())
        {
            return false;
        }

        Out.InstanceId = Source.SpawnId;
        Out.ItemDefinition = Source.ItemDefinition; // Temporary ARM/SaveGame reader.
        Out.CanonicalItemId = bPersistedCanonical ? Source.CanonicalItemId : NAME_None;
        Out.ItemLevel = Source.ItemLevel;
        Out.Quantity = Source.Quantity; // Legacy clamp remains in AddPlacedItemActor.
        Out.CellX = Source.CellX;
        Out.CellY = Source.CellY;
        Out.SurfaceSide = EGridEdge::None;
        return true;
    }
};
