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
			case EGridEdge::None:
				return EGridEdge::North;
			case EGridEdge::North:
				return EGridEdge::East;
			case EGridEdge::East:
				return EGridEdge::South;
			case EGridEdge::South:
				return EGridEdge::West;
			case EGridEdge::West:
				return EGridEdge::None;
			default:
				return EGridEdge::None;
		}
	}

	EGridMonsterPatrolMode GetNextPatrolMode(EGridMonsterPatrolMode Mode)
	{
		switch (Mode)
		{
			case EGridMonsterPatrolMode::None:
				return EGridMonsterPatrolMode::Loop;
			case EGridMonsterPatrolMode::Loop:
				return EGridMonsterPatrolMode::PingPong;
			case EGridMonsterPatrolMode::PingPong:
				return EGridMonsterPatrolMode::None;
			default:
				return EGridMonsterPatrolMode::None;
		}
	}

	void RedrawGridEditorViewports()
	{
		if (GEditor)
		{
			GEditor->RedrawAllViewports();
		}
	}
}

bool AGridLevelEditorActor::CanEditSelectedMonsterPatrolRoute() const
{
	const FGridLevelObjectData* Obj = GetSelectedObjectData();
	return LevelAsset && Obj && Obj->Type == EGridLevelObjectType::MonsterSpawn;
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

	if (!CanEditSelectedMonsterPatrolRoute())
	{
		bPatrolRouteEditMode = false;
		SelectedPatrolWaypointIndex = INDEX_NONE;
		UE_LOG(LogTemp, Warning, TEXT("[MON14.3.1] Patrol route edit requires a selected MonsterSpawn."));
		RedrawGridEditorViewports();
		return;
	}

	bPatrolRouteEditMode = true;
	const FGridLevelObjectData* Obj = GetSelectedObjectData();
	SelectedPatrolWaypointIndex = Obj && Obj->PatrolWaypoints.Num() > 0 ? 0 : INDEX_NONE;
	RedrawGridEditorViewports();
}

bool AGridLevelEditorActor::SetSelectedMonsterPatrolMode(EGridMonsterPatrolMode NewMode)
{
	FGridLevelObjectData* Obj = FindSelectedObjectMutable();
	if (!LevelAsset || !Obj || Obj->Type != EGridLevelObjectType::MonsterSpawn)
	{
		return false;
	}
	if (NewMode != EGridMonsterPatrolMode::None && Obj->PatrolWaypoints.Num() < 2)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MON14.3.1] Patrol mode requires at least two waypoints. ObjectId=%s"), *Obj->ObjectId.ToString());
		return false;
	}
	if (Obj->PatrolMode == NewMode)
	{
		return true;
	}

	const FScopedTransaction Transaction(FText::FromString(TEXT("Set Monster Patrol Mode")));
	LevelAsset->Modify();
	Obj->PatrolMode = NewMode;
	LevelAsset->MarkPackageDirty();
	RedrawGridEditorViewports();
	return true;
}

bool AGridLevelEditorActor::AddOrSelectPatrolWaypointAtHoveredCell()
{
	FGridLevelObjectData* Obj = FindSelectedObjectMutable();
	if (!LevelAsset || !Obj || Obj->Type != EGridLevelObjectType::MonsterSpawn || !LevelAsset->IsValidCoord(HoveredCellX, HoveredCellY))
	{
		return false;
	}

	const FIntPoint HoveredCell(HoveredCellX, HoveredCellY);
	for (int32 Index = 0; Index < Obj->PatrolWaypoints.Num(); ++Index)
	{
		if (Obj->PatrolWaypoints[Index].Cell == HoveredCell)
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
	SelectedPatrolWaypointIndex = Obj->PatrolWaypoints.Add(NewWaypoint);

	if (Obj->PatrolWaypoints.Num() >= 2 && Obj->PatrolMode == EGridMonsterPatrolMode::None)
	{
		Obj->PatrolMode = EGridMonsterPatrolMode::Loop;
	}

	LevelAsset->MarkPackageDirty();
	RedrawGridEditorViewports();
	return true;
}

bool AGridLevelEditorActor::SelectPatrolWaypointByIndex(int32 WaypointIndex)
{
	const FGridLevelObjectData* Obj = GetSelectedObjectData();
	if (!Obj || Obj->Type != EGridLevelObjectType::MonsterSpawn || !Obj->PatrolWaypoints.IsValidIndex(WaypointIndex))
	{
		return false;
	}

	SelectedPatrolWaypointIndex = WaypointIndex;
	RedrawGridEditorViewports();
	return true;
}

bool AGridLevelEditorActor::RemoveSelectedPatrolWaypoint()
{
	FGridLevelObjectData* Obj = FindSelectedObjectMutable();
	if (!LevelAsset || !Obj || Obj->Type != EGridLevelObjectType::MonsterSpawn || !Obj->PatrolWaypoints.IsValidIndex(SelectedPatrolWaypointIndex))
	{
		return false;
	}

	const FScopedTransaction Transaction(FText::FromString(TEXT("Remove Monster Patrol Waypoint")));
	LevelAsset->Modify();
	Obj->PatrolWaypoints.RemoveAt(SelectedPatrolWaypointIndex);

	if (Obj->PatrolWaypoints.Num() < 2)
	{
		Obj->PatrolMode = EGridMonsterPatrolMode::None;
	}

	if (Obj->PatrolWaypoints.Num() == 0)
	{
		SelectedPatrolWaypointIndex = INDEX_NONE;
	}
	else
	{
		SelectedPatrolWaypointIndex = FMath::Clamp(SelectedPatrolWaypointIndex, 0, Obj->PatrolWaypoints.Num() - 1);
	}

	LevelAsset->MarkPackageDirty();
	RedrawGridEditorViewports();
	return true;
}

bool AGridLevelEditorActor::ClearSelectedMonsterPatrolRoute()
{
	FGridLevelObjectData* Obj = FindSelectedObjectMutable();
	if (!LevelAsset || !Obj || Obj->Type != EGridLevelObjectType::MonsterSpawn)
	{
		return false;
	}
	if (Obj->PatrolWaypoints.Num() == 0 && Obj->PatrolMode == EGridMonsterPatrolMode::None)
	{
		SelectedPatrolWaypointIndex = INDEX_NONE;
		return true;
	}

	const FScopedTransaction Transaction(FText::FromString(TEXT("Clear Monster Patrol Route")));
	LevelAsset->Modify();
	Obj->PatrolWaypoints.Reset();
	Obj->PatrolMode = EGridMonsterPatrolMode::None;
	SelectedPatrolWaypointIndex = INDEX_NONE;
	LevelAsset->MarkPackageDirty();
	RedrawGridEditorViewports();
	return true;
}

bool AGridLevelEditorActor::MoveSelectedPatrolWaypoint(int32 IndexDelta)
{
	FGridLevelObjectData* Obj = FindSelectedObjectMutable();
	if (!LevelAsset || !Obj || Obj->Type != EGridLevelObjectType::MonsterSpawn || !Obj->PatrolWaypoints.IsValidIndex(SelectedPatrolWaypointIndex) ||
		IndexDelta == 0)
	{
		return false;
	}

	const int32 NewIndex = FMath::Clamp(SelectedPatrolWaypointIndex + IndexDelta, 0, Obj->PatrolWaypoints.Num() - 1);
	if (NewIndex == SelectedPatrolWaypointIndex)
	{
		return false;
	}

	const FScopedTransaction Transaction(FText::FromString(TEXT("Reorder Monster Patrol Waypoint")));
	LevelAsset->Modify();
	Obj->PatrolWaypoints.Swap(SelectedPatrolWaypointIndex, NewIndex);
	SelectedPatrolWaypointIndex = NewIndex;
	LevelAsset->MarkPackageDirty();
	RedrawGridEditorViewports();
	return true;
}

bool AGridLevelEditorActor::SetSelectedPatrolWaypointFacing(EGridEdge NewFacing)
{
	FGridLevelObjectData* Obj = FindSelectedObjectMutable();
	if (!LevelAsset || !Obj || Obj->Type != EGridLevelObjectType::MonsterSpawn || !Obj->PatrolWaypoints.IsValidIndex(SelectedPatrolWaypointIndex) ||
		!IsPatrolFacingValid(NewFacing))
	{
		return false;
	}

	FGridMonsterPatrolWaypoint& Waypoint = Obj->PatrolWaypoints[SelectedPatrolWaypointIndex];
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
	FGridLevelObjectData* Obj = FindSelectedObjectMutable();
	if (!LevelAsset || !Obj || Obj->Type != EGridLevelObjectType::MonsterSpawn || !Obj->PatrolWaypoints.IsValidIndex(SelectedPatrolWaypointIndex) ||
		!FMath::IsFinite(NewWaitSeconds))
	{
		return false;
	}

	FGridMonsterPatrolWaypoint& Waypoint = Obj->PatrolWaypoints[SelectedPatrolWaypointIndex];
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
	const FGridLevelObjectData* Obj = GetSelectedObjectData();
	if (!Obj || Obj->Type != EGridLevelObjectType::MonsterSpawn)
	{
		return;
	}
	SetSelectedMonsterPatrolMode(GetNextPatrolMode(Obj->PatrolMode));
}

void AGridLevelEditorActor::CycleSelectedPatrolWaypointFacing()
{
	const FGridLevelObjectData* Obj = GetSelectedObjectData();
	if (!Obj || Obj->Type != EGridLevelObjectType::MonsterSpawn || !Obj->PatrolWaypoints.IsValidIndex(SelectedPatrolWaypointIndex))
	{
		return;
	}
	SetSelectedPatrolWaypointFacing(GetNextPatrolFacing(Obj->PatrolWaypoints[SelectedPatrolWaypointIndex].Facing));
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
	const FGridLevelObjectData* Obj = GetSelectedObjectData();
	if (!Obj || Obj->Type != EGridLevelObjectType::MonsterSpawn || !Obj->PatrolWaypoints.IsValidIndex(SelectedPatrolWaypointIndex))
	{
		return;
	}
	SetSelectedPatrolWaypointWaitSeconds(Obj->PatrolWaypoints[SelectedPatrolWaypointIndex].WaitSeconds + 0.5f);
}

void AGridLevelEditorActor::DecreaseSelectedPatrolWaypointWait()
{
	const FGridLevelObjectData* Obj = GetSelectedObjectData();
	if (!Obj || Obj->Type != EGridLevelObjectType::MonsterSpawn || !Obj->PatrolWaypoints.IsValidIndex(SelectedPatrolWaypointIndex))
	{
		return;
	}
	SetSelectedPatrolWaypointWaitSeconds(Obj->PatrolWaypoints[SelectedPatrolWaypointIndex].WaitSeconds - 0.5f);
}

#endif
