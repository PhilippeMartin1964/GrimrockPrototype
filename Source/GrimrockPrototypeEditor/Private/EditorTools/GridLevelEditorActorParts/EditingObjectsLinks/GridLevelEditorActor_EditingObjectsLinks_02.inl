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
	const UGridObjectArchetypeAsset* ObjectArchetype = FindObjectArchetypeById(ObjectArchetypeId);
	const bool bSuppressBaseWall = ObjectArchetype && ObjectArchetype->SuppressesBaseWall();
	const bool bIsStoneAlcoveReceptacle = ObjectArchetypeId == FName(TEXT("Receptacle_Alcove_Stone"));
	const bool bPlaceObjectOnEdge = ObjectArchetype ? ObjectArchetype->PlacementSurface == EGridObjectPlacementKind::Wall
											 : IsEdgePlacedObject(PaintObjectType, ObjectArchetypeId);
	if (bPlaceObjectOnEdge && SelectedEdge == EGridEdge::None)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridLevelEditorActor: this object type requires a valid edge."));
		return;
	}
	if (ObjectArchetype)
	{
		RemoveObjectsConflictingWithPlacementInternal(PaintObjectType, ObjectArchetypeId, bPlaceObjectOnEdge);
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
	FGridLevelObjectData NewObject;
	NewObject.Type = PaintObjectType;
	NewObject.CellX = SelectedCellX;
	NewObject.CellY = SelectedCellY;
	NewObject.Edge = bPlaceObjectOnEdge ? SelectedEdge : EGridEdge::None;
	NewObject.LocalYaw = 0.f;
	NewObject.InitialFacing = NewObject.Type == EGridLevelObjectType::MonsterSpawn ? EGridEdge::North : EGridEdge::None;
	NewObject.ArchetypeId = ObjectArchetypeId;
	NewObject.bInitiallyEnabled = bObjectInitiallyEnabled;
	NewObject.bInitiallyActive = bObjectInitiallyActive;
	NewObject.Tag = ObjectTag;
	NewObject.Notes = ObjectNotes;
	NewObject.PaletteEntryId = SelectedPaletteEntryId;
	NewObject.Behavior = ObjectBehavior;
	if (NewObject.Type == EGridLevelObjectType::MonsterSpawn && ObjectPalette)
	{
		if (const FGridObjectPaletteEntry* PaletteEntry = ObjectPalette->FindEntryById(SelectedPaletteEntryId))
		{
			NewObject.MonsterDefinitionAsset = PaletteEntry->DefaultMonsterDefinition;
		}
	}
	if (NewObject.Type == EGridLevelObjectType::StoryCompanion && ObjectPalette)
	{
		if (const FGridObjectPaletteEntry* PaletteEntry = ObjectPalette->FindEntryById(SelectedPaletteEntryId))
		{
			NewObject.StoryCompanionDefinition = PaletteEntry->DefaultStoryCompanionDefinition;
		}
	}
	if (NewObject.Type == EGridLevelObjectType::Item)
	{
		// Current authoring schema: a placed item owns a direct definition asset
		// reference. ItemDefinitionId is runtime/save identity and must not become
		// a second authoring authority.
		NewObject.ItemDefinitionAsset = ObjectBehavior.Item.ItemDefinitionAsset;
		NewObject.ItemDefinitionId = NAME_None;
		NewObject.ReadableContentAsset = ObjectBehavior.Item.DefaultReadableContentAsset;
		NewObject.ReadableContentId = ObjectBehavior.Item.DefaultReadableContentId;
		NewObject.ReadTitleOverride = ObjectBehavior.Item.DefaultReadTitleOverride;
		NewObject.ReadTextOverride = ObjectBehavior.Item.DefaultReadTextOverride;
	}

	if (bIsStoneAlcoveReceptacle)
	{
		NewObject.Type = EGridLevelObjectType::Receptacle;
		NewObject.bInitiallyEnabled = true;
		NewObject.bInitiallyActive = NewObject.Behavior.Receptacle.InitialContent.Num() > 0;
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
	const FGuid NewId = LevelAsset->AddObject(NewObject);
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

	for (int32 Index = LevelAsset->Objects.Num() - 1; Index >= 0; --Index)
	{
		const FGridLevelObjectData& Obj = LevelAsset->Objects[Index];

		if (Obj.CellX != SelectedCellX || Obj.CellY != SelectedCellY)
		{
			continue;
		}

		if (IsEdgePlacedObject(Obj) && Obj.Edge != SelectedEdge)
		{
			continue;
		}
		LastSelectedObjectId = Obj.ObjectId;
		PaintObjectType = Obj.Type;
		SelectedEdge = Obj.Edge;
		bObjectInitiallyEnabled = Obj.bInitiallyEnabled;
		bObjectInitiallyActive = Obj.bInitiallyActive;
		ObjectArchetypeId = Obj.ArchetypeId;
		ObjectTag = Obj.Tag;
		ObjectNotes = Obj.Notes;
		SelectedPaletteEntryId = Obj.PaletteEntryId;
		ObjectBehavior = Obj.Behavior;
		return;
	}

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