#include "Core/GridLevelAsset.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"

namespace
{
	bool IsValidMonsterSpawnFacing(EGridEdge Facing)
	{
		return Facing == EGridEdge::North || Facing == EGridEdge::East || Facing == EGridEdge::South || Facing == EGridEdge::West;
	}

	bool IsValidMonsterSpawnInitialState(EGridMonsterState State)
	{
		return State == EGridMonsterState::Idle || State == EGridMonsterState::Dormant;
	}

	bool IsValidPatrolWaypointFacing(EGridEdge Facing)
	{
		return Facing == EGridEdge::None || IsValidMonsterSpawnFacing(Facing);
	}

	FString GetMonsterSpawnLabel(const FGridMonsterSpawnInstance& Spawn)
	{
		return Spawn.SpawnId.IsValid() ? Spawn.SpawnId.ToString(EGuidFormats::DigitsWithHyphens)
			: FString::Printf(TEXT("at (%d,%d)"), Spawn.CellX, Spawn.CellY);
	}

	bool RemoveTypedPlacementById(UGridLevelAsset& Level, const FGuid& ObjectId)
	{
		if (!ObjectId.IsValid())
		{
			return false;
		}

		int32 RemovedCount = 0;
		RemovedCount += Level.WorldObjectInstances.RemoveAll(
			[&ObjectId](const FGridWorldObjectInstance& Instance)
			{
				return Instance.InstanceId == ObjectId;
			});
		RemovedCount += Level.LooseItemInstances.RemoveAll(
			[&ObjectId](const FGridLooseItemInstance& Instance)
			{
				return Instance.InstanceId == ObjectId;
			});
		RemovedCount += Level.MonsterSpawns.RemoveAll(
			[&ObjectId](const FGridMonsterSpawnInstance& Spawn)
			{
				return Spawn.SpawnId == ObjectId;
			});
		RemovedCount += Level.ItemSpawns.RemoveAll(
			[&ObjectId](const FGridItemSpawnInstance& Spawn)
			{
				return Spawn.SpawnId == ObjectId;
			});
		RemovedCount += Level.LogicObjects.RemoveAll(
			[&ObjectId](const FGridLogicObjectInstance& Instance)
			{
				return Instance.InstanceId == ObjectId;
			});
		return RemovedCount > 0;
	}
}

void UGridLevelAsset::EnsureCellCount()
{
	const int32 Expected = FMath::Max(1, Width) * FMath::Max(1, Height);
	if (Cells.Num() != Expected)
	{
		Cells.SetNum(Expected);
	}
}

bool UGridLevelAsset::IsValidCoord(int32 X, int32 Y) const
{
	return X >= 0 && X < Width && Y >= 0 && Y < Height;
}

bool UGridLevelAsset::SetWorldObjectInstanceConfig(
	const FGuid& ObjectId, const FGridWorldObjectInstanceConfig& NewConfig)
{
	FGridWorldObjectInstance* Instance = FindWorldObjectInstanceById(ObjectId);
	if (!Instance)
	{
		return false;
	}

#if WITH_EDITOR
	Modify();
#endif

	Instance->InstanceConfig = NewConfig;

#if WITH_EDITOR
	MarkPackageDirty();
#endif

	return true;
}

int32 UGridLevelAsset::GetIndex(int32 X, int32 Y) const
{
	return Y * Width + X;
}

const FGridLevelCellData& UGridLevelAsset::GetCell(int32 X, int32 Y) const
{
	check(IsValidCoord(X, Y));
	return Cells[GetIndex(X, Y)];
}

FGridLevelCellData& UGridLevelAsset::GetCellMutable(int32 X, int32 Y)
{
	check(IsValidCoord(X, Y));
	return Cells[GetIndex(X, Y)];
}

bool UGridLevelAsset::IsStartCellValid() const
{
	if (!IsValidCoord(StartCellX, StartCellY))
	{
		return false;
	}

	const int32 CellIndex = GetIndex(StartCellX, StartCellY);
	if (!Cells.IsValidIndex(CellIndex))
	{
		return false;
	}

	const FGridLevelCellData& StartCell = Cells[CellIndex];
	return StartCell.CellType != EGridCellType::Empty && !StartCell.bBlocksOccupancy;
}

FIntPoint UGridLevelAsset::GetStartCell() const
{
	return FIntPoint(StartCellX, StartCellY);
}

void UGridLevelAsset::ClearLevel()
{
#if WITH_EDITOR
	Modify();
#endif

	EnsureCellCount();
	for (FGridLevelCellData& Cell : Cells)
	{
		Cell = FGridLevelCellData();
	}

	WorldObjectInstances.Reset();
	LooseItemInstances.Reset();
	MonsterSpawns.Reset();
	ItemSpawns.Reset();
	LogicObjects.Reset();
	Links.Reset();

#if WITH_EDITOR
	MarkPackageDirty();
#endif
}

bool UGridLevelAsset::RemoveObjectById(const FGuid& ObjectId)
{
#if WITH_EDITOR
	Modify();
#endif

	if (!RemoveTypedPlacementById(*this, ObjectId))
	{
		return false;
	}
	RemoveLinksForObject(ObjectId);

#if WITH_EDITOR
	MarkPackageDirty();
#endif
	return true;
}

void UGridLevelAsset::RemoveLinksForObject(const FGuid& ObjectId)
{
	Links.RemoveAll(
		[&](const FGridObjectLink& Link)
		{
			return Link.SourceObjectId == ObjectId || Link.TargetObjectId == ObjectId;
		});
}

void UGridLevelAsset::EnsureObjectIds()
{
#if WITH_EDITOR
	Modify();
#endif

	for (FGridWorldObjectInstance& Instance : WorldObjectInstances)
	{
		if (!Instance.InstanceId.IsValid())
		{
			Instance.InstanceId = FGuid::NewGuid();
		}
	}
	for (FGridLooseItemInstance& Instance : LooseItemInstances)
	{
		if (!Instance.InstanceId.IsValid())
		{
			Instance.InstanceId = FGuid::NewGuid();
		}
	}
	for (FGridMonsterSpawnInstance& Spawn : MonsterSpawns)
	{
		if (!Spawn.SpawnId.IsValid())
		{
			Spawn.SpawnId = FGuid::NewGuid();
		}
	}
	for (FGridItemSpawnInstance& Spawn : ItemSpawns)
	{
		if (!Spawn.SpawnId.IsValid())
		{
			Spawn.SpawnId = FGuid::NewGuid();
		}
	}
	for (FGridLogicObjectInstance& Instance : LogicObjects)
	{
		if (!Instance.InstanceId.IsValid())
		{
			Instance.InstanceId = FGuid::NewGuid();
		}
	}

#if WITH_EDITOR
	MarkPackageDirty();
#endif
}

bool UGridLevelAsset::ValidateMonsterSpawns(TArray<FString>& OutErrors) const
{
	OutErrors.Reset();

	TMap<FGuid, int32> ObjectIdCounts;
	const auto CountId = [&ObjectIdCounts](const FGuid& ObjectId)
	{
		if (ObjectId.IsValid())
		{
			++ObjectIdCounts.FindOrAdd(ObjectId);
		}
	};
	for (const FGridWorldObjectInstance& WorldObjectInstance : WorldObjectInstances) CountId(WorldObjectInstance.InstanceId);
	for (const FGridLooseItemInstance& LooseItemInstance : LooseItemInstances) CountId(LooseItemInstance.InstanceId);
	for (const FGridMonsterSpawnInstance& MonsterSpawn : MonsterSpawns) CountId(MonsterSpawn.SpawnId);
	for (const FGridItemSpawnInstance& ItemSpawn : ItemSpawns) CountId(ItemSpawn.SpawnId);
	for (const FGridLogicObjectInstance& LogicInstance : LogicObjects) CountId(LogicInstance.InstanceId);

	TMap<FIntPoint, FGuid> EnabledSpawnByCell;
	TMap<FName, TMap<int32, TMap<FIntPoint, FGuid>>> EncounterSpawnByWaveAndCell;
	for (const FGridMonsterSpawnInstance& Spawn : MonsterSpawns)
	{
		const FString SpawnLabel = GetMonsterSpawnLabel(Spawn);
		if (!Spawn.SpawnId.IsValid())
		{
			OutErrors.Add(FString::Printf(TEXT("MonsterSpawn %s requires a valid ObjectId/SpawnId."), *SpawnLabel));
		}
		else if (ObjectIdCounts.FindRef(Spawn.SpawnId) != 1)
		{
			OutErrors.Add(FString::Printf(TEXT("MonsterSpawn %s does not have a unique ObjectId/SpawnId."), *SpawnLabel));
		}

		if (!IsValidCoord(Spawn.CellX, Spawn.CellY))
		{
			OutErrors.Add(FString::Printf(TEXT("MonsterSpawn %s is outside grid bounds at (%d,%d)."), *SpawnLabel, Spawn.CellX, Spawn.CellY));
		}
		else
		{
			const int32 CellIndex = GetIndex(Spawn.CellX, Spawn.CellY);
			if (!Cells.IsValidIndex(CellIndex))
			{
				OutErrors.Add(FString::Printf(TEXT("MonsterSpawn %s cannot resolve its cell because the level cell array is incomplete."), *SpawnLabel));
			}
			else
			{
				const FGridLevelCellData& Cell = Cells[CellIndex];
				if (Cell.CellType == EGridCellType::Empty || Cell.bBlocksOccupancy)
				{
					OutErrors.Add(FString::Printf(TEXT("MonsterSpawn %s must be placed on a non-empty cell that allows occupancy."), *SpawnLabel));
				}

				if (Spawn.bSpawnAtStart)
				{
					const FIntPoint SpawnCell(Spawn.CellX, Spawn.CellY);
					if (const FGuid* ExistingSpawnId = EnabledSpawnByCell.Find(SpawnCell))
					{
						OutErrors.Add(FString::Printf(TEXT("MonsterSpawn %s shares initial cell (%d,%d) with enabled MonsterSpawn %s."), *SpawnLabel,
							Spawn.CellX, Spawn.CellY, *ExistingSpawnId->ToString(EGuidFormats::DigitsWithHyphens)));
					}
					else
					{
						EnabledSpawnByCell.Add(SpawnCell, Spawn.SpawnId);
					}
				}
			}
		}

		if (!IsValidMonsterSpawnFacing(Spawn.Facing))
		{
			OutErrors.Add(FString::Printf(TEXT("MonsterSpawn %s requires a cardinal InitialFacing."), *SpawnLabel));
		}
		if (!IsValidMonsterSpawnInitialState(Spawn.InitialMonsterState))
		{
			OutErrors.Add(FString::Printf(TEXT("MonsterSpawn %s requires InitialMonsterState Idle or Dormant."), *SpawnLabel));
		}
		if (Spawn.PatrolMode != EGridMonsterPatrolMode::None && Spawn.PatrolWaypoints.Num() < 2)
		{
			OutErrors.Add(FString::Printf(TEXT("MonsterSpawn %s patrol mode %s requires at least two waypoints."), *SpawnLabel,
				*UEnum::GetValueAsString(Spawn.PatrolMode)));
		}

		for (int32 WaypointIndex = 0; WaypointIndex < Spawn.PatrolWaypoints.Num(); ++WaypointIndex)
		{
			const FGridMonsterPatrolWaypoint& Waypoint = Spawn.PatrolWaypoints[WaypointIndex];
			if (!IsValidCoord(Waypoint.Cell.X, Waypoint.Cell.Y))
			{
				OutErrors.Add(FString::Printf(TEXT("MonsterSpawn %s patrol waypoint %d is outside grid bounds at (%d,%d)."), *SpawnLabel, WaypointIndex,
					Waypoint.Cell.X, Waypoint.Cell.Y));
			}
			else
			{
				const int32 WaypointCellIndex = GetIndex(Waypoint.Cell.X, Waypoint.Cell.Y);
				if (!Cells.IsValidIndex(WaypointCellIndex))
				{
					OutErrors.Add(FString::Printf(TEXT("MonsterSpawn %s patrol waypoint %d cannot resolve its level cell."), *SpawnLabel, WaypointIndex));
				}
				else
				{
					const FGridLevelCellData& WaypointCell = Cells[WaypointCellIndex];
					if (WaypointCell.CellType == EGridCellType::Empty || WaypointCell.bBlocksOccupancy)
					{
						OutErrors.Add(FString::Printf(TEXT("MonsterSpawn %s patrol waypoint %d must use a non-empty cell that allows occupancy."), *SpawnLabel,
							WaypointIndex));
					}
				}
			}

			if (!IsValidPatrolWaypointFacing(Waypoint.Facing))
			{
				OutErrors.Add(FString::Printf(TEXT("MonsterSpawn %s patrol waypoint %d requires Facing=None or a cardinal direction."), *SpawnLabel,
					WaypointIndex));
			}
			if (!FMath::IsFinite(Waypoint.WaitSeconds) || Waypoint.WaitSeconds < 0.0f)
			{
				OutErrors.Add(FString::Printf(TEXT("MonsterSpawn %s patrol waypoint %d requires a finite non-negative WaitSeconds."), *SpawnLabel,
					WaypointIndex));
			}
		}

		if (Spawn.EncounterWaveIndex < 0)
		{
			OutErrors.Add(FString::Printf(TEXT("MonsterSpawn %s requires EncounterWaveIndex >= 0."), *SpawnLabel));
		}
		if (Spawn.EncounterGroupId.IsNone() && Spawn.EncounterWaveIndex > 0)
		{
			OutErrors.Add(FString::Printf(TEXT("MonsterSpawn %s requires EncounterGroupId when EncounterWaveIndex is greater than 0."), *SpawnLabel));
		}
		if (!Spawn.EncounterGroupId.IsNone() && Spawn.EncounterWaveIndex > 0 && Spawn.bSpawnAtStart)
		{
			OutErrors.Add(FString::Printf(TEXT("MonsterSpawn %s belongs to future encounter wave %d and must not spawn at start."), *SpawnLabel,
				Spawn.EncounterWaveIndex));
		}
		if (!Spawn.EncounterGroupId.IsNone() && Spawn.EncounterWaveIndex >= 0 && IsValidCoord(Spawn.CellX, Spawn.CellY))
		{
			TMap<FIntPoint, FGuid>& SpawnByCell = EncounterSpawnByWaveAndCell.FindOrAdd(Spawn.EncounterGroupId).FindOrAdd(Spawn.EncounterWaveIndex);
			const FIntPoint SpawnCell(Spawn.CellX, Spawn.CellY);
			if (const FGuid* ExistingSpawnId = SpawnByCell.Find(SpawnCell))
			{
				OutErrors.Add(FString::Printf(TEXT("MonsterSpawn %s shares encounter wave %d cell (%d,%d) with MonsterSpawn %s in encounter '%s'."),
					*SpawnLabel, Spawn.EncounterWaveIndex, Spawn.CellX, Spawn.CellY, *ExistingSpawnId->ToString(EGuidFormats::DigitsWithHyphens),
					*Spawn.EncounterGroupId.ToString()));
			}
			else
			{
				SpawnByCell.Add(SpawnCell, Spawn.SpawnId);
			}
		}

		const UGridMonsterDefinitionAsset* Definition = Spawn.MonsterDefinition;
		if (!Definition)
		{
			OutErrors.Add(FString::Printf(TEXT("MonsterSpawn %s requires MonsterDefinition."), *SpawnLabel));
		}

		if (Definition)
		{
			FString DefinitionError;
			if (!Definition->ValidateDefinition(DefinitionError))
			{
				OutErrors.Add(FString::Printf(TEXT("MonsterSpawn %s references invalid MonsterDefinition '%s': %s"), *SpawnLabel, *GetNameSafe(Definition),
					*DefinitionError));
			}
		}
	}

	return OutErrors.IsEmpty();
}

FGridWorldObjectInstance* UGridLevelAsset::FindWorldObjectInstanceById(const FGuid& ObjectId)
{
	return const_cast<FGridWorldObjectInstance*>(static_cast<const UGridLevelAsset*>(this)->FindWorldObjectInstanceById(ObjectId));
}

FGridLooseItemInstance* UGridLevelAsset::FindLooseItemInstanceById(const FGuid& ObjectId)
{
	return const_cast<FGridLooseItemInstance*>(static_cast<const UGridLevelAsset*>(this)->FindLooseItemInstanceById(ObjectId));
}

FGridMonsterSpawnInstance* UGridLevelAsset::FindMonsterSpawnInstanceById(const FGuid& ObjectId)
{
	return const_cast<FGridMonsterSpawnInstance*>(static_cast<const UGridLevelAsset*>(this)->FindMonsterSpawnInstanceById(ObjectId));
}

FGridItemSpawnInstance* UGridLevelAsset::FindItemSpawnInstanceById(const FGuid& ObjectId)
{
	return const_cast<FGridItemSpawnInstance*>(static_cast<const UGridLevelAsset*>(this)->FindItemSpawnInstanceById(ObjectId));
}

FGridLogicObjectInstance* UGridLevelAsset::FindLogicObjectInstanceById(const FGuid& ObjectId)
{
	return const_cast<FGridLogicObjectInstance*>(static_cast<const UGridLevelAsset*>(this)->FindLogicObjectInstanceById(ObjectId));
}

TArray<FGuid> UGridLevelAsset::GetTypedPlacementIdsAtCell(int32 CellX, int32 CellY) const
{
	TArray<FGuid> ObjectIds;
	for (const FGridWorldObjectInstance& WorldObjectInstance : WorldObjectInstances)
	{
		if (WorldObjectInstance.CellX == CellX && WorldObjectInstance.CellY == CellY) ObjectIds.Add(WorldObjectInstance.InstanceId);
	}
	for (const FGridLooseItemInstance& LooseItemInstance : LooseItemInstances)
	{
		if (LooseItemInstance.CellX == CellX && LooseItemInstance.CellY == CellY) ObjectIds.Add(LooseItemInstance.InstanceId);
	}
	for (const FGridMonsterSpawnInstance& MonsterSpawn : MonsterSpawns)
	{
		if (MonsterSpawn.CellX == CellX && MonsterSpawn.CellY == CellY) ObjectIds.Add(MonsterSpawn.SpawnId);
	}
	for (const FGridItemSpawnInstance& ItemSpawn : ItemSpawns)
	{
		if (ItemSpawn.CellX == CellX && ItemSpawn.CellY == CellY) ObjectIds.Add(ItemSpawn.SpawnId);
	}
	for (const FGridLogicObjectInstance& LogicInstance : LogicObjects)
	{
		if (LogicInstance.CellX == CellX && LogicInstance.CellY == CellY) ObjectIds.Add(LogicInstance.InstanceId);
	}
	return ObjectIds;
}
