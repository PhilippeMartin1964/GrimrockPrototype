#include "Runtime/GridLevelRuntimeActor.h"

AGridLevelRuntimeActor::AGridLevelRuntimeActor ()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent> (TEXT ("Root"));
    SetRootComponent (SceneRoot);

    FloorISM = CreateDefaultSubobject<UInstancedStaticMeshComponent> (TEXT ("FloorISM"));
    FloorISM->SetupAttachment (SceneRoot);

    WallISM = CreateDefaultSubobject<UInstancedStaticMeshComponent> (TEXT ("WallISM"));
    WallISM->SetupAttachment (SceneRoot);

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
    if (FloorISM)
    {
        FloorISM->ClearInstances ();
    }

    if (WallISM)
    {
        WallISM->ClearInstances ();
    }

    if (CeilingISM)
    {
        CeilingISM->ClearInstances ();
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
    if (!FloorISM)
    {
        return;
    }

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
    if (!CeilingISM)
    {
        return;
    }

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

    if (!LevelAsset || !FloorISM || !WallISM || !CeilingISM)
    {
        return;
    }

    LevelAsset->EnsureCellCount ();

    FloorISM->SetStaticMesh (FloorMesh);
    WallISM->SetStaticMesh (WallMesh);
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

            if (Cell.NorthWall == EGridWallType::Solid)
            {
                AddEdgeInstance (WallISM, X, Y, EGridEdge::North, CellSize);
            }

            if (Cell.EastWall == EGridWallType::Solid)
            {
                AddEdgeInstance (WallISM, X, Y, EGridEdge::East, CellSize);
            }

            const bool bDrawSouth =
                Cell.SouthWall == EGridWallType::Solid &&
                (!LevelAsset->IsValidCoord (X, Y - 1) ||
                 LevelAsset->GetCell (X, Y - 1).CellType == EGridCellType::Empty);

            if (bDrawSouth)
            {
                AddEdgeInstance (WallISM, X, Y, EGridEdge::South, CellSize);
            }

            const bool bDrawWest =
                Cell.WestWall == EGridWallType::Solid &&
                (!LevelAsset->IsValidCoord (X - 1, Y) ||
                 LevelAsset->GetCell (X - 1, Y).CellType == EGridCellType::Empty);

            if (bDrawWest)
            {
                AddEdgeInstance (WallISM, X, Y, EGridEdge::West, CellSize);
            }
        }
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

    const EGridWallType Wall = GetWallOnEdge (FromX, FromY, Direction);

    switch (Wall)
    {
        case EGridWallType::None:
            return true;

        case EGridWallType::DoorOpen:
            return true;

        case EGridWallType::Solid:
            return false;

        case EGridWallType::Door:
            return false;

        case EGridWallType::Secret:
            return false;

        default:
            return false;
    }
}