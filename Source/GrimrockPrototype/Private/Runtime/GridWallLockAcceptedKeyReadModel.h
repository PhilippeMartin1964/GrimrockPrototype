#pragma once

#include "Core/GridObjectBehavior.h"
#include "Runtime/GridItemDefinitionAsset.h"

/** ARM-CLEAN06.4X.29: one read-only effective list of accepted lock ItemIds.
 * A populated canonical AcceptedKeyIds list is authoritative: the legacy
 * AcceptedKeyItems array is a transitional backup ONLY for old definitions
 * that have no explicit IDs at all. No UObject or SaveGame is mutated.
 */
struct FGridWallLockAcceptedKeyReadModel
{
	static TArray<FName> Resolve(const FGridLockBehaviorParams& Lock)
	{
		TArray<FName> OutIds;
		if (!Lock.AcceptedKeyIds.IsEmpty())
		{
			for (const FName Id : Lock.AcceptedKeyIds)
			{
				if (!Id.IsNone())
				{
					OutIds.AddUnique(Id);
				}
			}
			return OutIds; // Even invalid explicit entries must not fall back.
		}

		for (const UGridItemDefinitionAsset* Definition : Lock.AcceptedKeyItems)
		{
			if (IsValid(Definition) && !Definition->ItemDefinitionId.IsNone())
			{
				OutIds.AddUnique(Definition->ItemDefinitionId);
			}
		}
		return OutIds;
	}
};
