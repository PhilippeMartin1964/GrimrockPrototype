#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridDoorActor.h"
#include "Core/GridTypes.h"
#include "Core/GridObjectArchetypeAsset.h"
#include "Runtime/GridButtonActor.h"
#include "Runtime/GridLeverActor.h"
#include "Runtime/GridPressurePlateActor.h"
#include "Runtime/GridEditorPreviewObjectActor.h"
#include "EngineUtils.h"

AGridLevelRuntimeActor::AGridLevelRuntimeActor ()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent> (TEXT ("Root"));
    SetRootComponent (SceneRoot);

    FloorISM = CreateDefaultSubobject<UInstancedStaticMeshComponent> (TEXT ("FloorISM"));
    FloorISM->SetupAttachment (SceneRoot);

    WallISM = CreateDefaultSubobject<UInstancedStaticMeshComponent> (TEXT ("WallISM"));
    WallISM->SetupAttachment (SceneRoot);

    SecretWallISM = CreateDefaultSubobject<UInstancedStaticMeshComponent> (TEXT ("SecretWallISM"));
    SecretWallISM->SetupAttachment (SceneRoot);

    CeilingISM = CreateDefaultSubobject<UInstancedStaticMeshComponent> (TEXT ("CeilingISM"));
    CeilingISM->SetupAttachment (SceneRoot);

    EditorSolidBlockISM = CreateDefaultSubobject<UInstancedStaticMeshComponent> (TEXT ("EditorSolidBlockISM"));
    EditorSolidBlockISM->SetupAttachment (SceneRoot);
}

void AGridLevelRuntimeActor::OnConstruction (const FTransform& Transform)
{
    Super::OnConstruction (Transform);

    if (bRebuildInConstruction)
    {
        RebuildLevel ();
    }
}

void AGridLevelRuntimeActor::BeginPlay ()
{
    Super::BeginPlay ();
    RebuildLevel ();
}

void AGridLevelRuntimeActor::ClearVisuals ()
{
    if (FloorISM) FloorISM->ClearInstances ();
    if (WallISM) WallISM->ClearInstances ();
    if (SecretWallISM) SecretWallISM->ClearInstances ();

    if (CeilingISM)
    {
        CeilingISM->ClearInstances ();
        CeilingISM->EmptyOverrideMaterials ();
    }

    ClearRuntimeObjectActors ();
    RuntimeBlockedDoorEdges.Empty ();
    ClearEditorPreviewObjects ();
    ActiveObjectIds.Empty ();
    if (EditorSolidBlockISM)
    {
        EditorSolidBlockISM->ClearInstances ();
        EditorSolidBlockISM->EmptyOverrideMaterials ();
    }
}

bool AGridLevelRuntimeActor::IsValidCell (int32 X, int32 Y) const
{
    return LevelAsset && LevelAsset->IsValidCoord (X, Y);
}

const FGridLevelCellData& AGridLevelRuntimeActor::GetCell (int32 X, int32 Y) const
{
    check (LevelAsset);
    return LevelAsset->GetCell (X, Y);
}

FVector AGridLevelRuntimeActor::CellToWorld (int32 X, int32 Y, float ZOffset) const
{
    const float Size = LevelAsset ? LevelAsset->CellSize : 200.f;
    return GridOrigin + FVector (X * Size, Y * Size, ZOffset);
}

FVector AGridLevelRuntimeActor::GetCellCenterWorld (int32 X, int32 Y, float ZOffset) const
{
    const float CellSize = LevelAsset ? LevelAsset->CellSize : 200.f;

    return GetActorLocation () + GridOrigin + FVector (
        (X * CellSize) + (CellSize * 0.5f),
        (Y * CellSize) + (CellSize * 0.5f),
        ZOffset
    );
}

void AGridLevelRuntimeActor::AddFloor (int32 X, int32 Y, float CellSize)
{
    const FVector Base = CellToWorld (X, Y, 0.f);
    const FVector CenterOffset (CellSize * 0.5f, CellSize * 0.5f, 0.f);

    const FTransform T (
        FRotator::ZeroRotator,
        Base + CenterOffset,
        FVector (CellSize / 100.f, CellSize / 100.f, 1.f)
    );

    FloorISM->AddInstance (T);
}

void AGridLevelRuntimeActor::AddCeiling (int32 X, int32 Y, float CellSize)
{
    const FVector Base = CellToWorld (X, Y, 200.f);
    const FVector CenterOffset (CellSize * 0.5f, CellSize * 0.5f, 0.f);

    const FTransform T (
        FRotator::ZeroRotator,
        Base + CenterOffset,
        FVector (1.f, 1.f, 1.f)
    );

    CeilingISM->AddInstance (T);
}

void AGridLevelRuntimeActor::AddEdgeInstance (UInstancedStaticMeshComponent* TargetISM, int32 X, int32 Y, EGridEdge Edge, float CellSize)
{
    if (!TargetISM)
    {
        return;
    }

    const FVector Base = CellToWorld (X, Y, 0.f);

    FVector Pos = Base;
    FRotator Rot = FRotator::ZeroRotator;
    FVector Scale (CellSize / 100.f, 1.f, 1.f);

    switch (Edge)
    {
        case EGridEdge::North:
            Pos = Base + FVector (CellSize * 0.5f, CellSize, 0.f);
            Rot = FRotator (0.f, 0.f, 0.f);
            break;

        case EGridEdge::East:
            Pos = Base + FVector (CellSize, CellSize * 0.5f, 0.f);
            Rot = FRotator (0.f, 90.f, 0.f);
            break;

        case EGridEdge::South:
            Pos = Base + FVector (CellSize * 0.5f, 0.f, 0.f);
            Rot = FRotator (0.f, 0.f, 0.f);
            break;

        case EGridEdge::West:
            Pos = Base + FVector (0.f, CellSize * 0.5f, 0.f);
            Rot = FRotator (0.f, 90.f, 0.f);
            break;

        default:
            return;
    }

    const FTransform T (Rot, Pos, Scale);
    TargetISM->AddInstance (T);
}

void AGridLevelRuntimeActor::RebuildLevel ()
{
    ClearVisuals ();

    if (!LevelAsset || !FloorISM || !WallISM || !SecretWallISM || !CeilingISM)
    {
        return;
    }

    LevelAsset->EnsureCellCount ();

    FloorISM->SetStaticMesh (FloorMesh);
    WallISM->SetStaticMesh (WallMesh);
    SecretWallISM->SetStaticMesh (SecretWallMesh);
    CeilingISM->SetStaticMesh (CeilingMesh);

    if (EditorSolidBlockISM)
    {
        EditorSolidBlockISM->SetStaticMesh (EditorSolidBlockMesh);

        if (EditorSolidBlockMaterial)
        {
            EditorSolidBlockISM->SetMaterial (0, EditorSolidBlockMaterial);
        }
    }

    const bool bIsGameWorld = GetWorld () && GetWorld ()->IsGameWorld ();

    UMaterialInterface* DesiredCeilingMaterial = nullptr;

    if (bIsGameWorld)
    {
        DesiredCeilingMaterial = CeilingMaterial;
    } else
    {
        DesiredCeilingMaterial = CeilingEditorMaterial
            ? CeilingEditorMaterial
            : CeilingMaterial;
    }

    CeilingISM->SetMaterial (0, DesiredCeilingMaterial);

    const float CellSize = LevelAsset->CellSize;
    TArray<FTransform> EditorSolidBlockTransforms;

    if (!bIsGameWorld &&
        bShowEditorSolidBlocks &&
        EditorSolidBlockISM &&
        EditorSolidBlockMesh)
    {
        EditorSolidBlockTransforms.Reserve (LevelAsset->Width * LevelAsset->Height);
    }

    for (int32 Y = 0; Y < LevelAsset->Height; ++Y)
    {
        for (int32 X = 0; X < LevelAsset->Width; ++X)
        {
            const FGridLevelCellData& Cell = LevelAsset->GetCell (X, Y);

            if (Cell.CellType == EGridCellType::Empty)
            {
                if (!bIsGameWorld &&
                    bShowEditorSolidBlocks &&
                    EditorSolidBlockISM &&
                    EditorSolidBlockMesh)
                {
                    const FVector Base = CellToWorld (X, Y, 0.f);

                    const FVector Pos =
                        Base + FVector (
                            CellSize * 0.5f,
                            CellSize * 0.5f,
                            0.f);

                    EditorSolidBlockTransforms.Add (
                        FTransform (
                            FRotator::ZeroRotator,
                            Pos,
                            FVector (
                                CellSize / 100.f,
                                CellSize / 100.f,
                                EditorSolidBlockHeight / 100.f)));
                }

                continue;
            }

            AddFloor (X, Y, CellSize);

            if (Cell.bHasCeiling)
            {
                AddCeiling (X, Y, CellSize);
            }

            auto DrawEdgeIfNeeded =
                [&] (EGridEdge Edge, EGridWallType WallType, bool bShouldDraw)
            {
                if (!bShouldDraw)
                {
                    return;
                }

                switch (WallType)
                {
                    case EGridWallType::Solid:
                    AddEdgeInstance (WallISM, X, Y, Edge, CellSize);
                    break;

                    case EGridWallType::Door:
                    case EGridWallType::DoorOpen:
                    break;

                    case EGridWallType::Secret:
                    AddEdgeInstance (SecretWallISM, X, Y, Edge, CellSize);
                    break;

                    default:
                    break;
                }
            };

            DrawEdgeIfNeeded (
                EGridEdge::North,
                Cell.NorthWall,
                Cell.NorthWall != EGridWallType::None);

            DrawEdgeIfNeeded (
                EGridEdge::East,
                Cell.EastWall,
                Cell.EastWall != EGridWallType::None);

            const bool bDrawSouth =
                Cell.SouthWall != EGridWallType::None &&
                (!LevelAsset->IsValidCoord (X, Y - 1) ||
                    LevelAsset->GetCell (X, Y - 1).CellType == EGridCellType::Empty);

            DrawEdgeIfNeeded (
                EGridEdge::South,
                Cell.SouthWall,
                bDrawSouth);

            const bool bDrawWest =
                Cell.WestWall != EGridWallType::None &&
                (!LevelAsset->IsValidCoord (X - 1, Y) ||
                    LevelAsset->GetCell (X - 1, Y).CellType == EGridCellType::Empty);

            DrawEdgeIfNeeded (
                EGridEdge::West,
                Cell.WestWall,
                bDrawWest);
        }
    }
    if (!GetWorld () || !GetWorld ()->IsGameWorld ())
    {
        RebuildEditorPreviewObjects ();
    }
    if (EditorSolidBlockTransforms.Num () > 0 && EditorSolidBlockISM)
    {
        EditorSolidBlockISM->AddInstances (
            EditorSolidBlockTransforms,
            false,
            true);
    }
    if (bIsGameWorld)
    {
        RebuildRuntimeObjects ();
    }
}

bool AGridLevelRuntimeActor::IsWalkableCell (int32 X, int32 Y) const
{
    if (!LevelAsset || !LevelAsset->IsValidCoord (X, Y))
    {
        return false;
    }

    const FGridLevelCellData& Cell = LevelAsset->GetCell (X, Y);

    if (Cell.CellType == EGridCellType::Empty)
    {
        return false;
    }

    if (Cell.bBlocksOccupancy)
    {
        return false;
    }

    return true;
}

bool AGridLevelRuntimeActor::TryGetNeighborCell (int32 X, int32 Y, EGridEdge Direction, int32& OutX, int32& OutY) const
{
    OutX = X;
    OutY = Y;

    if (!LevelAsset)
    {
        return false;
    }

    switch (Direction)
    {
        case EGridEdge::North:
            OutY = Y + 1;
            break;

        case EGridEdge::East:
            OutX = X + 1;
            break;

        case EGridEdge::South:
            OutY = Y - 1;
            break;

        case EGridEdge::West:
            OutX = X - 1;
            break;

        default:
            return false;
    }

    return LevelAsset->IsValidCoord (OutX, OutY);
}

EGridWallType AGridLevelRuntimeActor::GetWallOnEdge (int32 X, int32 Y, EGridEdge Edge) const
{
    if (!LevelAsset || !LevelAsset->IsValidCoord (X, Y))
    {
        return EGridWallType::Solid;
    }

    const FGridLevelCellData& Cell = LevelAsset->GetCell (X, Y);

    switch (Edge)
    {
        case EGridEdge::North:
            return Cell.NorthWall;

        case EGridEdge::East:
            return Cell.EastWall;

        case EGridEdge::South:
            return Cell.SouthWall;

        case EGridEdge::West:
            return Cell.WestWall;

        default:
            return EGridWallType::Solid;
    }
}

bool AGridLevelRuntimeActor::CanMove (int32 FromX, int32 FromY, EGridEdge Direction) const
{
    if (!IsWalkableCell (FromX, FromY))
    {
        return false;
    }

    int32 ToX = INDEX_NONE;
    int32 ToY = INDEX_NONE;

    if (!TryGetNeighborCell (FromX, FromY, Direction, ToX, ToY))
    {
        return false;
    }

    if (!IsWalkableCell (ToX, ToY))
    {
        return false;
    }

    if (HasDoorOnEdge (FromX, FromY, Direction))
    {
        const FGridObjectEdgeKey Key (FromX, FromY, Direction);
        return !RuntimeBlockedDoorEdges.Contains (Key);
    }

    const EGridWallType Wall = GetWallOnEdge (FromX, FromY, Direction);

    switch (Wall)
    {
        case EGridWallType::None:
        case EGridWallType::DoorOpen:
            return true;

        case EGridWallType::Solid:
        case EGridWallType::Door:
        case EGridWallType::Secret:
        default:
            return false;
    }
}

void AGridLevelRuntimeActor::GetEdgeTransform (
    int32 X,
    int32 Y,
    EGridEdge Edge,
    float CellSize,
    FVector& OutWorldLocation,
    FRotator& OutWorldRotation) const
{
    const FVector Base = GetActorLocation () + CellToWorld (X, Y, 0.f);

    switch (Edge)
    {
        case EGridEdge::North:
            OutWorldLocation = Base + FVector (CellSize * 0.5f, CellSize, 0.f);
            OutWorldRotation = FRotator (0.f, 0.f, 0.f);
            break;

        case EGridEdge::East:
            OutWorldLocation = Base + FVector (CellSize, CellSize * 0.5f, 0.f);
            OutWorldRotation = FRotator (0.f, 90.f, 0.f);
            break;

        case EGridEdge::South:
            OutWorldLocation = Base + FVector (CellSize * 0.5f, 0.f, 0.f);
            OutWorldRotation = FRotator (0.f, 0.f, 0.f);
            break;

        case EGridEdge::West:
            OutWorldLocation = Base + FVector (0.f, CellSize * 0.5f, 0.f);
            OutWorldRotation = FRotator (0.f, 90.f, 0.f);
            break;

        default:
            OutWorldLocation = Base;
            OutWorldRotation = FRotator::ZeroRotator;
            break;
    }
}

void AGridLevelRuntimeActor::AddRuntimeDoorActor (
    const FGridLevelObjectData& DoorObjectData)
{
    UStaticMesh* Mesh = nullptr;
    UMaterialInterface* Material = nullptr;
    FTransform Transform;

    AGridDoorActor* DoorActor =
        SpawnRuntimeObjectActor<AGridDoorActor> (DoorObjectData, Mesh, Material, Transform);

    if (!DoorActor)
    {
        return;
    }
    const FVector Location = Transform.GetLocation ();
    const FRotator Rotation = Transform.GetRotation ().Rotator ();
    const bool bStartOpen = DoorObjectData.bInitiallyActive;

    DoorActor->InitializeDoor (DoorObjectData, Mesh, Material, Location, Rotation, bStartOpen);

    DoorActor->OnDoorAnimationFinished.AddDynamic (this, &AGridLevelRuntimeActor::HandleDoorAnimationFinished);

    SetDoorPassageBlocked (DoorObjectData.CellX, DoorObjectData.CellY, DoorObjectData.Edge, !bStartOpen);
}

AGridDoorActor* AGridLevelRuntimeActor::FindRuntimeDoorActor (int32 X, int32 Y, EGridEdge Edge) const
{
    const FGridLevelObjectData* ObjectData = FindObjectDataAtEdge (EGridLevelObjectType::Door, X, Y, Edge);

    return ObjectData ? FindRuntimeObjectActor<AGridDoorActor> (ObjectData->ObjectId) : nullptr;
}

void AGridLevelRuntimeActor::SetDoorPassageBlocked (int32 X, int32 Y, EGridEdge Edge, bool bBlocked)
{
    const FGridObjectEdgeKey Key (X, Y, Edge);
    if (bBlocked)
    {
        RuntimeBlockedDoorEdges.Add (Key);
    } else
    {
        RuntimeBlockedDoorEdges.Remove (Key);
    }
}

void AGridLevelRuntimeActor::HandleDoorAnimationFinished (int32 X, int32 Y, EGridEdge Edge)
{
    AGridDoorActor* DoorActor = FindRuntimeDoorActor (X, Y, Edge);
    if (!DoorActor)
    {
        return;
    }

    if (DoorActor->IsFullyOpen ())
    {
        SetDoorPassageBlocked (X, Y, Edge, false);
    } else
    {
        SetDoorPassageBlocked (X, Y, Edge, true);
    }
}

bool AGridLevelRuntimeActor::HasDoorOnEdge (int32 X, int32 Y, EGridEdge Edge) const
{
    return FindObjectDataAtEdge (EGridLevelObjectType::Door, X, Y, Edge) != nullptr;
}

bool AGridLevelRuntimeActor::IsDoorOpenOnEdge (int32 X, int32 Y, EGridEdge Edge) const
{
    const AGridDoorActor* DoorActor = FindRuntimeDoorActor (X, Y, Edge);
    return DoorActor && DoorActor->IsFullyOpen ();
}

bool AGridLevelRuntimeActor::OpenDoorOnEdge (int32 X, int32 Y, EGridEdge Edge)
{
    AGridDoorActor* DoorActor = FindRuntimeDoorActor (X, Y, Edge);
    if (!DoorActor)
    {
        return false;
    }

    DoorActor->OpenDoor ();
    SetDoorPassageBlocked (X, Y, Edge, true);
    return true;
}

bool AGridLevelRuntimeActor::CloseDoorOnEdge (int32 X, int32 Y, EGridEdge Edge)
{
    AGridDoorActor* DoorActor = FindRuntimeDoorActor (X, Y, Edge);
    if (!DoorActor)
    {
        return false;
    }

    DoorActor->CloseDoor ();
    SetDoorPassageBlocked (X, Y, Edge, true);
    return true;
}

bool AGridLevelRuntimeActor::ToggleDoorOnEdge (int32 X, int32 Y, EGridEdge Edge)
{
    AGridDoorActor* DoorActor = FindRuntimeDoorActor (X, Y, Edge);
    if (!DoorActor)
    {
        return false;
    }

    if (DoorActor->IsFullyOpen ())
    {
        return CloseDoorOnEdge (X, Y, Edge);
    }

    if (DoorActor->IsFullyClosed ())
    {
        return OpenDoorOnEdge (X, Y, Edge);
    }

    if (DoorActor->IsAnimating ())
    {
        if (DoorActor->bIsOpen)
        {
            return CloseDoorOnEdge (X, Y, Edge);
        }

        return OpenDoorOnEdge (X, Y, Edge);
    }

    return false;
}

const FGridLevelObjectData* AGridLevelRuntimeActor::FindObjectById (FGuid ObjectId) const
{
    if (!LevelAsset || !ObjectId.IsValid ())
    {
        return nullptr;
    }

    for (const FGridLevelObjectData& ObjectData : LevelAsset->Objects)
    {
        if (ObjectData.ObjectId == ObjectId)
        {
            return &ObjectData;
        }
    }

    return nullptr;
}

const FGridLevelObjectData* AGridLevelRuntimeActor::FindInteractableObjectOnEdge (int32 X, int32 Y, EGridEdge Edge) const
{
    if (!LevelAsset)
    {
        return nullptr;
    }

    for (const FGridLevelObjectData& ObjectData : LevelAsset->Objects)
    {
        if (!ObjectData.bInitiallyEnabled)
        {
            continue;
        }

        if (ObjectData.CellX != X || ObjectData.CellY != Y || ObjectData.Edge != Edge)
        {
            continue;
        }

        switch (ObjectData.Type)
        {
            case EGridLevelObjectType::Button:
            case EGridLevelObjectType::Lever:
                return &ObjectData;

            default:
                break;
        }
    }

    return nullptr;
}

EGridLinkAction AGridLevelRuntimeActor::GetResolvedLinkAction (EGridLinkAction Action, bool bInvert) const
{
    if (!bInvert)
    {
        return Action;
    }

    switch (Action)
    {
        case EGridLinkAction::Open:       return EGridLinkAction::Close;
        case EGridLinkAction::Close:      return EGridLinkAction::Open;
        case EGridLinkAction::Activate:   return EGridLinkAction::Deactivate;
        case EGridLinkAction::Deactivate: return EGridLinkAction::Activate;
        case EGridLinkAction::Toggle:
        default:
            return EGridLinkAction::Toggle;
    }
}

bool AGridLevelRuntimeActor::ApplyLinkAction (const FGridLevelLinkData& LinkData)
{
    return ApplyLinkAction (LinkData, false);
}

bool AGridLevelRuntimeActor::ApplyLinkAction (const FGridLevelLinkData& LinkData, bool bInvert)
{
    const FGridLevelObjectData* TargetObject = FindObjectById (LinkData.TargetObjectId);
    if (!TargetObject)
    {
        return false;
    }

    const EGridLinkAction ResolvedAction = GetResolvedLinkAction (LinkData.Action, bInvert);

    switch (TargetObject->Type)
    {
        case EGridLevelObjectType::Door:
            switch (ResolvedAction)
            {
                case EGridLinkAction::Toggle:
                    return ToggleDoorOnEdge (TargetObject->CellX, TargetObject->CellY, TargetObject->Edge);

                case EGridLinkAction::Open:
                case EGridLinkAction::Activate:
                    return OpenDoorOnEdge (TargetObject->CellX, TargetObject->CellY, TargetObject->Edge);

                case EGridLinkAction::Close:
                case EGridLinkAction::Deactivate:
                    return CloseDoorOnEdge (TargetObject->CellX, TargetObject->CellY, TargetObject->Edge);

                default:
                    return false;
            }

        default:
            break;
    }

    return false;
}

bool AGridLevelRuntimeActor::ExecuteLinksFromObject (FGuid SourceObjectId, bool bInvert)
{
    if (!LevelAsset || !SourceObjectId.IsValid ())
    {
        return false;
    }

    bool bAnyApplied = false;

    for (const FGridLevelLinkData& LinkData : LevelAsset->Links)
    {
        if (LinkData.SourceObjectId != SourceObjectId)
        {
            continue;
        }

        const bool bApplied = ApplyLinkAction (LinkData, bInvert);
        bAnyApplied = bAnyApplied || bApplied;
    }

    return bAnyApplied;
}

bool AGridLevelRuntimeActor::ActivateObject (const FGridLevelObjectData& ObjectData)
{
    switch (ObjectData.Type)
    {
        case EGridLevelObjectType::Button:
        {
            if (AGridButtonActor* ButtonActor = FindRuntimeButtonActor (ObjectData.CellX, ObjectData.CellY, ObjectData.Edge))
            {
                ButtonActor->TriggerPress ();
            }

            return ExecuteLinksFromObject (ObjectData.ObjectId, false);
        }

        case EGridLevelObjectType::Lever:
        {
            const bool bWasActive = ActiveObjectIds.Contains (ObjectData.ObjectId);
            const bool bNewActive = !bWasActive;

            if (bNewActive)
            {
                ActiveObjectIds.Add (ObjectData.ObjectId);
            } else
            {
                ActiveObjectIds.Remove (ObjectData.ObjectId);
            }

            if (AGridLeverActor* LeverActor = FindRuntimeLeverActor (ObjectData.CellX, ObjectData.CellY, ObjectData.Edge))
            {
                LeverActor->SetLeverState (bNewActive);
            }

            return ExecuteLinksFromObject (ObjectData.ObjectId, bWasActive);
        }

        default:
            break;
    }

    return false;
}

bool AGridLevelRuntimeActor::TryInteractAtEdge (int32 FromCellX, int32 FromCellY, EGridEdge Edge)
{
    const FGridLevelObjectData* ObjectData = FindInteractableObjectOnEdge (FromCellX, FromCellY, Edge);
    if (!ObjectData)
    {
        return false;
    }

    return ActivateObject (*ObjectData);
}

void AGridLevelRuntimeActor::AddRuntimeButtonActor (
    const FGridLevelObjectData& ButtonObjectData)
{
    UStaticMesh* Mesh = nullptr;
    UMaterialInterface* Material = nullptr;
    FTransform Transform;

    AGridButtonActor* ButtonActor =
        SpawnRuntimeObjectActor<AGridButtonActor> (ButtonObjectData, Mesh, Material, Transform);
    if (!ButtonActor)
    {
        return;
    }
    ButtonActor->InitializeButton (ButtonObjectData, Mesh, Material, Transform.GetLocation (), Transform.GetRotation ().Rotator ());
}

AGridButtonActor* AGridLevelRuntimeActor::FindRuntimeButtonActor (int32 X, int32 Y, EGridEdge Edge) const
{
    const FGridLevelObjectData* ObjectData = FindObjectDataAtEdge (EGridLevelObjectType::Button, X, Y, Edge);
    return ObjectData ? FindRuntimeObjectActor<AGridButtonActor> (ObjectData->ObjectId) : nullptr;
}

const FGridLevelObjectData* AGridLevelRuntimeActor::FindPressurePlateObjectAtCell (int32 X, int32 Y) const
{
    if (!LevelAsset)
    {
        return nullptr;
    }

    for (const FGridLevelObjectData& ObjectData : LevelAsset->Objects)
    {
        if (!ObjectData.bInitiallyEnabled)
        {
            continue;
        }

        if (ObjectData.Type != EGridLevelObjectType::PressurePlate)
        {
            continue;
        }

        if (ObjectData.CellX == X && ObjectData.CellY == Y)
        {
            return &ObjectData;
        }
    }

    return nullptr;
}

bool AGridLevelRuntimeActor::ActivatePressurePlateAtCell (int32 X, int32 Y)
{
    const FGridLevelObjectData* PlateData = FindPressurePlateObjectAtCell (X, Y);
    if (!PlateData)
    {
        return false;
    }

    if (ActiveObjectIds.Contains (PlateData->ObjectId))
    {
        return false;
    }

    ActiveObjectIds.Add (PlateData->ObjectId);

    if (AGridPressurePlateActor* PlateActor = FindRuntimePressurePlateActor (X, Y))
    {
        PlateActor->SetPressed (true);
    }

    return ExecuteLinksFromObject (PlateData->ObjectId, false);
}

bool AGridLevelRuntimeActor::DeactivatePressurePlateAtCell (int32 X, int32 Y)
{
    const FGridLevelObjectData* PlateData = FindPressurePlateObjectAtCell (X, Y);
    if (!PlateData)
    {
        return false;
    }

    if (!ActiveObjectIds.Contains (PlateData->ObjectId))
    {
        return false;
    }

    ActiveObjectIds.Remove (PlateData->ObjectId);

    if (AGridPressurePlateActor* PlateActor = FindRuntimePressurePlateActor (X, Y))
    {
        PlateActor->SetPressed (false);
    }

    return ExecuteLinksFromObject (PlateData->ObjectId, true);
}

void AGridLevelRuntimeActor::HandlePartyCellChanged (int32 OldCellX, int32 OldCellY, int32 NewCellX, int32 NewCellY)
{
    if (OldCellX == NewCellX && OldCellY == NewCellY)
    {
        ActivatePressurePlateAtCell (NewCellX, NewCellY);
        return;
    }

    DeactivatePressurePlateAtCell (OldCellX, OldCellY);
    ActivatePressurePlateAtCell (NewCellX, NewCellY);
}

void AGridLevelRuntimeActor::AddRuntimeLeverActor (const FGridLevelObjectData& LeverObjectData)
{
    UStaticMesh* Mesh = nullptr;
    UMaterialInterface* Material = nullptr;
    FTransform Transform;

    AGridLeverActor* LeverActor =
        SpawnRuntimeObjectActor<AGridLeverActor> (LeverObjectData, Mesh, Material, Transform);

    if (!LeverActor)
    {
        return;
    }
    LeverActor->InitializeLever (LeverObjectData, Mesh, Material, Transform.GetLocation (), Transform.GetRotation ().Rotator (), LeverObjectData.bInitiallyActive);
    if (LeverObjectData.bInitiallyActive)
    {
        ActiveObjectIds.Add (LeverObjectData.ObjectId);
    }
}

AGridLeverActor* AGridLevelRuntimeActor::FindRuntimeLeverActor (int32 X, int32 Y, EGridEdge Edge) const
{
    const FGridLevelObjectData* ObjectData = FindObjectDataAtEdge (EGridLevelObjectType::Lever, X, Y, Edge);
    return ObjectData ? FindRuntimeObjectActor<AGridLeverActor> (ObjectData->ObjectId) : nullptr;
}

void AGridLevelRuntimeActor::AddRuntimePressurePlateActor (const FGridLevelObjectData& PlateObjectData)
{
    UStaticMesh* Mesh = nullptr;
    UMaterialInterface* Material = nullptr;
    FTransform Transform;

    AGridPressurePlateActor* PlateActor =
        SpawnRuntimeObjectActor<AGridPressurePlateActor> (PlateObjectData, Mesh, Material, Transform);
    if (!PlateActor)
    {
        return;
    }
    PlateActor->InitializePlate (PlateObjectData, Mesh, Material, Transform.GetLocation (), PlateObjectData.bInitiallyActive);
    if (PlateObjectData.bInitiallyActive)
    {
        ActiveObjectIds.Add (PlateObjectData.ObjectId);
    }
}

AGridPressurePlateActor* AGridLevelRuntimeActor::FindRuntimePressurePlateActor (int32 X, int32 Y) const
{
    const FGridLevelObjectData* ObjectData = FindObjectDataAtCell (EGridLevelObjectType::PressurePlate, X, Y);
    return ObjectData ? FindRuntimeObjectActor<AGridPressurePlateActor> (ObjectData->ObjectId) : nullptr;
}

void AGridLevelRuntimeActor::ClearEditorPreviewObjects ()
{
    UWorld* World = GetWorld ();

    if (World)
    {
        for (TActorIterator<AGridEditorPreviewObjectActor> It (World); It; ++It)
        {
            AGridEditorPreviewObjectActor* PreviewActor = *It;

            if (!IsValid (PreviewActor))
            {
                continue;
            }

            if (PreviewActor->GetOwner () == this ||
                SpawnedEditorPreviewObjects.Contains (PreviewActor))
            {
                PreviewActor->Destroy ();
            }
        }
    } else
    {
        for (AGridEditorPreviewObjectActor* Actor : SpawnedEditorPreviewObjects)
        {
            if (IsValid (Actor))
            {
                Actor->Destroy ();
            }
        }
    }

    SpawnedEditorPreviewObjects.Empty ();
    CurrentSelectedEditorObjectId.Invalidate ();
    CurrentHoveredEditorObjectId.Invalidate ();
}

void AGridLevelRuntimeActor::AddEditorPreviewObject (const FGridLevelObjectData& ObjectData)
{
    if (!EditorPreviewObjectActorClass || !LevelAsset)
    {
        return;
    }
    UStaticMesh* Mesh = GetObjectMesh (ObjectData);
    UMaterialInterface* Material = GetObjectMaterial (ObjectData);
    if (!Mesh)
    {
        return;
    }
    UWorld* World = GetWorld ();
    if (!World)
    {
        return;
    }
    FTransform PlacementTransform;
    if (!GetObjectPlacementTransform (ObjectData, PlacementTransform))
    {
        return;
    }
    const FVector Location = PlacementTransform.GetLocation ();
    const FRotator Rotation = PlacementTransform.GetRotation ().Rotator ();

    FActorSpawnParameters Params;
    Params.Owner = this;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AGridEditorPreviewObjectActor* PreviewActor =
        World->SpawnActor<AGridEditorPreviewObjectActor> (EditorPreviewObjectActorClass, Location, Rotation, Params);

    if (!PreviewActor)
    {
        return;
    }
    PreviewActor->InitializePreviewObject (ObjectData, Mesh, Material);
    SpawnedEditorPreviewObjects.Add (PreviewActor);
}

void AGridLevelRuntimeActor::RebuildEditorPreviewObjects ()
{
    ClearEditorPreviewObjects ();

    if (!LevelAsset)
    {
        return;
    }

    for (const FGridLevelObjectData& ObjectData : LevelAsset->Objects)
    {
        if (!IsEditorPreviewableObject (ObjectData))
        {
            continue;
        }

        AddEditorPreviewObject (ObjectData);
    }
}

void AGridLevelRuntimeActor::SetEditorHoveredObject (FGuid ObjectId)
{
    CurrentHoveredEditorObjectId = ObjectId;

    for (AGridEditorPreviewObjectActor* Actor : SpawnedEditorPreviewObjects)
    {
        if (!IsValid (Actor))
        {
            continue;
        }
        Actor->SetHovered (ObjectId.IsValid () && Actor->ObjectId == ObjectId);
    }
}

void AGridLevelRuntimeActor::SetEditorSelectedObject (FGuid ObjectId)
{
    CurrentSelectedEditorObjectId = ObjectId;

    for (AGridEditorPreviewObjectActor* Actor : SpawnedEditorPreviewObjects)
    {
        if (!IsValid (Actor))
        {
            continue;
        }
        Actor->SetSelected (ObjectId.IsValid () && Actor->ObjectId == ObjectId);
    }
}

const UGridObjectArchetypeAsset* AGridLevelRuntimeActor::FindObjectArchetype (FName ArchetypeId) const
{
    if (ArchetypeId.IsNone ())
    {
        return nullptr;
    }

    for (const UGridObjectArchetypeAsset* Archetype : ObjectArchetypes)
    {
        if (!Archetype)
        {
            continue;
        }

        if (Archetype->ArchetypeId == ArchetypeId)
        {
            return Archetype;
        }
    }

    return nullptr;
}

UStaticMesh* AGridLevelRuntimeActor::GetObjectMesh (const FGridLevelObjectData& ObjectData) const
{
    const UGridObjectArchetypeAsset* Archetype = FindObjectArchetype (ObjectData.ArchetypeId);
    return Archetype ? Archetype->PreviewMesh.Get () : nullptr;
}

UMaterialInterface* AGridLevelRuntimeActor::GetObjectMaterial (const FGridLevelObjectData& ObjectData) const
{
    const UGridObjectArchetypeAsset* Archetype = FindObjectArchetype (ObjectData.ArchetypeId);
    return Archetype ? Archetype->PreviewMaterial.Get () : nullptr;
}

bool AGridLevelRuntimeActor::GetWallMountedObjectTransform (
    const FGridLevelObjectData& ObjectData,
    float ZOffset,
    float WallInset,
    FTransform& OutTransform) const
{
    if (!LevelAsset || ObjectData.Edge == EGridEdge::None)
    {
        return false;
    }

    const float CellSize = LevelAsset->CellSize;
    const FVector Base =
        GetActorLocation () +
        CellToWorld (ObjectData.CellX, ObjectData.CellY, ZOffset);

    FVector Pos = Base;
    FRotator Rot = FRotator::ZeroRotator;

    switch (ObjectData.Edge)
    {
        case EGridEdge::North:
        Pos = Base + FVector (CellSize * 0.5f, CellSize - WallInset, 0.f);
        Rot = FRotator (0.f, 90.f, 0.f);
        break;

        case EGridEdge::East:
        Pos = Base + FVector (CellSize - WallInset, CellSize * 0.5f, 0.f);
        Rot = FRotator (0.f, 0.f, 0.f);
        break;

        case EGridEdge::South:
        Pos = Base + FVector (CellSize * 0.5f, WallInset, 0.f);
        Rot = FRotator (0.f, -90.f, 0.f);
        break;

        case EGridEdge::West:
        Pos = Base + FVector (WallInset, CellSize * 0.5f, 0.f);
        Rot = FRotator (0.f, 180.f, 0.f);
        break;

        default:
        return false;
    }

    OutTransform = FTransform (Rot, Pos, FVector::OneVector);
    return true;
}

bool AGridLevelRuntimeActor::GetCenteredObjectTransform (
    const FGridLevelObjectData& ObjectData,
    float ZOffset,
    FTransform& OutTransform) const
{
    if (!LevelAsset)
    {
        return false;
    }
    const FVector Pos = GetActorLocation () +
        CellToWorld (ObjectData.CellX, ObjectData.CellY, ZOffset) +
        FVector (LevelAsset->CellSize * 0.5f, LevelAsset->CellSize * 0.5f, 0.f);

    OutTransform = FTransform (FRotator::ZeroRotator, Pos, FVector::OneVector);
    return true;
}

bool AGridLevelRuntimeActor::GetObjectPlacementTransform (
    const FGridLevelObjectData& ObjectData,
    FTransform& OutTransform) const
{
    if (!LevelAsset)
    {
        return false;
    }

    switch (ObjectData.Type)
    {
        case EGridLevelObjectType::Door:
        {
            FVector Pos = FVector::ZeroVector;
            FRotator Rot = FRotator::ZeroRotator;

            GetEdgeTransform (ObjectData.CellX, ObjectData.CellY, ObjectData.Edge, LevelAsset->CellSize, Pos, Rot);

            OutTransform = FTransform (Rot, Pos, FVector::OneVector);
            return true;
        }

        case EGridLevelObjectType::Button:
        return GetWallMountedObjectTransform (ObjectData, 80.f, 6.f, OutTransform);

        case EGridLevelObjectType::Lever:
        return GetWallMountedObjectTransform (ObjectData, 95.f, 8.f, OutTransform);

        case EGridLevelObjectType::PressurePlate:
        return GetCenteredObjectTransform (ObjectData, 0.f, OutTransform);

        case EGridLevelObjectType::Decoration:
        case EGridLevelObjectType::MonsterSpawn:
        case EGridLevelObjectType::ItemSpawn:
        case EGridLevelObjectType::Light:
        case EGridLevelObjectType::Teleporter:
        case EGridLevelObjectType::Trigger:
        return GetCenteredObjectTransform (ObjectData, 12.f, OutTransform);

        default:
        return false;
    }
}

void AGridLevelRuntimeActor::RegisterRuntimeObjectActor (const FGuid& ObjectId, AActor* Actor)
{
    if (!ObjectId.IsValid () || !IsValid (Actor))
    {
        return;
    }

    SpawnedRuntimeObjectActors.Add (ObjectId, Actor);
}

void AGridLevelRuntimeActor::ClearRuntimeObjectActors ()
{
    for (TPair<FGuid, TObjectPtr<AActor>>& Pair : SpawnedRuntimeObjectActors)
    {
        if (IsValid (Pair.Value))
        {
            Pair.Value->Destroy ();
        }
    }

    SpawnedRuntimeObjectActors.Empty ();
}

const FGridLevelObjectData* AGridLevelRuntimeActor::FindObjectDataAtEdge (EGridLevelObjectType Type, int32 X, int32 Y, EGridEdge Edge) const
{
    if (!LevelAsset)
    {
        return nullptr;
    }

    for (const FGridLevelObjectData& ObjectData : LevelAsset->Objects)
    {
        if (ObjectData.Type == Type && ObjectData.CellX == X && ObjectData.CellY == Y && ObjectData.Edge == Edge)
        {
            return &ObjectData;
        }
    }

    return nullptr;
}

const FGridLevelObjectData* AGridLevelRuntimeActor::FindObjectDataAtCell (EGridLevelObjectType Type, int32 X, int32 Y) const
{
    if (!LevelAsset)
    {
        return nullptr;
    }

    for (const FGridLevelObjectData& ObjectData : LevelAsset->Objects)
    {
        if (ObjectData.Type == Type && ObjectData.CellX == X && ObjectData.CellY == Y)
        {
            return &ObjectData;
        }
    }

    return nullptr;
}

TSubclassOf<AActor> AGridLevelRuntimeActor::GetObjectRuntimeActorClass (const FGridLevelObjectData& ObjectData) const
{
    const UGridObjectArchetypeAsset* Archetype = FindObjectArchetype (ObjectData.ArchetypeId);
    return Archetype ? Archetype->RuntimeActorClass : nullptr;
}

bool AGridLevelRuntimeActor::IsRuntimeSpawnableObject (
    const FGridLevelObjectData& ObjectData) const
{
    if (!LevelAsset)
    {
        return false;
    }

    if (!ObjectData.bInitiallyEnabled)
    {
        return false;
    }

    if (!LevelAsset->IsValidCoord (ObjectData.CellX, ObjectData.CellY))
    {
        return false;
    }

    switch (ObjectData.Type)
    {
        case EGridLevelObjectType::Door:
        case EGridLevelObjectType::Button:
        case EGridLevelObjectType::Lever:
        return ObjectData.Edge != EGridEdge::None;

        case EGridLevelObjectType::PressurePlate:
        return true;

        default:
        return false;
    }
}

void AGridLevelRuntimeActor::AddRuntimeObjectActor (
    const FGridLevelObjectData& ObjectData)
{
    switch (ObjectData.Type)
    {
        case EGridLevelObjectType::Door:
        AddRuntimeDoorActor (ObjectData);
        break;

        case EGridLevelObjectType::Button:
        AddRuntimeButtonActor (ObjectData);
        break;

        case EGridLevelObjectType::Lever:
        AddRuntimeLeverActor (ObjectData);
        break;

        case EGridLevelObjectType::PressurePlate:
        AddRuntimePressurePlateActor (ObjectData);
        break;

        default:
        break;
    }
}

void AGridLevelRuntimeActor::RebuildRuntimeObjects ()
{
    if (!LevelAsset)
    {
        return;
    }

    LevelAsset->EnsureCellCount ();

    for (const FGridLevelObjectData& ObjectData : LevelAsset->Objects)
    {
        if (!IsRuntimeSpawnableObject (ObjectData))
        {
            continue;
        }

        AddRuntimeObjectActor (ObjectData);
    }
}

bool AGridLevelRuntimeActor::IsEditorPreviewableObject (
    const FGridLevelObjectData& ObjectData) const
{
    if (!LevelAsset)
    {
        return false;
    }

    if (!ObjectData.bInitiallyEnabled)
    {
        return false;
    }

    if (!LevelAsset->IsValidCoord (ObjectData.CellX, ObjectData.CellY))
    {
        return false;
    }

    if (!GetObjectMesh (ObjectData))
    {
        return false;
    }

    return true;
}