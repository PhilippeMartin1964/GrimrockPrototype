#pragma once

#include "Core/GridLevelPlacementTypes.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Runtime/GridRuntimeWorldObjectData.h"

/**
 * WORLDOBJ-MIG09 definition/instance behavior resolver.
 *
 * Shared behavior belongs to the world-object definition. Only true instance-owned
 * values are overlaid at runtime. Native placements and the non-persistent
 * world-object runtime payload share the same five instance-owned overrides.
 */
namespace GridObjectInstanceBehavior
{
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
		// Without a definition, the native payload contains behavior defaults and
		// the five instance-owned overrides copied from the world-object placement.
		if (!Definition)
		{
			return ObjectData.Behavior;
		}

		FGridObjectBehaviorParams Resolved = Definition->DefaultBehavior;
		ApplyInstanceOwnedOverrides(ObjectData.Behavior, Resolved);
		return Resolved;
	}

}
