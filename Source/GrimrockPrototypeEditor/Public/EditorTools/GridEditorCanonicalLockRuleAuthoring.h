#pragma once

#include "Core/GridLevelPlacementTypes.h"
#include "Runtime/Items/GridItemSubsystem.h"

/** New overrides copy explicit key IDs atomically.
 * Script-owned IDs, duplicates and order are preserved without inference.
 */
struct FGridEditorCanonicalLockRuleAuthoring
{
    static bool TryCopyDefaults(const UGridItemSubsystem* Registry,
        const FGridLockBehaviorParams& Defaults,
        FGridLockAcceptedKeyInstanceRuleValues& OutOverride)
    {
        if (!IsValid(Registry) || !Registry->IsConfigured())
        {
            return false;
        }
        FGridLockAcceptedKeyInstanceRuleValues Proposed;
        Proposed.AcceptedKeyIds = Defaults.AcceptedKeyIds;
        OutOverride = MoveTemp(Proposed);
        return true;
    }
};
