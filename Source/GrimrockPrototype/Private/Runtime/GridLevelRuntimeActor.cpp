#include "Runtime/GridLevelRuntimeActor.h"
#include "Core/GridTypes.h"
#include "Core/GridDirectionUtils.h"
#include "Core/GridObjectArchetypeAsset.h"
#include "Runtime/GridRuntimeObjectActor.h"
#include "Runtime/GridActivationComponent.h"
#include "Runtime/GridDoorSystemComponent.h"
#include "Runtime/GridEditorPreviewComponent.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridMechanismActor.h"
#include "Runtime/GridReceptacleActor.h"
#include "UI/ReadableMessageWidget.h"
#include "Blueprint/UserWidget.h"

bool AGridLevelRuntimeActor::IsSafeRuntimeRenderTransform (const FTransform& Transform)
{
    constexpr float MinAbsScale = 0.001f;
    const FVector Scale = Transform.GetScale3D ();

    return Transform.IsValid () &&
        !Transform.GetLocation ().ContainsNaN () &&
        !Scale.ContainsNaN () &&
        Transform.GetRotation ().IsNormalized () &&
        FMath::Abs (Scale.X) >= MinAbsScale &&
        FMath::Abs (Scale.Y) >= MinAbsScale &&
        FMath::Abs (Scale.Z) >= MinAbsScale;
}

void AGridLevelRuntimeActor::LogUnsafeInstanceTransform (const TCHAR* FunctionName, const UInstancedStaticMeshComponent* Component,
    int32 X, int32 Y, EGridEdge Edge, const FTransform& Transform) const
{
    UE_LOG (LogTemp, Error,
        TEXT ("Unsafe runtime render transform skipped: Function=%s Component=%s StaticMesh=%s Cell=(%d,%d) Edge=%d Location=%s Rotation=%s Scale=%s"),
        FunctionName,
        *GetNameSafe (Component),
        *GetNameSafe (Component ? Component->GetStaticMesh () : nullptr),
        X,
        Y,
        static_cast<int32> (Edge),
        *Transform.GetLocation ().ToCompactString (),
        *Transform.GetRotation ().ToString (),
        *Transform.GetScale3D ().ToCompactString ());
}

void AGridLevelRuntimeActor::LogUnsafeObjectTransform (const TCHAR* FunctionName, const FGridLevelObjectData& ObjectData,
    const UStaticMesh* StaticMesh, const FTransform& Transform) const
{
    UE_LOG (LogTemp, Error,
        TEXT ("Unsafe runtime render transform skipped: Function=%s ObjectId=%s ArchetypeId=%s Tag=%s Cell=(%d,%d) Edge=%d StaticMesh=%s Location=%s Rotation=%s Scale=%s"),
        FunctionName,
        *ObjectData.ObjectId.ToString (),
        *ObjectData.ArchetypeId.ToString (),
        *ObjectData.Tag.ToString (),
        ObjectData.CellX,
        ObjectData.CellY,
        static_cast<int32> (ObjectData.Edge),
        *GetNameSafe (StaticMesh),
        *Transform.GetLocation ().ToCompactString (),
        *Transform.GetRotation ().ToString (),
        *Transform.GetScale3D ().ToCompactString ());
}

void AGridLevelRuntimeActor::LogUnsafeItemTransform (const TCHAR* FunctionName, FName ArchetypeId, const AActor* OwnerActor,
    const USceneComponent* AttachParent, const UStaticMesh* StaticMesh, const FTransform& Transform) const
{
    UE_LOG (LogTemp, Error,
        TEXT ("Unsafe runtime item transform skipped: Function=%s ArchetypeId=%s Owner=%s AttachParent=%s StaticMesh=%s Location=%s Rotation=%s Scale=%s"),
        FunctionName,
        *ArchetypeId.ToString (),
        *GetNameSafe (OwnerActor),
        *GetNameSafe (AttachParent),
        *GetNameSafe (StaticMesh),
        *Transform.GetLocation ().ToCompactString (),
        *Transform.GetRotation ().ToString (),
        *Transform.GetScale3D ().ToCompactString ());
}

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

    ActivationComponent = CreateDefaultSubobject<UGridActivationComponent> (TEXT ("ActivationComponent"));

    DoorSystemComponent = CreateDefaultSubobject<UGridDoorSystemComponent> (TEXT ("DoorSystemComponent"));

    EditorPreviewComponent = CreateDefaultSubobject<UGridEditorPreviewComponent> (TEXT ("EditorPreviewComponent"));
}

void AGridLevelRuntimeActor::ShowReadableMessage (const FText& MessageText)
{
    if (MessageText.IsEmpty ())
    {
        return;
    }
    UWorld* World = GetWorld ();
    if (!World || !ReadableMessageWidgetClass)
    {
        UE_LOG (LogTemp, Warning, TEXT ("ShowReadableMessage failed: missing world or widget class."));
        return;
    }
    APlayerController* PlayerController = World->GetFirstPlayerController ();
    if (!PlayerController)
    {
        UE_LOG (LogTemp, Warning, TEXT ("ShowReadableMessage failed: missing player controller."));
        return;
    }
    if (!ActiveReadableMessageWidget)
    {
        ActiveReadableMessageWidget = CreateWidget<UReadableMessageWidget> (PlayerController, ReadableMessageWidgetClass);

        if (!ActiveReadableMessageWidget)
        {
            UE_LOG (LogTemp, Warning, TEXT ("ShowReadableMessage failed: widget creation failed."));
            return;
        }
        ActiveReadableMessageWidget->AddToViewport (50);
    }
    ActiveReadableMessageWidget->SetReadableText (MessageText);
    GetWorldTimerManager ().ClearTimer (ReadableMessageTimerHandle);
    GetWorldTimerManager ().SetTimer (ReadableMessageTimerHandle, this, &AGridLevelRuntimeActor::HideReadableMessage, ReadableMessageDuration, false);
    if (!ActiveReadableMessageWidget->IsInViewport ())
    {
        ActiveReadableMessageWidget->AddToViewport (50);
    }
}

void AGridLevelRuntimeActor::HideReadableMessage ()
{
    GetWorldTimerManager ().ClearTimer (ReadableMessageTimerHandle);

    if (ActiveReadableMessageWidget)
    {
        ActiveReadableMessageWidget->RemoveFromParent ();
        ActiveReadableMessageWidget = nullptr;
    }
}

void AGridLevelRuntimeActor::OnConstruction (const FTransform& Transform)
{
    Super::OnConstruction (Transform);
    if (!bRebuildInConstruction)
    {
        return;
    }
    UWorld* World = GetWorld ();
    if (!World || World->IsGameWorld ())
    {
        return;
    }
    RebuildLevel (EGridRuntimeRebuildMode::GeometryOnly);
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
    if (!IsSafeRuntimeRenderTransform (T))
    {
        LogUnsafeInstanceTransform (TEXT ("AddFloor"), FloorISM, X, Y, EGridEdge::None, T);
        return;
    }
    FloorISM->AddInstance (T);
}

void AGridLevelRuntimeActor::AddCeiling (int32 X, int32 Y, float CellSize)
{
    const FVector Base = CellToWorld (X, Y, 200.f);
    const FVector CenterOffset (CellSize * 0.5f, CellSize * 0.5f, 0.f);
    const FTransform T (FRotator::ZeroRotator, Base + CenterOffset, FVector (1.f, 1.f, 1.f));
    if (!IsSafeRuntimeRenderTransform (T))
    {
        LogUnsafeInstanceTransform (TEXT ("AddCeiling"), CeilingISM, X, Y, EGridEdge::None, T);
        return;
    }
    CeilingISM->AddInstance (T);
}

bool AGridLevelRuntimeActor::ShouldSuppressStandardWallForEdge (int32 X, int32 Y, EGridEdge Edge) const
{
    if (!LevelAsset || Edge == EGridEdge::None)
    {
        return false;
    }

    for (const FGridLevelObjectData& ObjectData : LevelAsset->Objects)
    {
        if (ObjectData.CellX == X &&
            ObjectData.CellY == Y &&
            ObjectData.Edge == Edge)
        {
            const UGridObjectArchetypeAsset* Archetype = FindObjectArchetype (ObjectData.ArchetypeId);
            if (Archetype && Archetype->bReplacesStandardWall)
            {
                return true;
            }
        }
    }

    return false;
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
    if (!IsSafeRuntimeRenderTransform (T))
    {
        LogUnsafeInstanceTransform (TEXT ("AddEdgeInstance"), TargetISM, X, Y, Edge, T);
        return;
    }
    TargetISM->AddInstance (T);
}

void AGridLevelRuntimeActor::RebuildLevel (EGridRuntimeRebuildMode RebuildMode)
{
    ClearVisuals (RebuildMode);

    if (!LevelAsset || !FloorISM || !WallISM || !CeilingISM)
    {
        return;
    }

    LevelAsset->EnsureCellCount ();
#if WITH_EDITOR
    for (const UGridObjectArchetypeAsset* Archetype : ObjectArchetypes)
    {
        if (!Archetype)
        {
            continue;
        }

        TArray<FGridArchetypeValidationMessage> ValidationMessages;
        Archetype->ValidateArchetype (ValidationMessages);

        bool bHasWarningOrError = false;
        for (const FGridArchetypeValidationMessage& Message : ValidationMessages)
        {
            if (Message.Severity == EGridArchetypeValidationSeverity::Warning ||
                Message.Severity == EGridArchetypeValidationSeverity::Error)
            {
                bHasWarningOrError = true;
                break;
            }
        }

        if (bHasWarningOrError)
        {
            UE_LOG (LogTemp, Warning, TEXT ("%s"), *Archetype->GetValidationSummary ());
        }
    }
#endif
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
                if (!bShouldDraw || ShouldSuppressStandardWallForEdge (X, Y, Edge))
                {
                    return;
                }
                switch (WallType)
                {
                    case EGridWallType::Solid:
                    AddEdgeInstance (WallISM, X, Y, Edge, CellSize);
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
    int32 ResolvedX = INDEX_NONE;
    int32 ResolvedY = INDEX_NONE;
    EGridEdge ResolvedEdge = EGridEdge::None;
    bool bResolvedOpposite = false;
    return TryResolveDoorEdge (X, Y, Edge, ResolvedX, ResolvedY, ResolvedEdge, bResolvedOpposite);
}

bool AGridLevelRuntimeActor::TryGetOppositeEdge (int32 X, int32 Y, EGridEdge Edge, int32& OutX, int32& OutY, EGridEdge& OutEdge) const
{
    OutX = X;
    OutY = Y;
    OutEdge = EGridEdge::None;

    if (Edge == EGridEdge::None)
    {
        return false;
    }
    if (!TryGetNeighborCell (X, Y, Edge, OutX, OutY))
    {
        return false;
    }
    OutEdge = GridDirectionUtils::GetBackward (Edge);
    return OutEdge != EGridEdge::None;
}

bool AGridLevelRuntimeActor::TryResolveDoorEdge (int32 X, int32 Y, EGridEdge Edge, int32& OutX, int32& OutY, EGridEdge& OutEdge, bool& bOutResolvedOpposite) const
{
    OutX = X;
    OutY = Y;
    OutEdge = Edge;
    bOutResolvedOpposite = false;

    if (!DoorSystemComponent)
    {
        return false;
    }
    if (DoorSystemComponent->HasDoorOnEdge (X, Y, Edge))
    {
        return true;
    }
    if (!TryGetOppositeEdge (X, Y, Edge, OutX, OutY, OutEdge))
    {
        return false;
    }
    bOutResolvedOpposite = DoorSystemComponent->HasDoorOnEdge (OutX, OutY, OutEdge);
    return bOutResolvedOpposite;
}

bool AGridLevelRuntimeActor::IsDoorOpenOnEdge (int32 X, int32 Y, EGridEdge Edge) const
{
    int32 ResolvedX = INDEX_NONE;
    int32 ResolvedY = INDEX_NONE;
    EGridEdge ResolvedEdge = EGridEdge::None;
    bool bResolvedOpposite = false;
    return TryResolveDoorEdge (X, Y, Edge, ResolvedX, ResolvedY, ResolvedEdge, bResolvedOpposite) &&
        DoorSystemComponent->IsDoorOpenOnEdge (ResolvedX, ResolvedY, ResolvedEdge);
}

bool AGridLevelRuntimeActor::ToggleDoorOnEdge (int32 X, int32 Y, EGridEdge Edge)
{
    int32 ResolvedX = INDEX_NONE;
    int32 ResolvedY = INDEX_NONE;
    EGridEdge ResolvedEdge = EGridEdge::None;
    bool bResolvedOpposite = false;
    if (TryResolveDoorEdge (X, Y, Edge, ResolvedX, ResolvedY, ResolvedEdge, bResolvedOpposite) &&
        DoorSystemComponent->ToggleDoorOnEdge (ResolvedX, ResolvedY, ResolvedEdge))
    {
        if (bResolvedOpposite)
        {
            UE_LOG (LogTemp, Log, TEXT ("Grid Use: toggled door on opposite edge (%d,%d,%d)."),
                ResolvedX, ResolvedY, static_cast<int32> (ResolvedEdge));
        }
        return true;
    }
    return false;
}

bool AGridLevelRuntimeActor::OpenDoorOnEdge (int32 X, int32 Y, EGridEdge Edge)
{
    int32 ResolvedX = INDEX_NONE;
    int32 ResolvedY = INDEX_NONE;
    EGridEdge ResolvedEdge = EGridEdge::None;
    bool bResolvedOpposite = false;
    if (TryResolveDoorEdge (X, Y, Edge, ResolvedX, ResolvedY, ResolvedEdge, bResolvedOpposite) &&
        DoorSystemComponent->OpenDoorOnEdge (ResolvedX, ResolvedY, ResolvedEdge))
    {
        if (bResolvedOpposite)
        {
            UE_LOG (LogTemp, Log, TEXT ("Grid Use: opened door on opposite edge (%d,%d,%d)."),
                ResolvedX, ResolvedY, static_cast<int32> (ResolvedEdge));
        }
        return true;
    }
    return false;
}

bool AGridLevelRuntimeActor::CloseDoorOnEdge (int32 X, int32 Y, EGridEdge Edge)
{
    int32 ResolvedX = INDEX_NONE;
    int32 ResolvedY = INDEX_NONE;
    EGridEdge ResolvedEdge = EGridEdge::None;
    bool bResolvedOpposite = false;
    if (TryResolveDoorEdge (X, Y, Edge, ResolvedX, ResolvedY, ResolvedEdge, bResolvedOpposite) &&
        DoorSystemComponent->CloseDoorOnEdge (ResolvedX, ResolvedY, ResolvedEdge))
    {
        if (bResolvedOpposite)
        {
            UE_LOG (LogTemp, Log, TEXT ("Grid Use: closed door on opposite edge (%d,%d,%d)."),
                ResolvedX, ResolvedY, static_cast<int32> (ResolvedEdge));
        }
        return true;
    }
    return false;
}

bool AGridLevelRuntimeActor::TryInteractAtEdge (int32 FromCellX, int32 FromCellY, EGridEdge Edge, AGrimrockPartyPawn* PartyPawn)
{
    if (!ActivationComponent)
    {
        return false;
    }
    if (ActivationComponent->TryInteractAtEdge (FromCellX, FromCellY, Edge, PartyPawn))
    {
        return true;
    }
    int32 OppositeX = INDEX_NONE;
    int32 OppositeY = INDEX_NONE;
    EGridEdge OppositeEdge = EGridEdge::None;
    if (TryGetOppositeEdge (FromCellX, FromCellY, Edge, OppositeX, OppositeY, OppositeEdge) &&
        ActivationComponent->TryInteractAtEdge (OppositeX, OppositeY, OppositeEdge, PartyPawn))
    {
        UE_LOG (LogTemp, Log, TEXT ("Grid Use: interacted with object on opposite edge (%d,%d,%d)."),
            OppositeX, OppositeY, static_cast<int32> (OppositeEdge));
        return true;
    }
    return false;
}

bool AGridLevelRuntimeActor::ExecuteLinksFromRuntimeObject (FGuid SourceObjectId, EGridObjectEvent SourceEvent)
{
    return ActivationComponent ? ActivationComponent->ExecuteLinksFromObjectForEvent (SourceObjectId, SourceEvent) : false;
}

void AGridLevelRuntimeActor::HandlePartyCellChanged (int32 OldCellX, int32 OldCellY, int32 NewCellX, int32 NewCellY)
{
    if (ActivationComponent)
    {
        ActivationComponent->HandlePartyCellChanged (OldCellX, OldCellY, NewCellX, NewCellY);
    }
}

void AGridLevelRuntimeActor::NotifyPawnEnteredCell (int32 CellX, int32 CellY)
{
    if (ActivationComponent)
    {
        ActivationComponent->NotifyPawnEnteredCell (CellX, CellY);
    }
}

void AGridLevelRuntimeActor::NotifyPawnExitedCell (int32 CellX, int32 CellY)
{
    if (ActivationComponent)
    {
        ActivationComponent->NotifyPawnExitedCell (CellX, CellY);
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
    if (!Archetype)
    {
        return nullptr;
    }

    if (Archetype->PreviewMesh)
    {
        return Archetype->PreviewMesh.Get ();
    }

    if (Archetype->MovingMesh)
    {
        return Archetype->MovingMesh.Get ();
    }

    if (Archetype->FixedMesh)
    {
        return Archetype->FixedMesh.Get ();
    }

    return nullptr;
}

UMaterialInterface* AGridLevelRuntimeActor::GetObjectMaterial (const FGridLevelObjectData& ObjectData) const
{
    const UGridObjectArchetypeAsset* Archetype = FindObjectArchetype (ObjectData.ArchetypeId);
    if (!Archetype)
    {
        return nullptr;
    }

    if (Archetype->PreviewMaterial)
    {
        return Archetype->PreviewMaterial.Get ();
    }

    if (Archetype->MovingMaterial)
    {
        return Archetype->MovingMaterial.Get ();
    }

    if (Archetype->FixedMaterial)
    {
        return Archetype->FixedMaterial.Get ();
    }

    return nullptr;
}

AGridItemActor* AGridLevelRuntimeActor::SpawnItemActorForArchetype (FName ItemArchetypeId, AActor* OwnerActor, USceneComponent* AttachParent) const
{
    const UGridObjectArchetypeAsset* ItemArchetype = FindObjectArchetype (ItemArchetypeId);
    if (!ItemArchetype)
    {
        UE_LOG (LogTemp, Warning, TEXT ("Grid item spawn failed: archetype %s not found."), *ItemArchetypeId.ToString ());
        return nullptr;
    }

    UWorld* World = GetWorld ();
    if (!World)
    {
        return nullptr;
    }

    TSubclassOf<AGridItemActor> ItemClass = ItemArchetype->ItemActorClass;
    if (!ItemClass)
    {
        ItemClass = AGridItemActor::StaticClass ();
    }

    UStaticMesh* ItemMesh = ItemArchetype->MovingMesh ? ItemArchetype->MovingMesh.Get () : ItemArchetype->PreviewMesh.Get ();
    if (!ItemMesh)
    {
        ItemMesh = ItemArchetype->FixedMesh.Get ();
    }

    const FTransform SpawnTransform (
        AttachParent ? AttachParent->GetComponentRotation () : FRotator::ZeroRotator,
        AttachParent ? AttachParent->GetComponentLocation () : GetActorLocation (),
        FVector::OneVector);
    if (!IsSafeRuntimeRenderTransform (SpawnTransform))
    {
        LogUnsafeItemTransform (TEXT ("SpawnItemActorForArchetype"), ItemArchetypeId, OwnerActor, AttachParent, ItemMesh, SpawnTransform);
        return nullptr;
    }

    FActorSpawnParameters Params;
    Params.Owner = OwnerActor ? OwnerActor : const_cast<AGridLevelRuntimeActor*> (this);
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AGridItemActor* ItemActor = World->SpawnActor<AGridItemActor> (
        ItemClass,
        SpawnTransform.GetLocation (),
        SpawnTransform.GetRotation ().Rotator (),
        Params);

    if (!ItemActor)
    {
        return nullptr;
    }

    UMaterialInterface* ItemMaterial = ItemArchetype->MovingMaterial ? ItemArchetype->MovingMaterial.Get () : ItemArchetype->PreviewMaterial.Get ();
    if (!ItemMaterial)
    {
        ItemMaterial = ItemArchetype->FixedMaterial.Get ();
    }

    ItemActor->InitializeItem (ItemArchetype->ArchetypeId, ItemArchetype->ItemTags, ItemMesh, ItemMaterial);

    if (AttachParent)
    {
        ItemActor->ConfigureAsAttachedItem ();
        ItemActor->AttachToComponent (AttachParent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
        ItemActor->SetActorRelativeTransform (FTransform::Identity);
    }

    return ItemActor;
}

bool AGridLevelRuntimeActor::GetFloorEdgeObjectTransform (const FGridLevelObjectData& ObjectData, float ZOffset, float EdgeInset,
    FTransform& OutTransform) const
{
    if (!LevelAsset || ObjectData.Edge == EGridEdge::None)
    {
        return false;
    }

    const float CellSize = LevelAsset->CellSize;
    const FVector Base = GetActorLocation () + CellToWorld (ObjectData.CellX, ObjectData.CellY, ZOffset);
    FVector Pos = Base + FVector (CellSize * 0.5f, CellSize * 0.5f, 0.f);
    FRotator Rot = FRotator::ZeroRotator;

    switch (ObjectData.Edge)
    {
        case EGridEdge::North:
            Pos = Base + FVector (CellSize * 0.5f, CellSize - EdgeInset, 0.f);
            Rot = FRotator (0.f, 0.f, 0.f);
            break;

        case EGridEdge::South:
            Pos = Base + FVector (CellSize * 0.5f, EdgeInset, 0.f);
            Rot = FRotator (0.f, 180.f, 0.f);
            break;

        case EGridEdge::East:
            Pos = Base + FVector (CellSize - EdgeInset, CellSize * 0.5f, 0.f);
            Rot = FRotator (0.f, 90.f, 0.f);
            break;

        case EGridEdge::West:
            Pos = Base + FVector (EdgeInset, CellSize * 0.5f, 0.f);
            Rot = FRotator (0.f, -90.f, 0.f);
            break;

        default:
            return false;
    }

    Rot.Yaw += ObjectData.LocalYaw;
    OutTransform = FTransform (Rot, Pos, FVector::OneVector);
    return true;
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

    FRotator Rotation = FRotator::ZeroRotator;
    Rotation.Yaw = ObjectData.LocalYaw;

    OutTransform = FTransform (Rotation, Pos, FVector::OneVector);
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
    if (ObjectData.Type == EGridLevelObjectType::Item && ObjectData.Edge != EGridEdge::None)
    {
        const float EdgeInset = FMath::Max (Archetype->WallInset, 18.f);
        return GetFloorEdgeObjectTransform (ObjectData, Archetype->PlacementZOffset, EdgeInset, OutTransform);
    }
    if (Archetype->IsEdgePlaced ())
    {
        return GetWallMountedObjectTransform (ObjectData, Archetype->PlacementZOffset, Archetype->WallInset,
            Archetype->LocalOffsetAlongWall, Archetype->LocalOffsetVertical, OutTransform);
    }
    if (Archetype->IsCenterPlaced ())
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
    for (AGridItemActor* ItemActor : SpawnedItemActors)
    {
        if (IsValid (ItemActor))
        {
            ItemActor->OnRemovedFromWorld ();
            ItemActor->Destroy ();
        }
    }
    SpawnedItemActors.Reset ();
    SpawnedItemEntries.Reset ();

    for (TPair<FGuid, TObjectPtr<AGridRuntimeObjectActor>>& Pair : SpawnedRuntimeObjectActors)
    {
        if (IsValid (Pair.Value))
        {
            Pair.Value->Destroy ();
        }
    }
    SpawnedRuntimeObjectActors.Empty ();
}

bool AGridLevelRuntimeActor::TryPickupItemAtCell (int32 CellX, int32 CellY, AGrimrockPartyPawn* PartyPawn)
{
    if (!PartyPawn)
    {
        return false;
    }

    const FIntPoint TargetCell (CellX, CellY);
    for (int32 EntryIndex = 0; EntryIndex < SpawnedItemEntries.Num (); ++EntryIndex)
    {
        FGridSpawnedItemRuntimeEntry& Entry = SpawnedItemEntries[EntryIndex];
        if (Entry.Cell != TargetCell)
        {
            continue;
        }

        AGridItemActor* ItemActor = Entry.ItemActor.Get ();
        if (!IsValid (ItemActor))
        {
            SpawnedItemEntries.RemoveAtSwap (EntryIndex);
            SpawnedItemActors.RemoveAllSwap ([] (const TObjectPtr<AGridItemActor>& SpawnedItemActor)
            {
                return !IsValid (SpawnedItemActor.Get ());
            });
            return false;
        }

        const FName ItemArchetypeId = Entry.ItemArchetypeId.IsNone ()
            ? ItemActor->GetItemArchetypeId ()
            : Entry.ItemArchetypeId;
        if (ItemArchetypeId.IsNone ())
        {
            UE_LOG (LogTemp, Warning, TEXT ("Item pickup failed at cell %d,%d: missing item archetype id."), CellX, CellY);
            return false;
        }

        if (!PartyPawn->AddInventoryItem (ItemArchetypeId, 1))
        {
            UE_LOG (LogTemp, Warning, TEXT ("Item pickup failed at cell %d,%d for item %s."), CellX, CellY, *ItemArchetypeId.ToString ());
            return false;
        }

        if (IsValid (ItemActor))
        {
            ItemActor->OnRemovedFromWorld ();
            ItemActor->Destroy ();
        }
        SpawnedItemActors.RemoveAllSwap ([ItemActor] (const TObjectPtr<AGridItemActor>& SpawnedItemActor)
        {
            return SpawnedItemActor.Get () == ItemActor;
        });

        SpawnedItemEntries.RemoveAtSwap (EntryIndex);

        UE_LOG (LogTemp, Log, TEXT ("Picked up item %s from cell %d,%d."), *ItemArchetypeId.ToString (), CellX, CellY);
        return true;
    }

    return false;
}

bool AGridLevelRuntimeActor::TryPickupItemActor (AGridItemActor* ItemActor, AGrimrockPartyPawn* PartyPawn)
{
    if (!IsValid (ItemActor) || !PartyPawn)
    {
        return false;
    }

    for (int32 EntryIndex = 0; EntryIndex < SpawnedItemEntries.Num (); ++EntryIndex)
    {
        FGridSpawnedItemRuntimeEntry& Entry = SpawnedItemEntries[EntryIndex];
        if (Entry.ItemActor.Get () != ItemActor)
        {
            continue;
        }

        const FName ItemArchetypeId = Entry.ItemArchetypeId.IsNone ()
            ? ItemActor->GetItemArchetypeId ()
            : Entry.ItemArchetypeId;
        if (ItemArchetypeId.IsNone ())
        {
            UE_LOG (LogTemp, Warning, TEXT ("Item pickup failed for actor %s: missing item archetype id."), *ItemActor->GetName ());
            return false;
        }

        if (!PartyPawn->AddInventoryItem (ItemArchetypeId, 1))
        {
            UE_LOG (LogTemp, Warning, TEXT ("Item pickup failed for actor %s and item %s."), *ItemActor->GetName (), *ItemArchetypeId.ToString ());
            return false;
        }

        ItemActor->OnRemovedFromWorld ();
        ItemActor->Destroy ();

        SpawnedItemActors.RemoveAllSwap ([ItemActor] (const TObjectPtr<AGridItemActor>& SpawnedItemActor)
        {
            return SpawnedItemActor.Get () == ItemActor;
        });

        const FIntPoint PickedCell = Entry.Cell;
        SpawnedItemEntries.RemoveAtSwap (EntryIndex);

        UE_LOG (LogTemp, Log, TEXT ("Picked up item %s from clicked actor at cell %d,%d."), *ItemArchetypeId.ToString (), PickedCell.X, PickedCell.Y);
        return true;
    }

    return false;
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

    if (ObjectData.Type == EGridLevelObjectType::Item)
    {
        return !ObjectData.ArchetypeId.IsNone ();
    }

    const UGridObjectArchetypeAsset* Archetype = FindObjectArchetype (ObjectData.ArchetypeId);
    if (Archetype && Archetype->RuntimeActorClass)
    {
        return !Archetype->IsEdgePlaced () || ObjectData.Edge != EGridEdge::None;
    }

    switch (ObjectData.Type)
    {
        case EGridLevelObjectType::Door:
        case EGridLevelObjectType::Button:
        case EGridLevelObjectType::Lever:
        case EGridLevelObjectType::Receptacle:
        return ObjectData.Edge != EGridEdge::None;

        case EGridLevelObjectType::PressurePlate:
        return true;

        case EGridLevelObjectType::Trigger:
        return true;

        default:
        return false;
    }
}

void AGridLevelRuntimeActor::AddPlacedItemActor (const FGridLevelObjectData& ObjectData)
{
    FTransform Transform;
    if (!GetObjectPlacementTransform (ObjectData, Transform))
    {
        UE_LOG (LogTemp, Warning, TEXT ("Placed item skipped: could not compute placement transform for object %s."), *ObjectData.ObjectId.ToString ());
        return;
    }
    if (!IsSafeRuntimeRenderTransform (Transform))
    {
        LogUnsafeObjectTransform (TEXT ("AddPlacedItemActor"), ObjectData, GetObjectMesh (ObjectData), Transform);
        return;
    }

    AGridItemActor* ItemActor = SpawnItemActorForArchetype (ObjectData.ArchetypeId, this, nullptr);
    if (!ItemActor)
    {
        UE_LOG (LogTemp, Warning, TEXT ("Placed item skipped: failed to spawn item archetype %s."), *ObjectData.ArchetypeId.ToString ());
        return;
    }

    ItemActor->SetActorTransform (Transform);
    ItemActor->SetRuntimeCell (ObjectData.CellX, ObjectData.CellY);
    ItemActor->ConfigureAsWorldPickup ();
    ItemActor->OnRemovedFromWorld ();
    SpawnedItemActors.Add (ItemActor);

    FGridSpawnedItemRuntimeEntry Entry;
    Entry.Cell = FIntPoint (ObjectData.CellX, ObjectData.CellY);
    Entry.ItemActor = ItemActor;
    Entry.ItemArchetypeId = ObjectData.ArchetypeId;
    SpawnedItemEntries.Add (Entry);

    UE_LOG (LogTemp, Log, TEXT ("Placed item spawned: %s at object %s."), *ObjectData.ArchetypeId.ToString (), *ObjectData.ObjectId.ToString ());
}

void AGridLevelRuntimeActor::AddRuntimeObjectActor (const FGridLevelObjectData& ObjectData)
{
    UStaticMesh* Mesh = nullptr;
    UMaterialInterface* Material = nullptr;
    FTransform Transform;

    AGridRuntimeObjectActor* Actor = SpawnRuntimeObjectActor<AGridRuntimeObjectActor> (ObjectData, Mesh, Material, Transform);
    UE_LOG (LogTemp, Warning, TEXT ("Runtime object: Type=%d Archetype=%s Tag=%s Id=%s"),
        static_cast<int32>(ObjectData.Type),
        *ObjectData.ArchetypeId.ToString (),
        *ObjectData.Tag.ToString (),
        *ObjectData.ObjectId.ToString ());

    if (!Actor)
    {
        return;
    }
    FGridLevelObjectData RuntimeObjectData = ObjectData;
    const UGridObjectArchetypeAsset* Archetype = FindObjectArchetype (ObjectData.ArchetypeId);

    if (AGridMechanismActor* MechanismActor = Cast<AGridMechanismActor> (Actor))
    {
        MechanismActor->InitializeMechanismVisuals (RuntimeObjectData, Archetype, Transform);
        Actor->InitializeGridObject (RuntimeObjectData, Mesh, Material, Transform);
    } else if (AGridGenericObjectActor* GenericActor = Cast<AGridGenericObjectActor> (Actor))
    {
        GenericActor->InitializeGenericObject (RuntimeObjectData, Archetype, Mesh, Material, Transform);
    } else
    {
        Actor->InitializeGridObject (RuntimeObjectData, Mesh, Material, Transform);
    }
    if (AGridReceptacleActor* ReceptacleActor = Cast<AGridReceptacleActor> (Actor))
    {
        if (Archetype)
        {
            ReceptacleActor->ConfigureContainedItemVisual (
                Archetype->MovingMesh,
                Archetype->MovingMaterial
            );
        }
        if (!RuntimeObjectData.Behavior.Receptacle.InitialContainedItemArchetypeId.IsNone ())
        {
            if (AGridItemActor* ItemActor = SpawnItemActorForArchetype (
                RuntimeObjectData.Behavior.Receptacle.InitialContainedItemArchetypeId,
                ReceptacleActor,
                ReceptacleActor->ItemAttachPoint))
            {
                ReceptacleActor->SetInitialContainedItemActor (ItemActor);
            }
        }
    }    
    if (ActivationComponent)
    {
        ActivationComponent->RegisterInitialObjectState (RuntimeObjectData);
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
        if (ObjectData.Type == EGridLevelObjectType::Item)
        {
            if (ObjectData.bInitiallyEnabled)
            {
                AddPlacedItemActor (ObjectData);
            }
            continue;
        }

        if (!IsRuntimeSpawnableObject (ObjectData))
        {
            if (ObjectData.bInitiallyEnabled)
            {
                const UGridObjectArchetypeAsset* Archetype = FindObjectArchetype (ObjectData.ArchetypeId);
                if (Archetype && !Archetype->RuntimeActorClass)
                {
                    UE_LOG (
                        LogTemp,
                        Warning,
                        TEXT ("Runtime object skipped: archetype %s has no RuntimeActorClass."),
                        *ObjectData.ArchetypeId.ToString ());
                }
            }
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
    Result += FString::Printf (TEXT ("Spawned Items=%d\n"), SpawnedItemActors.Num ());
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

FString AGridLevelRuntimeActor::GetLevelAssetDiagnostics () const
{
    const UWorld* World = GetWorld ();
    FString WorldType = TEXT ("None");
    FString MapName = TEXT ("None");

    if (World)
    {
        MapName = World->GetMapName ();
        switch (World->WorldType)
        {
            case EWorldType::Game:
                WorldType = TEXT ("Game");
                break;

            case EWorldType::PIE:
                WorldType = TEXT ("PIE");
                break;

            case EWorldType::Editor:
                WorldType = TEXT ("Editor");
                break;

            case EWorldType::EditorPreview:
                WorldType = TEXT ("EditorPreview");
                break;

            case EWorldType::GamePreview:
                WorldType = TEXT ("GamePreview");
                break;

            case EWorldType::Inactive:
                WorldType = TEXT ("Inactive");
                break;

            default:
                WorldType = FString::Printf (TEXT ("Unknown(%d)"), static_cast<int32> (World->WorldType));
                break;
        }
    }

    FString Result;
    Result += TEXT ("Grid LevelAsset Diagnostics\n");
    Result += FString::Printf (TEXT ("RuntimeActor=%s\n"), *GetPathName ());
#if WITH_EDITOR
    Result += FString::Printf (TEXT ("ActorLabel=%s\n"), *GetActorLabel ());
#else
    Result += FString::Printf (TEXT ("ActorName=%s\n"), *GetName ());
#endif
    Result += FString::Printf (TEXT ("World=%s\n"), *GetNameSafe (World));
    Result += FString::Printf (TEXT ("WorldType=%s\n"), *WorldType);
    Result += FString::Printf (TEXT ("Map=%s\n"), *MapName);
    Result += FString::Printf (TEXT ("LevelAsset=%s\n"), LevelAsset ? *LevelAsset->GetPathName () : TEXT ("None"));

    if (!LevelAsset)
    {
        Result += TEXT ("Status=ERROR: missing LevelAsset reference.\n");
        return Result;
    }

    const int32 ExpectedCellCount = LevelAsset->Width * LevelAsset->Height;
    int32 NonEmptyCellCount = 0;
    int32 BlockingCellCount = 0;
    int32 CeilingCellCount = 0;

    for (const FGridLevelCellData& Cell : LevelAsset->Cells)
    {
        if (Cell.CellType != EGridCellType::Empty)
        {
            ++NonEmptyCellCount;
        }
        if (Cell.bBlocksOccupancy)
        {
            ++BlockingCellCount;
        }
        if (Cell.bHasCeiling)
        {
            ++CeilingCellCount;
        }
    }

    Result += FString::Printf (TEXT ("AssetPackage=%s\n"), *GetNameSafe (LevelAsset->GetOutermost ()));
    Result += FString::Printf (TEXT ("GridSize=%dx%d\n"), LevelAsset->Width, LevelAsset->Height);
    Result += FString::Printf (TEXT ("CellSize=%.2f\n"), LevelAsset->CellSize);
    Result += FString::Printf (TEXT ("StartCell=(%d,%d) StartFacing=%s StartCellValid=%s\n"),
        LevelAsset->StartCellX,
        LevelAsset->StartCellY,
        StaticEnum<EGridEdge> () ? *StaticEnum<EGridEdge> ()->GetNameStringByValue (static_cast<int64> (LevelAsset->StartFacing)) : TEXT ("Unknown"),
        LevelAsset->IsStartCellValid () ? TEXT ("true") : TEXT ("false"));
    Result += FString::Printf (TEXT ("Cells=%d ExpectedCells=%d\n"), LevelAsset->Cells.Num (), ExpectedCellCount);
    Result += FString::Printf (TEXT ("NonEmptyCells=%d BlockingCells=%d CeilingCells=%d\n"),
        NonEmptyCellCount,
        BlockingCellCount,
        CeilingCellCount);
    Result += FString::Printf (TEXT ("Objects=%d Links=%d\n"), LevelAsset->Objects.Num (), LevelAsset->Links.Num ());
    Result += FString::Printf (TEXT ("ObjectArchetypesOnRuntimeActor=%d\n"), ObjectArchetypes.Num ());
    Result += FString::Printf (TEXT ("FloorMesh=%s WallMesh=%s CeilingMesh=%s\n"),
        *GetNameSafe (FloorMesh),
        *GetNameSafe (WallMesh),
        *GetNameSafe (CeilingMesh));

    if (LevelAsset->Cells.Num () != ExpectedCellCount)
    {
        Result += TEXT ("Status=WARNING: Cells.Num does not match Width*Height. Run EnsureLevelReady from the editor actor.\n");
    }
    else
    {
        Result += TEXT ("Status=OK\n");
    }

    return Result;
}

void AGridLevelRuntimeActor::LogLevelAssetDiagnostics () const
{
    const FString Diagnostics = GetLevelAssetDiagnostics ();
    UE_LOG (LogTemp, Log, TEXT ("%s"), *Diagnostics);
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
