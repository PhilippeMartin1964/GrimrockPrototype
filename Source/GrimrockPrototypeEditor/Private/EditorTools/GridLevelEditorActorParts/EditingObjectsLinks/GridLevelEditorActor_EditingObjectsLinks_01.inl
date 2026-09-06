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
	// WORLDOBJ-MIG07-C: existing editor widgets still mutate the compatibility
	// FGridLevelObjectData mirror. Commit the selected mirror object into the typed
	// authority before any preview consumer is allowed to rebuild from the level.
	if (LevelAsset && LevelAsset->bTypedPlacementStorageAuthoritative && LastSelectedObjectId.IsValid())
	{
		LevelAsset->CommitCompatibilityObjectEdit(LastSelectedObjectId);
		LevelAsset->RefreshLegacyObjectMirrorFromTyped();
	}

	ResolvePreviewRuntimeActor();
	if (PreviewRuntimeActor)
	{
		PreviewRuntimeActor->LevelAsset = LevelAsset;
		SyncPreviewRuntimeObjectArchetypesFromPalette();
		PreviewRuntimeActor->RebuildLevel();
	}
}

void AGridLevelEditorActor::SyncPreviewRuntimeObjectArchetypesFromPalette()
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
		if (Entry.DefaultArchetype)
		{
			PreviewRuntimeActor->ObjectArchetypes.AddUnique(Entry.DefaultArchetype);
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
	const TArray<FGridLevelObjectData>& Objects = LevelAsset->GetObjectCompatibilityView();
	for (int32 Index = Objects.Num() - 1; Index >= 0; --Index)
	{
		const FGridLevelObjectData& Obj = Objects[Index];
		if (Obj.CellX != SelectedCellX || Obj.CellY != SelectedCellY)
		{
			continue;
		}
		if (bSameTypeOnly && Obj.Type != FilterType)
		{
			continue;
		}

		const bool bRemove = IsEdgePlacedObject(Obj) ? Obj.Edge == SelectedEdge : true;
		if (bRemove)
		{
			RemovedIds.Add(Obj.ObjectId);
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

int32 AGridLevelEditorActor::RemoveObjectsConflictingWithPlacementInternal(EGridLevelObjectType NewObjectType, FName NewArchetypeId, bool bNewObjectOnEdge)
{
	(void)NewObjectType;
	(void)bNewObjectOnEdge;

	if (!HasValidLevelAsset() || !IsValidSelectedCell() || SelectedEdge == EGridEdge::None)
	{
		return 0;
	}

	const UGridObjectArchetypeAsset* NewArchetype = FindObjectArchetypeById(NewArchetypeId);
	if (!NewArchetype || !NewArchetype->OccupiesBoundary())
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
	const TArray<FGridLevelObjectData>& Objects = LevelAsset->GetObjectCompatibilityView();
	for (const FGridLevelObjectData& ExistingObject : Objects)
	{
		if (ExistingObject.Edge == EGridEdge::None)
		{
			continue;
		}

		const UGridObjectArchetypeAsset* ExistingArchetype = FindObjectArchetypeById(ExistingObject.ArchetypeId);
		if (!ExistingArchetype || !ExistingArchetype->OccupiesBoundary())
		{
			continue;
		}

		const FGridBoundaryKey ExistingBoundary =
			FGridBoundaryKey::MakeCanonical(ExistingObject.CellX, ExistingObject.CellY, ExistingObject.Edge);
		if (ExistingBoundary.IsValid() && ExistingBoundary == NewBoundary)
		{
			RemovedIds.Add(ExistingObject.ObjectId);
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
