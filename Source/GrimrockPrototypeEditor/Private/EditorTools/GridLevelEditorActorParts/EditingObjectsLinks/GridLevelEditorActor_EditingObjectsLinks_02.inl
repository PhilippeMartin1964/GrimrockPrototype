continue;
}
const UGridObjectArchetypeAsset* ExistingArchetype = FindObjectArchetypeById(ExistingObject.ArchetypeId);
const bool bExistingCanShareCell = ExistingArchetype ? ExistingArchetype->bCanShareCell : true;
const bool bExistingCanShareAnchor = ExistingArchetype ? ExistingArchetype->bCanShareAnchor : true;
const bool bExistingObjectOnEdge = IsEdgePlacedObject(ExistingObject);
const bool bSameAnchor = bNewObjectOnEdge && bExistingObjectOnEdge ? ExistingObject.Edge == SelectedEdge : !bNewObjectOnEdge && !bExistingObjectOnEdge;
bool bShouldRemove = false;
if (!bNewCanShareCell || !bExistingCanShareCell)
{
	bShouldRemove = true;
}
else if (bSameAnchor && (!bNewCanShareAnchor || !bExistingCanShareAnchor))
{
	bShouldRemove = true;
}
if (bShouldRemove)
{
	RemovedIds.Add(ExistingObject.ObjectId);
	IndicesToRemove.Add(Index);
}
}
if (IndicesToRemove.Num() == 0)
{
	return 0;
}
#if WITH_EDITOR
LevelAsset->Modify();
#endif
for (int32 IndexToRemove : IndicesToRemove)
{
	LevelAsset->Objects.RemoveAt(IndexToRemove);
}
LevelAsset->Links.RemoveAll(
	[&](const FGridObjectLink& Link)
	{
		return RemovedIds.Contains(Link.SourceObjectId) || RemovedIds.Contains(Link.TargetObjectId);
	});
#if WITH_EDITOR
LevelAsset->MarkPackageDirty();
#endif
return RemovedIds.Num();
}

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
	const bool bIsWallReplacingObject = ObjectArchetype && ObjectArchetype->bReplacesStandardWall;
	const bool bIsStoneAlcoveReceptacle = ObjectArchetypeId == FName(TEXT("Receptacle_Alcove_Stone"));
	const bool bPlaceObjectOnEdge = bIsWallReplacingObject || IsEdgePlacedObject(PaintObjectType, ObjectArchetypeId);
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
			if (NewObject.MonsterDefinitionAsset)
			{
				NewObject.MonsterDefinitionId = NewObject.MonsterDefinitionAsset->MonsterId;
			}
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

	if (bIsWallReplacingObject)
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
