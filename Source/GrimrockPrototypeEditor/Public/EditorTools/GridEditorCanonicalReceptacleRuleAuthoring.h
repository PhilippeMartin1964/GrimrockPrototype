#pragma once

#include "Core/GridObjectBehavior.h"
#include "Runtime/Items/GridItemSubsystem.h"

/** Author explicit registered acceptance identities; no old asset picker.
 * None is an editable empty draft, never an effective acceptance rule.
 */
struct FGridEditorCanonicalReceptacleRuleAuthoring
{
    static bool TrySelectCanonicalId(const UGridItemSubsystem* Registry,
        FName ItemId, FGridReceptacleAcceptedItemConfig& InOut)
    {
        if (!IsValid(Registry) || !Registry->IsConfigured() ||
            (!ItemId.IsNone() && !Registry->HasCanonicalItem(ItemId)))
        {
            return false;
        }
        FGridReceptacleAcceptedItemConfig NewRule;
        NewRule.CanonicalItemId = ItemId;
        InOut = NewRule;
        return true;
    }

    static bool TryCopyDefaults(const UGridItemSubsystem* Registry,
        const TArray<FGridReceptacleAcceptedItemConfig>& Defaults,
        TArray<FGridReceptacleAcceptedItemConfig>& OutRules)
    {
        OutRules.Reset();
        if (!IsValid(Registry) || !Registry->IsConfigured()) return false;
        for (const FGridReceptacleAcceptedItemConfig& Rule : Defaults)
        {
            if (Rule.ItemDefinition || Rule.CanonicalItemId.IsNone() ||
                !Registry->HasCanonicalItem(Rule.CanonicalItemId))
            {
                return false;
            }
        }
        OutRules = Defaults;
        return true;
    }
};
