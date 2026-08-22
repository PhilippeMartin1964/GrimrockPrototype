bool AGridLevelEditorActor::HasAnyObjectInSelectedCell () const
{
    if (!HasValidLevelAsset () || !IsValidSelectedCell ())
    {
        return false;
    }

    for (const FGridLevelObjectData& Obj : LevelAsset->Objects)
    {
        if (Obj.CellX == SelectedCellX && Obj.CellY == SelectedCellY)
        {
            return true;
        }
    }

    return false;
}

bool AGridLevelEditorActor::HasAnyWallInSelectedCell () const
{
    if (!HasValidLevelAsset () || !IsValidSelectedCell ())
    {
        return false;
    }

    const FGridLevelCellData& CellData = LevelAsset->GetCell (SelectedCellX, SelectedCellY);

    return CellData.NorthWall != EGridWallType::None ||
        CellData.EastWall != EGridWallType::None ||
        CellData.SouthWall != EGridWallType::None ||
        CellData.WestWall != EGridWallType::None;
}

void AGridLevelEditorActor::EraseAtSelection ()
{
    if (!HasValidLevelAsset () || !IsValidSelectedCell ())
    {
        return;
    }

    const int32 RemovedObjectCount = RemoveObjectsAtSelectionInternal (false);

    if (RemovedObjectCount > 0)
    {
        LastSelectedObjectId.Invalidate ();
        RebuildPreview ();
        return;
    }

    if (FGridLevelCellData* CellData = GetSelectedCellMutable ())
    {
        if (EGridWallType* WallPtr = GetSelectedWallMutable (*CellData))
        {
            if (*WallPtr != EGridWallType::None)
            {
                ClearSelectedWall ();
                return;
            }
        }

        if (CellData->CellType != EGridCellType::Empty &&
            !HasAnyObjectInSelectedCell () &&
            !HasAnyWallInSelectedCell ())
        {
            ClearSelectedCell ();
            return;
        }
    }
}

bool AGridLevelEditorActor::UpdateHoveredObjectFromWorldPoint (const FVector& WorldPoint)
{
    ResolvePreviewRuntimeActor ();

    HoveredObjectId.Invalidate ();

    if (PreviewRuntimeActor)
    {
        PreviewRuntimeActor->SetEditorHoveredObject (FGuid ());
    }

    if (!HasValidLevelAsset ())
    {
        return false;
    }

    float BestDistSq = FMath::Square (ObjectHoverPickRadius);
    const FGridLevelObjectData* BestObject = nullptr;

    for (const FGridLevelObjectData& Obj : LevelAsset->Objects)
    {
        FVector ObjLocation = FVector::ZeroVector;

        if (!TryGetObjectWorldLocation (Obj, ObjLocation))
        {
            continue;
        }

        const float DistSq = FVector::DistSquared2D (WorldPoint, ObjLocation);

        if (DistSq <= BestDistSq)
        {
            BestDistSq = DistSq;
            BestObject = &Obj;
        }
    }

    if (!BestObject)
    {
        return false;
    }
    HoveredObjectId = BestObject->ObjectId;

    if (PreviewRuntimeActor)
    {
        PreviewRuntimeActor->SetEditorHoveredObject (HoveredObjectId);
    }

    return true;
}

bool AGridLevelEditorActor::SelectHoveredObject ()
{
    if (!HoveredObjectId.IsValid ())
    {
        return false;
    }

    return SelectObjectById (HoveredObjectId);
}

bool AGridLevelEditorActor::TryGetHoveredObjectWorldLocation (FVector& OutWorldLocation) const
{
    if (!HoveredObjectId.IsValid ())
    {
        return false;
    }

    return TryGetObjectWorldLocationById (HoveredObjectId, OutWorldLocation);
}

void AGridLevelEditorActor::PaintSelectedCell ()
{
    FGridLevelCellData* CellData = GetSelectedCellMutable ();
    if (!CellData)
    {
        return;
    }

#if WITH_EDITOR
    LevelAsset->Modify ();
#endif

    CellData->CellType = PaintCellType;
    CellData->bHasCeiling = bPaintCellHasCeiling;
    CellData->bBlocksOccupancy = bPaintCellBlocksOccupancy;

#if WITH_EDITOR
    LevelAsset->MarkPackageDirty ();
#endif

    RebuildGeometryPreview ();
}

void AGridLevelEditorActor::RebuildGeometryPreview ()
{
    ResolvePreviewRuntimeActor ();

    if (PreviewRuntimeActor)
    {
        PreviewRuntimeActor->LevelAsset = LevelAsset;
        PreviewRuntimeActor->RebuildLevel (EGridRuntimeRebuildMode::GeometryOnly);
    }
}

void AGridLevelEditorActor::EnsureCoordinateHoverLabel ()
{
    if (CoordinateHoverLabel || !SceneRoot)
    {
        return;
    }
    CoordinateHoverLabel = NewObject<UTextRenderComponent> (this, UTextRenderComponent::StaticClass (),
        TEXT ("CoordinateHoverLabel"), RF_Transactional);
    if (!CoordinateHoverLabel)
    {
        return;
    }
    CoordinateHoverLabel->CreationMethod = EComponentCreationMethod::Instance;
    AddInstanceComponent (CoordinateHoverLabel);
    CoordinateHoverLabel->AttachToComponent (SceneRoot, FAttachmentTransformRules::KeepRelativeTransform);

    CoordinateHoverLabel->SetCollisionEnabled (ECollisionEnabled::NoCollision);
    CoordinateHoverLabel->SetHiddenInGame (false);
    CoordinateHoverLabel->SetHorizontalAlignment (EHTA_Center);
    CoordinateHoverLabel->SetVerticalAlignment (EVRTA_TextCenter);
    CoordinateHoverLabel->SetTextRenderColor (FColor::White);
    CoordinateHoverLabel->SetRelativeRotation (FRotator (90.f, -90.f, 0.f));

    CoordinateHoverLabel->RegisterComponentWithWorld (GetWorld ());
}

void AGridLevelEditorActor::UpdateCoordinateHoverLabel ()
{
    if (!bShowCoordinateGrid || !bShowCoordinateLabels || !LevelAsset || !LevelAsset->IsValidCoord (HoveredCellX, HoveredCellY))
    {
        if (CoordinateHoverLabel)
        {
            CoordinateHoverLabel->SetVisibility (false, true);
        }
        return;
    }
    EnsureCoordinateHoverLabel ();
    if (!CoordinateHoverLabel)
    {
        return;
    }
    const TCHAR* EdgeText = TEXT (" ");

    switch (HoveredEdge)
    {
        case EGridEdge::North: EdgeText = TEXT ("N"); break;
        case EGridEdge::East:  EdgeText = TEXT ("E"); break;
        case EGridEdge::South: EdgeText = TEXT ("S"); break;
        case EGridEdge::West:  EdgeText = TEXT ("W"); break;
        default: break;
    }
    const float CellSize = LevelAsset->CellSize;
    CoordinateHoverLabel->SetWorldSize (CoordinateLabelWorldSize);
    CoordinateHoverLabel->SetText (
        FText::FromString (
            FString::Printf (TEXT ("X:%d   Y:%d  %s"),
                HoveredCellX, HoveredCellY, EdgeText)));
    CoordinateHoverLabel->SetRelativeLocation (
        FVector ((HoveredCellX + 0.5f) * CellSize, (HoveredCellY + 0.5f) * CellSize, CoordinateGridZOffset + 4.f));
    CoordinateHoverLabel->SetVisibility (true, true);
    CoordinateHoverLabel->MarkRenderStateDirty ();
}

void AGridLevelEditorActor::UpdateCoordinateGridPlane ()
{
    if (!CoordinateGridPlane)
    {
        return;
    }
    const bool bVisible = bShowCoordinateGrid && LevelAsset != nullptr && CoordinateGridPlaneMesh != nullptr;
    CoordinateGridPlane->SetVisibility (bVisible, true);
    if (!bVisible)
    {
        return;
    }
    const float CellSize = LevelAsset->CellSize;
    const int32 Width = LevelAsset->Width;
    const int32 Height = LevelAsset->Height;
    CoordinateGridPlane->SetStaticMesh (CoordinateGridPlaneMesh);
    if (CoordinateGridMaterial)
    {
        CoordinateGridPlane->SetMaterial (0, CoordinateGridMaterial);
    }
    CoordinateGridPlane->SetRelativeLocation (FVector (Width * CellSize * 0.5f, Height * CellSize * 0.5f, CoordinateGridZOffset));
    CoordinateGridPlane->SetRelativeScale3D (FVector (Width * CellSize / 100.f, Height * CellSize / 100.f, 1.f));
}