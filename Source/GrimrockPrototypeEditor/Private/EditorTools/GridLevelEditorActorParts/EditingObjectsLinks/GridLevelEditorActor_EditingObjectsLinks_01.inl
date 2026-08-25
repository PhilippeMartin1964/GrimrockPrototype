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
	for (int32 Index = LevelAsset->Objects.Num() - 1; Index >= 0; --Index)
	{
		const FGridLevelObjectData& Obj = LevelAsset->Objects[Index];
		if (Obj.CellX != SelectedCellX || Obj.CellY != SelectedCellY)
		{
			continue;
		}
		if (bSameTypeOnly && Obj.Type != FilterType)
		{
			continue;
		}
		bool bRemove = false;
		if (IsEdgePlacedObject(Obj))
		{
			bRemove = (Obj.Edge == SelectedEdge);
		}
		else
		{
			bRemove = true;
		}
		if (bRemove)
		{
			RemovedIds.Add(Obj.ObjectId);
			LevelAsset->Objects.RemoveAt(Index);
		}
	}
	if (RemovedIds.Num() > 0)
	{
		LevelAsset->Links.RemoveAll(
			[&](const FGridObjectLink& Link)
			{
				return RemovedIds.Contains(Link.SourceObjectId) || RemovedIds.Contains(Link.TargetObjectId);
			});
	}

#if WITH_EDITOR
	LevelAsset->MarkPackageDirty();
#endif

	return RemovedIds.Num();
}

int32 AGridLevelEditorActor::RemoveObjectsConflictingWithPlacementInternal(EGridLevelObjectType NewObjectType, FName NewArchetypeId, bool bNewObjectOnEdge)
{
	if (!HasValidLevelAsset() || !IsValidSelectedCell())
	{
		return 0;
	}
	const UGridObjectArchetypeAsset* NewArchetype = FindObjectArchetypeById(NewArchetypeId);
	const bool bNewCanShareCell = NewArchetype ? NewArchetype->bCanShareCell : true;
	const bool bNewCanShareAnchor = NewArchetype ? NewArchetype->bCanShareAnchor : true;
	TArray<int32> IndicesToRemove;
	TArray<FGuid> RemovedIds;
	for (int32 Index = LevelAsset->Objects.Num() - 1; Index >= 0; --Index)
	{
		const FGridLevelObjectData& ExistingObject = LevelAsset->Objects[Index];
		if (ExistingObject.CellX != SelectedCellX || ExistingObject.CellY != SelectedCellY)
		{
