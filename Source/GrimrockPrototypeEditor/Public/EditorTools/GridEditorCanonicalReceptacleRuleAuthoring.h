#pragma once

#include "Core/GridObjectBehavior.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"

/** ARM-CLEAN06.4X.49: new registered acceptance rules are authored
 * as canonical ItemIds without copying an obsolete DA_Item/DA_Weapon.
 * Legacy unknown identities still retain their source until registered.
 * Existing shared assets are NOT migrated here. */
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
        const bool bRegistered = IsValid(Source) &&
            Registry->HasCanonicalItem(Source->ItemDefinitionId);
        NewRule.CanonicalItemId = bRegistered
            ? Source->ItemDefinitionId : NAME_None;
        NewRule.ItemDefinition = bRegistered ? nullptr : Source;
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
                // Do not copy the shared definition's historical pointer
                // into a newly authored per-placement acceptance override.
                Next.ItemDefinition = nullptr;
            }
            else if (IsValid(Legacy))
            {
                if (Legacy->ItemDefinitionId.IsNone()) {return false;}
                if (Registry->HasCanonicalItem(Legacy->ItemDefinitionId))
                {
                    Next.CanonicalItemId = Legacy->ItemDefinitionId;
                    Next.ItemDefinition = nullptr;
                }
            }
            Proposed.Add(Next); // Preserve rule order and legacy-only unknowns.
        }
        OutRules = MoveTemp(Proposed);
        return true;
    }
};
