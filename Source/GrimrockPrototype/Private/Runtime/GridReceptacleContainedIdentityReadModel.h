#pragma once

#include "Runtime/GridItemActor.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "GridWorldPickupIdentityReadModel.h"

/** A registered receptacle ItemId is not renamed by its old DA_Item.
 * The legacy definition remains an authoring source ONLY when the
 * original initial object has no explicit ItemId in pre-06.4 assets.
 */
struct FGridReceptacleContainedIdentityReadModel
{
    static FName Resolve(const UGridItemSubsystem* Registry,
        const UGridItemDefinitionAsset* Legacy, FName ExplicitItemId)
    {
        const FName LegacyId = IsValid(Legacy)
            ? Legacy->ItemDefinitionId : NAME_None;
        if (Registry && Registry->HasCanonicalItem(ExplicitItemId))
            return ExplicitItemId;
        if (ExplicitItemId.IsNone())
            return LegacyId; // Legacy initial-content authoring only.
        if (Registry && Registry->HasCanonicalItem(LegacyId))
            return NAME_None; // Never rename an unrelated ID to a canonical one.
        return LegacyId.IsNone() ? ExplicitItemId : LegacyId;
    }

    static FName ResolveActor(const AGridItemActor* Actor,
        FName ContainedItemId, int32 FixedLevel)
    {
        return FGridWorldPickupIdentityReadModel::Resolve(
            Actor, ContainedItemId, FixedLevel);
    }
};
