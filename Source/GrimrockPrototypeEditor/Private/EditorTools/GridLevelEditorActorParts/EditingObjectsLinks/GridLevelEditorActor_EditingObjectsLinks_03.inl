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
#if WITH_EDITOR
			RunGridEditorTransaction(TEXT("Paint Grid Cell"), [this]() { PaintSelectedCell(); });
#else
			PaintSelectedCell();
#endif
			break;

		case EGridEditorTool::PaintWall:
#if WITH_EDITOR
			RunGridEditorTransaction(TEXT("Paint Grid Wall"), [this]() { PaintSelectedWall(); });
#else
			PaintSelectedWall();
#endif
			break;

		case EGridEditorTool::PaintObject:
#if WITH_EDITOR
			RunGridEditorTransaction(TEXT("Place Grid Object"), [this]() { PlaceSelectedObject(); });
#else
			PlaceSelectedObject();
#endif
			break;

		case EGridEditorTool::Erase:
		{
			TArray<FGuid> CandidateIds;
			if (HasValidLevelAsset() && IsValidSelectedCell())
			{
				for (const FGuid& ObjectId : LevelAsset->GetTypedPlacementIdsAtCell(SelectedCellX, SelectedCellY))
				{
					int32 CellX = INDEX_NONE;
					int32 CellY = INDEX_NONE;
					EGridEdge Edge = EGridEdge::None;
					if (LevelAsset->TryGetTypedPlacementLocation(ObjectId, CellX, CellY, Edge) &&
						(!IsEdgePlacedObject(ObjectId) || Edge == SelectedEdge))
					{
						CandidateIds.Add(ObjectId);
					}
				}
			}

#if WITH_EDITOR
			if (CandidateIds.Num() > 0)
			{
				HandleTargetedObjectErase(*this, CandidateIds);
				break;
			}
			RunGridEditorTransaction(TEXT("Erase Grid Element"), [this]() { EraseAtSelection(); });
#else
			EraseAtSelection();
#endif
			break;
		}

		case EGridEditorTool::Link:
			SelectHoveredObject();
#if WITH_EDITOR
			RunGridEditorTransaction(TEXT("Create Grid Link"), [this]() { BeginOrCompleteLinkAtSelection(); });
#else
			BeginOrCompleteLinkAtSelection();
#endif
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
#if WITH_EDITOR
			RunGridEditorTransaction(TEXT("Clear Grid Cell"), [this]() { ClearSelectedCell(); });
#else
			ClearSelectedCell();
#endif
			break;

		case EGridEditorTool::PaintWall:
#if WITH_EDITOR
			RunGridEditorTransaction(TEXT("Clear Grid Wall"), [this]() { ClearSelectedWall(); });
#else
			ClearSelectedWall();
#endif
			break;

		case EGridEditorTool::PaintObject:
#if WITH_EDITOR
			RunGridEditorTransaction(TEXT("Remove Grid Objects"), [this]() { RemoveObjectsAtSelection(); });
#else
			RemoveObjectsAtSelection();
#endif
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

FGuid AGridLevelEditorActor::FindObjectIdAtSelection() const
{
	if (!HasValidLevelAsset() || !IsValidSelectedCell()) return FGuid();
	const TArray<FGuid> ObjectIds = LevelAsset->GetTypedPlacementIdsAtCell(SelectedCellX, SelectedCellY);
	for (int32 Index = ObjectIds.Num() - 1; Index >= 0; --Index)
	{
		int32 CellX, CellY;
		EGridEdge Edge;
		const FGuid ObjectId = ObjectIds[Index];
		if (LevelAsset->TryGetTypedPlacementLocation(ObjectId, CellX, CellY, Edge) &&
			(!IsEdgePlacedObject(ObjectId) || Edge == SelectedEdge)) return ObjectId;
	}
	return FGuid();
}
