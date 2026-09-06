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

	float GetYawForFacing(EGridEdge Facing)
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

	void NormalizeMonsterSpawnData(FGridLevelObjectData& ObjectData)
	{
		if (ObjectData.Type != EGridLevelObjectType::MonsterSpawn)
		{
			return;
		}

		// InitialFacing is authoritative; LocalYaw is retained only so the
		// existing generic editor preview keeps the same orientation.
		ObjectData.LocalYaw = GetYawForFacing(ObjectData.InitialFacing);
	}

	FString GetMonsterSpawnLabel(const FGridLevelObjectData& Spawn)
	{
		return Spawn.ObjectId.IsValid() ? Spawn.ObjectId.ToString(EGuidFormats::DigitsWithHyphens)
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
		Level.SparseBehaviorOverrideObjectIds.Remove(ObjectId);
		return RemovedCount > 0;
	}

	void UpsertTypedPlacementFromCompatibility(UGridLevelAsset& Level, const FGridLevelObjectData& Source)
	{
		if (!Source.ObjectId.IsValid())
		{
			return;
		}

		FGridWorldObjectInstance PreviousWorld;
		bool bHadWorld = false;
		if (const FGridWorldObjectInstance* Existing = Level.WorldObjectInstances.FindByPredicate(
				[&Source](const FGridWorldObjectInstance& Instance)
				{
					return Instance.InstanceId == Source.ObjectId;
				}))
		{
			PreviousWorld = *Existing;
			bHadWorld = true;
		}

		FGridLooseItemInstance PreviousLooseItem;
		bool bHadLooseItem = false;
		if (const FGridLooseItemInstance* Existing = Level.LooseItemInstances.FindByPredicate(
				[&Source](const FGridLooseItemInstance& Instance)
				{
					return Instance.InstanceId == Source.ObjectId;
				}))
		{
			PreviousLooseItem = *Existing;
			bHadLooseItem = true;
		}

		FGridItemSpawnInstance PreviousItemSpawn;
		bool bHadItemSpawn = false;
		if (const FGridItemSpawnInstance* Existing = Level.ItemSpawns.FindByPredicate(
				[&Source](const FGridItemSpawnInstance& Spawn)
				{
					return Spawn.SpawnId == Source.ObjectId;
				}))
		{
			PreviousItemSpawn = *Existing;
			bHadItemSpawn = true;
		}

		RemoveTypedPlacementById(Level, Source.ObjectId);

		switch (GridLevelPlacementConversion::GetBucket(Source.Type))
		{
			case EGridLevelPlacementBucket::WorldObject:
			{
				FGridWorldObjectInstance Instance = GridLevelPlacementConversion::ToWorldObject(Source);
				if (bHadWorld)
				{
					FTransform PreservedTransform = PreviousWorld.LocalTransformOverride;
					FRotator PreservedRotation = PreservedTransform.Rotator();
					PreservedRotation.Yaw = Source.LocalYaw;
					PreservedTransform.SetRotation(PreservedRotation.Quaternion());

					const bool bHasTypedOnlyTransform = !PreservedTransform.GetLocation().IsNearlyZero() ||
						!PreservedTransform.GetScale3D().Equals(FVector::OneVector) || !FMath::IsNearlyZero(PreservedRotation.Pitch) ||
						!FMath::IsNearlyZero(PreservedRotation.Roll);
					Instance.bHasLocalTransformOverride = bHasTypedOnlyTransform || !FMath::IsNearlyZero(Source.LocalYaw);
					Instance.LocalTransformOverride = PreservedTransform;
				}
				Level.WorldObjectInstances.Add(MoveTemp(Instance));
				Level.SparseBehaviorOverrideObjectIds.Add(Source.ObjectId);
				break;
			}

			case EGridLevelPlacementBucket::LooseItem:
			{
				FGridLooseItemInstance Instance = GridLevelPlacementConversion::ToLooseItem(Source);
				if (bHadLooseItem)
				{
					Instance.Quantity = PreviousLooseItem.Quantity;
					Instance.LocalOffset = PreviousLooseItem.LocalOffset;
				}
				Level.LooseItemInstances.Add(MoveTemp(Instance));
				break;
			}

			case EGridLevelPlacementBucket::MonsterSpawn:
				Level.MonsterSpawns.Add(GridLevelPlacementConversion::ToMonsterSpawn(Source));
				break;

			case EGridLevelPlacementBucket::ItemSpawn:
			{
				FGridItemSpawnInstance Spawn = GridLevelPlacementConversion::ToItemSpawn(Source);
				if (bHadItemSpawn)
				{
					Spawn.Quantity = PreviousItemSpawn.Quantity;
				}
				Level.ItemSpawns.Add(MoveTemp(Spawn));
				break;
			}

			case EGridLevelPlacementBucket::LogicObject:
				Level.LogicObjects.Add(GridLevelPlacementConversion::ToLogicObject(Source));
				break;

			case EGridLevelPlacementBucket::None:
			default:
				break;
		}
	}
}

void UGridLevelAsset::PostLoad()
{
	Super::PostLoad();

	if (bTypedPlacementStorageAuthoritative)
	{
		// WORLDOBJ-MIG07-C: a migrated asset never trusts its serialized legacy
		// mirror. Rebuild it immediately from the typed source of truth.
		RefreshLegacyObjectMirrorFromTyped();
		return;
	}

	for (FGridLevelObjectData& ObjectData : Objects)
	{
		NormalizeMonsterSpawnData(ObjectData);
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

	Objects.Reset();
	WorldObjectInstances.Reset();
	LooseItemInstances.Reset();
	MonsterSpawns.Reset();
	ItemSpawns.Reset();
	LogicObjects.Reset();
	SparseBehaviorOverrideObjectIds.Reset();
	Links.Reset();

#if WITH_EDITOR
	MarkPackageDirty();
#endif
}

FGuid UGridLevelAsset::AddObject(const FGridLevelObjectData& NewObject)
{
#if WITH_EDITOR
	Modify();
#endif

	FGridLevelObjectData Obj = NewObject;

	if (!Obj.ObjectId.IsValid())
	{
		Obj.ObjectId = FGuid::NewGuid();
	}

	NormalizeMonsterSpawnData(Obj);

	if (bTypedPlacementStorageAuthoritative)
	{
		UpsertTypedPlacementFromCompatibility(*this, Obj);
		RefreshLegacyObjectMirrorFromTyped();
	}
	else
	{
		Objects.Add(Obj);
	}

#if WITH_EDITOR
	MarkPackageDirty();
#endif

	return Obj.ObjectId;
}

bool UGridLevelAsset::RemoveObjectById(const FGuid& ObjectId)
{
#if WITH_EDITOR
	Modify();
#endif

	if (bTypedPlacementStorageAuthoritative)
	{
		if (!RemoveTypedPlacementById(*this, ObjectId))
		{
			return false;
		}
		RefreshLegacyObjectMirrorFromTyped();
	}
	else
	{
		const int32 Index = Objects.IndexOfByPredicate(
			[&](const FGridLevelObjectData& Obj)
			{
				return Obj.ObjectId == ObjectId;
			});

		if (Index == INDEX_NONE)
		{
			return false;
		}

		Objects.RemoveAt(Index);
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

	if (bTypedPlacementStorageAuthoritative)
	{
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
		RefreshLegacyObjectMirrorFromTyped();
	}
	else
	{
		for (FGridLevelObjectData& Obj : Objects)
		{
			if (!Obj.ObjectId.IsValid())
			{
				Obj.ObjectId = FGuid::NewGuid();
			}

			NormalizeMonsterSpawnData(Obj);
		}
	}

#if WITH_EDITOR
	MarkPackageDirty();
#endif
}

bool UGridLevelAsset::CommitCompatibilityObjectEdit(const FGuid& ObjectId)
{
	if (!bTypedPlacementStorageAuthoritative)
	{
		return true;
	}
	if (!ObjectId.IsValid())
	{
		return false;
	}

	const FGridLevelObjectData* StoredObject = Objects.FindByPredicate(
		[&ObjectId](const FGridLevelObjectData& Object)
		{
			return Object.ObjectId == ObjectId;
		});
	if (!StoredObject)
	{
		return false;
	}

	FGridLevelObjectData EditedSnapshot = *StoredObject;
	NormalizeMonsterSpawnData(EditedSnapshot);
	UpsertTypedPlacementFromCompatibility(*this, EditedSnapshot);
	return true;
}

bool UGridLevelAsset::ValidateMonsterSpawns(TArray<FString>& OutErrors) const
{
	OutErrors.Reset();
	const TArray<FGridLevelObjectData>& ObjectView = GetObjectCompatibilityView();

	TMap<FGuid, int32> ObjectIdCounts;
	for (const FGridLevelObjectData& ObjectData : ObjectView)
	{
		if (ObjectData.ObjectId.IsValid())
		{
			++ObjectIdCounts.FindOrAdd(ObjectData.ObjectId);
		}
	}

	TMap<FIntPoint, FGuid> EnabledSpawnByCell;
	TMap<FName, TMap<int32, TMap<FIntPoint, FGuid>>> EncounterSpawnByWaveAndCell;
	for (const FGridLevelObjectData& Spawn : ObjectView)
	{
		if (Spawn.Type != EGridLevelObjectType::MonsterSpawn)
		{
			continue;
		}

		const FString SpawnLabel = GetMonsterSpawnLabel(Spawn);
		if (!Spawn.ObjectId.IsValid())
		{
			OutErrors.Add(FString::Printf(TEXT("MonsterSpawn %s requires a valid ObjectId/SpawnId."), *SpawnLabel));
		}
		else if (ObjectIdCounts.FindRef(Spawn.ObjectId) != 1)
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

				if (Spawn.bInitiallyEnabled)
				{
					const FIntPoint SpawnCell(Spawn.CellX, Spawn.CellY);
					if (const FGuid* ExistingSpawnId = EnabledSpawnByCell.Find(SpawnCell))
					{
						OutErrors.Add(FString::Printf(TEXT("MonsterSpawn %s shares initial cell (%d,%d) with enabled MonsterSpawn %s."), *SpawnLabel,
							Spawn.CellX, Spawn.CellY, *ExistingSpawnId->ToString(EGuidFormats::DigitsWithHyphens)));
					}
					else
					{
						EnabledSpawnByCell.Add(SpawnCell, Spawn.ObjectId);
					}
				}
			}
		}

		if (Spawn.Edge != EGridEdge::None)
		{
			OutErrors.Add(FString::Printf(TEXT("MonsterSpawn %s is cell-centered and requires Edge=None."), *SpawnLabel));
		}

		if (!IsValidMonsterSpawnFacing(Spawn.InitialFacing))
		{
			OutErrors.Add(FString::Printf(TEXT("MonsterSpawn %s requires a cardinal InitialFacing."), *SpawnLabel));
		}

		if (!IsValidMonsterSpawnInitialState(Spawn.InitialMonsterState))
		{
			OutErrors.Add(FString::Printf(TEXT("MonsterSpawn %s requires InitialMonsterState Idle or Dormant."), *SpawnLabel));
		}

		if (Spawn.PatrolMode != EGridMonsterPatrolMode::None && Spawn.PatrolWaypoints.Num() < 2)
		{
			OutErrors.Add(FString::Printf(
				TEXT("MonsterSpawn %s patrol mode %s requires at least two waypoints."), *SpawnLabel, *UEnum::GetValueAsString(Spawn.PatrolMode)));
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
						OutErrors.Add(FString::Printf(
							TEXT("MonsterSpawn %s patrol waypoint %d must use a non-empty cell that allows occupancy."), *SpawnLabel, WaypointIndex));
					}
				}
			}

			if (!IsValidPatrolWaypointFacing(Waypoint.Facing))
			{
				OutErrors.Add(
					FString::Printf(TEXT("MonsterSpawn %s patrol waypoint %d requires Facing=None or a cardinal direction."), *SpawnLabel, WaypointIndex));
			}
			if (!FMath::IsFinite(Waypoint.WaitSeconds) || Waypoint.WaitSeconds < 0.0f)
			{
				OutErrors.Add(
					FString::Printf(TEXT("MonsterSpawn %s patrol waypoint %d requires a finite non-negative WaitSeconds."), *SpawnLabel, WaypointIndex));
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
		if (!Spawn.EncounterGroupId.IsNone() && Spawn.EncounterWaveIndex > 0 && Spawn.bInitiallyEnabled)
		{
			OutErrors.Add(FString::Printf(
				TEXT("MonsterSpawn %s belongs to future encounter wave %d and must be disabled at start."), *SpawnLabel, Spawn.EncounterWaveIndex));
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
				SpawnByCell.Add(SpawnCell, Spawn.ObjectId);
			}
		}

		const UGridMonsterDefinitionAsset* Definition = Spawn.MonsterDefinitionAsset;
		const FName AssetDefinitionId = Definition ? Definition->MonsterId : NAME_None;
		const FName ResolvedDefinitionId = !AssetDefinitionId.IsNone() ? AssetDefinitionId : Spawn.MonsterDefinitionId;

		if (ResolvedDefinitionId.IsNone())
		{
			OutErrors.Add(FString::Printf(TEXT("MonsterSpawn %s requires MonsterDefinitionAsset or MonsterDefinitionId."), *SpawnLabel));
		}

		if (Definition)
		{
			FString DefinitionError;
			if (!Definition->ValidateDefinition(DefinitionError))
			{
				OutErrors.Add(FString::Printf(
					TEXT("MonsterSpawn %s references invalid MonsterDefinition '%s': %s"), *SpawnLabel, *GetNameSafe(Definition), *DefinitionError));
			}

			if (!Spawn.MonsterDefinitionId.IsNone() && Spawn.MonsterDefinitionId != AssetDefinitionId)
			{
				OutErrors.Add(FString::Printf(TEXT("MonsterSpawn %s stores MonsterDefinitionId '%s' but its asset resolves to '%s'."), *SpawnLabel,
					*Spawn.MonsterDefinitionId.ToString(), *AssetDefinitionId.ToString()));
			}
		}
	}

	return OutErrors.IsEmpty();
}

const FGridLevelObjectData* UGridLevelAsset::FindMonsterSpawnById(const FGuid& SpawnId) const
{
	if (!SpawnId.IsValid())
	{
		return nullptr;
	}

	return GetObjectCompatibilityView().FindByPredicate(
		[&SpawnId](const FGridLevelObjectData& ObjectData)
		{
			return ObjectData.Type == EGridLevelObjectType::MonsterSpawn && ObjectData.ObjectId == SpawnId;
		});
}
