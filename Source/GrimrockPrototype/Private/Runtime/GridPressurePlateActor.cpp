#include "Runtime/GridPressurePlateActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"

AGridPressurePlateActor::AGridPressurePlateActor ()
{
    PrimaryActorTick.bCanEverTick = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent> (TEXT ("Root"));
    SetRootComponent (SceneRoot);

    PlateMeshComponent = CreateDefaultSubobject<UStaticMeshComponent> (TEXT ("PlateMesh"));
    PlateMeshComponent->SetupAttachment (SceneRoot);
    PlateMeshComponent->SetMobility (EComponentMobility::Movable);
    PlateMeshComponent->SetCollisionEnabled (ECollisionEnabled::NoCollision);
}

void AGridPressurePlateActor::BeginPlay ()
{
    Super::BeginPlay ();
}

void AGridPressurePlateActor::Tick (float DeltaSeconds)
{
    Super::Tick (DeltaSeconds);

    if (bIsAnimating)
    {
        UpdateAnimation (DeltaSeconds);
    }
}

void AGridPressurePlateActor::InitializePlate (
    UStaticMesh* InPlateMesh,
    UMaterialInterface* InMaterial,
    const FVector& InWorldLocation,
    int32 InCellX,
    int32 InCellY,
    bool bStartPressed)
{
    CellX = InCellX;
    CellY = InCellY;

    if (PlateMeshComponent)
    {
        PlateMeshComponent->SetStaticMesh (InPlateMesh);

        if (InMaterial)
        {
            PlateMeshComponent->SetMaterial (0, InMaterial);
        }
    }

    ReleasedLocation = InWorldLocation + FVector (0.f, 0.f, ReleasedHeightAboveFloor);
    PressedLocation = InWorldLocation + FVector (0.f, 0.f, PressedHeightAboveFloor);

    bIsPressed = bStartPressed;
    bIsAnimating = false;
    AnimElapsed = 0.f;

    SetActorLocation (bIsPressed ? PressedLocation : ReleasedLocation);
}

void AGridPressurePlateActor::SetPressed (bool bNewPressed)
{
    if (bIsPressed == bNewPressed && !bIsAnimating)
    {
        return;
    }

    bIsPressed = bNewPressed;
    AnimElapsed = 0.f;
    bIsAnimating = true;

    AnimStartLocation = GetActorLocation ();
    AnimTargetLocation = bIsPressed ? PressedLocation : ReleasedLocation;
}

bool AGridPressurePlateActor::MatchesCell (int32 InCellX, int32 InCellY) const
{
    return CellX == InCellX && CellY == InCellY;
}

void AGridPressurePlateActor::UpdateAnimation (float DeltaSeconds)
{
    const float SafeDuration = FMath::Max (0.01f, MoveDuration);

    AnimElapsed += DeltaSeconds;
    const float Alpha = FMath::Clamp (AnimElapsed / SafeDuration, 0.f, 1.f);

    SetActorLocation (FMath::Lerp (AnimStartLocation, AnimTargetLocation, Alpha));

    if (Alpha >= 1.f)
    {
        SetActorLocation (AnimTargetLocation);
        bIsAnimating = false;
        AnimElapsed = 0.f;
    }
}