#include "Core/GridObjectInstanceBehavior.h"

void AGridLevelEditorActor::PlaceSelectedObject()
{
	if (!HasValidLevelAsset() || !IsValidSelectedCell())
	{
		UE_LOG(LogTemp, Warning, TEXT("GridLevelEditorActor: invalid LevelAsset or selected cell."));
		return;
	}
	if (PaintObjectType == EGridLevelObjectType::None)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridLevelEditorActor: PaintObjectType is None."));
		return;
	}
	const UGridWorldObjectDefinitionAsset* WorldObjectDefinition = FindWorldObjectDefinitionById(WorldObjectDefinitionId);
	const bool bSuppressBaseWall = WorldObjectDefinition && WorldObjectDefinition->SuppressesBaseWall();
	const bool bIsStoneAlcoveReceptacle = WorldObjectDefinitionId == FName(TEXT("Receptacle_Alcove_Stone"));
	const bool bPlaceObjectOnEdge = WorldObjectDefinition ? WorldObjectDefinition->PlacementSurface == EGridObjectPlacementKind::Wall
											 : IsEdgePlacedObject(PaintObjectType, WorldObjectDefinitionId);
	if (bPlaceObjectOnEdge && SelectedEdge == EGridEdge::None)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridLevelEditorActor: this object type requires a valid edge."));
		return;
	}
	if (WorldObjectDefinition)
	{
		RemoveObjectsConflictingWithPlacementInternal(PaintObjectType, WorldObjectDefinitionId, bPlaceObjectOnEdge);
	}
	else
	{
		if (PlacementPolicy == EGridEditorObjectPlacementPolicy::ReplaceSameSlotOnly)
		{
			RemoveObjectsAtSelectionInternal(true);
		}
		else
		{
			RemoveObjectsAtSelectionInternal(false);
		}
	}
	LevelAsset->Modify();
	const FGuid NewId = FGuid::NewGuid();
	// Presence is implied by placement. Shared authoring initializes only spatial/metadata fields.
	const auto InitializeAuthoring = [this](auto& Placement)
	{
		Placement.CellX = SelectedCellX;
		Placement.CellY = SelectedCellY;
		Placement.Notes = ObjectNotes;
		Placement.PaletteEntryId = SelectedPaletteEntryId;
	};
	const FGridObjectPaletteEntry* PaletteEntry = ObjectPalette ? ObjectPalette->FindEntryById(SelectedPaletteEntryId) : nullptr;
	if (PaintObjectType == EGridLevelObjectType::Item)
	{
		FGridLooseItemInstance& LooseItemInstance = LevelAsset->LooseItemInstances.AddDefaulted_GetRef();
		InitializeAuthoring(LooseItemInstance);
		LooseItemInstance.InstanceId = NewId;
		LooseItemInstance.SurfaceSide = bPlaceObjectOnEdge ? SelectedEdge : EGridEdge::None;
		LooseItemInstance.ItemDefinition = ObjectBehavior.Item.ItemDefinitionAsset;
		LooseItemInstance.ReadableContentAsset = ObjectBehavior.Item.DefaultReadableContentAsset;
		LooseItemInstance.ReadableContentId = ObjectBehavior.Item.DefaultReadableContentId;
		LooseItemInstance.ReadTitleOverride = ObjectBehavior.Item.DefaultReadTitleOverride;
		LooseItemInstance.ReadTextOverride = ObjectBehavior.Item.DefaultReadTextOverride;
	}
	else if (PaintObjectType == EGridLevelObjectType::MonsterSpawn)
	{
		FGridMonsterSpawnInstance& MonsterSpawn = LevelAsset->MonsterSpawns.AddDefaulted_GetRef();
		InitializeAuthoring(MonsterSpawn);
		MonsterSpawn.SpawnId = NewId;
		MonsterSpawn.MonsterDefinition = PaletteEntry ? PaletteEntry->DefaultMonsterDefinition : nullptr;
		MonsterSpawn.bSpawnAtStart = bObjectInitiallyEnabled;
	}
	else if (PaintObjectType == EGridLevelObjectType::ItemSpawn)
	{
		FGridItemSpawnInstance& ItemSpawn = LevelAsset->ItemSpawns.AddDefaulted_GetRef();
		InitializeAuthoring(ItemSpawn);
		ItemSpawn.SpawnId = NewId;
		ItemSpawn.ItemDefinition = ObjectBehavior.Item.ItemDefinitionAsset;
		ItemSpawn.bSpawnAtStart = bObjectInitiallyEnabled;
	}
	else if (PaintObjectType == EGridLevelObjectType::Logic || PaintObjectType == EGridLevelObjectType::StoryCompanion ||
		PaintObjectType == EGridLevelObjectType::CustomRecruiter)
	{
		FGridLogicObjectInstance& LogicInstance = LevelAsset->LogicObjects.AddDefaulted_GetRef();
		InitializeAuthoring(LogicInstance);
		LogicInstance.InstanceId = NewId;
		LogicInstance.Type = PaintObjectType;
		LogicInstance.StoryCompanionDefinition = PaletteEntry ? PaletteEntry->DefaultStoryCompanionDefinition : nullptr;
	}
	else
	{
		FGridWorldObjectInstance& WorldObjectInstance = LevelAsset->WorldObjectInstances.AddDefaulted_GetRef();
		InitializeAuthoring(WorldObjectInstance);
		WorldObjectInstance.InstanceId = NewId;
		WorldObjectInstance.Type = bIsStoneAlcoveReceptacle ? EGridLevelObjectType::Receptacle : PaintObjectType;
		WorldObjectInstance.WorldObjectDefinitionId = WorldObjectDefinitionId;
		WorldObjectInstance.WallSide = bPlaceObjectOnEdge ? SelectedEdge : EGridEdge::None;
		WorldObjectInstance.InstanceConfig.Teleporter = ObjectBehavior.Teleporter;
		WorldObjectInstance.InstanceConfig.Transition = ObjectBehavior.Transition;
		WorldObjectInstance.InstanceConfig.Pit = ObjectBehavior.Pit;
		WorldObjectInstance.InstanceConfig.ReceptacleInitialContent = ObjectBehavior.Receptacle.InitialContent;
		WorldObjectInstance.InstanceConfig.bStartsUnlocked = ObjectBehavior.Lock.bStartsUnlocked;
	}

	if (bSuppressBaseWall)
	{
		if (FGridLevelCellData* CellData = GetSelectedCellMutable())
		{
			if (EGridWallType* WallPtr = GetSelectedWallMutable(*CellData))
			{
#if WITH_EDITOR
				LevelAsset->Modify();
#endif
				*WallPtr = EGridWallType::Solid;
#if WITH_EDITOR
				LevelAsset->MarkPackageDirty();
#endif
			}
		}
	}
	LevelAsset->MarkPackageDirty();
	LastSelectedObjectId = NewId;
	RebuildPreview();
}

void AGridLevelEditorActor::RemoveObjectsAtSelection()
{
	if (!HasValidLevelAsset() || !IsValidSelectedCell())
	{
		UE_LOG(LogTemp, Warning, TEXT("GridLevelEditorActor: invalid LevelAsset or selected cell."));
		return;
	}

	RemoveObjectsAtSelectionInternal(false);
	LastSelectedObjectId.Invalidate();
	RebuildPreview();
}

void AGridLevelEditorActor::SelectObjectAtSelection()
{
	ClearSelectedObjectState();

	if (!HasValidLevelAsset() || !IsValidSelectedCell())
	{
		UE_LOG(LogTemp, Warning, TEXT("GridLevelEditorActor: invalid LevelAsset or selected cell."));
		return;
	}

	const FGuid ObjectId = FindObjectIdAtSelection();
	if (ObjectId.IsValid() && SelectObjectById(ObjectId)) return;

	ClearSelectedObjectState();
	UE_LOG(LogTemp, Log, TEXT("GridLevelEditorActor: no object found at current selection."));
}

bool AGridLevelEditorActor::TryConvertWorldHitToSelection(const FVector& WorldHitLocation, const FVector& /*HitNormal*/)
{
	return ApplyGridHoverFromWorldPoint(WorldHitLocation) && CommitHoveredCellSelection();
}

bool AGridLevelEditorActor::ApplyViewportHitSelection(const FVector& WorldHitLocation, const FVector& HitNormal)
{
	return TryConvertWorldHitToSelection(WorldHitLocation, HitNormal);
}

bool AGridLevelEditorActor::IsSelectionValidForEditing() const
{
	return HasValidLevelAsset() && IsValidSelectedCell();
}

bool AGridLevelEditorActor::SelectCellFromOverview(int32 CellX, int32 CellY)
{
	if (!HasValidLevelAsset() || !LevelAsset->IsValidCoord(CellX, CellY))
	{
		UE_LOG(LogTemp, Warning, TEXT("GridLevelEditorActor: overview cell selection is outside grid bounds X=%d Y=%d."), CellX, CellY);
		return false;