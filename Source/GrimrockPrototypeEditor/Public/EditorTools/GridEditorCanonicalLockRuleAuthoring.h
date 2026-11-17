#pragma once

#include "Core/GridLevelPlacementTypes.h"
#include "EditorTools/GridEditorLockKeyMigrationPreview.h"
#include "Runtime/Items/GridItemSubsystem.h"

/** X.50: a NEW lock override must not re-persist historical key pointers.
 * The existing preview preserves every authored key identity and order,
 * including script-owned keys outside ItemSystem, but refuses unknown/null
 * old pointers because their effective key set cannot be safely projected.
 * This is authoring-only; old native lock definitions stay unchanged.
 */
struct FGridEditorCanonicalLockRuleAuthoring
{
    static bool TryCopyDefaults(const UGridItemSubsystem* Registry,
        const FGridLockBehaviorParams& Defaults,
        FGridLockAcceptedKeyInstanceRuleValues& OutOverride)
    {
        TArray<FName> ProposedIds;
        FGridLockKeyPreviewStats Stats;
        if (!FGridLockKeyMigrationPreview::TryPreview(
            Registry, Defaults.AcceptedKeyItems, Defaults.AcceptedKeyIds,
            ProposedIds, Stats))
        {
            return false; // Never partially rewrite the existing override.
        }

        FGridLockAcceptedKeyInstanceRuleValues Proposed;
        Proposed.AcceptedKeyIds = MoveTemp(ProposedIds);
        // Leave AcceptedKeyItems EMPTY even if the shared Definition
        // retains its historical pointers until the UE-native migration.
        OutOverride = MoveTemp(Proposed);
        return true;
    }
};
