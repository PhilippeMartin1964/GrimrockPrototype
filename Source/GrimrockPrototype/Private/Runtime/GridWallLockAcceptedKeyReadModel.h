#pragma once

#include "Core/GridObjectBehavior.h"

/** Effective lock acceptance uses only authored IDs, including script-owned IDs.
 * An empty or malformed list cannot recover any historical key pointer.
 */
struct FGridWallLockAcceptedKeyReadModel
{
	static TArray<FName> Resolve(const FGridLockBehaviorParams& Lock)
	{
		TArray<FName> OutIds;
		for (const FName Id : Lock.AcceptedKeyIds)
		{
			if (!Id.IsNone())
			{
				OutIds.AddUnique(Id);
			}
		}
		return OutIds;
	}
};
