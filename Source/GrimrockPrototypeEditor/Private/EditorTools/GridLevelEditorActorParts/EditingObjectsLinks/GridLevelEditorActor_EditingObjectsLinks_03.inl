}

Modify();
SelectedCellX = CellX;
SelectedCellY = CellY;
SelectedEdge = EGridEdge::None;
HoveredCellX = CellX;
HoveredCellY = CellY;
HoveredEdge = EGridEdge::None;
UpdateCoordinateHoverLabel();
return true;
}

EGridEdge AGridLevelEditorActor::GetEdgeFromPointInCell(const FVector2D& LocalInCell, float CellSize) const
{
	const float DistNorth = FMath::Abs(CellSize - LocalInCell.Y);
	const float DistEast = FMath::Abs(CellSize - LocalInCell.X);
	const float DistSouth = FMath::Abs(LocalInCell.Y);
	const float DistWest = FMath::Abs(LocalInCell.X);

	float BestDist = DistNorth;
	EGridEdge BestEdge = EGridEdge::North;

	if (DistEast < BestDist)
	{
		BestDist = DistEast;
		BestEdge = EGridEdge::East;
	}

	if (DistSouth < BestDist)
	{
		BestDist = DistSouth;
		BestEdge = EGridEdge::South;
	}

	if (DistWest < BestDist)
	{
		BestDist = DistWest;
		BestEdge = EGridEdge::West;
	}

	return BestEdge;
}

bool AGridLevelEditorActor::ApplyGridHoverFromWorldPoint(const FVector& WorldPoint)
{
	if (!HasValidLevelAsset())
	{
		HoveredCellX = INDEX_NONE;
		HoveredCellY = INDEX_NONE;
		HoveredEdge = EGridEdge::None;
		UpdateCoordinateHoverLabel();
		return false;
	}

	ResolvePreviewRuntimeActor();

	const float CellSize = LevelAsset->CellSize;
	if (CellSize <= KINDA_SMALL_NUMBER)
	{
		HoveredCellX = INDEX_NONE;
		HoveredCellY = INDEX_NONE;
		HoveredEdge = EGridEdge::None;
		UpdateCoordinateHoverLabel();
		return false;
	}

	FVector GridWorldOrigin = FVector::ZeroVector;
	if (PreviewRuntimeActor)
	{
		GridWorldOrigin = PreviewRuntimeActor->GetActorLocation() + PreviewRuntimeActor->GridOrigin;
	}

	const FVector Local = WorldPoint - GridWorldOrigin;

	const int32 NewCellX = FMath::FloorToInt(Local.X / CellSize);
	const int32 NewCellY = FMath::FloorToInt(Local.Y / CellSize);

	if (!LevelAsset->IsValidCoord(NewCellX, NewCellY))
	{
		HoveredCellX = INDEX_NONE;
		HoveredCellY = INDEX_NONE;
		HoveredEdge = EGridEdge::None;
		UpdateCoordinateHoverLabel();
		return false;
	}

	const float LocalInCellX = Local.X - (static_cast<float>(NewCellX) * CellSize);
	const float LocalInCellY = Local.Y - (static_cast<float>(NewCellY) * CellSize);

	HoveredCellX = NewCellX;
	HoveredCellY = NewCellY;
	HoveredEdge = GetEdgeFromPointInCell(FVector2D(LocalInCellX, LocalInCellY), CellSize);
	UpdateCoordinateHoverLabel();
	return true;
}

bool AGridLevelEditorActor::CommitHoveredCellSelection()
{
	if (!HasValidLevelAsset() || !LevelAsset->IsValidCoord(HoveredCellX, HoveredCellY))
	{
		return false;
	}

	const bool bSelectionChanged = SelectedCellX != HoveredCellX || SelectedCellY != HoveredCellY || SelectedEdge != HoveredEdge;

	if (bSelectionChanged)
	{
		Modify();
		SelectedCellX = HoveredCellX;
		SelectedCellY = HoveredCellY;
		SelectedEdge = HoveredEdge;
	}

	return true;
}

FVector AGridLevelEditorActor::GetSelectionPreviewCenter(float ZOffset) const
{
	return GetSelectedCellWorldCenter(ZOffset);
}

void AGridLevelEditorActor::ApplyPrimaryToolAction()
{
	switch (ActiveTool)
	{
		case EGridEditorTool::Select:
			if (!SelectHoveredObject())
			{
				SelectObjectAtSelection();
			}
			break;

		case EGridEditorTool::PaintCell:
			PaintSelectedCell();
			break;

		case EGridEditorTool::PaintWall:
			PaintSelectedWall();
			break;

		case EGridEditorTool::PaintObject:
			PlaceSelectedObject();
			break;

		case EGridEditorTool::Erase:
			EraseAtSelection();
			break;

		case EGridEditorTool::Link:
			SelectHoveredObject();
			BeginOrCompleteLinkAtSelection();
			break;

		default:
			break;
	}
}

void AGridLevelEditorActor::ApplySecondaryToolAction()
{
	switch (ActiveTool)
	{
		case EGridEditorTool::PaintCell:
			ClearSelectedCell();
			break;

		case EGridEditorTool::PaintWall:
			ClearSelectedWall();
			break;

		case EGridEditorTool::PaintObject:
			RemoveObjectsAtSelection();
			break;

		case EGridEditorTool::Link:
			ClearPendingLinkSource();
			break;

		case EGridEditorTool::Select:
		case EGridEditorTool::Erase:
		default:
			break;
	}
}

const FGridLevelObjectData* AGridLevelEditorActor::FindObjectAtSelection() const
{
	if (!HasValidLevelAsset() || !IsValidSelectedCell())
	{
		return nullptr;
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

		return &Obj;
	}
	return nullptr;
}

const FGridLevelObjectData* AGridLevelEditorActor::FindObjectById(const FGuid& ObjectId) const
{
	if (!HasValidLevelAsset() || !ObjectId.IsValid())
	{
		return nullptr;
	}

	for (const FGridLevelObjectData& Obj : LevelAsset->Objects)
