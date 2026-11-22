#pragma once

#include "Core/GridObjectBehavior.h"
#include "Runtime/Items/GridItemSubsystem.h"

/** The acceptance rule is a registered ItemId, independent of a physical level.
 * Unknown, empty or unavailable identities cannot produce acceptance.
 */
struct FGridReceptacleAcceptedReadModel
{
    static bool TryResolveItemId(const UGridItemSubsystem* Registry,
        const FGridReceptacleAcceptedItemConfig& Rule, FName& OutItemId)
    {
        OutItemId = NAME_None;
        if (!IsValid(Registry) || !Registry->IsConfigured() ||
            Rule.CanonicalItemId.IsNone() ||
            !Registry->ContainsItem(Rule.CanonicalItemId))
        {
            return false;
        }
        OutItemId = Rule.CanonicalItemId;
        return true;
    }
};
