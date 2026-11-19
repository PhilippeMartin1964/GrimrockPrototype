#pragma once

#include "Core/GridLevelPlacementTypes.h"
#include "Runtime/Items/GridItemSubsystem.h"

/** New overrides copy the explicit key IDs only. A remaining historical
 * pointer indicates an unretired source and blocks authoring atomically.
 * Script-owned IDs, duplicates and order are preserved without inference.
 */
struct FGridEditorCanonicalLockRuleAuthoring
{
    static bool TryCopyDefaults(const UGridItemSubsystem* Registry,
        const FGridLockBehaviorParams& Defaults,
        FGridLockAcceptedKeyInstanceRuleValues& OutOverride)
    {
        if (!IsValid(Registry) || !Registry->IsConfigured() ||
            !Defaults.AcceptedKeyItems.IsEmpty())
        {
            return false;
        }
        FGridLockAcceptedKeyInstanceRuleValues Proposed;
        Proposed.AcceptedKeyIds = Defaults.AcceptedKeyIds;
        OutOverride = MoveTemp(Proposed);
        return true;
    }
};
