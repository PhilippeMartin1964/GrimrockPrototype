bool AGridLevelEditorActor::SetSelectedObjectInitiallyEnabled(bool bNewInitiallyEnabled)
{
	if (!LevelAsset) return false;
	if (FGridMonsterSpawnInstance* MonsterSpawn = LevelAsset->FindMonsterSpawnInstanceById(LastSelectedObjectId))
	{
		LevelAsset->Modify();
		MonsterSpawn->bInitiallyEnabled = bNewInitiallyEnabled;
	}
	else if (FGridItemSpawnInstance* ItemSpawn = LevelAsset->FindItemSpawnInstanceById(LastSelectedObjectId))
	{
		LevelAsset->Modify();
		ItemSpawn->bInitiallyEnabled = bNewInitiallyEnabled;
	}
	else
	{
		return false;
	}
	bObjectInitiallyEnabled = bNewInitiallyEnabled;
	LevelAsset->MarkPackageDirty();
	RebuildPreview();
	return true;
}

bool AGridLevelEditorActor::SetSelectedObjectInitiallyActive(bool bNewInitiallyActive)
{
	if (!LevelAsset) return false;
	FGridWorldObjectInstance* WorldObjectInstance = LevelAsset->FindWorldObjectInstanceById(LastSelectedObjectId);
	if (!WorldObjectInstance) return false;

	LevelAsset->Modify();
	if (WorldObjectInstance->Type == EGridLevelObjectType::Door)
	{
		WorldObjectInstance->InstanceConfig.bDoorInitiallyOpen = bNewInitiallyActive;
	}
	else if (WorldObjectInstance->Type == EGridLevelObjectType::Teleporter)
	{
		WorldObjectInstance->InstanceConfig.bTeleporterInitiallyEnabled = bNewInitiallyActive;
	}
	else
	{
		return false;
	}
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
