#pragma once

#include "Core/GridObjectBehavior.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"

/** ARM-CLEAN06.4X.16.2: one editor transaction owns mutation;
 * validated canonical identity never derives from a stale old pointer. */
struct FGridEditorCanonicalReceptacleRuleAuthoring
{
    static bool TryAssign(const UGridItemSubsystem* Registry,
        UGridItemDefinitionAsset* Source, FGridReceptacleAcceptedItemConfig& InOut)
    {
        if (!IsValid(Registry) || !Registry->IsConfigured() ||
            (IsValid(Source) && Source->ItemDefinitionId.IsNone()))
        {
            return false;
        }
        FGridReceptacleAcceptedItemConfig NewRule = InOut;
        NewRule.ItemDefinition = Source;
        NewRule.CanonicalItemId = IsValid(Source) &&
            Registry->HasCanonicalItem(Source->ItemDefinitionId)
            ? Source->ItemDefinitionId : NAME_None;
        InOut = NewRule;
        return true;
    }

    static bool TryCopyDefaults(const UGridItemSubsystem* Registry,
        const TArray<FGridReceptacleAcceptedItemConfig>& Defaults,
        TArray<FGridReceptacleAcceptedItemConfig>& OutRules)
    {
        OutRules.Reset();
        if (!IsValid(Registry) || !Registry->IsConfigured())
        {
            return false;
        }
        TArray<FGridReceptacleAcceptedItemConfig> Proposed;
        Proposed.Reserve(Defaults.Num());
        for (const FGridReceptacleAcceptedItemConfig& Rule : Defaults)
        {
            FGridReceptacleAcceptedItemConfig Next = Rule;
            const UGridItemDefinitionAsset* Legacy = Rule.ItemDefinition.Get();
            if (!Rule.CanonicalItemId.IsNone())
            {
                if (!Registry->HasCanonicalItem(Rule.CanonicalItemId) ||
                    (IsValid(Legacy) && Legacy->ItemDefinitionId != Rule.CanonicalItemId))
                {
                    return false;
                }
            }
            else if (IsValid(Legacy))
            {
                if (Legacy->ItemDefinitionId.IsNone()) {return false;}
                if (Registry->HasCanonicalItem(Legacy->ItemDefinitionId))
                {
                    Next.CanonicalItemId = Legacy->ItemDefinitionId;
                }
            }
            Proposed.Add(Next); // Never reorder/dedupe/default historical sources.
        }
        OutRules = MoveTemp(Proposed);
        return true;
    }
};
