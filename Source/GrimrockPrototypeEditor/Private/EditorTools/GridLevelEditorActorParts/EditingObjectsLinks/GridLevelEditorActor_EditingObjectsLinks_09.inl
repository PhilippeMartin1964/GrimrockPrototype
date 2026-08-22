    {
        return false;
    }

#if WITH_EDITOR
    Modify ();
    LevelAsset->Modify ();
#endif

    SelectedObject->OverrideReadableText = NewReadableText;

#if WITH_EDITOR
    LevelAsset->MarkPackageDirty ();
#endif

    RebuildPreview ();
    return true;
}

bool AGridLevelEditorActor::SetSelectedObjectInitiallyEnabled (bool bNewInitiallyEnabled)
{
    FGridLevelObjectData* Obj = FindSelectedObjectMutable ();
    if (!Obj)
    {
        return false;
    }

#if WITH_EDITOR
    LevelAsset->Modify ();
#endif

    Obj->bInitiallyEnabled = bNewInitiallyEnabled;
    bObjectInitiallyEnabled = bNewInitiallyEnabled;

#if WITH_EDITOR
    LevelAsset->MarkPackageDirty ();
#endif

    RebuildPreview ();
    return true;
}

bool AGridLevelEditorActor::SetSelectedObjectInitiallyActive (bool bNewInitiallyActive)
{
    FGridLevelObjectData* Obj = FindSelectedObjectMutable ();
    if (!Obj)
    {
        return false;
    }

#if WITH_EDITOR
    LevelAsset->Modify ();
#endif

    Obj->bInitiallyActive = bNewInitiallyActive;
    bObjectInitiallyActive = bNewInitiallyActive;

#if WITH_EDITOR
    LevelAsset->MarkPackageDirty ();
#endif

    RebuildPreview ();
    return true;
}

bool AGridLevelEditorActor::MoveSelectedObjectToCurrentSelection ()
{
    if (!HasValidLevelAsset () || !IsValidSelectedCell ())
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridLevelEditorActor: cannot move selected object, destination cell is invalid."));
        return false;
    }

    FGridLevelObjectData* SelectedObject = FindSelectedObjectMutable ();
    if (!SelectedObject)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridLevelEditorActor: cannot move selected object, no object is selected."));
        return false;
    }

    const bool bRequiresEdge = IsEdgePlacedObject (*SelectedObject);
    if (bRequiresEdge && SelectedEdge == EGridEdge::None)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridLevelEditorActor: cannot move selected edge-based object to Edge=None."));
        return false;
    }

    const EGridEdge DestinationEdge = bRequiresEdge ? SelectedEdge : EGridEdge::None;
    const bool bAlreadyAtDestination = SelectedObject->CellX == SelectedCellX &&
        SelectedObject->CellY == SelectedCellY &&
        SelectedObject->Edge == DestinationEdge;

    if (bAlreadyAtDestination)
    {
        UE_LOG (LogTemp, Log, TEXT ("GridLevelEditorActor: selected object is already at the current selection."));
        return true;
    }

    const FGuid SelectedObjectId = SelectedObject->ObjectId;
    const EGridLevelObjectType SelectedObjectType = SelectedObject->Type;
    const bool bDestinationOccupied = LevelAsset->Objects.ContainsByPredicate (
        [this, SelectedObjectId, SelectedObjectType, bRequiresEdge, DestinationEdge] (const FGridLevelObjectData& Obj)
    {
        if (Obj.ObjectId == SelectedObjectId ||
            Obj.CellX != SelectedCellX ||
            Obj.CellY != SelectedCellY ||
            Obj.Type != SelectedObjectType)
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
        UE_LOG (
            LogTemp,
            Warning,
            TEXT ("GridLevelEditorActor: cannot move selected object, destination already contains an object of the same type."));
        return false;
    }

#if WITH_EDITOR
    LevelAsset->Modify ();
#endif

    SelectedObject->CellX = SelectedCellX;
    SelectedObject->CellY = SelectedCellY;
    SelectedObject->Edge = DestinationEdge;

#if WITH_EDITOR
    LevelAsset->MarkPackageDirty ();
#endif

    RebuildPreview ();
    UE_LOG (
        LogTemp,
        Log,
        TEXT ("GridLevelEditorActor: moved selected object %s to X=%d Y=%d Edge=%d."),
        *SelectedObjectId.ToString (),
        SelectedCellX,
        SelectedCellY,
        static_cast<int32> (DestinationEdge));
    return true;
}

