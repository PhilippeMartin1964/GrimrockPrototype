#pragma once

#include "Core/GridLevelAsset.h"
#include "Core/GridObjectArchetypeAsset.h"
#include "Runtime/GridRuntimeWorldObjectData.h"

/**
 * WORLDOBJ-MIG09 definition/instance behavior resolver.
 *
 * Shared behavior belongs to the world-object definition. Only true instance-owned
 * values are overlaid at runtime. MIG09-E2 introduces FGridRuntimeWorldObjectData
 * as the non-persistent runtime boundary; the historical FGridLevelObjectData
 * overloads remain temporary adapters for editor/tests not migrated yet.
 */
namespace GridObjectInstanceBehavior
{
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
		const FGridRuntimeWorldObjectData& ObjectData, const UGridObjectArchetypeAsset* Archetype)
	{
		// Direct compatibility callers without a definition still retain their raw
		// snapshot semantics during E2. Native production placements are expected
		// to resolve a definition.
		if (!Archetype)
		{
			return ObjectData.Behavior;
		}

		FGridObjectBehaviorParams Resolved = Archetype->DefaultBehavior;
		ApplyInstanceOwnedOverrides(ObjectData.Behavior, Resolved);
		return Resolved;
	}

	/** Temporary E2 adapter for editor/tests still typed against the legacy DTO. */
	inline FGridObjectBehaviorParams Resolve(
		const FGridLevelObjectData& ObjectData, const UGridObjectArchetypeAsset* Archetype)
	{
		return Resolve(FGridRuntimeWorldObjectData(ObjectData), Archetype);
	}

	/** Temporary E2 adapter; LevelAsset no longer participates in authority selection. */
	inline FGridObjectBehaviorParams Resolve(
		const UGridLevelAsset* LevelAsset, const FGridLevelObjectData& ObjectData, const UGridObjectArchetypeAsset* Archetype)
	{
		(void)LevelAsset;
		return Resolve(ObjectData, Archetype);
	}
}
