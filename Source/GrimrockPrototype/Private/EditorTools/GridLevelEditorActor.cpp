#include "EditorTools/GridLevelEditorActor.h"

#include "Kismet/GameplayStatics.h"
#include "Runtime/GridLevelRuntimeActor.h"

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

void AGridLevelEditorActor::PickSelectionFromViewport ()
{
#if WITH_EDITOR
    if (!HasValidLevelAsset ())
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridLevelEditorActor: LevelAsset is null."));
        return;
    }

    ResolvePreviewRuntimeActor ();

    if (!GetWorld ())
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridLevelEditorActor: no editor world."));
        return;
    }

    UUnrealEditorSubsystem* EditorSubsystem = GEditor
        ? GEditor->GetEditorSubsystem<UUnrealEditorSubsystem> ()
        : nullptr;

    if (!EditorSubsystem)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridLevelEditorActor: UUnrealEditorSubsystem unavailable."));
        return;
    }

    FVector CameraLocation = FVector::ZeroVector;
    FRotator CameraRotation = FRotator::ZeroRotator;

    if (!EditorSubsystem->GetLevelViewportCameraInfo (CameraLocation, CameraRotation))
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridLevelEditorActor: unable to read level viewport camera."));
        return;
    }

    const FVector TraceStart = CameraLocation;
    const FVector TraceEnd = TraceStart + (CameraRotation.Vector () * ViewportPickTraceDistance);

    FHitResult Hit;
    FCollisionQueryParams QueryParams (SCENE_QUERY_STAT (GridEditorViewportPick), true);
    QueryParams.AddIgnoredActor (this);

    // à garder ou non selon votre test
    // if (PreviewRuntimeActor)
    // {
    //     QueryParams.AddIgnoredActor (PreviewRuntimeActor);
    // }

    const bool bHit = GetWorld ()->LineTraceSingleByChannel (
        Hit,
        TraceStart,
        TraceEnd,
        ECC_Visibility,
        QueryParams);

    if (!bHit)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridLevelEditorActor: viewport pick trace hit nothing."));
        return;
    }

    if (!TryConvertWorldHitToSelection (Hit.ImpactPoint, Hit.ImpactNormal))
    {
        UE_LOG (
            LogTemp,
            Warning,
            TEXT ("GridLevelEditorActor: trace hit at %s but conversion to grid selection failed."),
            *Hit.ImpactPoint.ToString ());
        return;
    }

    if (bSnapAfterViewportPick)
    {
        const bool bWasAuto = bAutoSelectFromActorTransform;
        bAutoSelectFromActorTransform = false;
        SnapActorToSelectedCell ();
        bAutoSelectFromActorTransform = bWasAuto;
    }

    UE_LOG (
        LogTemp,
        Log,
        TEXT ("GridLevelEditorActor: viewport picked Cell=(%d,%d), Edge=%d"),
        SelectedCellX,
        SelectedCellY,
        static_cast<int32>(SelectedEdge));
#endif
}

void AGridLevelEditorActor::PickSelectionAndPlaceObjectFromViewport ()
{
    PickSelectionFromViewport ();
    PlaceSelectedObject ();
}

void AGridLevelEditorActor::Tick (float DeltaTime)
{
    Super::Tick (DeltaTime);

#if WITH_EDITOR
    if (!GetWorld () || GetWorld ()->IsGameWorld ())
    {
        return;
    }

    // Debug : dessiner la cellule sélectionnée
    if (HasValidLevelAsset () && IsValidSelectedCell ())
    {
        const FVector Center = GetSelectedCellWorldCenter (5.f);
        const float HalfSize = LevelAsset->CellSize * 0.5f;

        DrawDebugBox (
            GetWorld (),
            Center,
            FVector (HalfSize, HalfSize, 5.f),
            FColor::Yellow,
            false,
            0.f,
            0,
            2.f
        );
    }

    if (bIsPainting)
    {
        PickSelectionFromViewport ();
        PlaceSelectedObject ();
    }
#endif
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

FVector AGridLevelEditorActor::GetSelectionPreviewCenter () const
{
    return GetSelectedCellWorldCenter (4.f);
}