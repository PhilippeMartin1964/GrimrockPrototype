#pragma once

#include "Core/GridLevelPlacementTypes.h"

namespace GridRelocation
{
	inline bool IsConfigured(const FGridRelocationBehaviorParams& Params)
	{
		return Params.TargetCellX >= 0 && Params.TargetCellY >= 0;
	}

	inline bool IsCandidate(const FGridWorldObjectInstance& Object)
	{
		return Object.Type != EGridLevelObjectType::Pit && (Object.Type == EGridLevelObjectType::Teleporter || IsConfigured(Object.InstanceConfig.Relocation));
	}
}
