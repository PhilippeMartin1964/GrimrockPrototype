#pragma once

#include "Core/GridLevelPlacementTypes.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Runtime/GridRuntimeWorldObjectData.h"

/**
 * WORLDOBJ-MIG09 definition/instance behavior resolver.
 *
 * Shared behavior belongs to the world-object definition. Only true instance-owned
 * values are overlaid at runtime. Door-chain presence uses a tri-state sparse override
 * so "inherit" remains distinguishable from an explicit enabled/disabled decision.
 */
namespace GridObjectInstanceBehavior
{
	inline void ApplyDoorChainOverrides(
		EGridDoorChainMode ChainMode,
		bool bOverrideChainPullDuration,
		float ChainPullDuration,
		FGridDoorAnimationParams& InOutDoorAnimation)
	{
		switch (ChainMode)
		{
			case EGridDoorChainMode::Enabled:
				InOutDoorAnimation.bHasChainMechanism = true;
				break;
			case EGridDoorChainMode::Disabled:
				InOutDoorAnimation.bHasChainMechanism = false;
				break;
			case EGridDoorChainMode::Inherit:
			default:
				break;
		}

		if (bOverrideChainPullDuration)
		{
			InOutDoorAnimation.ChainPullDuration = ChainPullDuration;
		}
	}

	inline FGridObjectBehaviorParams Resolve(
		const FGridWorldObjectInstance& WorldObjectInstance, const UGridWorldObjectDefinitionAsset* Definition)
	{
		FGridObjectBehaviorParams Resolved = Definition ? Definition->DefaultBehavior : FGridObjectBehaviorParams();
		const FGridWorldObjectInstanceConfig& Config = WorldObjectInstance.InstanceConfig;
		Resolved.Teleporter = Config.Teleporter;
		Resolved.Transition = Config.Transition;
		Resolved.Pit = Config.Pit;
		Resolved.Receptacle.InitialContent = Config.ReceptacleInitialContent;
		Resolved.Lock.bStartsUnlocked = Config.bStartsUnlocked;
		ApplyDoorChainOverrides(
			Config.DoorChainMode,
			Config.bOverrideChainPullDuration,
			Config.ChainPullDuration,
			Resolved.DoorAnimation);
		return Resolved;
	}

	inline FGridObjectBehaviorParams BuildSparseOverrides(const FGridObjectBehaviorParams& Source)
	{
		FGridObjectBehaviorParams Overrides;

		// True level-instance data only. Shared rules stay in the definition.
		Overrides.Teleporter = Source.Teleporter;
		Overrides.Transition = Source.Transition;
		Overrides.Pit = Source.Pit;
		Overrides.Receptacle.InitialContent = Source.Receptacle.InitialContent;
		Overrides.Lock.bStartsUnlocked = Source.Lock.bStartsUnlocked;

		return Overrides;
	}

	inline void ApplyInstanceOwnedOverrides(const FGridObjectBehaviorParams& Overrides, FGridObjectBehaviorParams& InOutBehavior)
	{
		InOutBehavior.Teleporter = Overrides.Teleporter;
		InOutBehavior.Transition = Overrides.Transition;
		InOutBehavior.Pit = Overrides.Pit;
		InOutBehavior.Receptacle.InitialContent = Overrides.Receptacle.InitialContent;
		InOutBehavior.Lock.bStartsUnlocked = Overrides.Lock.bStartsUnlocked;
	}

	inline FGridObjectBehaviorParams Resolve(
		const FGridRuntimeWorldObjectData& ObjectData, const UGridWorldObjectDefinitionAsset* Definition)
	{
		// Without a definition, start from the sparse behavior payload and still apply
		// the separately transported door-chain controls.
		if (!Definition)
		{
			FGridObjectBehaviorParams Resolved = ObjectData.Behavior;
			ApplyDoorChainOverrides(
				ObjectData.DoorChainMode,
				ObjectData.bOverrideChainPullDuration,
				ObjectData.ChainPullDuration,
				Resolved.DoorAnimation);
			return Resolved;
		}

		FGridObjectBehaviorParams Resolved = Definition->DefaultBehavior;
		ApplyInstanceOwnedOverrides(ObjectData.Behavior, Resolved);
		ApplyDoorChainOverrides(
			ObjectData.DoorChainMode,
			ObjectData.bOverrideChainPullDuration,
			ObjectData.ChainPullDuration,
			Resolved.DoorAnimation);
		return Resolved;
	}

}
