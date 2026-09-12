#pragma once

#include "Runtime/GridLevelRuntimeActor.h"

/**
 * Shared monster traversal policy for dynamic dungeon hazards.
 *
 * Open pits are intentionally monster-only blockers: the party keeps its
 * existing fall-through behaviour. A trapdoor remains blocked while either
 * opening or closing because AGridLevelRuntimeActor::IsPitOpen() reports an
 * animating trapdoor as open until both moving leaves have fully closed.
 */
namespace GridMonsterTraversalUtils
{
	inline bool IsOpenPitCell(const AGridLevelRuntimeActor* RuntimeActor, const FIntPoint& Cell)
	{
		if (!RuntimeActor || !RuntimeActor->LevelAsset)
		{
			return false;
		}

		for (const FGridWorldObjectInstance& ObjectData : RuntimeActor->LevelAsset->WorldObjectInstances)
		{
			if (ObjectData.CellX != Cell.X || ObjectData.CellY != Cell.Y)
			{
				continue;
			}

			bool bIsPit = ObjectData.Type == EGridLevelObjectType::Pit;
			if (!bIsPit)
			{
				const UGridWorldObjectDefinitionAsset* Definition = RuntimeActor->FindWorldObjectDefinition(ObjectData.WorldObjectDefinitionId);
				bIsPit = Definition && Definition->SupportedType == EGridLevelObjectType::Pit;
			}

			if (!bIsPit)
			{
				continue;
			}

			const bool bOpen = ObjectData.InstanceId.IsValid()
				? RuntimeActor->IsPitOpen(ObjectData.InstanceId)
				: ObjectData.InstanceConfig.Pit.bInitiallyOpen;
			if (bOpen)
			{
				return true;
			}
		}

		return false;
	}
}
