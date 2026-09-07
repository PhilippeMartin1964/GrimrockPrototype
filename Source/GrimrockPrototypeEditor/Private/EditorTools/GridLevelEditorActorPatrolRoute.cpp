#include "EditorTools/GridLevelEditorActor.h"

#if WITH_EDITOR

#include "Editor.h"
#include "ScopedTransaction.h"

namespace
{
	bool IsPatrolFacingValid(EGridEdge Facing)
	{
		return Facing == EGridEdge::None || Facing == EGridEdge::North || Facing == EGridEdge::East || Facing == EGridEdge::South || Facing == EGridEdge::West;
	}

	EGridEdge GetNextPatrolFacing(EGridEdge Facing)
	{
		switch (Facing)
		{
			case EGridEdge::None: return EGridEdge::North;
			case EGridEdge::North: return EGridEdge::East;
			case EGridEdge::East: return EGridEdge::South;
			case EGridEdge::South: return EGridEdge::West;
			case EGridEdge::West: return EGridEdge::None;
			default: return EGridEdge::None;
		}
	}

	EGridMonsterPatrolMode GetNextPatrolMode(EGridMonsterPatrolMode Mode)
	{
		switch (Mode)
		{
			case EGridMonsterPatrolMode::None: return EGridMonsterPatrolMode::Loop;
			case EGridMonsterPatrolMode::Loop: return EGridMonsterPatrolMode::PingPong;
			case EGridMonsterPatrolMode::PingPong: return EGridMonsterPatrolMode::None;
			default: return EGridMonsterPatrolMode::None;
		}
	}

	void RedrawGridEditorViewports()
	{
		if (GEditor)
		{
			GEditor->RedrawAllViewports();
		}
	}

	FGridMonsterSpawnInstance* FindMonsterSpawnMutable(UGridLevelAsset* LevelAsset, const FGuid& SpawnId)
	{
		if (!LevelAsset || !SpawnId.IsValid())
		{
			return nullptr;
		}
		return LevelAsset->MonsterSpawns.FindByPredicate(
			[&SpawnId](const FGridMonsterSpawnInstance& Spawn)
			{
				return Spawn.SpawnId == SpawnId;
			});
	}

	const FGridMonsterSpawnInstance* FindMonsterSpawn(const UGridLevelAsset* LevelAsset, const FGuid& SpawnId)
	{
		if (!LevelAsset || !SpawnId.IsValid())
		{
			return nullptr;
		}
		return LevelAsset->MonsterSpawns.FindByPredicate(
			[&SpawnId](const FGridMonsterSpawnInstance& Spawn)
			{
				return Spawn.SpawnId == SpawnId;
			});
	}
}

bool AGridLevelEditorActor::CanEditSelectedMonsterPatrolRoute() const
{
	return FindMonsterSpawn(LevelAsset, LastSelectedObjectId) != nullptr;
}

bool AGridLevelEditorActor::IsPatrolRouteEditModeActive() const
{
	return bPatrolRouteEditMode && CanEditSelectedMonsterPatrolRoute();
}

void AGridLevelEditorActor::ToggleSelectedMonsterPatrolRouteEditing()
{
	if (IsPatrolRouteEditModeActive())
	{
		bPatrolRouteEditMode = false;
		SelectedPatrolWaypointIndex = INDEX_NONE;
		RedrawGridEditorViewports();
		return;
	}

	const FGridMonsterSpawnInstance* Spawn = FindMonsterSpawn(LevelAsset, LastSelectedObjectId);
	if (!Spawn)
	{
		bPatrolRouteEditMode = false;
		SelectedPatrolWaypointIndex = INDEX_NONE;
		UE_LOG(LogTemp, Warning, TEXT("[MON14.3.1] Patrol route edit requires a selected MonsterSpawn."));
		RedrawGridEditorViewports();
		return;
	}

	bPatrolRouteEditMode = true;
	SelectedPatrolWaypointIndex = Spawn->PatrolWaypoints.Num() > 0 ? 0 : INDEX_NONE;
	RedrawGridEditorViewports();
}

bool AGridLevelEditorActor::SetSelectedMonsterPatrolMode(EGridMonsterPatrolMode NewMode)
{
	FGridMonsterSpawnInstance* Spawn = FindMonsterSpawnMutable(LevelAsset, LastSelectedObjectId);
	if (!Spawn)
	{
		return false;
	}
	if (NewMode != EGridMonsterPatrolMode::None && Spawn->PatrolWaypoints.Num() < 2)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MON14.3.1] Patrol mode requires at least two waypoints. SpawnId=%s"), *Spawn->SpawnId.ToString());
		return false;
	}
	if (Spawn->PatrolMode == NewMode)
	{
		return true;
	}

	const FScopedTransaction Transaction(FText::FromString(TEXT("Set Monster Patrol Mode")));
	LevelAsset->Modify();
	Spawn->PatrolMode = NewMode;
	LevelAsset->MarkPackageDirty();
	RedrawGridEditorViewports();
	return true;
}

bool AGridLevelEditorActor::AddOrSelectPatrolWaypointAtHoveredCell()
{
	FGridMonsterSpawnInstance* Spawn = FindMonsterSpawnMutable(LevelAsset, LastSelectedObjectId);
	if (!Spawn || !LevelAsset->IsValidCoord(HoveredCellX, HoveredCellY))
	{
		return false;
	}

	const FIntPoint HoveredCell(HoveredCellX, HoveredCellY);
	for (int32 Index = 0; Index < Spawn->PatrolWaypoints.Num(); ++Index)
	{
		if (Spawn->PatrolWaypoints[Index].Cell == HoveredCell)
		{
			SelectedPatrolWaypointIndex = Index;
			RedrawGridEditorViewports();
			return true;
		}
	}

	const FScopedTransaction Transaction(FText::FromString(TEXT("Add Monster Patrol Waypoint")));
	LevelAsset->Modify();

	FGridMonsterPatrolWaypoint NewWaypoint;
	NewWaypoint.Cell = HoveredCell;
	NewWaypoint.Facing = EGridEdge::None;
	NewWaypoint.WaitSeconds = 0.0f;
	SelectedPatrolWaypointIndex = Spawn->PatrolWaypoints.Add(NewWaypoint);

	if (Spawn->PatrolWaypoints.Num() >= 2 && Spawn->PatrolMode == EGridMonsterPatrolMode::None)
	{
		Spawn->PatrolMode = EGridMonsterPatrolMode::Loop;
	}

	LevelAsset->MarkPackageDirty();
	RedrawGridEditorViewports();
	return true;
}

bool AGridLevelEditorActor::SelectPatrolWaypointByIndex(int32 WaypointIndex)
{
	const FGridMonsterSpawnInstance* Spawn = FindMonsterSpawn(LevelAsset, LastSelectedObjectId);
	if (!Spawn || !Spawn->PatrolWaypoints.IsValidIndex(WaypointIndex))
	{
		return false;
	}

	SelectedPatrolWaypointIndex = WaypointIndex;
	RedrawGridEditorViewports();
	return true;
}

bool AGridLevelEditorActor::RemoveSelectedPatrolWaypoint()
{
	FGridMonsterSpawnInstance* Spawn = FindMonsterSpawnMutable(LevelAsset, LastSelectedObjectId);
	if (!Spawn || !Spawn->PatrolWaypoints.IsValidIndex(SelectedPatrolWaypointIndex))
	{
		return false;
	}

	const FScopedTransaction Transaction(FText::FromString(TEXT("Remove Monster Patrol Waypoint")));
	LevelAsset->Modify();
	Spawn->PatrolWaypoints.RemoveAt(SelectedPatrolWaypointIndex);

	if (Spawn->PatrolWaypoints.Num() < 2)
	{
		Spawn->PatrolMode = EGridMonsterPatrolMode::None;
	}

	SelectedPatrolWaypointIndex = Spawn->PatrolWaypoints.Num() == 0
		? INDEX_NONE
		: FMath::Clamp(SelectedPatrolWaypointIndex, 0, Spawn->PatrolWaypoints.Num() - 1);

	LevelAsset->MarkPackageDirty();
	RedrawGridEditorViewports();
	return true;
}

bool AGridLevelEditorActor::ClearSelectedMonsterPatrolRoute()
{
	FGridMonsterSpawnInstance* Spawn = FindMonsterSpawnMutable(LevelAsset, LastSelectedObjectId);
	if (!Spawn)
	{
		return false;
	}
	if (Spawn->PatrolWaypoints.Num() == 0 && Spawn->PatrolMode == EGridMonsterPatrolMode::None)
	{
		SelectedPatrolWaypointIndex = INDEX_NONE;
		return true;
	}

	const FScopedTransaction Transaction(FText::FromString(TEXT("Clear Monster Patrol Route")));
	LevelAsset->Modify();
	Spawn->PatrolWaypoints.Reset();
	Spawn->PatrolMode = EGridMonsterPatrolMode::None;
	SelectedPatrolWaypointIndex = INDEX_NONE;
	LevelAsset->MarkPackageDirty();
	RedrawGridEditorViewports();
	return true;
}

bool AGridLevelEditorActor::MoveSelectedPatrolWaypoint(int32 IndexDelta)
{
	FGridMonsterSpawnInstance* Spawn = FindMonsterSpawnMutable(LevelAsset, LastSelectedObjectId);
	if (!Spawn || !Spawn->PatrolWaypoints.IsValidIndex(SelectedPatrolWaypointIndex) || IndexDelta == 0)
	{
		return false;
	}

	const int32 NewIndex = FMath::Clamp(SelectedPatrolWaypointIndex + IndexDelta, 0, Spawn->PatrolWaypoints.Num() - 1);
	if (NewIndex == SelectedPatrolWaypointIndex)
	{
		return false;
	}

	const FScopedTransaction Transaction(FText::FromString(TEXT("Reorder Monster Patrol Waypoint")));
	LevelAsset->Modify();
	Spawn->PatrolWaypoints.Swap(SelectedPatrolWaypointIndex, NewIndex);
	SelectedPatrolWaypointIndex = NewIndex;
	LevelAsset->MarkPackageDirty();
	RedrawGridEditorViewports();
	return true;
}

bool AGridLevelEditorActor::SetSelectedPatrolWaypointFacing(EGridEdge NewFacing)
{
	FGridMonsterSpawnInstance* Spawn = FindMonsterSpawnMutable(LevelAsset, LastSelectedObjectId);
	if (!Spawn || !Spawn->PatrolWaypoints.IsValidIndex(SelectedPatrolWaypointIndex) || !IsPatrolFacingValid(NewFacing))
	{
		return false;
	}

	FGridMonsterPatrolWaypoint& Waypoint = Spawn->PatrolWaypoints[SelectedPatrolWaypointIndex];
	if (Waypoint.Facing == NewFacing)
	{
		return true;
	}

	const FScopedTransaction Transaction(FText::FromString(TEXT("Set Monster Patrol Waypoint Facing")));
	LevelAsset->Modify();
	Waypoint.Facing = NewFacing;
	LevelAsset->MarkPackageDirty();
	RedrawGridEditorViewports();
	return true;
}

bool AGridLevelEditorActor::SetSelectedPatrolWaypointWaitSeconds(float NewWaitSeconds)
{
	FGridMonsterSpawnInstance* Spawn = FindMonsterSpawnMutable(LevelAsset, LastSelectedObjectId);
	if (!Spawn || !Spawn->PatrolWaypoints.IsValidIndex(SelectedPatrolWaypointIndex) || !FMath::IsFinite(NewWaitSeconds))
	{
		return false;
	}

	FGridMonsterPatrolWaypoint& Waypoint = Spawn->PatrolWaypoints[SelectedPatrolWaypointIndex];
	const float ClampedWait = FMath::Max(0.0f, NewWaitSeconds);
	if (FMath::IsNearlyEqual(Waypoint.WaitSeconds, ClampedWait))
	{
		return true;
	}

	const FScopedTransaction Transaction(FText::FromString(TEXT("Set Monster Patrol Waypoint Wait")));
	LevelAsset->Modify();
	Waypoint.WaitSeconds = ClampedWait;
	LevelAsset->MarkPackageDirty();
	RedrawGridEditorViewports();
	return true;
}

void AGridLevelEditorActor::CycleSelectedMonsterPatrolMode()
{
	const FGridMonsterSpawnInstance* Spawn = FindMonsterSpawn(LevelAsset, LastSelectedObjectId);
	if (Spawn)
	{
		SetSelectedMonsterPatrolMode(GetNextPatrolMode(Spawn->PatrolMode));
	}
}

void AGridLevelEditorActor::CycleSelectedPatrolWaypointFacing()
{
	const FGridMonsterSpawnInstance* Spawn = FindMonsterSpawn(LevelAsset, LastSelectedObjectId);
	if (Spawn && Spawn->PatrolWaypoints.IsValidIndex(SelectedPatrolWaypointIndex))
	{
		SetSelectedPatrolWaypointFacing(GetNextPatrolFacing(Spawn->PatrolWaypoints[SelectedPatrolWaypointIndex].Facing));
	}
}

void AGridLevelEditorActor::MoveSelectedPatrolWaypointEarlier()
{
	MoveSelectedPatrolWaypoint(-1);
}

void AGridLevelEditorActor::MoveSelectedPatrolWaypointLater()
{
	MoveSelectedPatrolWaypoint(1);
}

void AGridLevelEditorActor::IncreaseSelectedPatrolWaypointWait()
{
	const FGridMonsterSpawnInstance* Spawn = FindMonsterSpawn(LevelAsset, LastSelectedObjectId);
	if (Spawn && Spawn->PatrolWaypoints.IsValidIndex(SelectedPatrolWaypointIndex))
	{
		SetSelectedPatrolWaypointWaitSeconds(Spawn->PatrolWaypoints[SelectedPatrolWaypointIndex].WaitSeconds + 0.5f);
	}
}

void AGridLevelEditorActor::DecreaseSelectedPatrolWaypointWait()
{
	const FGridMonsterSpawnInstance* Spawn = FindMonsterSpawn(LevelAsset, LastSelectedObjectId);
	if (Spawn && Spawn->PatrolWaypoints.IsValidIndex(SelectedPatrolWaypointIndex))
	{
		SetSelectedPatrolWaypointWaitSeconds(Spawn->PatrolWaypoints[SelectedPatrolWaypointIndex].WaitSeconds - 0.5f);
	}
}

#endif
