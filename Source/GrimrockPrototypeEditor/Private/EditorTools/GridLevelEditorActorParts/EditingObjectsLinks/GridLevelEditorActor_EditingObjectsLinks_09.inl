	{
		return false;
	}

	FGridLevelObjectData EditedObject = *SelectedObject;
	EditedObject.OverrideReadableText = NewReadableText;

#if WITH_EDITOR
	Modify();
	LevelAsset->Modify();
#endif

	if (!ApplyGridEditorObjectSnapshotToAuthority(LevelAsset, EditedObject))
	{
		return false;
	}

#if WITH_EDITOR
	LevelAsset->MarkPackageDirty();
#endif

	RebuildPreview();
	return true;
}

bool AGridLevelEditorActor::SetSelectedObjectInitiallyEnabled(bool bNewInitiallyEnabled)
{
	const FGridLevelObjectData* Obj = FindObjectById(LastSelectedObjectId);
	if (!Obj)
	{
		return false;
	}

	FGridLevelObjectData EditedObject = *Obj;
	EditedObject.bInitiallyEnabled = bNewInitiallyEnabled;

#if WITH_EDITOR
	LevelAsset->Modify();
#endif
	if (!ApplyGridEditorObjectSnapshotToAuthority(LevelAsset, EditedObject))
	{
		return false;
	}
	bObjectInitiallyEnabled = bNewInitiallyEnabled;
#if WITH_EDITOR
	LevelAsset->MarkPackageDirty();
#endif
	RebuildPreview();
	return true;
}

bool AGridLevelEditorActor::SetSelectedObjectInitiallyActive(bool bNewInitiallyActive)
{
	const FGridLevelObjectData* Obj = FindObjectById(LastSelectedObjectId);
	if (!Obj)
	{
		return false;
	}

	FGridLevelObjectData EditedObject = *Obj;
	EditedObject.bInitiallyActive = bNewInitiallyActive;

#if WITH_EDITOR
	LevelAsset->Modify();
#endif
	if (!ApplyGridEditorObjectSnapshotToAuthority(LevelAsset, EditedObject))
	{
		return false;
	}
	bObjectInitiallyActive = bNewInitiallyActive;
#if WITH_EDITOR
	LevelAsset->MarkPackageDirty();
#endif
	RebuildPreview();
	return true;
}

bool AGridLevelEditorActor::MoveSelectedObjectToCurrentSelection()
{
	if (!HasValidLevelAsset() || !IsValidSelectedCell())
	{
		UE_LOG(LogTemp, Warning, TEXT("GridLevelEditorActor: cannot move selected object, destination cell is invalid."));
		return false;
	}

	const FGridLevelObjectData* SelectedObject = FindObjectById(LastSelectedObjectId);
	if (!SelectedObject)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridLevelEditorActor: cannot move selected object, no object is selected."));
		return false;
	}

	const bool bRequiresEdge = IsEdgePlacedObject(*SelectedObject);
	if (bRequiresEdge && SelectedEdge == EGridEdge::None)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridLevelEditorActor: cannot move selected edge-based object to Edge=None."));
		return false;
	}

	const EGridEdge DestinationEdge = bRequiresEdge ? SelectedEdge : EGridEdge::None;
	const bool bAlreadyAtDestination =
		SelectedObject->CellX == SelectedCellX && SelectedObject->CellY == SelectedCellY && SelectedObject->Edge == DestinationEdge;
	if (bAlreadyAtDestination)
	{
		UE_LOG(LogTemp, Log, TEXT("GridLevelEditorActor: selected object is already at the current selection."));
		return true;
	}

	const FGuid SelectedObjectId = SelectedObject->ObjectId;
	const EGridLevelObjectType SelectedObjectType = SelectedObject->Type;
	TArray<FGridLevelObjectData> CompatibilityObjects;
	LevelAsset->BuildCompatibilityObjectProjectionFromTyped(CompatibilityObjects);
	const bool bDestinationOccupied = CompatibilityObjects.ContainsByPredicate(
		[this, SelectedObjectId, SelectedObjectType, bRequiresEdge, DestinationEdge](const FGridLevelObjectData& Obj)
		{
			if (Obj.ObjectId == SelectedObjectId || Obj.CellX != SelectedCellX || Obj.CellY != SelectedCellY || Obj.Type != SelectedObjectType)
			{
				return false;
			}

			if (bRequiresEdge)
			{
				return Obj.Edge == DestinationEdge;
			}

			return true;
		});
	if (bDestinationOccupied)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridLevelEditorActor: cannot move selected object, destination already contains an object of the same type."));
		return false;
	}

	FGridLevelObjectData EditedObject = *SelectedObject;
	EditedObject.CellX = SelectedCellX;
	EditedObject.CellY = SelectedCellY;
	EditedObject.Edge = DestinationEdge;

#if WITH_EDITOR
	LevelAsset->Modify();
#endif
	if (!ApplyGridEditorObjectSnapshotToAuthority(LevelAsset, EditedObject))
	{
		return false;
	}
#if WITH_EDITOR
	LevelAsset->MarkPackageDirty();
#endif

	RebuildPreview();
	UE_LOG(LogTemp, Log, TEXT("GridLevelEditorActor: moved selected object %s to X=%d Y=%d Edge=%d."), *SelectedObjectId.ToString(), SelectedCellX,
		SelectedCellY, static_cast<int32>(DestinationEdge));
	return true;
}
