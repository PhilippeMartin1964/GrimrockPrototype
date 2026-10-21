#include "Core/GridObjectInstanceBehavior.h"

void AGridLevelEditorActor::PlaceSelectedObject()
{
	if (!HasValidLevelAsset() || !IsValidSelectedCell())
	{
		UE_LOG(LogGridEditor, Warning, TEXT("GridLevelEditorActor: invalid LevelAsset or selected cell."));
		return;
	}
	if (PaintObjectType == EGridLevelObjectType::None)
	{
		UE_LOG(LogGridEditor, Warning, TEXT("GridLevelEditorActor: PaintObjectType is None."));
		return;
	}
	const UGridWorldObjectDefinitionAsset* WorldObjectDefinition = FindWorldObjectDefinitionById(WorldObjectDefinitionId);
	const bool bSuppressBaseWall = WorldObjectDefinition && WorldObjectDefinition->SuppressesBaseWall();
	const bool bIsStoneAlcoveReceptacle = WorldObjectDefinitionId == FName(TEXT("Receptacle_Alcove_Stone"));
	const bool bPlaceObjectOnEdge = WorldObjectDefinition ? WorldObjectDefinition->PlacementSurface == EGridObjectPlacementKind::Wall
											 : IsEdgePlacedObject(PaintObjectType, WorldObjectDefinitionId);
	if (bPlaceObjectOnEdge && SelectedEdge == EGridEdge::None)
	{
		UE_LOG(LogGridEditor, Warning, TEXT("GridLevelEditorActor: this object type requires a valid edge."));
		return;
	}
	if (WorldObjectDefinition)
	{
		RemoveObjectsConflictingWithPlacementInternal(PaintObjectType, WorldObjectDefinitionId, bPlaceObjectOnEdge);
	}
	else if (PaintObjectType != EGridLevelObjectType::Item)
	{
		// ITEMSTACK01.2: loose items always accumulate. Deletion requires the
		// explicit Erase/Remove command, regardless of the placement policy.
		RemoveObjectsAtSelectionInternal(PlacementPolicy == EGridEditorObjectPlacementPolicy::ReplaceSameSlotOnly);
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
		// Select a free authored floor position BEFORE appending the new item.
		// The LevelAsset stores every item independently; older positions remain
		// untouched. Real meshes/Chaos still determine final runtime settling.
		FVector2D NewFloorOffset = FVector2D::ZeroVector;
		if (!bPlaceObjectOnEdge)
		{
			TArray<FVector2D> OccupiedOffsets;
			for (const FGridLooseItemInstance& Existing : LevelAsset->LooseItemInstances)
			{
				if (Existing.CellX == SelectedCellX && Existing.CellY == SelectedCellY && Existing.SurfaceSide == EGridEdge::None)
				{
					OccupiedOffsets.Emplace(Existing.LocalOffset.X, Existing.LocalOffset.Y);
				}
			}

			if (!OccupiedOffsets.IsEmpty())
			{
				const float Step = LevelAsset->CellSize * 0.06f;
				float BestClearanceSq = -1.0f;
				// 9 x 9 candidate points inside the middle 48% of the cell.
				for (int32 Y = -4; Y <= 4; ++Y)
				{
					for (int32 X = -4; X <= 4; ++X)
					{
						const FVector2D Candidate(X * Step, Y * Step);
						float ClearanceSq = TNumericLimits<float>::Max();
						for (const FVector2D& Occupied : OccupiedOffsets)
						{
							ClearanceSq = FMath::Min(ClearanceSq, FVector2D::DistSquared(Candidate, Occupied));
						}
						if (ClearanceSq > BestClearanceSq)
						{
							BestClearanceSq = ClearanceSq;
							NewFloorOffset = Candidate;
						}
					}
				}
			}
		}

		FGridLooseItemInstance& LooseItemInstance = LevelAsset->LooseItemInstances.AddDefaulted_GetRef();
		InitializeAuthoring(LooseItemInstance);
		LooseItemInstance.LocalOffset = FVector(NewFloorOffset.X, NewFloorOffset.Y, 0.0f);
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
		WorldObjectInstance.InstanceConfig.Relocation = ObjectBehavior.Relocation;
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
		UE_LOG(LogGridEditor, Warning, TEXT("GridLevelEditorActor: invalid LevelAsset or selected cell."));
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
		UE_LOG(LogGridEditor, Warning, TEXT("GridLevelEditorActor: invalid LevelAsset or selected cell."));
		return;
	}

	const FGuid ObjectId = FindObjectIdAtSelection();
	if (ObjectId.IsValid() && SelectObjectById(ObjectId)) return;

	ClearSelectedObjectState();
	UE_LOG(LogGridEditor, Verbose, TEXT("GridLevelEditorActor: no object found at current selection."));
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
		UE_LOG(LogGridEditor, Warning, TEXT("GridLevelEditorActor: overview cell selection is outside grid bounds X=%d Y=%d."), CellX, CellY);
		return false;