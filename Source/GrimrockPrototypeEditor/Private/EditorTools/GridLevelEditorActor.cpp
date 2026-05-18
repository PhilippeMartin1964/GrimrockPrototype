#include "EditorTools/GridLevelEditorActor.h"

#include "Kismet/GameplayStatics.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Core/GridObjectPaletteAsset.h"
#include "Core/GridObjectArchetypeAsset.h"
#include "Components/TextRenderComponent.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

namespace
{
    EGridWallType GetWallTypeForEdge (const FGridLevelCellData& CellData, EGridEdge Edge)
    {
        switch (Edge)
        {
            case EGridEdge::North: return CellData.NorthWall;
            case EGridEdge::East:  return CellData.EastWall;
            case EGridEdge::South: return CellData.SouthWall;
            case EGridEdge::West:  return CellData.WestWall;
            default:               return EGridWallType::None;
        }
    }

    EGridLevelValidationSeverity ConvertArchetypeValidationSeverity (EGridArchetypeValidationSeverity Severity)
    {
        switch (Severity)
        {
            case EGridArchetypeValidationSeverity::Error:
                return EGridLevelValidationSeverity::Error;

            case EGridArchetypeValidationSeverity::Warning:
                return EGridLevelValidationSeverity::Warning;

            case EGridArchetypeValidationSeverity::Info:
            default:
                return EGridLevelValidationSeverity::Info;
        }
    }
}

AGridLevelEditorActor::AGridLevelEditorActor ()
{
    PrimaryActorTick.bCanEverTick = false;

#if WITH_EDITORONLY_DATA
    bIsEditorOnlyActor = true;
#endif
    SceneRoot = CreateDefaultSubobject<USceneComponent> (TEXT ("SceneRoot"));
    SetRootComponent (SceneRoot);
    CoordinateGridPlane = CreateDefaultSubobject<UStaticMeshComponent> (TEXT ("CoordinateGridPlane"));
    CoordinateGridPlane->SetupAttachment (RootComponent);
    CoordinateGridPlane->SetCollisionEnabled (ECollisionEnabled::NoCollision);
    CoordinateGridPlane->SetMobility (EComponentMobility::Movable);
    CoordinateGridPlane->SetHiddenInGame (true);
}

void AGridLevelEditorActor::OnConstruction (const FTransform& Transform)
{
    Super::OnConstruction (Transform);

    ResolvePreviewRuntimeActor ();

    if (PreviewRuntimeActor && LevelAsset && PreviewRuntimeActor->LevelAsset != LevelAsset)
    {
        PreviewRuntimeActor->LevelAsset = LevelAsset;
    }
    UpdateCoordinateGridPlane ();
    UpdateCoordinateHoverLabel ();
}

#if WITH_EDITOR
void AGridLevelEditorActor::PostEditChangeProperty (FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty (PropertyChangedEvent);

    const FName PropertyName = PropertyChangedEvent.Property ? PropertyChangedEvent.Property->GetFName () : NAME_None;

    if (PropertyName == GET_MEMBER_NAME_CHECKED (AGridLevelEditorActor, CoordinateGridPlaneMesh)
        || PropertyName == GET_MEMBER_NAME_CHECKED (AGridLevelEditorActor, CoordinateGridMaterial)
        || PropertyName == GET_MEMBER_NAME_CHECKED (AGridLevelEditorActor, bShowCoordinateGrid)
        || PropertyName == GET_MEMBER_NAME_CHECKED (AGridLevelEditorActor, CoordinateGridZOffset))
    {
        UpdateCoordinateGridPlane ();
        return;
    }
    if (PropertyName == GET_MEMBER_NAME_CHECKED (AGridLevelEditorActor, bShowCoordinateLabels)
        || PropertyName == GET_MEMBER_NAME_CHECKED (AGridLevelEditorActor, CoordinateLabelWorldSize))
    {
        UpdateCoordinateHoverLabel ();
        return;
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
        case EGridLevelObjectType::Receptacle:
            return true;
        default:
            return false;
    }
}

bool AGridLevelEditorActor::IsEdgePlacedObject (const FGridLevelObjectData& ObjectData) const
{
    return IsEdgePlacedObject (ObjectData.Type, ObjectData.ArchetypeId);
}

bool AGridLevelEditorActor::IsEdgePlacedObject (EGridLevelObjectType ObjectType, FName ArchetypeId) const
{
    if (const UGridObjectArchetypeAsset* Archetype = FindObjectArchetypeById (ArchetypeId))
    {
        return Archetype->IsEdgePlaced () || Archetype->IsWallPlaced ();
    }

    return RequiresEdge (ObjectType);
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

const UGridObjectArchetypeAsset* AGridLevelEditorActor::FindObjectArchetypeById (FName ArchetypeId) const
{
    if (ArchetypeId.IsNone () || !ObjectPalette)
    {
        return nullptr;
    }

    for (const FGridObjectPaletteEntry& Entry : ObjectPalette->Entries)
    {
        if (Entry.DefaultArchetype && Entry.DefaultArchetype->ArchetypeId == ArchetypeId)
        {
            return Entry.DefaultArchetype;
        }
    }

    return nullptr;
}

bool AGridLevelEditorActor::RotateSelectedObjectYawStep ()
{
    if (!LevelAsset || !LastSelectedObjectId.IsValid ())
    {
        return false;
    }
    FGridLevelObjectData* SelectedObject = nullptr;
    for (FGridLevelObjectData& ObjectData : LevelAsset->Objects)
    {
        if (ObjectData.ObjectId == LastSelectedObjectId)
        {
            SelectedObject = &ObjectData;
            break;
        }
    }
    if (!SelectedObject)
    {
        return false;
    }
    const UGridObjectArchetypeAsset* Archetype = FindObjectArchetypeById (SelectedObject->ArchetypeId);
    if (!Archetype)
    {
        return false;
    }
    const float Step = Archetype->RotationStepYaw > 0.f ? Archetype->RotationStepYaw : -90.f;
    Modify ();
    LevelAsset->Modify ();
    SelectedObject->LocalYaw = FMath::Fmod (SelectedObject->LocalYaw + Step, 360.f);

    if (SelectedObject->LocalYaw < 0.f)
    {
        SelectedObject->LocalYaw += 360.f;
    }
	RebuildPreview ();
    return true;
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

FVector AGridLevelEditorActor::GetSelectedCellWorldCenter (float ZOffset) const
{
    if (PreviewRuntimeActor)
    {
        return PreviewRuntimeActor->GetCellCenterWorld (SelectedCellX, SelectedCellY, ZOffset);
    }
    const float CellSize = LevelAsset ? LevelAsset->CellSize : 200.f;
    return GetActorLocation () + FVector::ZeroVector +
        FVector ((SelectedCellX * CellSize) + (CellSize * 0.5f), (SelectedCellY * CellSize) + (CellSize * 0.5f), ZOffset);
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
    RebuildGeometryPreview ();
}

void AGridLevelEditorActor::PaintSelectedWall ()
{
    FGridLevelCellData* CellData = GetSelectedCellMutable ();
    if (!CellData)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridLevelEditorActor: invalid selected cell."));
        return;
    }
    if (CellData->CellType == EGridCellType::Empty)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridLevelEditorActor: cannot paint wall on empty cell."));
        return;
    }
    EGridWallType* WallPtr = GetSelectedWallMutable (*CellData);
    if (!WallPtr)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridLevelEditorActor: SelectedEdge must be North/East/South/West."));
        return;
    }
#if WITH_EDITOR
    if (*WallPtr == PaintWallType)
    {
        return;
    }
    LevelAsset->Modify ();
#endif
    * WallPtr = PaintWallType;
#if WITH_EDITOR
    LevelAsset->MarkPackageDirty ();
#endif
    RebuildGeometryPreview ();
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
    if (*WallPtr == EGridWallType::None)
    {
        return;
    }
    LevelAsset->Modify ();
#endif

    * WallPtr = EGridWallType::None;

#if WITH_EDITOR
    LevelAsset->MarkPackageDirty ();
#endif

    RebuildGeometryPreview ();
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
        if (IsEdgePlacedObject (Obj))
        {
            bRemove = (Obj.Edge == SelectedEdge);
        } else
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
        LevelAsset->Links.RemoveAll ([&] (const FGridLevelLinkData& Link)
        {
            return RemovedIds.Contains (Link.SourceObjectId) || RemovedIds.Contains (Link.TargetObjectId);
        });
    }

#if WITH_EDITOR
    LevelAsset->MarkPackageDirty ();
#endif

    return RemovedIds.Num ();
}

int32 AGridLevelEditorActor::RemoveObjectsConflictingWithPlacementInternal (
    EGridLevelObjectType NewObjectType,
    FName NewArchetypeId,
    bool bNewObjectOnEdge)
{
    if (!HasValidLevelAsset () || !IsValidSelectedCell ())
    {
        return 0;
    }
    const UGridObjectArchetypeAsset* NewArchetype = FindObjectArchetypeById (NewArchetypeId);
    const bool bNewCanShareCell = NewArchetype ? NewArchetype->bCanShareCell : true;
    const bool bNewCanShareAnchor = NewArchetype ? NewArchetype->bCanShareAnchor : true;
    TArray<int32> IndicesToRemove;
    TArray<FGuid> RemovedIds;
    for (int32 Index = LevelAsset->Objects.Num () - 1; Index >= 0; --Index)
    {
        const FGridLevelObjectData& ExistingObject = LevelAsset->Objects[Index];
        if (ExistingObject.CellX != SelectedCellX || ExistingObject.CellY != SelectedCellY)
        {
            continue;
        }
        const UGridObjectArchetypeAsset* ExistingArchetype = FindObjectArchetypeById (ExistingObject.ArchetypeId);
        const bool bExistingCanShareCell = ExistingArchetype ? ExistingArchetype->bCanShareCell : true;
        const bool bExistingCanShareAnchor = ExistingArchetype ? ExistingArchetype->bCanShareAnchor : true;
        const bool bExistingObjectOnEdge = IsEdgePlacedObject (ExistingObject);
        const bool bSameAnchor = bNewObjectOnEdge && bExistingObjectOnEdge ? ExistingObject.Edge == SelectedEdge : !bNewObjectOnEdge && !bExistingObjectOnEdge;
        bool bShouldRemove = false;
        if (!bNewCanShareCell || !bExistingCanShareCell)
        {
            bShouldRemove = true;
        } else if (bSameAnchor && (!bNewCanShareAnchor || !bExistingCanShareAnchor))
        {
            bShouldRemove = true;
        }
        if (bShouldRemove)
        {
            RemovedIds.Add (ExistingObject.ObjectId);
            IndicesToRemove.Add (Index);
        }
    }
    if (IndicesToRemove.Num () == 0)
    {
        return 0;
    }
#if WITH_EDITOR
    LevelAsset->Modify ();
#endif
    for (int32 IndexToRemove : IndicesToRemove)
    {
        LevelAsset->Objects.RemoveAt (IndexToRemove);
    }
    LevelAsset->Links.RemoveAll ([&] (const FGridLevelLinkData& Link)
    {
        return RemovedIds.Contains (Link.SourceObjectId) || RemovedIds.Contains (Link.TargetObjectId);
    });
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
    const bool bPlaceObjectOnEdge = IsEdgePlacedObject (PaintObjectType, ObjectArchetypeId);
    if (bPlaceObjectOnEdge && SelectedEdge == EGridEdge::None)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridLevelEditorActor: this object type requires a valid edge."));
        return;
    }
    if (FindObjectArchetypeById (ObjectArchetypeId))
    {
        RemoveObjectsConflictingWithPlacementInternal (PaintObjectType, ObjectArchetypeId, bPlaceObjectOnEdge);
    } else
    {
        if (PlacementPolicy == EGridEditorObjectPlacementPolicy::ReplaceSameSlotOnly)
        {
            RemoveObjectsAtSelectionInternal (true);
        } else
        {
            RemoveObjectsAtSelectionInternal (false);
        }
    }
    FGridLevelObjectData NewObject;
    NewObject.Type = PaintObjectType;
    NewObject.CellX = SelectedCellX;
    NewObject.CellY = SelectedCellY;
    NewObject.Edge = bPlaceObjectOnEdge ? SelectedEdge : EGridEdge::None;
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
    ClearSelectedObjectState ();

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

        if (IsEdgePlacedObject (Obj) && Obj.Edge != SelectedEdge)
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

    ClearSelectedObjectState ();
    UE_LOG (LogTemp, Log, TEXT ("GridLevelEditorActor: no object found at current selection."));
}

bool AGridLevelEditorActor::TryConvertWorldHitToSelection (const FVector& WorldHitLocation, const FVector& /*HitNormal*/)
{
    return ApplyGridHoverFromWorldPoint (WorldHitLocation) && CommitHoveredCellSelection ();
}

bool AGridLevelEditorActor::ApplyViewportHitSelection (const FVector& WorldHitLocation, const FVector& HitNormal)
{
    return TryConvertWorldHitToSelection (WorldHitLocation, HitNormal);
}

bool AGridLevelEditorActor::IsSelectionValidForEditing () const
{
    return HasValidLevelAsset () && IsValidSelectedCell ();
}

bool AGridLevelEditorActor::SelectCellFromOverview (int32 CellX, int32 CellY)
{
    if (!HasValidLevelAsset () || !LevelAsset->IsValidCoord (CellX, CellY))
    {
        UE_LOG (
            LogTemp,
            Warning,
            TEXT ("GridLevelEditorActor: overview cell selection is outside grid bounds X=%d Y=%d."),
            CellX,
            CellY);
        return false;
    }

    Modify ();
    SelectedCellX = CellX;
    SelectedCellY = CellY;
    SelectedEdge = EGridEdge::None;
    HoveredCellX = CellX;
    HoveredCellY = CellY;
    HoveredEdge = EGridEdge::None;
    UpdateCoordinateHoverLabel ();
    return true;
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
        HoveredCellX = INDEX_NONE;
        HoveredCellY = INDEX_NONE;
        HoveredEdge = EGridEdge::None;
        UpdateCoordinateHoverLabel ();
        return false;
    }

    ResolvePreviewRuntimeActor ();

    const float CellSize = LevelAsset->CellSize;
    if (CellSize <= KINDA_SMALL_NUMBER)
    {
        HoveredCellX = INDEX_NONE;
        HoveredCellY = INDEX_NONE;
        HoveredEdge = EGridEdge::None;
        UpdateCoordinateHoverLabel ();
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
        HoveredCellX = INDEX_NONE;
        HoveredCellY = INDEX_NONE;
        HoveredEdge = EGridEdge::None;
        UpdateCoordinateHoverLabel ();
        return false;
    }

    const float LocalInCellX = Local.X - (static_cast<float>(NewCellX) * CellSize);
    const float LocalInCellY = Local.Y - (static_cast<float>(NewCellY) * CellSize);

    HoveredCellX = NewCellX;
    HoveredCellY = NewCellY;
    HoveredEdge = GetEdgeFromPointInCell (FVector2D (LocalInCellX, LocalInCellY), CellSize);
    UpdateCoordinateHoverLabel ();
    return true;
}

bool AGridLevelEditorActor::CommitHoveredCellSelection ()
{
    if (!HasValidLevelAsset () || !LevelAsset->IsValidCoord (HoveredCellX, HoveredCellY))
    {
        return false;
    }

    const bool bSelectionChanged =
        SelectedCellX != HoveredCellX ||
        SelectedCellY != HoveredCellY ||
        SelectedEdge != HoveredEdge;

    if (bSelectionChanged)
    {
        Modify ();
        SelectedCellX = HoveredCellX;
        SelectedCellY = HoveredCellY;
        SelectedEdge = HoveredEdge;
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
            if (!SelectHoveredObject ())
            {
                SelectObjectAtSelection ();
            }
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
            EraseAtSelection ();
            break;

        case EGridEditorTool::Link:
            SelectHoveredObject ();
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

        if (IsEdgePlacedObject (Obj) && Obj.Edge != SelectedEdge)
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

FGridLevelObjectData* AGridLevelEditorActor::FindSelectedObjectMutable ()
{
    if (!HasValidLevelAsset () || !LastSelectedObjectId.IsValid ())
    {
        return nullptr;
    }

    return LevelAsset->Objects.FindByPredicate (
        [this] (const FGridLevelObjectData& Obj)
    {
        return Obj.ObjectId == LastSelectedObjectId;
    });
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

    if (IsEdgePlacedObject (ObjectData))
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
            Link.SourceEvent == LinkSourceEvent &&
            Link.Action == LinkAction;
    });

    if (!bAlreadyExists)
    {
        FGridLevelLinkData NewLink;
        NewLink.SourceObjectId = PendingLinkSourceObjectId;
        NewLink.TargetObjectId = SelectedObject->ObjectId;
        NewLink.SourceEvent = LinkSourceEvent;
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

bool AGridLevelEditorActor::ApplyEditedSelectedObject ()
{
    if (!HasValidLevelAsset () || !LastSelectedObjectId.IsValid ())
    {
        return false;
    }

#if WITH_EDITOR
    LevelAsset->Modify ();
#endif

    for (FGridLevelObjectData& Obj : LevelAsset->Objects)
    {
        if (Obj.ObjectId != LastSelectedObjectId)
        {
            continue;
        }

        Obj.Type = PaintObjectType;
        Obj.Edge = IsEdgePlacedObject (PaintObjectType, ObjectArchetypeId) ? SelectedEdge : EGridEdge::None;
        Obj.ArchetypeId = ObjectArchetypeId;
        Obj.PaletteEntryId = SelectedPaletteEntryId;
        Obj.bInitiallyEnabled = bObjectInitiallyEnabled;
        Obj.bInitiallyActive = bObjectInitiallyActive;
        Obj.Tag = ObjectTag;
        Obj.Notes = ObjectNotes;
        Obj.Behavior = ObjectBehavior;

#if WITH_EDITOR
        LevelAsset->MarkPackageDirty ();
#endif

        RebuildPreview ();
        return true;
    }

    return false;
}

bool AGridLevelEditorActor::RemoveLinkByIndexForSelectedObject (int32 LinkIndex)
{
    if (!HasValidLevelAsset () || !LastSelectedObjectId.IsValid ())
    {
        return false;
    }

    int32 CurrentIndex = 0;

#if WITH_EDITOR
    LevelAsset->Modify ();
#endif

    for (int32 Index = 0; Index < LevelAsset->Links.Num (); ++Index)
    {
        const FGridLevelLinkData& Link = LevelAsset->Links[Index];

        if (Link.SourceObjectId != LastSelectedObjectId &&
            Link.TargetObjectId != LastSelectedObjectId)
        {
            continue;
        }

        if (CurrentIndex == LinkIndex)
        {
            LevelAsset->Links.RemoveAt (Index);

#if WITH_EDITOR
            LevelAsset->MarkPackageDirty ();
#endif

            RebuildPreview ();
            return true;
        }

        ++CurrentIndex;
    }
    return false;
}

bool AGridLevelEditorActor::RemoveAllLinksForSelectedObject ()
{
    if (!HasValidLevelAsset () || !LastSelectedObjectId.IsValid ())
    {
        return false;
    }

#if WITH_EDITOR
    LevelAsset->Modify ();
#endif

    const int32 RemovedCount = LevelAsset->Links.RemoveAll (
        [this] (const FGridLevelLinkData& Link)
    {
        return Link.SourceObjectId == LastSelectedObjectId ||
            Link.TargetObjectId == LastSelectedObjectId;
    });

    if (RemovedCount <= 0)
    {
        return false;
    }

#if WITH_EDITOR
    LevelAsset->MarkPackageDirty ();
#endif

    RebuildPreview ();
    return true;
}

void AGridLevelEditorActor::ClearSelectedObjectState ()
{
    LastSelectedObjectId.Invalidate ();

    PaintObjectType = EGridLevelObjectType::None;
    ObjectArchetypeId = NAME_None;
    SelectedArchetypeId = NAME_None;
    SelectedPaletteEntryId = NAME_None;

    bObjectInitiallyEnabled = true;
    bObjectInitiallyActive = false;

    ObjectTag = NAME_None;
    ObjectNotes.Empty ();
    ObjectBehavior = FGridObjectBehaviorParams ();
    ResolvePreviewRuntimeActor ();

    if (PreviewRuntimeActor)
    {
        PreviewRuntimeActor->SetEditorSelectedObject (FGuid ());
    }
}

bool AGridLevelEditorActor::RemoveExactLink (
    FGuid SourceObjectId,
    FGuid TargetObjectId,
    EGridObjectEventType SourceEvent,
    EGridLinkAction Action)
{
    if (!HasValidLevelAsset () || !SourceObjectId.IsValid () || !TargetObjectId.IsValid ())
    {
        return false;
    }

#if WITH_EDITOR
    LevelAsset->Modify ();
#endif

    const int32 RemovedCount = LevelAsset->Links.RemoveAll (
        [&] (const FGridLevelLinkData& Link)
    {
        return Link.SourceObjectId == SourceObjectId &&
            Link.TargetObjectId == TargetObjectId &&
            Link.SourceEvent == SourceEvent &&
            Link.Action == Action;
    });

    if (RemovedCount <= 0)
    {
        return false;
    }

#if WITH_EDITOR
    LevelAsset->MarkPackageDirty ();
#endif

    RebuildPreview ();
    return true;
}

const FGridLevelObjectData* AGridLevelEditorActor::GetSelectedObjectData () const
{
    return FindObjectById (LastSelectedObjectId);
}

bool AGridLevelEditorActor::SelectObjectById (FGuid ObjectId)
{
    if (!HasValidLevelAsset () || !ObjectId.IsValid ())
    {
        ClearSelectedObjectState ();
        return false;
    }

    const FGridLevelObjectData* Obj = FindObjectById (ObjectId);
    if (!Obj)
    {
        ClearSelectedObjectState ();
        return false;
    }

    LastSelectedObjectId = Obj->ObjectId;

    SelectedCellX = Obj->CellX;
    SelectedCellY = Obj->CellY;
    SelectedEdge = Obj->Edge;

    PaintObjectType = Obj->Type;
    ObjectArchetypeId = Obj->ArchetypeId;
    SelectedArchetypeId = Obj->ArchetypeId;
    SelectedPaletteEntryId = Obj->PaletteEntryId;

    bObjectInitiallyEnabled = Obj->bInitiallyEnabled;
    bObjectInitiallyActive = Obj->bInitiallyActive;

    ObjectTag = Obj->Tag;
    ObjectNotes = Obj->Notes;
    ObjectBehavior = Obj->Behavior;

    ResolvePreviewRuntimeActor ();

    if (PreviewRuntimeActor)
    {
        PreviewRuntimeActor->SetEditorSelectedObject (LastSelectedObjectId);
    }
    return true;
}

bool AGridLevelEditorActor::TryGetObjectWorldLocationById (
    FGuid ObjectId,
    FVector& OutWorldLocation) const
{
    const FGridLevelObjectData* Obj = FindObjectById (ObjectId);
    return Obj ? TryGetObjectWorldLocation (*Obj, OutWorldLocation) : false;
}

bool AGridLevelEditorActor::FocusSelectedObject ()
{
    if (!LastSelectedObjectId.IsValid ())
    {
        return false;
    }

    const FGridLevelObjectData* Obj = FindObjectById (LastSelectedObjectId);
    if (!Obj)
    {
        return false;
    }

    SelectedCellX = Obj->CellX;
    SelectedCellY = Obj->CellY;
    SelectedEdge = Obj->Edge;

#if WITH_EDITOR
    if (GEditor)
    {
        FVector WorldLocation = FVector::ZeroVector;
        if (TryGetObjectWorldLocation (*Obj, WorldLocation))
        {
            GEditor->MoveViewportCamerasToActor (*this, false);
        }
    }
#endif

    return true;
}

bool AGridLevelEditorActor::ApplyBehaviorToSelectedObject (
    const FGridObjectBehaviorParams& NewBehavior)
{
    if (!HasValidLevelAsset () || !LastSelectedObjectId.IsValid ())
    {
        return false;
    }

#if WITH_EDITOR
    LevelAsset->Modify ();
#endif

    for (FGridLevelObjectData& Obj : LevelAsset->Objects)
    {
        if (Obj.ObjectId != LastSelectedObjectId)
        {
            continue;
        }

        Obj.Behavior = NewBehavior;
        ObjectBehavior = NewBehavior;

#if WITH_EDITOR
        LevelAsset->MarkPackageDirty ();
#endif

        RebuildPreview ();
        return true;
    }

    return false;
}

bool AGridLevelEditorActor::SetSelectedObjectArchetypeId (FName NewArchetypeId)
{
    FGridLevelObjectData* Obj = FindSelectedObjectMutable ();
    if (!Obj)
    {
        return false;
    }

#if WITH_EDITOR
    LevelAsset->Modify ();
#endif

    Obj->ArchetypeId = NewArchetypeId;
    ObjectArchetypeId = NewArchetypeId;
    SelectedArchetypeId = NewArchetypeId;

#if WITH_EDITOR
    LevelAsset->MarkPackageDirty ();
#endif

    RebuildPreview ();
    return true;
}

bool AGridLevelEditorActor::SetSelectedObjectTag (FName NewTag)
{
    FGridLevelObjectData* Obj = FindSelectedObjectMutable ();
    if (!Obj)
    {
        return false;
    }

#if WITH_EDITOR
    LevelAsset->Modify ();
#endif

    Obj->Tag = NewTag;
    ObjectTag = NewTag;

#if WITH_EDITOR
    LevelAsset->MarkPackageDirty ();
#endif

    RebuildPreview ();
    return true;
}

bool AGridLevelEditorActor::SetSelectedObjectNotes (const FString& NewNotes)
{
    FGridLevelObjectData* Obj = FindSelectedObjectMutable ();
    if (!Obj)
    {
        return false;
    }

#if WITH_EDITOR
    LevelAsset->Modify ();
#endif

    Obj->Notes = NewNotes;
    ObjectNotes = NewNotes;

#if WITH_EDITOR
    LevelAsset->MarkPackageDirty ();
#endif

    RebuildPreview ();
    return true;
}

bool AGridLevelEditorActor::SetSelectedObjectReadableText (const FText& NewReadableText)
{
    FGridLevelObjectData* SelectedObject = FindSelectedObjectMutable ();
    if (!SelectedObject || !LevelAsset)
    {
        return false;
    }

#if WITH_EDITOR
    Modify ();
    LevelAsset->Modify ();
#endif

    SelectedObject->OverrideReadableText = NewReadableText;

#if WITH_EDITOR
    LevelAsset->MarkPackageDirty ();
#endif

    RebuildPreview ();
    return true;
}

bool AGridLevelEditorActor::SetSelectedObjectInitiallyEnabled (bool bNewInitiallyEnabled)
{
    FGridLevelObjectData* Obj = FindSelectedObjectMutable ();
    if (!Obj)
    {
        return false;
    }

#if WITH_EDITOR
    LevelAsset->Modify ();
#endif

    Obj->bInitiallyEnabled = bNewInitiallyEnabled;
    bObjectInitiallyEnabled = bNewInitiallyEnabled;

#if WITH_EDITOR
    LevelAsset->MarkPackageDirty ();
#endif

    RebuildPreview ();
    return true;
}

bool AGridLevelEditorActor::SetSelectedObjectInitiallyActive (bool bNewInitiallyActive)
{
    FGridLevelObjectData* Obj = FindSelectedObjectMutable ();
    if (!Obj)
    {
        return false;
    }

#if WITH_EDITOR
    LevelAsset->Modify ();
#endif

    Obj->bInitiallyActive = bNewInitiallyActive;
    bObjectInitiallyActive = bNewInitiallyActive;

#if WITH_EDITOR
    LevelAsset->MarkPackageDirty ();
#endif

    RebuildPreview ();
    return true;
}

bool AGridLevelEditorActor::SetSelectedObjectOverrideBehavior (bool bNewOverrideBehavior)
{
    FGridLevelObjectData* Obj = FindSelectedObjectMutable ();
    if (!Obj)
    {
        return false;
    }

#if WITH_EDITOR
    LevelAsset->Modify ();
#endif

    Obj->bOverrideBehavior = bNewOverrideBehavior;

#if WITH_EDITOR
    LevelAsset->MarkPackageDirty ();
#endif

    RebuildPreview ();
    return true;
}

bool AGridLevelEditorActor::MoveSelectedObjectToCurrentSelection ()
{
    if (!HasValidLevelAsset () || !IsValidSelectedCell ())
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridLevelEditorActor: cannot move selected object, destination cell is invalid."));
        return false;
    }

    FGridLevelObjectData* SelectedObject = FindSelectedObjectMutable ();
    if (!SelectedObject)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridLevelEditorActor: cannot move selected object, no object is selected."));
        return false;
    }

    const bool bRequiresEdge = IsEdgePlacedObject (*SelectedObject);
    if (bRequiresEdge && SelectedEdge == EGridEdge::None)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridLevelEditorActor: cannot move selected edge-based object to Edge=None."));
        return false;
    }

    const EGridEdge DestinationEdge = bRequiresEdge ? SelectedEdge : EGridEdge::None;
    const bool bAlreadyAtDestination = SelectedObject->CellX == SelectedCellX &&
        SelectedObject->CellY == SelectedCellY &&
        SelectedObject->Edge == DestinationEdge;

    if (bAlreadyAtDestination)
    {
        UE_LOG (LogTemp, Log, TEXT ("GridLevelEditorActor: selected object is already at the current selection."));
        return true;
    }

    const FGuid SelectedObjectId = SelectedObject->ObjectId;
    const EGridLevelObjectType SelectedObjectType = SelectedObject->Type;
    const bool bDestinationOccupied = LevelAsset->Objects.ContainsByPredicate (
        [this, SelectedObjectId, SelectedObjectType, bRequiresEdge, DestinationEdge] (const FGridLevelObjectData& Obj)
    {
        if (Obj.ObjectId == SelectedObjectId ||
            Obj.CellX != SelectedCellX ||
            Obj.CellY != SelectedCellY ||
            Obj.Type != SelectedObjectType)
        {
            return false;
        }

        if (bRequiresEdge)
        {
            return Obj.Edge == DestinationEdge;
        }

        return true;
    });

    if (bDestinationOccupied)
    {
        UE_LOG (
            LogTemp,
            Warning,
            TEXT ("GridLevelEditorActor: cannot move selected object, destination already contains an object of the same type."));
        return false;
    }

#if WITH_EDITOR
    LevelAsset->Modify ();
#endif

    SelectedObject->CellX = SelectedCellX;
    SelectedObject->CellY = SelectedCellY;
    SelectedObject->Edge = DestinationEdge;

#if WITH_EDITOR
    LevelAsset->MarkPackageDirty ();
#endif

    RebuildPreview ();
    UE_LOG (
        LogTemp,
        Log,
        TEXT ("GridLevelEditorActor: moved selected object %s to X=%d Y=%d Edge=%d."),
        *SelectedObjectId.ToString (),
        SelectedCellX,
        SelectedCellY,
        static_cast<int32> (DestinationEdge));
    return true;
}

TArray<FGridLevelValidationMessage> AGridLevelEditorActor::ValidateCurrentLevel ()
{
    LastValidationMessages.Reset ();

    auto AddMessage = [this] (
        EGridLevelValidationSeverity Severity,
        const FString& Message,
        const FGuid& OptionalObjectId = FGuid ())
    {
        FGridLevelValidationMessage ValidationMessage;
        ValidationMessage.Severity = Severity;
        ValidationMessage.Message = Message;
        ValidationMessage.OptionalObjectId = OptionalObjectId;
        LastValidationMessages.Add (ValidationMessage);
    };

    auto AddArchetypeValidationMessages = [this, &AddMessage] ()
    {
        if (!ObjectPalette)
        {
            return;
        }

        TSet<const UGridObjectArchetypeAsset*> ValidatedArchetypes;

        for (const FGridObjectPaletteEntry& Entry : ObjectPalette->Entries)
        {
            const UGridObjectArchetypeAsset* Archetype = Entry.DefaultArchetype.Get ();
            if (!Archetype || ValidatedArchetypes.Contains (Archetype))
            {
                continue;
            }

            ValidatedArchetypes.Add (Archetype);

            TArray<FGridArchetypeValidationMessage> ArchetypeMessages;
            Archetype->ValidateArchetype (ArchetypeMessages);

            const FString ArchetypeName = Archetype->ArchetypeId.IsNone ()
                ? Archetype->GetName ()
                : Archetype->ArchetypeId.ToString ();

            for (const FGridArchetypeValidationMessage& ArchetypeMessage : ArchetypeMessages)
            {
                AddMessage (
                    ConvertArchetypeValidationSeverity (ArchetypeMessage.Severity),
                    FString::Printf (
                        TEXT ("Archetype %s: %s"),
                        *ArchetypeName,
                        *ArchetypeMessage.Message));
            }
        }
    };

    AddArchetypeValidationMessages ();

    if (!LevelAsset)
    {
        AddMessage (
            EGridLevelValidationSeverity::Error,
            TEXT ("LevelAsset is missing."));
        return LastValidationMessages;
    }

    TSet<FGuid> SeenObjectIds;
    TSet<FGuid> ObjectIds;
    TMap<FGuid, int32> OutgoingLinkCountBySourceId;

    auto IsEdgeOrWallPlacedObject = [this] (const FGridLevelObjectData& ObjectData) -> bool
    {
        if (const UGridObjectArchetypeAsset* Archetype = FindObjectArchetypeById (ObjectData.ArchetypeId))
        {
            return Archetype->IsEdgePlaced () || Archetype->IsWallPlaced ();
        }

        return RequiresEdge (ObjectData.Type);
    };

    auto GetValidationAnchorKey = [&IsEdgeOrWallPlacedObject] (const FGridLevelObjectData& ObjectData) -> FString
    {
        if (!IsEdgeOrWallPlacedObject (ObjectData))
        {
            return TEXT ("Center");
        }

        switch (ObjectData.Edge)
        {
            case EGridEdge::North:
                return TEXT ("North");

            case EGridEdge::East:
                return TEXT ("East");

            case EGridEdge::South:
                return TEXT ("South");

            case EGridEdge::West:
                return TEXT ("West");

            case EGridEdge::None:
            default:
                return TEXT ("Center");
        }
    };

    for (const FGridLevelObjectData& Obj : LevelAsset->Objects)
    {
        if (!Obj.ObjectId.IsValid ())
        {
            AddMessage (
                EGridLevelValidationSeverity::Error,
                FString::Printf (
                    TEXT ("Object at X=%d Y=%d has an invalid ObjectId."),
                    Obj.CellX,
                    Obj.CellY));
        } else if (SeenObjectIds.Contains (Obj.ObjectId))
        {
            AddMessage (
                EGridLevelValidationSeverity::Error,
                TEXT ("Duplicate ObjectId found."),
                Obj.ObjectId);
        } else
        {
            SeenObjectIds.Add (Obj.ObjectId);
            ObjectIds.Add (Obj.ObjectId);
        }

        if (!LevelAsset->IsValidCoord (Obj.CellX, Obj.CellY))
        {
            AddMessage (
                EGridLevelValidationSeverity::Error,
                FString::Printf (
                    TEXT ("Object is outside grid bounds at X=%d Y=%d."),
                    Obj.CellX,
                    Obj.CellY),
                Obj.ObjectId);
            continue;
        }

        const UGridObjectArchetypeAsset* Archetype = FindObjectArchetypeById (Obj.ArchetypeId);
        if (IsEdgeOrWallPlacedObject (Obj) && Obj.Edge == EGridEdge::None)
        {
            AddMessage (
                EGridLevelValidationSeverity::Warning,
                TEXT ("Edge or wall placed object has Edge=None."),
                Obj.ObjectId);
        }

        if (Archetype && Archetype->bBlocksMovement)
        {
            const FGridLevelCellData& CellData = LevelAsset->GetCell (Obj.CellX, Obj.CellY);
            if (CellData.bBlocksOccupancy)
            {
                AddMessage (
                    EGridLevelValidationSeverity::Warning,
                    TEXT ("Object blocks movement on a cell that already blocks occupancy."),
                    Obj.ObjectId);
            }
        }

        if (Obj.Type == EGridLevelObjectType::Door)
        {
            const FGridLevelCellData& CellData = LevelAsset->GetCell (Obj.CellX, Obj.CellY);
            const EGridWallType WallType = GetWallTypeForEdge (CellData, Obj.Edge);
            if (WallType == EGridWallType::Solid)
            {
                AddMessage (
                    EGridLevelValidationSeverity::Warning,
                    TEXT ("Door is placed on an edge whose wall is Solid. A door edge must use WallType=None."),
                    Obj.ObjectId);
            }
        }

        if (Obj.Type == EGridLevelObjectType::Receptacle)
        {
            const FGridObjectBehaviorParams& Behavior = Obj.Behavior;
            if (!Behavior.InitialContainedItemArchetypeId.IsNone ()
                && !Behavior.AcceptedArchetypeIds.Contains (Behavior.InitialContainedItemArchetypeId))
            {
                AddMessage (
                    EGridLevelValidationSeverity::Warning,
                    FString::Printf (
                        TEXT ("Receptacle starts with '%s' but AcceptedArchetypeIds does not include it."),
                        *Behavior.InitialContainedItemArchetypeId.ToString ()),
                    Obj.ObjectId);
            }

            if (!Behavior.bAcceptAnyItem
                && Behavior.AcceptedItemTags.Num () == 0
                && Behavior.AcceptedArchetypeIds.Num () == 0)
            {
                AddMessage (
                    EGridLevelValidationSeverity::Error,
                    TEXT ("Receptacle accepts no item: bAcceptAnyItem=false and accepted lists are empty."),
                    Obj.ObjectId);
            }
        }
    }

    for (int32 ObjectIndex = 0; ObjectIndex < LevelAsset->Objects.Num (); ++ObjectIndex)
    {
        const FGridLevelObjectData& ObjectA = LevelAsset->Objects[ObjectIndex];
        const UGridObjectArchetypeAsset* ArchetypeA = FindObjectArchetypeById (ObjectA.ArchetypeId);
        if (!ArchetypeA || !LevelAsset->IsValidCoord (ObjectA.CellX, ObjectA.CellY))
        {
            continue;
        }

        const FString AnchorA = GetValidationAnchorKey (ObjectA);
        for (int32 OtherIndex = 0; OtherIndex < LevelAsset->Objects.Num (); ++OtherIndex)
        {
            if (ObjectIndex == OtherIndex)
            {
                continue;
            }

            const FGridLevelObjectData& ObjectB = LevelAsset->Objects[OtherIndex];
            if (ObjectA.CellX != ObjectB.CellX || ObjectA.CellY != ObjectB.CellY)
            {
                continue;
            }

            if (!ArchetypeA->bCanShareCell)
            {
                AddMessage (
                    EGridLevelValidationSeverity::Warning,
                    TEXT ("Object does not allow sharing its cell but another object is placed there."),
                    ObjectA.ObjectId);
                break;
            }

            if (!ArchetypeA->bCanShareAnchor && AnchorA == GetValidationAnchorKey (ObjectB))
            {
                AddMessage (
                    EGridLevelValidationSeverity::Warning,
                    FString::Printf (TEXT ("Object does not allow sharing anchor '%s' but another object uses it."), *AnchorA),
                    ObjectA.ObjectId);
                break;
            }
        }
    }

    for (const FGridLevelLinkData& Link : LevelAsset->Links)
    {
        if (!ObjectIds.Contains (Link.SourceObjectId))
        {
            AddMessage (
                EGridLevelValidationSeverity::Error,
                TEXT ("Link SourceObjectId was not found."),
                Link.SourceObjectId);
        } else
        {
            int32& OutgoingCount = OutgoingLinkCountBySourceId.FindOrAdd (Link.SourceObjectId);
            ++OutgoingCount;
        }

        if (!ObjectIds.Contains (Link.TargetObjectId))
        {
            AddMessage (
                EGridLevelValidationSeverity::Error,
                TEXT ("Link TargetObjectId was not found."),
                Link.TargetObjectId);
        }
    }

    for (const FGridLevelObjectData& Obj : LevelAsset->Objects)
    {
        if (Obj.Type == EGridLevelObjectType::Trigger && !OutgoingLinkCountBySourceId.Contains (Obj.ObjectId))
        {
            AddMessage (
                EGridLevelValidationSeverity::Warning,
                TEXT ("Trigger has no outgoing links."),
                Obj.ObjectId);
        }
    }

    if (LastValidationMessages.Num () == 0)
    {
        AddMessage (
            EGridLevelValidationSeverity::Info,
            TEXT ("Validation complete: no issues found."));
    }

    return LastValidationMessages;
}

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
