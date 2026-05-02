#include "EditorTools/GridLevelEditorActor.h"

#include "Kismet/GameplayStatics.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Core/GridObjectPaletteAsset.h"
#include "Core/GridObjectArchetypeAsset.h"
#include "Components/TextRenderComponent.h"

#if WITH_EDITOR
#include "Subsystems/UnrealEditorSubsystem.h"
#include "Editor.h"
#endif

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
    RebuildCoordinateGrid ();
}

#if WITH_EDITOR
void AGridLevelEditorActor::PostEditChangeProperty (FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty (PropertyChangedEvent);

    const FName PropertyName = PropertyChangedEvent.Property ? PropertyChangedEvent.Property->GetFName () : NAME_None;

    if (PropertyName == GET_MEMBER_NAME_CHECKED (AGridLevelEditorActor, LevelAsset))
    {
        RebuildPreview ();
        RebuildCoordinateGrid ();
        return;
    }
    if (PropertyName == GET_MEMBER_NAME_CHECKED (AGridLevelEditorActor, CoordinateGridPlaneMesh)
        ||
        PropertyName == GET_MEMBER_NAME_CHECKED (AGridLevelEditorActor, CoordinateGridMaterial)
        ||
        PropertyName == GET_MEMBER_NAME_CHECKED (AGridLevelEditorActor, bShowCoordinateGrid)
        ||
        PropertyName == GET_MEMBER_NAME_CHECKED (AGridLevelEditorActor, bShowCoordinateLabels)
        ||
        PropertyName == GET_MEMBER_NAME_CHECKED (AGridLevelEditorActor, CoordinateLabelWorldSize))
    {
        RebuildCoordinateGrid ();
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

void AGridLevelEditorActor::RefreshPreview ()
{
    RebuildPreview ();
    RebuildCoordinateGrid ();
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

        if (RequiresEdge (Obj.Type))
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

    ClearSelectedObjectState ();
    UE_LOG (LogTemp, Log, TEXT ("GridLevelEditorActor: no object found at current selection."));
}

bool AGridLevelEditorActor::TryConvertWorldHitToSelection (const FVector& WorldHitLocation, const FVector& HitNormal)
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
    const float LocalInCellX = Local.X - (static_cast<float> (NewCellX) * CellSize);
    const float LocalInCellY = Local.Y - (static_cast<float> (NewCellY) * CellSize);

    SelectedEdge = GetEdgeFromPointInCell (FVector2D (LocalInCellX, LocalInCellY),CellSize);
    return true;
}

bool AGridLevelEditorActor::ApplyViewportHitSelection (const FVector& WorldHitLocation, const FVector& HitNormal)
{
    return TryConvertWorldHitToSelection (WorldHitLocation, HitNormal);
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
            PaintSelectedCellAndWall ();
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
        Obj.Edge = RequiresEdge (PaintObjectType) ? SelectedEdge : EGridEdge::None;
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

bool AGridLevelEditorActor::RemoveExactLink (FGuid SourceObjectId, FGuid TargetObjectId, EGridLinkAction Action)
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

    SelectedCellX = BestObject->CellX;
    SelectedCellY = BestObject->CellY;
    SelectedEdge = BestObject->Edge;

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

void AGridLevelEditorActor::PaintSelectedCellAndWall ()
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

    if (EGridWallType* WallPtr = GetSelectedWallMutable (*CellData))
    {
        *WallPtr = PaintWallType;
    }

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

void AGridLevelEditorActor::ClearCoordinateLabels ()
{
    for (UTextRenderComponent* Label : CoordinateLabels)
    {
        if (!Label)
        {
            continue;
        }
        RemoveInstanceComponent (Label);
        Label->DestroyComponent ();
    }
    CoordinateLabels.Empty ();
}

void AGridLevelEditorActor::RebuildCoordinateGrid ()
{
    ClearCoordinateLabels ();
    if (!LevelAsset || !CoordinateGridPlane)
    {
        return;
    }
    const float CellSize = LevelAsset->CellSize;
    const int32 Width = LevelAsset->Width;
    const int32 Height = LevelAsset->Height;

    const FRotator LabelRotation (90.f, -90.f, 0.f);
    const bool bShowGrid = bShowCoordinateGrid && CoordinateGridPlaneMesh != nullptr;
    CoordinateGridPlane->SetVisibility (bShowGrid);
    if (bShowGrid)
    {
        CoordinateGridPlane->SetStaticMesh (CoordinateGridPlaneMesh);
        if (CoordinateGridMaterial)
        {
            CoordinateGridPlane->SetMaterial (0, CoordinateGridMaterial);
        }
        CoordinateGridPlane->SetRelativeLocation (
            FVector (Width * CellSize * 0.5f, Height * CellSize * 0.5f, CoordinateGridZOffset));

        CoordinateGridPlane->SetRelativeScale3D (
            FVector (Width * CellSize / 100.f, Height * CellSize / 100.f, 1.f));
    }
    if (!bShowCoordinateLabels)
    {
        return;
    }
    for (int32 Y = 0; Y < Height; ++Y)
    {
        for (int32 X = 0; X < Width; ++X)
        {
            UTextRenderComponent* Label = NewObject<UTextRenderComponent> (this, UTextRenderComponent::StaticClass (), NAME_None,
                    RF_Transactional);
            if (!Label)
            {
                continue;
            }
            Label->CreationMethod = EComponentCreationMethod::Instance;
            AddInstanceComponent (Label);
            Label->AttachToComponent (SceneRoot, FAttachmentTransformRules::KeepRelativeTransform);

            Label->SetCollisionEnabled (ECollisionEnabled::NoCollision);
            Label->SetHiddenInGame (false);
            Label->SetVisibility (true, true);
            Label->SetHorizontalAlignment (EHTA_Center);
            Label->SetVerticalAlignment (EVRTA_TextCenter);
            Label->SetWorldSize (CoordinateLabelWorldSize);
            Label->SetTextRenderColor (FColor::White);
            Label->SetRelativeRotation (LabelRotation);

            Label->SetRelativeLocation (FVector ((X + 0.5f) * CellSize, (Y + 0.5f) * CellSize, CoordinateGridZOffset));
            
            Label->SetText (FText::FromString (FString::Printf (TEXT ("%d,%d"), X, Y)));

            Label->RegisterComponentWithWorld (GetWorld ());
            Label->MarkRenderStateDirty ();
            CoordinateLabels.Add (Label);
        }
    }
}