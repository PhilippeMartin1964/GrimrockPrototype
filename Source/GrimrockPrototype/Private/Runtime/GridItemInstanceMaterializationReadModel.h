#pragma once

#include "Runtime/Items/GridItemSubsystem.h"

/**
 * ARM-CLEAN06.5G.94: validate physical identity against ItemSystem.
 * Physical instances do not carry cached name or mass.
 */
struct FGridItemInstanceMaterializationReadModel
{
    static bool TryApply(const UGridItemSubsystem* Registry, const FGridItemInstance& Instance)
    {
        FGridResolvedItem Resolved;
        return Registry && !Instance.ItemDefinitionId.IsNone() &&
            Registry->ResolveItem(Instance.ItemDefinitionId, Instance.ItemLevel, Resolved) &&
            (Resolved.bIsArmament || Instance.ItemLevel == 0) &&
            FMath::IsFinite(Resolved.WeightKg) && Resolved.WeightKg >= 0.0f;
    }
};
