#pragma once

#include "Core/GridLevelPlacementTypes.h"

namespace GridLevelPlacementCompatibility
{
	inline float GetYawForFacing(EGridEdge Facing)
	{
		switch (Facing)
		{
			case EGridEdge::East:
				return 90.0f;
			case EGridEdge::South:
				return 180.0f;
			case EGridEdge::West:
				return 270.0f;
			case EGridEdge::North:
			case EGridEdge::None:
			default:
				return 0.0f;
		}
	}

	inline FGridLevelObjectData ToLegacyWorldObject(const FGridWorldObjectInstance& Source)
	{
		FGridLevelObjectData Result;
		Result.ObjectId = Source.InstanceId;
		Result.LogicId = Source.LogicId;
		Result.Type = Source.Type;
		Result.CellX = Source.CellX;
		Result.CellY = Source.CellY;
		Result.Edge = Source.WallSide;
		Result.LocalYaw = Source.bHasLocalTransformOverride ? Source.LocalTransformOverride.Rotator().Yaw : 0.0f;
		Result.ArchetypeId = Source.WorldObjectDefinitionId;
		Result.bInitiallyEnabled = Source.bInitiallyEnabled;
		Result.bInitiallyActive = Source.bInitiallyActive;
		Result.Tag = Source.Tag;
		Result.Notes = Source.Notes;
		Result.OverrideReadableText = Source.ReadableTextOverride;
		Result.PaletteEntryId = Source.PaletteEntryId;

		Result.Behavior.Teleporter = Source.InstanceConfig.Teleporter;
		Result.Behavior.Transition = Source.InstanceConfig.Transition;
		Result.Behavior.Pit = Source.InstanceConfig.Pit;
		Result.Behavior.Receptacle.InitialContent = Source.InstanceConfig.ReceptacleInitialContent;
		Result.Behavior.Lock.bStartsUnlocked = Source.InstanceConfig.bStartsUnlocked;
		return Result;
	}

	inline FGridLevelObjectData ToLegacyLooseItem(const FGridLooseItemInstance& Source)
	{
		FGridLevelObjectData Result;
		Result.ObjectId = Source.InstanceId;
		Result.Type = EGridLevelObjectType::Item;
		Result.CellX = Source.CellX;
		Result.CellY = Source.CellY;
		Result.Edge = Source.SurfaceSide;
		Result.LocalYaw = Source.LocalYaw;
		Result.ItemDefinitionAsset = Source.ItemDefinition;
		Result.ReadableContentAsset = Source.ReadableContentAsset;
		Result.ReadableContentId = Source.ReadableContentId;
		Result.ReadTitleOverride = Source.ReadTitleOverride;
		Result.ReadTextOverride = Source.ReadTextOverride;
		Result.bInitiallyEnabled = Source.bInitiallyEnabled;
		Result.Tag = Source.Tag;
		Result.Notes = Source.Notes;
		Result.PaletteEntryId = Source.PaletteEntryId;
		return Result;
	}

	inline FGridLevelObjectData ToLegacyMonsterSpawn(const FGridMonsterSpawnInstance& Source)
	{
		FGridLevelObjectData Result;
		Result.ObjectId = Source.SpawnId;
		Result.LogicId = Source.LogicId;
		Result.Type = EGridLevelObjectType::MonsterSpawn;
		Result.CellX = Source.CellX;
		Result.CellY = Source.CellY;
		Result.Edge = EGridEdge::None;
		Result.LocalYaw = GetYawForFacing(Source.Facing);
		Result.MonsterDefinitionAsset = Source.MonsterDefinition;
		Result.InitialFacing = Source.Facing;
		Result.InitialMonsterState = Source.InitialMonsterState;
		Result.PatrolMode = Source.PatrolMode;
		Result.PatrolWaypoints = Source.PatrolWaypoints;
		Result.bInitiallyEnabled = Source.bInitiallyEnabled;
		Result.Tag = Source.Tag;
		Result.Notes = Source.Notes;
		Result.PaletteEntryId = Source.PaletteEntryId;
		Result.EncounterGroupId = Source.EncounterGroupId;
		Result.EncounterWaveIndex = Source.EncounterWaveIndex;
		return Result;
	}

	inline FGridLevelObjectData ToLegacyItemSpawn(const FGridItemSpawnInstance& Source)
	{
		FGridLevelObjectData Result;
		Result.ObjectId = Source.SpawnId;
		Result.LogicId = Source.LogicId;
		Result.Type = EGridLevelObjectType::ItemSpawn;
		Result.CellX = Source.CellX;
		Result.CellY = Source.CellY;
		Result.ItemDefinitionAsset = Source.ItemDefinition;
		Result.Behavior.Item.ItemDefinitionAsset = Source.ItemDefinition;
		Result.bInitiallyEnabled = Source.bInitiallyEnabled;
		Result.Tag = Source.Tag;
		Result.Notes = Source.Notes;
		Result.PaletteEntryId = Source.PaletteEntryId;
		return Result;
	}

	inline FGridLevelObjectData ToLegacyLogicObject(const FGridLogicObjectInstance& Source)
	{
		FGridLevelObjectData Result;
		Result.ObjectId = Source.InstanceId;
		Result.LogicId = Source.LogicId;
		Result.Type = Source.Type;
		Result.CellX = Source.CellX;
		Result.CellY = Source.CellY;
		Result.bInitiallyEnabled = Source.bInitiallyEnabled;
		Result.bInitiallyActive = Source.bInitiallyActive;
		Result.Logic = Source.Logic;
		Result.StoryCompanionDefinition = Source.StoryCompanionDefinition;
		Result.Tag = Source.Tag;
		Result.Notes = Source.Notes;
		Result.PaletteEntryId = Source.PaletteEntryId;
		return Result;
	}
}
