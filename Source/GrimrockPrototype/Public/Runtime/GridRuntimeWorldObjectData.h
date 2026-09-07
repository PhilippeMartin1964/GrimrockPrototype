#pragma once

#include "Core/GridLevelPlacementTypes.h"

/**
 * WORLDOBJ-MIG09-E2 runtime-only payload for one placed world object.
 *
 * This structure is intentionally not reflected and is never stored in UGridLevelAsset.
 * It isolates runtime actors from the historical monolithic FGridLevelObjectData DTO while
 * E2 migrates the remaining runtime/editor/test consumers to their native typed placements.
 */
struct GRIMROCKPROTOTYPE_API FGridRuntimeWorldObjectData
{
	FGuid ObjectId;
	EGridLevelObjectType Type = EGridLevelObjectType::None;
	int32 CellX = INDEX_NONE;
	int32 CellY = INDEX_NONE;
	EGridEdge Edge = EGridEdge::None;
	FName ArchetypeId = NAME_None;
	bool bInitiallyEnabled = true;
	bool bInitiallyActive = false;
	FText OverrideReadableText;
	FGridObjectBehaviorParams Behavior;

	FGridRuntimeWorldObjectData() = default;

	/** Temporary E2 bridge for callers not yet migrated away from FGridLevelObjectData. */
	FGridRuntimeWorldObjectData(const FGridLevelObjectData& Source)
		: ObjectId(Source.ObjectId)
		, Type(Source.Type)
		, CellX(Source.CellX)
		, CellY(Source.CellY)
		, Edge(Source.Edge)
		, ArchetypeId(Source.ArchetypeId)
		, bInitiallyEnabled(Source.bInitiallyEnabled)
		, bInitiallyActive(Source.bInitiallyActive)
		, OverrideReadableText(Source.OverrideReadableText)
		, Behavior(Source.Behavior)
	{
	}

	/** Native target path: build the runtime payload directly from a typed world-object placement. */
	explicit FGridRuntimeWorldObjectData(const FGridWorldObjectInstance& Source)
		: ObjectId(Source.InstanceId)
		, Type(Source.Type)
		, CellX(Source.CellX)
		, CellY(Source.CellY)
		, Edge(Source.WallSide)
		, ArchetypeId(Source.WorldObjectDefinitionId)
		, bInitiallyEnabled(Source.bInitiallyEnabled)
		, bInitiallyActive(Source.bInitiallyActive)
		, OverrideReadableText(Source.ReadableTextOverride)
	{
		Behavior.Teleporter = Source.InstanceConfig.Teleporter;
		Behavior.Transition = Source.InstanceConfig.Transition;
		Behavior.Pit = Source.InstanceConfig.Pit;
		Behavior.Receptacle.InitialContent = Source.InstanceConfig.ReceptacleInitialContent;
		Behavior.Lock.bStartsUnlocked = Source.InstanceConfig.bStartsUnlocked;
	}
};
