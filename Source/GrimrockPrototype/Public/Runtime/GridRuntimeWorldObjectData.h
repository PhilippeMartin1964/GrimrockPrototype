#pragma once

#include "Core/GridLevelPlacementTypes.h"

/**
 * Runtime-only initialization payload for one placed world object.
 *
 * This structure is intentionally not reflected and is never stored in UGridLevelAsset.
 * Shared rules are resolved from the Definition. Native placement state is carried in Behavior;
 * moving-part, door-chain and GEUI09 interaction overrides remain separate sparse channels.
 */
struct GRIMROCKPROTOTYPE_API FGridRuntimeWorldObjectData
{
	FGuid ObjectId;
	EGridLevelObjectType Type = EGridLevelObjectType::None;
	int32 CellX = INDEX_NONE;
	int32 CellY = INDEX_NONE;
	EGridEdge Edge = EGridEdge::None;
	FName WorldObjectDefinitionId = NAME_None;
	bool bDoorInitiallyOpen = false;
	bool bTeleporterInitiallyEnabled = true;
	FText OverrideReadableText;
	FGridObjectBehaviorParams Behavior;
	TArray<FGridWorldObjectMovingPartInstanceOverride> MovingPartOverrides;
	FGridWorldObjectInteractionOverrides InteractionOverrides;
	EGridDoorChainMode DoorChainMode = EGridDoorChainMode::Inherit;
	bool bOverrideChainPullDuration = false;
	float ChainPullDuration = 0.25f;

	FGridRuntimeWorldObjectData() = default;

	/** Native target path: build the runtime payload directly from a typed world-object placement. */
	explicit FGridRuntimeWorldObjectData(const FGridWorldObjectInstance& Source)
		: ObjectId(Source.InstanceId)
		, Type(Source.Type)
		, CellX(Source.CellX)
		, CellY(Source.CellY)
		, Edge(Source.WallSide)
		, WorldObjectDefinitionId(Source.WorldObjectDefinitionId)
		, bDoorInitiallyOpen(Source.InstanceConfig.bDoorInitiallyOpen)
		, bTeleporterInitiallyEnabled(Source.InstanceConfig.bTeleporterInitiallyEnabled)
		, OverrideReadableText(Source.ReadableTextOverride)
	{
		Behavior.Teleporter = Source.InstanceConfig.Teleporter;
		Behavior.Transition = Source.InstanceConfig.Transition;
		Behavior.Pit = Source.InstanceConfig.Pit;
		Behavior.Receptacle.InitialContent = Source.InstanceConfig.ReceptacleInitialContent;
		Behavior.Lock.bStartsUnlocked = Source.InstanceConfig.bStartsUnlocked;
		MovingPartOverrides = Source.InstanceConfig.MovingPartOverrides;
		InteractionOverrides = Source.InstanceConfig.InteractionOverrides;
		DoorChainMode = Source.InstanceConfig.DoorChainMode;
		bOverrideChainPullDuration = Source.InstanceConfig.bOverrideChainPullDuration;
		ChainPullDuration = Source.InstanceConfig.ChainPullDuration;
	}
};
