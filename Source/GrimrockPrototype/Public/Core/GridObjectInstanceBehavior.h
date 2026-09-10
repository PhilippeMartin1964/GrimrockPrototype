#pragma once

#include "Core/GridLevelPlacementTypes.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Runtime/GridRuntimeWorldObjectData.h"

/**
 * Definition/instance behavior resolver.
 *
 * Shared behavior belongs to the world-object Definition. True level-instance values are
 * overlaid at runtime. GEUI09 adds a deliberately sparse interaction/puzzle override layer;
 * it never serializes a second full FGridObjectBehaviorParams on the placement.
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
			InOutDoorAnimation.ChainPullDuration = FMath::Max(0.01f, ChainPullDuration);
		}
	}

	inline void ApplyInteractionOverrides(
		const FGridWorldObjectInteractionOverrides& Overrides,
		FGridObjectBehaviorParams& InOutBehavior)
	{
		if (Overrides.bOverrideButtonHoldTime)
		{
			InOutBehavior.ButtonAnimation.ButtonHoldTime = FMath::Max(0.0f, Overrides.ButtonHoldTime);
		}

		if (Overrides.bOverridePressurePlateWeight)
		{
			InOutBehavior.PressurePlateWeight = Overrides.PressurePlateWeight;
			InOutBehavior.PressurePlateWeight.RequiredItemWeight =
				FMath::Max(0.0f, InOutBehavior.PressurePlateWeight.RequiredItemWeight);
		}

		if (Overrides.bOverrideReceptacleRules)
		{
			InOutBehavior.Receptacle.bAcceptAnyItem = Overrides.ReceptacleRules.bAcceptAnyItem;
			InOutBehavior.Receptacle.AcceptedItems = Overrides.ReceptacleRules.AcceptedItems;
			InOutBehavior.Receptacle.MaxContainedItems = FMath::Max(1, Overrides.ReceptacleRules.MaxContainedItems);
		}

		if (Overrides.bOverrideAcceptedKeys)
		{
			InOutBehavior.Lock.AcceptedKeyItems = Overrides.AcceptedKeys.AcceptedKeyItems;
			InOutBehavior.Lock.AcceptedKeyIds = Overrides.AcceptedKeys.AcceptedKeyIds;
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

		ApplyInteractionOverrides(Config.InteractionOverrides, Resolved);
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

		// Native level-instance data only. GEUI09 interaction overrides travel separately.
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
		if (!Definition)
		{
			FGridObjectBehaviorParams Resolved = ObjectData.Behavior;
			ApplyInteractionOverrides(ObjectData.InteractionOverrides, Resolved);
			ApplyDoorChainOverrides(
				ObjectData.DoorChainMode,
				ObjectData.bOverrideChainPullDuration,
				ObjectData.ChainPullDuration,
				Resolved.DoorAnimation);
			return Resolved;
		}

		FGridObjectBehaviorParams Resolved = Definition->DefaultBehavior;
		ApplyInstanceOwnedOverrides(ObjectData.Behavior, Resolved);
		ApplyInteractionOverrides(ObjectData.InteractionOverrides, Resolved);
		ApplyDoorChainOverrides(
			ObjectData.DoorChainMode,
			ObjectData.bOverrideChainPullDuration,
			ObjectData.ChainPullDuration,
			Resolved.DoorAnimation);
		return Resolved;
	}
}
