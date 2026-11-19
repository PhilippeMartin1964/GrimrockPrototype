#pragma once

#include "Core/GridObjectBehavior.h"
#include "Runtime/Items/GridItemSubsystem.h"

/** The acceptance rule is a registered ItemId, independent of a physical level.
 * Historical pointers are not accepted, even if their old identity matches.
 */
struct FGridReceptacleAcceptedCanonicalReadModel
{
    static bool TryResolveItemId(const UGridItemSubsystem* Registry,
        const FGridReceptacleAcceptedItemConfig& Rule, FName& OutItemId)
    {
        OutItemId = NAME_None;
        if (!IsValid(Registry) || !Registry->IsConfigured() ||
            Rule.CanonicalItemId.IsNone() || Rule.ItemDefinition ||
            !Registry->HasCanonicalItem(Rule.CanonicalItemId))
        {
            return false;
        }
        OutItemId = Rule.CanonicalItemId;
        return true;
    }
};
