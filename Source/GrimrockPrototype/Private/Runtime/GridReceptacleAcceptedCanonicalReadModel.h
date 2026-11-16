#pragma once

#include "Core/GridObjectBehavior.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"

/**
 * ARM-CLEAN06.4X.16.1: item-identity read bridge for receptacle acceptance
 * rules. A rule matches ItemId only; it must not consume, change or infer
 * a physical ItemLevel. bAcceptAnyItem and instance override precedence
 * belong to GridObjectInstanceBehavior and AGridReceptacleActor.
 *
 * No native rule assets have yet been converted. Legacy-only entries
 * remain readable until their separately verified one-shot migration.
 */
struct FGridReceptacleAcceptedCanonicalReadModel
{
    static bool TryResolveItemId(const UGridItemSubsystem* Registry,
        const FGridReceptacleAcceptedItemConfig& Rule, FName& OutItemId)
    {
        OutItemId = NAME_None;
        const UGridItemDefinitionAsset* Legacy = Rule.ItemDefinition.Get();
        if (!Rule.CanonicalItemId.IsNone())
        {
            // Persisted identity is the only authority. Neither a stale
            // pointer nor an unconfigured registry may change it.
            if (!Registry || !Registry->HasCanonicalItem(Rule.CanonicalItemId) ||
                (IsValid(Legacy) && Legacy->ItemDefinitionId != Rule.CanonicalItemId))
            {
                return false;
            }
            OutItemId = Rule.CanonicalItemId;
            return true;
        }

        // Explicitly unmigrated rule or synthetic fixture. Historical
        // behavior retained; this cannot override a persisted ItemId.
        if (!IsValid(Legacy) || Legacy->ItemDefinitionId.IsNone())
        {
            return false;
        }
        OutItemId = Legacy->ItemDefinitionId;
        return true;
    }
};
