#include "Runtime/GridLeverActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"

AGridLeverActor::AGridLeverActor ()
{
    PrimaryActorTick.bCanEverTick = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent> (TEXT ("Root"));
    SetRootComponent (SceneRoot);

    LeverMeshComponent = CreateDefaultSubobject<UStaticMeshComponent> (TEXT ("LeverMesh"));
    LeverMeshComponent->SetupAttachment (SceneRoot);
    LeverMeshComponent->SetMobility (EComponentMobility::Movable);
    LeverMeshComponent->SetCollisionEnabled (ECollisionEnabled::NoCollision);
}

void AGridLeverActor::BeginPlay ()
{
    Super::BeginPlay ();
}

void AGridLeverActor::Tick (float DeltaSeconds)
{
    Super::Tick (DeltaSeconds);

    if (bIsAnimating)
    {
        UpdateAnimation (DeltaSeconds);
    }
}

void AGridLeverActor::InitializeLever (
    UStaticMesh* InLeverMesh,
    UMaterialInterface* InMaterial,
    const FVector& InWorldLocation,
    const FRotator& InWorldRotation,
    int32 InCellX,
    int32 InCellY,
    EGridEdge InEdge,
    bool bStartOn)
{
    CellX = InCellX;
    CellY = InCellY;
    Edge = InEdge;

    if (LeverMeshComponent)
    {
        LeverMeshComponent->SetStaticMesh (InLeverMesh);

        if (InMaterial)
        {
            LeverMeshComponent->SetMaterial (0, InMaterial);
        }
    }

    SetActorLocation (InWorldLocation);
    SetActorRotation (InWorldRotation);

    OffRelativeRotation = FRotator (LeverOffPitch, 0.f, 0.f);
    OnRelativeRotation = FRotator (LeverOnPitch, 0.f, 0.f);

    bIsOn = bStartOn;
    bIsAnimating = false;
    AnimElapsed = 0.f;

    LeverMeshComponent->SetRelativeRotation (bIsOn ? OnRelativeRotation : OffRelativeRotation);
}

void AGridLeverActor::SetLeverState (bool bNewOn)
{
    if (bIsOn == bNewOn && !bIsAnimating)
    {
        return;
    }

    bIsOn = bNewOn;
    AnimElapsed = 0.f;
    bIsAnimating = true;

    AnimStartRotation = LeverMeshComponent->GetRelativeRotation ();
    AnimTargetRotation = bIsOn ? OnRelativeRotation : OffRelativeRotation;
}

void AGridLeverActor::ToggleLever ()
{
    SetLeverState (!bIsOn);
}

bool AGridLeverActor::MatchesEdge (int32 InCellX, int32 InCellY, EGridEdge InEdge) const
{
    return CellX == InCellX && CellY == InCellY && Edge == InEdge;
}

void AGridLeverActor::UpdateAnimation (float DeltaSeconds)
{
    const float SafeDuration = FMath::Max (0.01f, ToggleDuration);

    AnimElapsed += DeltaSeconds;
    const float Alpha = FMath::Clamp (AnimElapsed / SafeDuration, 0.f, 1.f);

    LeverMeshComponent->SetRelativeRotation (
        FMath::Lerp (AnimStartRotation, AnimTargetRotation, Alpha));

    if (Alpha >= 1.f)
    {
        LeverMeshComponent->SetRelativeRotation (AnimTargetRotation);
        bIsAnimating = false;
        AnimElapsed = 0.f;
    }
}