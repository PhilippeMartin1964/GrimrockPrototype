#include "Core/GridBoundary.h"

FVector AGridLevelEditorActor::GetSelectedCellWorldCenter(float ZOffset) const
{
	if (PreviewRuntimeActor)
	{
		return PreviewRuntimeActor->GetCellCenterWorld(SelectedCellX, SelectedCellY, ZOffset);
	}
	const float CellSize = LevelAsset ? LevelAsset->CellSize : 200.f;
	return GetActorLocation() + FVector::ZeroVector +
		FVector((SelectedCellX * CellSize) + (CellSize * 0.5f), (SelectedCellY * CellSize) + (CellSize * 0.5f), ZOffset);
}

void AGridLevelEditorActor::EnsureLevelReady()
{
	if (!HasValidLevelAsset())
	{
		UE_LOG(LogTemp, Warning, TEXT("GridLevelEditorActor: LevelAsset is null."));
		return;
	}

#if WITH_EDITOR
	LevelAsset->Modify();
#endif

	LevelAsset->EnsureCellCount();
	LevelAsset->EnsureObjectIds();

#if WITH_EDITOR
	LevelAsset->MarkPackageDirty();
#endif

	RebuildPreview();
}

void AGridLevelEditorActor::RebuildPreview()
{
	// WORLDOBJ-MIG09-D3: preview rebuild is read-only with respect to level data.
	// Editor mutations must already have reached typed authority before this call.
	ResolvePreviewRuntimeActor();
	if (PreviewRuntimeActor)
	{
		PreviewRuntimeActor->LevelAsset = LevelAsset;
		SyncPreviewRuntimeWorldObjectDefinitionsFromPalette();
		PreviewRuntimeActor->RebuildLevel();
	}
}

void AGridLevelEditorActor::SyncPreviewRuntimeWorldObjectDefinitionsFromPalette()
{
	if (!PreviewRuntimeActor || !ObjectPalette)
	{
		return;
	}

#if WITH_EDITOR
	PreviewRuntimeActor->Modify();
#endif

	for (const FGridObjectPaletteEntry& Entry : ObjectPalette->Entries)
	{
		if (Entry.DefaultWorldObjectDefinition)
		{
			PreviewRuntimeActor->WorldObjectDefinitions.AddUnique(Entry.DefaultWorldObjectDefinition);
		}
	}
}

void AGridLevelEditorActor::ClearSelectedCell()
{
	FGridLevelCellData* CellData = GetSelectedCellMutable();
	if (!CellData)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridLevelEditorActor: invalid selected cell."));
		return;
	}
#if WITH_EDITOR
	LevelAsset->Modify();
#endif
	*CellData = FGridLevelCellData();
	RemoveObjectsAtSelectionInternal(false);
#if WITH_EDITOR
	LevelAsset->MarkPackageDirty();
#endif
	RebuildGeometryPreview();
}

void AGridLevelEditorActor::PaintSelectedWall()
{
	FGridLevelCellData* CellData = GetSelectedCellMutable();
	if (!CellData)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridLevelEditorActor: invalid selected cell."));
		return;
	}
	if (CellData->CellType == EGridCellType::Empty)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridLevelEditorActor: cannot paint wall on empty cell."));
		return;
	}
	EGridWallType* WallPtr = GetSelectedWallMutable(*CellData);
	if (!WallPtr)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridLevelEditorActor: SelectedEdge must be North/East/South/West."));
		return;
	}
#if WITH_EDITOR
	if (*WallPtr == PaintWallType)
	{
		return;
	}
	LevelAsset->Modify();
#endif
	// Shared walls are stored per cell. Do not mirror to the neighboring opposite edge.
	*WallPtr = PaintWallType;
#if WITH_EDITOR
	LevelAsset->MarkPackageDirty();
#endif
	RebuildGeometryPreview();
}

void AGridLevelEditorActor::ClearSelectedWall()
{
	FGridLevelCellData* CellData = GetSelectedCellMutable();
	if (!CellData)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridLevelEditorActor: invalid selected cell."));
		return;
	}

	EGridWallType* WallPtr = GetSelectedWallMutable(*CellData);
	if (!WallPtr)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridLevelEditorActor: SelectedEdge must be North/East/South/West."));
		return;
	}

#if WITH_EDITOR
	if (*WallPtr == EGridWallType::None)
	{
		return;
	}
	LevelAsset->Modify();
#endif

	// Keep the directional wall rule consistent with painting, rendering and movement.
	*WallPtr = EGridWallType::None;

#if WITH_EDITOR
	LevelAsset->MarkPackageDirty();
#endif

	RebuildGeometryPreview();
}

int32 AGridLevelEditorActor::RemoveObjectsAtSelectionInternal(bool bSameTypeOnly)
{
	if (!HasValidLevelAsset() || !IsValidSelectedCell())
	{
		return 0;
	}
#if WITH_EDITOR
	LevelAsset->Modify();
#endif

	TArray<FGuid> RemovedIds;
	const EGridLevelObjectType FilterType = PaintObjectType;
	const TArray<FGuid> ObjectIds = LevelAsset->GetTypedPlacementIdsAtCell(SelectedCellX, SelectedCellY);
	for (int32 Index = ObjectIds.Num() - 1; Index >= 0; --Index)
	{
		const FGuid ObjectId = ObjectIds[Index];
		if (bSameTypeOnly && LevelAsset->GetTypedPlacementType(ObjectId) != FilterType)
		{
			continue;
		}

		int32 CellX, CellY;
		EGridEdge Edge;
		if (!LevelAsset->TryGetTypedPlacementLocation(ObjectId, CellX, CellY, Edge)) continue;
		const bool bRemove = !IsEdgePlacedObject(ObjectId) || Edge == SelectedEdge;
		if (bRemove)
		{
			RemovedIds.Add(ObjectId);
		}
	}

	int32 RemovedCount = 0;
	for (const FGuid& RemovedId : RemovedIds)
	{
		RemovedCount += LevelAsset->RemoveObjectById(RemovedId) ? 1 : 0;
	}

#if WITH_EDITOR
	if (RemovedCount > 0)
	{
		LevelAsset->MarkPackageDirty();
	}
#endif

	return RemovedCount;
}

int32 AGridLevelEditorActor::RemoveObjectsConflictingWithPlacementInternal(EGridLevelObjectType NewObjectType, FName NewWorldObjectDefinitionId, bool bNewObjectOnEdge)
{
	(void)NewObjectType;
	(void)bNewObjectOnEdge;

	if (!HasValidLevelAsset() || !IsValidSelectedCell() || SelectedEdge == EGridEdge::None)
	{
		return 0;
	}

	const UGridWorldObjectDefinitionAsset* NewDefinition = FindWorldObjectDefinitionById(NewWorldObjectDefinitionId);
	if (!NewDefinition || !NewDefinition->OccupiesBoundary())
	{
		// Cell and wall-surface sharing are permissive by default in WORLDOBJ-MIG02.
		return 0;
	}

	const FGridBoundaryKey NewBoundary = FGridBoundaryKey::MakeCanonical(SelectedCellX, SelectedCellY, SelectedEdge);
	if (!NewBoundary.IsValid())
	{
		return 0;
	}

	TArray<FGuid> RemovedIds;
	for (const FGridWorldObjectInstance& ExistingObject : LevelAsset->WorldObjectInstances)
	{
		if (ExistingObject.WallSide == EGridEdge::None)
		{
			continue;
		}

		const UGridWorldObjectDefinitionAsset* ExistingDefinition = FindWorldObjectDefinitionById(ExistingObject.WorldObjectDefinitionId);
		if (!ExistingDefinition || !ExistingDefinition->OccupiesBoundary())
		{
			continue;
		}

		const FGridBoundaryKey ExistingBoundary =
			FGridBoundaryKey::MakeCanonical(ExistingObject.CellX, ExistingObject.CellY, ExistingObject.WallSide);
		if (ExistingBoundary.IsValid() && ExistingBoundary == NewBoundary)
		{
			RemovedIds.Add(ExistingObject.InstanceId);
		}
	}

	if (RemovedIds.Num() == 0)
	{
		return 0;
	}

#if WITH_EDITOR
	LevelAsset->Modify();
#endif

	int32 RemovedCount = 0;
	for (const FGuid& RemovedId : RemovedIds)
	{
		RemovedCount += LevelAsset->RemoveObjectById(RemovedId) ? 1 : 0;
	}

#if WITH_EDITOR
	if (RemovedCount > 0)
	{
		LevelAsset->MarkPackageDirty();
	}
#endif
	return RemovedCount;
}
