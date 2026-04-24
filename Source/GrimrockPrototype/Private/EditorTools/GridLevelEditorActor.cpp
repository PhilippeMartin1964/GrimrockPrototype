#include "EditorTools/GridLevelEditorActor.h"

#include "Kismet/GameplayStatics.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Core/GridObjectPaletteAsset.h"
#include "Core/GridObjectArchetypeAsset.h"

#if WITH_EDITOR
#include "Subsystems/UnrealEditorSubsystem.h"
#endif

AGridLevelEditorActor::AGridLevelEditorActor ()
{
    PrimaryActorTick.bCanEverTick = true;

#if WITH_EDITORONLY_DATA
    bIsEditorOnlyActor = true;
#endif
}

void AGridLevelEditorActor::OnConstruction (const FTransform& Transform)
{
    Super::OnConstruction (Transform);

    ResolvePreviewRuntimeActor ();

    if (PreviewRuntimeActor && LevelAsset && PreviewRuntimeActor->LevelAsset != LevelAsset)
    {
        PreviewRuntimeActor->LevelAsset = LevelAsset;
    }

    if (bAutoSelectFromActorTransform)
    {
        UpdateSelectionFromActorTransform ();
    }
}

#if WITH_EDITOR
void AGridLevelEditorActor::PostEditMove (bool bFinished)
{
    Super::PostEditMove (bFinished);

    if (!bAutoSelectFromActorTransform)
    {
        return;
    }

    UpdateSelectionFromActorTransform ();

    if (bAutoRebuildPreviewOnMove && bFinished)
    {
        RebuildPreview ();
    }
}

void AGridLevelEditorActor::PostEditChangeProperty (FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty (PropertyChangedEvent);

    ResolvePreviewRuntimeActor ();

    if (PreviewRuntimeActor && LevelAsset && PreviewRuntimeActor->LevelAsset != LevelAsset)
    {
        PreviewRuntimeActor->LevelAsset = LevelAsset;
    }

    static const FName NAME_SelectedCellX = GET_MEMBER_NAME_CHECKED (AGridLevelEditorActor, SelectedCellX);
    static const FName NAME_SelectedCellY = GET_MEMBER_NAME_CHECKED (AGridLevelEditorActor, SelectedCellY);
    static const FName NAME_SelectedEdge = GET_MEMBER_NAME_CHECKED (AGridLevelEditorActor, SelectedEdge);
    static const FName NAME_bAutoSelectFromActorTransform = GET_MEMBER_NAME_CHECKED (AGridLevelEditorActor, bAutoSelectFromActorTransform);

    const FName ChangedPropertyName =
        PropertyChangedEvent.Property ? PropertyChangedEvent.Property->GetFName () : NAME_None;

    if (ChangedPropertyName == NAME_bAutoSelectFromActorTransform && bAutoSelectFromActorTransform)
    {
        UpdateSelectionFromActorTransform ();
        return;
    }

    if (!bAutoSelectFromActorTransform &&
        (ChangedPropertyName == NAME_SelectedCellX ||
         ChangedPropertyName == NAME_SelectedCellY ||
         ChangedPropertyName == NAME_SelectedEdge))
    {
        SnapActorToSelectedCell ();
    }
}
#endif

bool AGridLevelEditorActor::HasValidLevelAsset () const
{
    return LevelAsset != nullptr;
}

bool AGridLevelEditorActor::IsValidSelectedCell () const
{
    return LevelAsset && LevelAsset->IsValidCoord (SelectedCellX, SelectedCellY);
}

bool AGridLevelEditorActor::RequiresEdge (EGridLevelObjectType ObjectType) const
{
    switch (ObjectType)
    {
        case EGridLevelObjectType::Door:
        case EGridLevelObjectType::Button:
        case EGridLevelObjectType::Lever:
            return true;

        default:
            return false;
    }
}

bool AGridLevelEditorActor::IsCellCenteredObject (EGridLevelObjectType ObjectType) const
{
    switch (ObjectType)
    {
        case EGridLevelObjectType::PressurePlate:
        case EGridLevelObjectType::MonsterSpawn:
        case EGridLevelObjectType::ItemSpawn:
        case EGridLevelObjectType::Light:
        case EGridLevelObjectType::Teleporter:
        case EGridLevelObjectType::Trigger:
        case EGridLevelObjectType::Decoration:
            return true;

        default:
            return false;
    }
}

FGridLevelCellData* AGridLevelEditorActor::GetSelectedCellMutable ()
{
    if (!IsValidSelectedCell ())
    {
        return nullptr;
    }

    return &LevelAsset->GetCellMutable (SelectedCellX, SelectedCellY);
}

EGridWallType* AGridLevelEditorActor::GetSelectedWallMutable (FGridLevelCellData& CellData)
{
    switch (SelectedEdge)
    {
        case EGridEdge::North: return &CellData.NorthWall;
        case EGridEdge::East:  return &CellData.EastWall;
        case EGridEdge::South: return &CellData.SouthWall;
        case EGridEdge::West:  return &CellData.WestWall;
        default:               return nullptr;
    }
}

void AGridLevelEditorActor::ResolvePreviewRuntimeActor ()
{
    if (!PreviewRuntimeActor)
    {
        PreviewRuntimeActor = Cast<AGridLevelRuntimeActor> (
            UGameplayStatics::GetActorOfClass (GetWorld (), AGridLevelRuntimeActor::StaticClass ()));
    }
}

EGridEdge AGridLevelEditorActor::GetEdgeFromYaw (float YawDegrees) const
{
    const float NormalizedYaw = FRotator::NormalizeAxis (YawDegrees);

    if (NormalizedYaw >= -45.f && NormalizedYaw < 45.f)
    {
        return EGridEdge::East;
    }

    if (NormalizedYaw >= 45.f && NormalizedYaw < 135.f)
    {
        return EGridEdge::North;
    }

    if (NormalizedYaw >= -135.f && NormalizedYaw < -45.f)
    {
        return EGridEdge::South;
    }

    return EGridEdge::West;
}

FVector AGridLevelEditorActor::GetSelectedCellWorldCenter (float ZOffset) const
{
    if (PreviewRuntimeActor)
    {
        return PreviewRuntimeActor->GetCellCenterWorld (SelectedCellX, SelectedCellY, ZOffset);
    }

    const float CellSize = LevelAsset ? LevelAsset->CellSize : 200.f;
    return GetActorLocation () + FVector::ZeroVector +
        FVector (
            (SelectedCellX * CellSize) + (CellSize * 0.5f),
            (SelectedCellY * CellSize) + (CellSize * 0.5f),
            ZOffset);
}

void AGridLevelEditorActor::EnsureLevelReady ()
{
    if (!HasValidLevelAsset ())
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridLevelEditorActor: LevelAsset is null."));
        return;
    }

#if WITH_EDITOR
    LevelAsset->Modify ();
#endif

    LevelAsset->EnsureCellCount ();
    LevelAsset->EnsureObjectIds ();

#if WITH_EDITOR
    LevelAsset->MarkPackageDirty ();
#endif

    RebuildPreview ();
}

void AGridLevelEditorActor::RebuildPreview ()
{
    ResolvePreviewRuntimeActor ();

    if (PreviewRuntimeActor)
    {
        PreviewRuntimeActor->LevelAsset = LevelAsset;
        PreviewRuntimeActor->RebuildLevel ();
    }
}

void AGridLevelEditorActor::UpdateSelectionFromActorTransform ()
{
    if (!HasValidLevelAsset ())
    {
        return;
    }

    const float CellSize = LevelAsset->CellSize;
    if (CellSize <= KINDA_SMALL_NUMBER)
    {
        return;
    }

    FVector Origin = FVector::ZeroVector;
    if (PreviewRuntimeActor)
    {
        Origin = PreviewRuntimeActor->GetActorLocation () + PreviewRuntimeActor->GridOrigin;
    }

    const FVector Local = GetActorLocation () - Origin;

    const int32 NewCellX = FMath::FloorToInt (Local.X / CellSize);
    const int32 NewCellY = FMath::FloorToInt (Local.Y / CellSize);

    SelectedCellX = FMath::Clamp (NewCellX, 0, FMath::Max (0, LevelAsset->Width - 1));
    SelectedCellY = FMath::Clamp (NewCellY, 0, FMath::Max (0, LevelAsset->Height - 1));
    SelectedEdge = GetEdgeFromYaw (GetActorRotation ().Yaw);
}

void AGridLevelEditorActor::SnapActorToSelectedCell ()
{
    if (!HasValidLevelAsset () || !IsValidSelectedCell ())
    {
        return;
    }

    const FVector CellCenter = GetSelectedCellWorldCenter (AutoSelectionZ);
    SetActorLocation (CellCenter);

    FRotator NewRotation = GetActorRotation ();

    switch (SelectedEdge)
    {
        case EGridEdge::East:
            NewRotation.Yaw = 0.f;
            break;
        case EGridEdge::North:
            NewRotation.Yaw = 90.f;
            break;
        case EGridEdge::South:
            NewRotation.Yaw = -90.f;
            break;
        case EGridEdge::West:
            NewRotation.Yaw = 180.f;
            break;
        default:
            break;
    }

    SetActorRotation (NewRotation);
}

void AGridLevelEditorActor::PaintSelectedCell ()
{
    FGridLevelCellData* CellData = GetSelectedCellMutable ();
    if (!CellData)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridLevelEditorActor: invalid selected cell."));
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

    RebuildPreview ();
}

void AGridLevelEditorActor::ClearSelectedCell ()
{
    FGridLevelCellData* CellData = GetSelectedCellMutable ();
    if (!CellData)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridLevelEditorActor: invalid selected cell."));
        return;
    }

#if WITH_EDITOR
    LevelAsset->Modify ();
#endif

    * CellData = FGridLevelCellData ();

    RemoveObjectsAtSelectionInternal (false);

#if WITH_EDITOR
    LevelAsset->MarkPackageDirty ();
#endif

    RebuildPreview ();
}

void AGridLevelEditorActor::PaintSelectedWall ()
{
    FGridLevelCellData* CellData = GetSelectedCellMutable ();
    if (!CellData)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridLevelEditorActor: invalid selected cell."));
        return;
    }

    EGridWallType* WallPtr = GetSelectedWallMutable (*CellData);
    if (!WallPtr)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridLevelEditorActor: SelectedEdge must be North/East/South/West."));
        return;
    }

#if WITH_EDITOR
    LevelAsset->Modify ();
#endif

    * WallPtr = PaintWallType;

#if WITH_EDITOR
    LevelAsset->MarkPackageDirty ();
#endif

    RebuildPreview ();
}

void AGridLevelEditorActor::ClearSelectedWall ()
{
    FGridLevelCellData* CellData = GetSelectedCellMutable ();
    if (!CellData)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridLevelEditorActor: invalid selected cell."));
        return;
    }

    EGridWallType* WallPtr = GetSelectedWallMutable (*CellData);
    if (!WallPtr)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridLevelEditorActor: SelectedEdge must be North/East/South/West."));
        return;
    }

#if WITH_EDITOR
    LevelAsset->Modify ();
#endif

    * WallPtr = EGridWallType::None;

#if WITH_EDITOR
    LevelAsset->MarkPackageDirty ();
#endif

    RebuildPreview ();
}

int32 AGridLevelEditorActor::RemoveObjectsAtSelectionInternal (bool bSameTypeOnly)
{
    if (!HasValidLevelAsset () || !IsValidSelectedCell ())
    {
        return 0;
    }

#if WITH_EDITOR
    LevelAsset->Modify ();
#endif

    TArray<FGuid> RemovedIds;

    const EGridLevelObjectType FilterType = PaintObjectType;
    const bool bUseEdge = RequiresEdge (PaintObjectType);
    const bool bUseCenter = IsCellCenteredObject (PaintObjectType);

    for (int32 Index = LevelAsset->Objects.Num () - 1; Index >= 0; --Index)
    {
        const FGridLevelObjectData& Obj = LevelAsset->Objects[Index];

        if (Obj.CellX != SelectedCellX || Obj.CellY != SelectedCellY)
        {
            continue;
        }

        if (bSameTypeOnly && Obj.Type != FilterType)
        {
            continue;
        }

        bool bRemove = false;

        if (bUseEdge)
        {
            bRemove = (Obj.Edge == SelectedEdge);
        } else if (bUseCenter)
        {
            bRemove = true;
        } else if (Obj.Type == FilterType)
        {
            bRemove = true;
        }

        if (bRemove)
        {
            RemovedIds.Add (Obj.ObjectId);
            LevelAsset->Objects.RemoveAt (Index);
        }
    }

    if (RemovedIds.Num () > 0)
    {
        LevelAsset->Links.RemoveAll (
            [&] (const FGridLevelLinkData& Link)
        {
            return RemovedIds.Contains (Link.SourceObjectId) || RemovedIds.Contains (Link.TargetObjectId);
        });
    }

#if WITH_EDITOR
    LevelAsset->MarkPackageDirty ();
#endif

    return RemovedIds.Num ();
}

void AGridLevelEditorActor::PlaceSelectedObject ()
{
    if (!HasValidLevelAsset () || !IsValidSelectedCell ())
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridLevelEditorActor: invalid LevelAsset or selected cell."));
        return;
    }

    if (PaintObjectType == EGridLevelObjectType::None)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridLevelEditorActor: PaintObjectType is None."));
        return;
    }

    if (RequiresEdge (PaintObjectType) && SelectedEdge == EGridEdge::None)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridLevelEditorActor: this object type requires a valid edge."));
        return;
    }

    if (PlacementPolicy == EGridEditorObjectPlacementPolicy::ReplaceSameSlotOnly)
    {
        RemoveObjectsAtSelectionInternal (true);
    } else
    {
        RemoveObjectsAtSelectionInternal (false);
    }

    FGridLevelObjectData NewObject;
    NewObject.Type = PaintObjectType;
    NewObject.CellX = SelectedCellX;
    NewObject.CellY = SelectedCellY;
    NewObject.Edge = RequiresEdge (PaintObjectType) ? SelectedEdge : EGridEdge::None;
    NewObject.ArchetypeId = ObjectArchetypeId;
    NewObject.bInitiallyEnabled = bObjectInitiallyEnabled;
    NewObject.bInitiallyActive = bObjectInitiallyActive;
    NewObject.Tag = ObjectTag;
    NewObject.Notes = ObjectNotes;
    NewObject.PaletteEntryId = SelectedPaletteEntryId;
    NewObject.Behavior = ObjectBehavior;

    const FGuid NewId = LevelAsset->AddObject (NewObject);
    LastSelectedObjectId = NewId;

    RebuildPreview ();
}

void AGridLevelEditorActor::RemoveObjectsAtSelection ()
{
    if (!HasValidLevelAsset () || !IsValidSelectedCell ())
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridLevelEditorActor: invalid LevelAsset or selected cell."));
        return;
    }

    RemoveObjectsAtSelectionInternal (false);
    LastSelectedObjectId.Invalidate ();
    RebuildPreview ();
}

void AGridLevelEditorActor::SelectObjectAtSelection ()
{
    LastSelectedObjectId.Invalidate ();

    if (!HasValidLevelAsset () || !IsValidSelectedCell ())
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridLevelEditorActor: invalid LevelAsset or selected cell."));
        return;
    }

    for (int32 Index = LevelAsset->Objects.Num () - 1; Index >= 0; --Index)
    {
        const FGridLevelObjectData& Obj = LevelAsset->Objects[Index];

        if (Obj.CellX != SelectedCellX || Obj.CellY != SelectedCellY)
        {
            continue;
        }

        if (RequiresEdge (Obj.Type) && Obj.Edge != SelectedEdge)
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

        UE_LOG (
            LogTemp,
            Log,
            TEXT ("GridLevelEditorActor: selected object %s at (%d, %d), edge=%d"),
            *Obj.ObjectId.ToString (),
            Obj.CellX,
            Obj.CellY,
            static_cast<int32>(Obj.Edge));

        return;
    }

    UE_LOG (LogTemp, Warning, TEXT ("GridLevelEditorActor: no object found at current selection."));
}

EGridEdge AGridLevelEditorActor::GetEdgeFromHitNormal (const FVector& HitNormal) const
{
    const FVector Normal = HitNormal.GetSafeNormal ();

    if (FMath::Abs (Normal.Z) > 0.75f)
    {
        return SelectedEdge;
    }

    if (FMath::Abs (Normal.X) >= FMath::Abs (Normal.Y))
    {
        return (Normal.X >= 0.f) ? EGridEdge::East : EGridEdge::West;
    }

    return (Normal.Y >= 0.f) ? EGridEdge::North : EGridEdge::South;
}

bool AGridLevelEditorActor::TryConvertWorldHitToSelection (
    const FVector& WorldHitLocation,
    const FVector& HitNormal)
{
    if (!HasValidLevelAsset ())
    {
        return false;
    }

    ResolvePreviewRuntimeActor ();

    const float CellSize = LevelAsset->CellSize;
    if (CellSize <= KINDA_SMALL_NUMBER)
    {
        return false;
    }

    FVector GridWorldOrigin = FVector::ZeroVector;
    if (PreviewRuntimeActor)
    {
        GridWorldOrigin = PreviewRuntimeActor->GetActorLocation () + PreviewRuntimeActor->GridOrigin;
    }

    const FVector Local = WorldHitLocation - GridWorldOrigin;

    const int32 NewCellX = FMath::FloorToInt (Local.X / CellSize);
    const int32 NewCellY = FMath::FloorToInt (Local.Y / CellSize);

    if (!LevelAsset->IsValidCoord (NewCellX, NewCellY))
    {
        return false;
    }

    SelectedCellX = NewCellX;
    SelectedCellY = NewCellY;

    if (bUseHitNormalForEdgeSelection)
    {
        SelectedEdge = GetEdgeFromHitNormal (HitNormal);
    } else
    {
        SelectedEdge = GetEdgeFromYaw (GetActorRotation ().Yaw);
    }

    return true;
}

bool AGridLevelEditorActor::ApplyViewportHitSelection (const FVector& WorldHitLocation, const FVector& HitNormal)
{
    const bool bOk = TryConvertWorldHitToSelection (WorldHitLocation, HitNormal);

    if (bOk && bSnapAfterViewportPick)
    {
        const bool bWasAuto = bAutoSelectFromActorTransform;
        bAutoSelectFromActorTransform = false;
        SnapActorToSelectedCell ();
        bAutoSelectFromActorTransform = bWasAuto;
    }

    return bOk;
}

bool AGridLevelEditorActor::IsSelectionValidForEditing () const
{
    return HasValidLevelAsset () && IsValidSelectedCell ();
}

EGridEdge AGridLevelEditorActor::GetEdgeFromPointInCell (const FVector2D& LocalInCell, float CellSize) const
{
    const float DistNorth = FMath::Abs (CellSize - LocalInCell.Y);
    const float DistEast = FMath::Abs (CellSize - LocalInCell.X);
    const float DistSouth = FMath::Abs (LocalInCell.Y);
    const float DistWest = FMath::Abs (LocalInCell.X);

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

bool AGridLevelEditorActor::ApplyGridHoverFromWorldPoint (const FVector& WorldPoint)
{
    if (!HasValidLevelAsset ())
    {
        return false;
    }

    ResolvePreviewRuntimeActor ();

    const float CellSize = LevelAsset->CellSize;
    if (CellSize <= KINDA_SMALL_NUMBER)
    {
        return false;
    }

    FVector GridWorldOrigin = FVector::ZeroVector;
    if (PreviewRuntimeActor)
    {
        GridWorldOrigin = PreviewRuntimeActor->GetActorLocation () + PreviewRuntimeActor->GridOrigin;
    }

    const FVector Local = WorldPoint - GridWorldOrigin;

    const int32 NewCellX = FMath::FloorToInt (Local.X / CellSize);
    const int32 NewCellY = FMath::FloorToInt (Local.Y / CellSize);

    if (!LevelAsset->IsValidCoord (NewCellX, NewCellY))
    {
        return false;
    }

    SelectedCellX = NewCellX;
    SelectedCellY = NewCellY;

    const float LocalInCellX = Local.X - (static_cast<float>(NewCellX) * CellSize);
    const float LocalInCellY = Local.Y - (static_cast<float>(NewCellY) * CellSize);

    SelectedEdge = GetEdgeFromPointInCell (FVector2D (LocalInCellX, LocalInCellY), CellSize);

    if (bSnapAfterViewportPick)
    {
        const bool bWasAuto = bAutoSelectFromActorTransform;
        bAutoSelectFromActorTransform = false;
        SnapActorToSelectedCell ();
        bAutoSelectFromActorTransform = bWasAuto;
    }

    return true;
}

FVector AGridLevelEditorActor::GetSelectionPreviewCenter (float ZOffset) const
{
    return GetSelectedCellWorldCenter (ZOffset);
}

void AGridLevelEditorActor::ApplyPrimaryToolAction ()
{
    switch (ActiveTool)
    {
        case EGridEditorTool::Select:
            SelectObjectAtSelection ();
            break;

        case EGridEditorTool::PaintCell:
            PaintSelectedCell ();
            break;

        case EGridEditorTool::PaintWall:
            PaintSelectedWall ();
            break;

        case EGridEditorTool::PaintObject:
            PlaceSelectedObject ();
            break;

        case EGridEditorTool::Erase:
        {
            RemoveObjectsAtSelection ();

            if (FGridLevelCellData* CellData = GetSelectedCellMutable ())
            {
                EGridWallType* WallPtr = GetSelectedWallMutable (*CellData);
                if (WallPtr && *WallPtr != EGridWallType::None)
                {
                    ClearSelectedWall ();
                } else if (CellData->CellType != EGridCellType::Empty)
                {
                    ClearSelectedCell ();
                }
            }
            break;
        }

        case EGridEditorTool::Link:
            BeginOrCompleteLinkAtSelection ();
            break;

        default:
            break;
    }
}

void AGridLevelEditorActor::ApplySecondaryToolAction ()
{
    switch (ActiveTool)
    {
        case EGridEditorTool::PaintCell:
            ClearSelectedCell ();
            break;

        case EGridEditorTool::PaintWall:
            ClearSelectedWall ();
            break;

        case EGridEditorTool::PaintObject:
            RemoveObjectsAtSelection ();
            break;

        case EGridEditorTool::Link:
            ClearPendingLinkSource ();
            break;

        case EGridEditorTool::Select:
        case EGridEditorTool::Erase:
        default:
            break;
    }
}

const FGridLevelObjectData* AGridLevelEditorActor::FindObjectAtSelection () const
{
    if (!HasValidLevelAsset () || !IsValidSelectedCell ())
    {
        return nullptr;
    }

    for (int32 Index = LevelAsset->Objects.Num () - 1; Index >= 0; --Index)
    {
        const FGridLevelObjectData& Obj = LevelAsset->Objects[Index];

        if (Obj.CellX != SelectedCellX || Obj.CellY != SelectedCellY)
        {
            continue;
        }

        if (RequiresEdge (Obj.Type) && Obj.Edge != SelectedEdge)
        {
            continue;
        }

        return &Obj;
    }

    return nullptr;
}

const FGridLevelObjectData* AGridLevelEditorActor::FindObjectById (const FGuid& ObjectId) const
{
    if (!HasValidLevelAsset () || !ObjectId.IsValid ())
    {
        return nullptr;
    }

    for (const FGridLevelObjectData& Obj : LevelAsset->Objects)
    {
        if (Obj.ObjectId == ObjectId)
        {
            return &Obj;
        }
    }

    return nullptr;
}

bool AGridLevelEditorActor::TryGetObjectWorldLocation (
    const FGridLevelObjectData& ObjectData,
    FVector& OutWorldLocation) const
{
    if (!HasValidLevelAsset ())
    {
        return false;
    }

    const float CellSize = LevelAsset->CellSize;

    FVector GridWorldOrigin = FVector::ZeroVector;
    if (PreviewRuntimeActor)
    {
        GridWorldOrigin = PreviewRuntimeActor->GetActorLocation () + PreviewRuntimeActor->GridOrigin;
    }

    const FVector CellCenter = GridWorldOrigin + FVector (
        (ObjectData.CellX * CellSize) + (CellSize * 0.5f),
        (ObjectData.CellY * CellSize) + (CellSize * 0.5f),
        12.f);

    if (RequiresEdge (ObjectData.Type))
    {
        switch (ObjectData.Edge)
        {
            case EGridEdge::North:
                OutWorldLocation = CellCenter + FVector (0.f, CellSize * 0.5f, 0.f);
                return true;

            case EGridEdge::East:
                OutWorldLocation = CellCenter + FVector (CellSize * 0.5f, 0.f, 0.f);
                return true;

            case EGridEdge::South:
                OutWorldLocation = CellCenter + FVector (0.f, -CellSize * 0.5f, 0.f);
                return true;

            case EGridEdge::West:
                OutWorldLocation = CellCenter + FVector (-CellSize * 0.5f, 0.f, 0.f);
                return true;

            default:
                return false;
        }
    }

    OutWorldLocation = CellCenter;
    return true;
}

bool AGridLevelEditorActor::TryGetSelectedObjectWorldLocation (FVector& OutWorldLocation) const
{
    const FGridLevelObjectData* Obj = FindObjectAtSelection ();
    return Obj ? TryGetObjectWorldLocation (*Obj, OutWorldLocation) : false;
}

bool AGridLevelEditorActor::TryGetPendingLinkSourceLocation (FVector& OutWorldLocation) const
{
    if (!bHasPendingLinkSource || !PendingLinkSourceObjectId.IsValid ())
    {
        return false;
    }

    const FGridLevelObjectData* Obj = FindObjectById (PendingLinkSourceObjectId);
    return Obj ? TryGetObjectWorldLocation (*Obj, OutWorldLocation) : false;
}

bool AGridLevelEditorActor::HasPendingLinkSource () const
{
    return bHasPendingLinkSource && PendingLinkSourceObjectId.IsValid ();
}

void AGridLevelEditorActor::ClearPendingLinkSource ()
{
    bHasPendingLinkSource = false;
    PendingLinkSourceObjectId.Invalidate ();
}

bool AGridLevelEditorActor::BeginOrCompleteLinkAtSelection ()
{
    if (!HasValidLevelAsset () || !IsValidSelectedCell ())
    {
        return false;
    }

    const FGridLevelObjectData* SelectedObject = FindObjectAtSelection ();
    if (!SelectedObject)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridLevelEditorActor: no object at selection for link mode."));
        return false;
    }

    if (!bHasPendingLinkSource)
    {
        PendingLinkSourceObjectId = SelectedObject->ObjectId;
        bHasPendingLinkSource = true;
        LastSelectedObjectId = SelectedObject->ObjectId;

        UE_LOG (
            LogTemp,
            Log,
            TEXT ("GridLevelEditorActor: link source set to %s"),
            *SelectedObject->ObjectId.ToString ());

        return true;
    }

    if (PendingLinkSourceObjectId == SelectedObject->ObjectId)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridLevelEditorActor: source and target are identical."));
        return false;
    }

#if WITH_EDITOR
    LevelAsset->Modify ();
#endif

    const bool bAlreadyExists = LevelAsset->Links.ContainsByPredicate (
        [&] (const FGridLevelLinkData& Link)
    {
        return Link.SourceObjectId == PendingLinkSourceObjectId &&
            Link.TargetObjectId == SelectedObject->ObjectId &&
            Link.Action == LinkAction;
    });

    if (!bAlreadyExists)
    {
        FGridLevelLinkData NewLink;
        NewLink.SourceObjectId = PendingLinkSourceObjectId;
        NewLink.TargetObjectId = SelectedObject->ObjectId;
        NewLink.Action = LinkAction;
        LevelAsset->Links.Add (NewLink);

#if WITH_EDITOR
        LevelAsset->MarkPackageDirty ();
#endif

        UE_LOG (
            LogTemp,
            Log,
            TEXT ("GridLevelEditorActor: link created %s -> %s"),
            *PendingLinkSourceObjectId.ToString (),
            *SelectedObject->ObjectId.ToString ());
    }

    LastSelectedObjectId = SelectedObject->ObjectId;
    ClearPendingLinkSource ();
    RebuildPreview ();
    return true;
}

bool AGridLevelEditorActor::RemoveLinksAtSelection ()
{
    if (!HasValidLevelAsset ())
    {
        return false;
    }

    const FGridLevelObjectData* SelectedObject = FindObjectAtSelection ();
    if (!SelectedObject)
    {
        return false;
    }

#if WITH_EDITOR
    LevelAsset->Modify ();
#endif

    const int32 RemovedCount = LevelAsset->Links.RemoveAll (
        [&] (const FGridLevelLinkData& Link)
    {
        return Link.SourceObjectId == SelectedObject->ObjectId ||
            Link.TargetObjectId == SelectedObject->ObjectId;
    });

    if (RemovedCount > 0)
    {
#if WITH_EDITOR
        LevelAsset->MarkPackageDirty ();
#endif
        RebuildPreview ();
        return true;
    }

    return false;
}

bool AGridLevelEditorActor::ApplyPaletteEntry (FName EntryId)
{
    if (!ObjectPalette)
    {
        return false;
    }

    const FGridObjectPaletteEntry* Entry = ObjectPalette->FindEntryById (EntryId);
    if (!Entry)
    {
        return false;
    }

    SelectedPaletteEntryId = Entry->EntryId;
    PaintObjectType = Entry->ObjectType;

    if (Entry->DefaultArchetype)
    {
        ObjectArchetypeId = Entry->DefaultArchetype->ArchetypeId;
        SelectedArchetypeId = Entry->DefaultArchetype->ArchetypeId;
        bObjectInitiallyEnabled = Entry->DefaultArchetype->bDefaultInitiallyEnabled;
        bObjectInitiallyActive = Entry->DefaultArchetype->bDefaultInitiallyActive;
        ObjectTag = Entry->DefaultArchetype->DefaultTag;
        ObjectBehavior = Entry->DefaultArchetype->DefaultBehavior;
    } else
    {
        ObjectArchetypeId = NAME_None;
        SelectedArchetypeId = NAME_None;
        ObjectBehavior = FGridObjectBehaviorParams ();
    }

    return true;
}

void AGridLevelEditorActor::ApplySelectedPaletteEntry ()
{
    ApplyPaletteEntry (SelectedPaletteEntryId);
}