#pragma once

#include "Core/GridLevelPlacementTypes.h"

// Pure compatibility resolver: reading a placement never migrates its serialized data.
namespace GridRelocation
{
	inline bool IsCandidate(const FGridWorldObjectInstance& Object)
	{
		return Object.Type != EGridLevelObjectType::Pit && (Object.Type == EGridLevelObjectType::Teleporter || Object.InstanceConfig.Transition.bIsTransition);
	}

	inline bool UsesLegacyTeleporter(const FGridWorldObjectInstance& Object)
	{
		return Object.Type == EGridLevelObjectType::Teleporter && !Object.InstanceConfig.Transition.bIsTransition;
	}

	inline FGridObjectTransitionParams Resolve(const FGridWorldObjectInstance& Object)
	{
		FGridObjectTransitionParams Result = Object.InstanceConfig.Transition;
		if (!IsCandidate(Object))
			return Result; // Pit retains its dedicated contract.
		if (UsesLegacyTeleporter(Object))
		{
			Result.TargetLevelId = NAME_None;
			Result.TargetCellX = Object.InstanceConfig.Teleporter.TargetCellX;
			Result.TargetCellY = Object.InstanceConfig.Teleporter.TargetCellY;
			Result.TargetFacing = EGridEdge::None;
		}
		Result.bIsTransition = true;
		Result.bRequireUseAction = false;
		return Result;
	}

	inline void ApplyAuthoringEdit(FGridWorldObjectInstanceConfig& Config, EGridLevelObjectType Type, const FGridObjectTransitionParams& Destination)
	{
		Config.Transition = Destination;
		if (Type == EGridLevelObjectType::Pit)
			return;
		Config.Transition.bIsTransition = true;
		Config.Transition.bRequireUseAction = false;
		if (Type == EGridLevelObjectType::Teleporter)
		{
			Config.Teleporter.TargetCellX = Destination.TargetCellX;
			Config.Teleporter.TargetCellY = Destination.TargetCellY;
		}
	}
}
