bool AGridLevelEditorActor::SetSelectedObjectInitiallyEnabled(bool bNewInitiallyEnabled)
{
	if (!EditGridPlacementAuthoring(LevelAsset, LastSelectedObjectId,
		[bNewInitiallyEnabled](auto& Placement) { Placement.bInitiallyEnabled = bNewInitiallyEnabled; })) return false;
	bObjectInitiallyEnabled = bNewInitiallyEnabled;
	RebuildPreview();
	return true;
}

bool AGridLevelEditorActor::SetSelectedObjectInitiallyActive(bool bNewInitiallyActive)
{
	if (!LevelAsset) return false;
	if (FGridWorldObjectInstance* WorldObjectInstance = LevelAsset->FindWorldObjectInstanceById(LastSelectedObjectId))
	{
		LevelAsset->Modify();
		WorldObjectInstance->bInitiallyActive = bNewInitiallyActive;
	}
	else if (FGridLogicObjectInstance* LogicInstance = LevelAsset->FindLogicObjectInstanceById(LastSelectedObjectId))
	{
		LevelAsset->Modify();
		LogicInstance->bInitiallyActive = bNewInitiallyActive;
	}
	else return false;
	bObjectInitiallyActive = bNewInitiallyActive;
	LevelAsset->MarkPackageDirty();
	RebuildPreview();
	return true;
}

bool AGridLevelEditorActor::MoveSelectedObjectToCurrentSelection()
{
	if (!HasValidLevelAsset() || !IsValidSelectedCell()) return false;
	int32 CurrentCellX, CurrentCellY;
	EGridEdge CurrentEdge;
	if (!LevelAsset->TryGetTypedPlacementLocation(LastSelectedObjectId, CurrentCellX, CurrentCellY, CurrentEdge)) return false;
	const bool bRequiresEdge = IsEdgePlacedObject(LastSelectedObjectId);
	if (bRequiresEdge && SelectedEdge == EGridEdge::None) return false;
	const EGridEdge DestinationEdge = bRequiresEdge ? SelectedEdge : EGridEdge::None;
	if (CurrentCellX == SelectedCellX && CurrentCellY == SelectedCellY && CurrentEdge == DestinationEdge) return true;
	const EGridLevelObjectType SelectedObjectType = LevelAsset->GetTypedPlacementType(LastSelectedObjectId);
	for (const FGuid& ObjectId : LevelAsset->GetTypedPlacementIdsAtCell(SelectedCellX, SelectedCellY))
	{
		if (ObjectId == LastSelectedObjectId || LevelAsset->GetTypedPlacementType(ObjectId) != SelectedObjectType) continue;
		int32 CellX, CellY;
		EGridEdge Edge;
		if (LevelAsset->TryGetTypedPlacementLocation(ObjectId, CellX, CellY, Edge) && (!bRequiresEdge || Edge == DestinationEdge))
		{
			UE_LOG(LogTemp, Warning, TEXT("GridLevelEditorActor: cannot move selected object, destination already contains an object of the same type."));
			return false;
		}
	}
	if (!EditGridPlacementAuthoring(LevelAsset, LastSelectedObjectId, [this](auto& Placement)
		{ Placement.CellX = SelectedCellX; Placement.CellY = SelectedCellY; })) return false;
	if (FGridWorldObjectInstance* WorldObjectInstance = LevelAsset->FindWorldObjectInstanceById(LastSelectedObjectId)) WorldObjectInstance->WallSide = DestinationEdge;
	else if (FGridLooseItemInstance* LooseItemInstance = LevelAsset->FindLooseItemInstanceById(LastSelectedObjectId)) LooseItemInstance->SurfaceSide = DestinationEdge;
	RebuildPreview();
	return true;
}
