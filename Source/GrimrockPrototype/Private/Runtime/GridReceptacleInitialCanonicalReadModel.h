#pragma once

#include "Runtime/GridReceptacleActor.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "GridWorldItemLevelReadModel.h"

/** ARM-CLEAN06.4X.9.1:
 * A read-only initial-content identity bridge, not a second registry.
 * The 18 real level-authored entries now persist physical ItemLevels.
 * Legacy-only synthetic editor fixtures keep their existing path.
 */
struct FGridReceptacleInitialCanonicalReadModel
{
    static bool TryResolve(const UGridItemSubsystem* Registry,
        const FGridReceptacleInitialItemConfig& Source,
        FGridInitialReceptacleItem& Out)
    {
        Out = FGridInitialReceptacleItem();
        const UGridItemDefinitionAsset* Legacy = Source.ItemDefinition.Get();
        if (!Source.CanonicalItemId.IsNone())
        {
            // A registered ID is the sole identity. A stale or missing
            // registry, an invalid N0 weapon or a contradictory old source
            // may not silently turn this into another physical item.
            if (!Registry || !Registry->HasCanonicalItem(Source.CanonicalItemId) ||
                Source.Quantity < 1 ||
                (IsValid(Legacy) &&
                 Legacy->ItemDefinitionId != Source.CanonicalItemId))
            {
                return false;
            }

            const FGridReceptacleItemMetadataReadView Physical =
                FGridWorldItemLevelReadModel::ResolvePersistedInstance(
                    Registry, nullptr, Source.CanonicalItemId, Source.ItemLevel);
            if (!Physical.bCanonicalIdentity || !Physical.bResolved ||
                Physical.FixedItemLevel != Source.ItemLevel)
            {
                return false;
            }
            Out.ItemDefinitionId = Source.CanonicalItemId;
            Out.ItemDefinition = Source.ItemDefinition; // Transient transition only.
            Out.ItemLevel = Physical.FixedItemLevel;
            Out.Quantity = Source.Quantity;
            return true;
        }

        // Unmigrated synthetic/default definition content (not among W's
        // 18 real per-instance references) preserves its old behavior.
        if (!IsValid(Legacy) || Legacy->ItemDefinitionId.IsNone())
        {
            return false;
        }
        Out.ItemDefinition = Source.ItemDefinition;
        Out.ItemDefinitionId = Legacy->ItemDefinitionId;
        Out.Quantity = FMath::Max(1, Source.Quantity);
        Out.ItemLevel = Source.ItemLevel;
        return true;
    }
};
