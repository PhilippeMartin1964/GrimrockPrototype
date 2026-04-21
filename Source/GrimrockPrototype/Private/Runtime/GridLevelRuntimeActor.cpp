#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridDoorActor.h"

AGridLevelRuntimeActor::AGridLevelRuntimeActor ()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent> (TEXT ("Root"));
    SetRootComponent (SceneRoot);

    FloorISM = CreateDefaultSubobject<UInstancedStaticMeshComponent> (TEXT ("FloorISM"));
    FloorISM->SetupAttachment (SceneRoot);

    WallISM = CreateDefaultSubobject<UInstancedStaticMeshComponent> (TEXT ("WallISM"));
    WallISM->SetupAttachment (SceneRoot);

    DoorISM = CreateDefaultSubobject<UInstancedStaticMeshComponent> (TEXT ("DoorISM"));
    DoorISM->SetupAttachment (SceneRoot);

    SecretWallISM = CreateDefaultSubobject<UInstancedStaticMeshComponent> (TEXT ("SecretWallISM"));
    SecretWallISM->SetupAttachment (SceneRoot);

    CeilingISM = CreateDefaultSubobject<UInstancedStaticMeshComponent> (TEXT ("CeilingISM"));
    CeilingISM->SetupAttachment (SceneRoot);
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
    if (DoorISM) DoorISM->ClearInstances ();
    if (SecretWallISM) SecretWallISM->ClearInstances ();
    if (CeilingISM) CeilingISM->ClearInstances ();

    ClearRuntimeDoors ();
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

    if (!LevelAsset || !FloorISM || !WallISM || !DoorISM || !SecretWallISM || !CeilingISM)
    {
        return;
    }

    LevelAsset->EnsureCellCount ();

    FloorISM->SetStaticMesh (FloorMesh);
    WallISM->SetStaticMesh (WallMesh);
    DoorISM->SetStaticMesh (DoorMesh);
    SecretWallISM->SetStaticMesh (SecretWallMesh);
    CeilingISM->SetStaticMesh (CeilingMesh);

    const float CellSize = LevelAsset->CellSize;

    for (int32 Y = 0; Y < LevelAsset->Height; ++Y)
    {
        for (int32 X = 0; X < LevelAsset->Width; ++X)
        {
            const FGridLevelCellData& Cell = LevelAsset->GetCell (X, Y);

            if (Cell.CellType == EGridCellType::Empty)
            {
                continue;
            }

            AddFloor (X, Y, CellSize);

            if (Cell.bHasCeiling)
            {
                AddCeiling (X, Y, CellSize);
            }

            auto DrawEdgeIfNeeded = [&] (EGridEdge Edge, EGridWallType WallType, bool bShouldDraw)
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
                        if (!GetWorld () || !GetWorld ()->IsGameWorld ())
                        {
                            AddEdgeInstance (DoorISM, X, Y, Edge, CellSize);
                        }
                        break;

                    case EGridWallType::DoorOpen:
                        break;

                    case EGridWallType::Secret:
                        AddEdgeInstance (SecretWallISM, X, Y, Edge, CellSize);
                        break;

                    default:
                        break;
                }
            };

            DrawEdgeIfNeeded (EGridEdge::North, Cell.NorthWall, Cell.NorthWall != EGridWallType::None);
            DrawEdgeIfNeeded (EGridEdge::East, Cell.EastWall, Cell.EastWall != EGridWallType::None);

            const bool bDrawSouth =
                Cell.SouthWall != EGridWallType::None &&
                (!LevelAsset->IsValidCoord (X, Y - 1) ||
                 LevelAsset->GetCell (X, Y - 1).CellType == EGridCellType::Empty);

            DrawEdgeIfNeeded (EGridEdge::South, Cell.SouthWall, bDrawSouth);

            const bool bDrawWest =
                Cell.WestWall != EGridWallType::None &&
                (!LevelAsset->IsValidCoord (X - 1, Y) ||
                 LevelAsset->GetCell (X - 1, Y).CellType == EGridCellType::Empty);

            DrawEdgeIfNeeded (EGridEdge::West, Cell.WestWall, bDrawWest);
        }
    }

    if (GetWorld () && GetWorld ()->IsGameWorld ())
    {
        RebuildRuntimeDoors ();
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
        const FString Key = MakeDoorEdgeKey (FromX, FromY, Direction);
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

void AGridLevelRuntimeActor::ClearRuntimeDoors ()
{
    UWorld* World = GetWorld ();
    if (!World)
    {
        SpawnedDoorActors.Empty ();
        RuntimeBlockedDoorEdges.Empty ();
        return;
    }

    for (AGridDoorActor* DoorActor : SpawnedDoorActors)
    {
        if (IsValid (DoorActor))
        {
            DoorActor->Destroy ();
        }
    }

    SpawnedDoorActors.Empty ();
    RuntimeBlockedDoorEdges.Empty ();
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

void AGridLevelRuntimeActor::AddRuntimeDoorActor (const FGridLevelObjectData& DoorObjectData)
{
    if (!DoorActorClass || !LevelAsset || !DoorMesh)
    {
        return;
    }

    UWorld* World = GetWorld ();
    if (!World)
    {
        return;
    }

    FVector DoorWorldLocation = FVector::ZeroVector;
    FRotator DoorWorldRotation = FRotator::ZeroRotator;
    GetEdgeTransform (
        DoorObjectData.CellX,
        DoorObjectData.CellY,
        DoorObjectData.Edge,
        LevelAsset->CellSize,
        DoorWorldLocation,
        DoorWorldRotation
    );

    FActorSpawnParameters Params;
    Params.Owner = this;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AGridDoorActor* DoorActor = World->SpawnActor<AGridDoorActor> (
        DoorActorClass,
        DoorWorldLocation,
        DoorWorldRotation,
        Params
    );

    if (!DoorActor)
    {
        return;
    }

    const bool bStartOpen = DoorObjectData.bInitiallyActive;

    DoorActor->InitializeDoor (
        DoorMesh,
        nullptr,
        DoorWorldLocation,
        DoorWorldRotation,
        DoorObjectData.CellX,
        DoorObjectData.CellY,
        DoorObjectData.Edge,
        bStartOpen
    );

    DoorActor->OnDoorAnimationFinished.AddDynamic (
        this,
        &AGridLevelRuntimeActor::HandleDoorAnimationFinished
    );

    SetDoorPassageBlocked (
        DoorObjectData.CellX,
        DoorObjectData.CellY,
        DoorObjectData.Edge,
        !bStartOpen
    );

    SpawnedDoorActors.Add (DoorActor);
}

void AGridLevelRuntimeActor::RebuildRuntimeDoors ()
{
    ClearRuntimeDoors ();

    if (!LevelAsset || !DoorActorClass)
    {
        return;
    }

    LevelAsset->EnsureCellCount ();

    for (const FGridLevelObjectData& ObjectData : LevelAsset->Objects)
    {
        if (ObjectData.Type != EGridLevelObjectType::Door)
        {
            continue;
        }

        if (!ObjectData.bInitiallyEnabled)
        {
            continue;
        }

        if (!LevelAsset->IsValidCoord (ObjectData.CellX, ObjectData.CellY))
        {
            continue;
        }

        if (ObjectData.Edge == EGridEdge::None)
        {
            continue;
        }

        AddRuntimeDoorActor (ObjectData);
    }
}

AGridDoorActor* AGridLevelRuntimeActor::FindRuntimeDoorActor (int32 X, int32 Y, EGridEdge Edge) const
{
    for (AGridDoorActor* DoorActor : SpawnedDoorActors)
    {
        if (IsValid (DoorActor) && DoorActor->MatchesEdge (X, Y, Edge))
        {
            return DoorActor;
        }
    }

    return nullptr;
}

const FGridLevelObjectData* AGridLevelRuntimeActor::FindDoorObjectData (int32 X, int32 Y, EGridEdge Edge) const
{
    if (!LevelAsset)
    {
        return nullptr;
    }

    for (const FGridLevelObjectData& ObjectData : LevelAsset->Objects)
    {
        if (ObjectData.Type == EGridLevelObjectType::Door &&
            ObjectData.CellX == X &&
            ObjectData.CellY == Y &&
            ObjectData.Edge == Edge)
        {
            return &ObjectData;
        }
    }

    return nullptr;
}

FString AGridLevelRuntimeActor::MakeDoorEdgeKey (int32 X, int32 Y, EGridEdge Edge) const
{
    return FString::Printf (TEXT ("%d_%d_%d"), X, Y, static_cast<int32>(Edge));
}

void AGridLevelRuntimeActor::SetDoorPassageBlocked (int32 X, int32 Y, EGridEdge Edge, bool bBlocked)
{
    const FString Key = MakeDoorEdgeKey (X, Y, Edge);

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
    return FindDoorObjectData (X, Y, Edge) != nullptr;
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