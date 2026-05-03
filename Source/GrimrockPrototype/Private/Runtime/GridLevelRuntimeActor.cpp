#include "Runtime/GridLevelRuntimeActor.h"
#include "Core/GridTypes.h"
#include "Core/GridObjectArchetypeAsset.h"
#include "Runtime/GridRuntimeObjectActor.h"
#include "Runtime/GridActivationComponent.h"
#include "Runtime/GridDoorSystemComponent.h"
#include "Runtime/GridEditorPreviewComponent.h"

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

    ActivationComponent = CreateDefaultSubobject<UGridActivationComponent> (TEXT ("ActivationComponent"));

    DoorSystemComponent = CreateDefaultSubobject<UGridDoorSystemComponent> (TEXT ("DoorSystemComponent"));

    EditorPreviewComponent = CreateDefaultSubobject<UGridEditorPreviewComponent> (TEXT ("EditorPreviewComponent"));
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
    if (ActivationComponent)
    {
        ActivationComponent->Initialize (this);
        ActivationComponent->ResetRuntimeState ();
    }
    if (DoorSystemComponent)
    {
        DoorSystemComponent->Initialize (this);
        DoorSystemComponent->ResetRuntimeState ();
    }
    if (EditorPreviewComponent)
    {
        EditorPreviewComponent->Initialize (this);
    }
    RebuildLevel ();
}

void AGridLevelRuntimeActor::ClearVisuals (EGridRuntimeRebuildMode RebuildMode)
{
    if (FloorISM) FloorISM->ClearInstances ();
    if (WallISM) WallISM->ClearInstances ();
    if (SecretWallISM) SecretWallISM->ClearInstances ();
    if (CeilingISM)
    {
        CeilingISM->ClearInstances ();
        CeilingISM->EmptyOverrideMaterials ();
    }
    if (RebuildMode != EGridRuntimeRebuildMode::GeometryOnly)
    {
        ClearRuntimeObjectActors ();
        if (EditorPreviewComponent)
        {
            EditorPreviewComponent->ClearPreviewObjects ();
        }
    }
    if (RebuildMode != EGridRuntimeRebuildMode::GeometryOnly)
    {
        if (DoorSystemComponent) DoorSystemComponent->ResetRuntimeState ();
        if (ActivationComponent) ActivationComponent->ResetRuntimeState ();
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
    return GetActorLocation () + GridOrigin + FVector ((X * CellSize) + (CellSize * 0.5f), (Y * CellSize) + (CellSize * 0.5f), ZOffset);
}

void AGridLevelRuntimeActor::AddFloor (int32 X, int32 Y, float CellSize)
{
    const FVector Base = CellToWorld (X, Y, 0.f);
    const FVector CenterOffset (CellSize * 0.5f, CellSize * 0.5f, 0.f);
    const FTransform T (FRotator::ZeroRotator, Base + CenterOffset, FVector::OneVector);
    FloorISM->AddInstance (T);
}

void AGridLevelRuntimeActor::AddCeiling (int32 X, int32 Y, float CellSize)
{
    const FVector Base = CellToWorld (X, Y, 200.f);
    const FVector CenterOffset (CellSize * 0.5f, CellSize * 0.5f, 0.f);
    const FTransform T (FRotator::ZeroRotator, Base + CenterOffset, FVector (1.f, 1.f, 1.f));
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
    switch (Edge)
    {
        case EGridEdge::North:
        {
            Pos = Base + FVector (CellSize * 0.5f, CellSize, 0.f);
            Rot = FRotator (0.f, 0.f, 0.f);
            break;
        }
        case EGridEdge::South:
        {
            Pos = Base + FVector (CellSize * 0.5f, 0.f, 0.f);
            Rot = FRotator (0.f, 180.f, 0.f);
            break;
        }
        case EGridEdge::East:
        {
            Pos = Base + FVector (CellSize, CellSize * 0.5f, 0.f);
            Rot = FRotator (0.f, -90.f, 0.f);
            break;
        }
        case EGridEdge::West:
        {
            Pos = Base + FVector (0.f, CellSize * 0.5f, 0.f);
            Rot = FRotator (0.f, 90.f, 0.f);
            break;
        }
        default:
        {
            return;
        }
    }
    const FTransform T (Rot, Pos, FVector::OneVector); 
    TargetISM->AddInstance (T);
}

void AGridLevelRuntimeActor::RebuildLevel (EGridRuntimeRebuildMode RebuildMode)
{
    ClearVisuals (RebuildMode);

    if (!LevelAsset || !FloorISM || !WallISM || !SecretWallISM || !CeilingISM)
    {
        return;
    }

    LevelAsset->EnsureCellCount ();
    if (ActivationComponent)
    {
        ActivationComponent->Initialize (this);
        ActivationComponent->RebuildIndexes ();
    }

    if (DoorSystemComponent)
    {
        DoorSystemComponent->Initialize (this);
        DoorSystemComponent->RebuildIndexes ();
    }
    if (EditorPreviewComponent)
    {
        EditorPreviewComponent->Initialize (this);
    }
    FloorISM->SetStaticMesh (FloorMesh);
    WallISM->SetStaticMesh (WallMesh);
    SecretWallISM->SetStaticMesh (SecretWallMesh);
    CeilingISM->SetStaticMesh (CeilingMesh);

    const bool bIsGameWorld = GetWorld () && GetWorld ()->IsGameWorld ();

    if (!bIsGameWorld && CeilingEditorMaterial && CeilingMesh)
    {
        const int32 MaterialCount = CeilingMesh->GetStaticMaterials ().Num ();

        for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
        {
            CeilingISM->SetMaterial (MaterialIndex, CeilingEditorMaterial);
        }
    }
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

                    case EGridWallType::Secret:
                    AddEdgeInstance (SecretWallISM, X, Y, Edge, CellSize);
                    break;

                    default:
                    break;
                }
            };
            DrawEdgeIfNeeded (EGridEdge::North, Cell.NorthWall, Cell.NorthWall != EGridWallType::None);
            DrawEdgeIfNeeded (EGridEdge::East, Cell.EastWall, Cell.EastWall != EGridWallType::None);
            DrawEdgeIfNeeded (EGridEdge::South, Cell.SouthWall, Cell.SouthWall != EGridWallType::None);
            DrawEdgeIfNeeded (EGridEdge::West, Cell.WestWall, Cell.WestWall != EGridWallType::None);
        }
    }
    if (RebuildMode != EGridRuntimeRebuildMode::GeometryOnly)
    {
        if (!GetWorld () || !GetWorld ()->IsGameWorld ())
        {
            if (EditorPreviewComponent)
            {
                EditorPreviewComponent->RebuildPreviewObjects ();
            }
        }
    }    
    if (RebuildMode != EGridRuntimeRebuildMode::GeometryOnly)
    {
        if (bIsGameWorld)
        {
            RebuildRuntimeObjects ();
        }
    }
    if (bEnableRuntimeDebugLog)
    {
        LogRuntimeDebugSummary ();
    }

    if (bEnableRuntimeDebugScreen)
    {
        ShowRuntimeDebugSummary ();
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
    if (DoorSystemComponent && DoorSystemComponent->IsDoorPassageBlocked (FromX, FromY, Direction))
    {
        return false;
    }
    const EGridWallType Wall = GetWallOnEdge (FromX, FromY, Direction);

    switch (Wall)
    {
        case EGridWallType::None:
            return true;

        case EGridWallType::Solid:
        case EGridWallType::Secret:
        default:
            return false;
    }
}

void AGridLevelRuntimeActor::GetEdgeTransform (int32 X, int32 Y, EGridEdge Edge, float CellSize, FVector& OutWorldLocation, FRotator& OutWorldRotation) const
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
        OutWorldRotation = FRotator (0.f, -90.f, 0.f);
        break;

        case EGridEdge::South:
        OutWorldLocation = Base + FVector (CellSize * 0.5f, 0.f, 0.f);
        OutWorldRotation = FRotator (0.f, 180.f, 0.f);
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

bool AGridLevelRuntimeActor::HasDoorOnEdge (int32 X, int32 Y, EGridEdge Edge) const
{
    return DoorSystemComponent ? DoorSystemComponent->HasDoorOnEdge (X, Y, Edge) : false;
}

bool AGridLevelRuntimeActor::IsDoorOpenOnEdge (int32 X, int32 Y, EGridEdge Edge) const
{
    return DoorSystemComponent ? DoorSystemComponent->IsDoorOpenOnEdge (X, Y, Edge) : false;
}

bool AGridLevelRuntimeActor::ToggleDoorOnEdge (int32 X, int32 Y, EGridEdge Edge)
{
    return DoorSystemComponent ? DoorSystemComponent->ToggleDoorOnEdge (X, Y, Edge) : false;
}

bool AGridLevelRuntimeActor::OpenDoorOnEdge (int32 X, int32 Y, EGridEdge Edge)
{
    return DoorSystemComponent ? DoorSystemComponent->OpenDoorOnEdge (X, Y, Edge) : false;
}

bool AGridLevelRuntimeActor::CloseDoorOnEdge (int32 X, int32 Y, EGridEdge Edge)
{
    return DoorSystemComponent ? DoorSystemComponent->CloseDoorOnEdge (X, Y, Edge) : false;
}

bool AGridLevelRuntimeActor::TryInteractAtEdge (int32 FromCellX, int32 FromCellY, EGridEdge Edge)
{
    return ActivationComponent ? ActivationComponent->TryInteractAtEdge (FromCellX, FromCellY, Edge) : false;
}

void AGridLevelRuntimeActor::HandlePartyCellChanged (int32 OldCellX, int32 OldCellY, int32 NewCellX, int32 NewCellY)
{
    if (ActivationComponent)
    {
        ActivationComponent->HandlePartyCellChanged (OldCellX, OldCellY, NewCellX, NewCellY);
    }
}

void AGridLevelRuntimeActor::SetEditorHoveredObject (FGuid ObjectId)
{
    if (EditorPreviewComponent)
    {
        EditorPreviewComponent->SetHoveredObject (ObjectId);
    }
}

void AGridLevelRuntimeActor::SetEditorSelectedObject (FGuid ObjectId)
{
    if (EditorPreviewComponent)
    {
        EditorPreviewComponent->SetSelectedObject (ObjectId);
    }
}

void AGridLevelRuntimeActor::CleanupOrphanEditorPreviewObjects ()
{
    if (EditorPreviewComponent)
    {
		EditorPreviewComponent->CleanupOrphanPreviewObjects ();
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

bool AGridLevelRuntimeActor::GetWallMountedObjectTransform (const FGridLevelObjectData& ObjectData, float ZOffset, float WallInset,
    float LocalOffsetAlongWall, float LocalOffsetVertical, FTransform& OutTransform) const
{
    if (!LevelAsset || ObjectData.Edge == EGridEdge::None)
    {
        return false;
    }

    const float CellSize = LevelAsset->CellSize;
    const float FinalZ = ZOffset + LocalOffsetVertical;
    const FVector Base = GetActorLocation () + CellToWorld (ObjectData.CellX, ObjectData.CellY, FinalZ);
    FVector Pos = Base;
    FRotator Rot = FRotator::ZeroRotator;

    switch (ObjectData.Edge)
    {
        case EGridEdge::North:
            Pos = Base + FVector ((CellSize * 0.5f) + LocalOffsetAlongWall, CellSize - WallInset, 0.f);
            Rot = FRotator (0.f, 90.f, 0.f);
            break;

        case EGridEdge::South:
            Pos = Base + FVector ((CellSize * 0.5f) - LocalOffsetAlongWall, WallInset, 0.f);
            Rot = FRotator (0.f, -90.f, 0.f);
            break;

        case EGridEdge::East:
            Pos = Base + FVector (CellSize - WallInset, (CellSize * 0.5f) - LocalOffsetAlongWall, 0.f);
            Rot = FRotator (0.f, 0.f, 0.f);
            break;

        case EGridEdge::West:
            Pos = Base + FVector (WallInset, (CellSize * 0.5f) + LocalOffsetAlongWall, 0.f);
            Rot = FRotator (0.f, 180.f, 0.f);
            break;
    }
    OutTransform = FTransform (Rot, Pos, FVector::OneVector);
    return true;
}

bool AGridLevelRuntimeActor::GetCenteredObjectTransform (const FGridLevelObjectData& ObjectData, float ZOffset, FTransform& OutTransform) const
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

bool AGridLevelRuntimeActor::GetObjectPlacementTransform (const FGridLevelObjectData& ObjectData, FTransform& OutTransform) const
{
    if (!LevelAsset)
    {
        return false;
    }
    const UGridObjectArchetypeAsset* Archetype = FindObjectArchetype (ObjectData.ArchetypeId);
    if (!Archetype)
    {
        return false;
    }
    if (ObjectData.Type == EGridLevelObjectType::Door)
    {
        FVector Pos = FVector::ZeroVector;
        FRotator Rot = FRotator::ZeroRotator;

        GetEdgeTransform (ObjectData.CellX, ObjectData.CellY, ObjectData.Edge, LevelAsset->CellSize, Pos, Rot);

        OutTransform = FTransform (Rot, Pos, FVector::OneVector);
        return true;
    }
    if (Archetype->bPlaceOnEdge)
    {
        return GetWallMountedObjectTransform (ObjectData, Archetype->PlacementZOffset, Archetype->WallInset,
            Archetype->LocalOffsetAlongWall, Archetype->LocalOffsetVertical, OutTransform);
    }
    if (Archetype->bPlaceAtCellCenter)
    {
        return GetCenteredObjectTransform (ObjectData, Archetype->PlacementZOffset, OutTransform);
    }
    return false;
}

void AGridLevelRuntimeActor::RegisterRuntimeObjectActor (const FGuid& ObjectId, AGridRuntimeObjectActor* Actor)
{
    if (!ObjectId.IsValid () || !IsValid (Actor))
    {
        return;
    }
    SpawnedRuntimeObjectActors.Add (ObjectId, Actor);
}

void AGridLevelRuntimeActor::ClearRuntimeObjectActors ()
{
    for (TPair<FGuid, TObjectPtr<AGridRuntimeObjectActor>>& Pair : SpawnedRuntimeObjectActors)
    {
        if (IsValid (Pair.Value))
        {
            Pair.Value->Destroy ();
        }
    }
    SpawnedRuntimeObjectActors.Empty ();
}

TSubclassOf<AGridRuntimeObjectActor> AGridLevelRuntimeActor::GetObjectRuntimeActorClass (const FGridLevelObjectData& ObjectData) const
{
    const UGridObjectArchetypeAsset* Archetype = FindObjectArchetype (ObjectData.ArchetypeId);
    return Archetype ? Archetype->RuntimeActorClass : nullptr;
}

bool AGridLevelRuntimeActor::IsRuntimeSpawnableObject (const FGridLevelObjectData& ObjectData) const
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

        case EGridLevelObjectType::Trigger:
        return true;

        default:
        return false;
    }
}

void AGridLevelRuntimeActor::AddRuntimeObjectActor (const FGridLevelObjectData& ObjectData)
{
    UStaticMesh* Mesh = nullptr;
    UMaterialInterface* Material = nullptr;
    FTransform Transform;

    AGridRuntimeObjectActor* Actor = SpawnRuntimeObjectActor<AGridRuntimeObjectActor> (ObjectData, Mesh, Material, Transform);
    if (!Actor)
    {
        return;
    }
    Actor->InitializeGridObject (ObjectData, Mesh, Material, Transform);
    if (ActivationComponent)
    {
        ActivationComponent->RegisterInitialObjectState (ObjectData);
    }
    if (ObjectData.Type == EGridLevelObjectType::Door && DoorSystemComponent)
    {
        DoorSystemComponent->RegisterDoorObject (ObjectData, Actor);
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

FString AGridLevelRuntimeActor::GetRuntimeDebugSummary () const
{
    FString Result;

    Result += FString::Printf (TEXT ("Grid Runtime | Level=%s\n"), LevelAsset ? *LevelAsset->GetName () : TEXT ("None"));
    Result += FString::Printf (TEXT ("Runtime Actors=%d\n"), SpawnedRuntimeObjectActors.Num ());
    Result += ActivationComponent ? ActivationComponent->GetDebugSummary () : TEXT ("Activation | Missing");
    Result += TEXT ("\n");
    Result += DoorSystemComponent ? DoorSystemComponent->GetDebugSummary () : TEXT ("Doors | Missing");
    return Result;
}

void AGridLevelRuntimeActor::LogRuntimeDebugSummary () const
{
    const FString Summary = GetRuntimeDebugSummary ();
    UE_LOG (LogTemp, Log, TEXT ("%s"), *Summary);
    if (ActivationComponent)
    {
        ActivationComponent->LogDebugSummary ();
    }
    if (DoorSystemComponent)
    {
        DoorSystemComponent->LogDebugSummary ();
    }
}

void AGridLevelRuntimeActor::ShowRuntimeDebugSummary (float Duration) const
{
    if (!GEngine)
    {
        return;
    }
    GEngine->AddOnScreenDebugMessage (-1, Duration, FColor::Green, GetRuntimeDebugSummary ()
    );
}