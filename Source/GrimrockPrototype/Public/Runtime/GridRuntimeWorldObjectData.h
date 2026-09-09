#pragma once

#include "Core/GridLevelPlacementTypes.h"

/**
 * Runtime-only initialization payload for one placed world object.
 *
 * This structure is intentionally not reflected and is never stored in UGridLevelAsset.
 * It carries world-object identity, pose and initial state to the actor hierarchy.
 * Its only placement constructor accepts FGridWorldObjectInstance: no loose item,
 * monster spawn, item generator or logic placement is represented here.
 * Behavior contains defaults plus the five instance-owned overrides; shared rules
 * are resolved from the definition by GridObjectInstanceBehavior at initialization.
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
