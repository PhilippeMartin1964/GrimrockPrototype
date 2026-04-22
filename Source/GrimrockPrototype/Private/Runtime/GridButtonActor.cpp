#include "Runtime/GridButtonActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"

AGridButtonActor::AGridButtonActor ()
{
    PrimaryActorTick.bCanEverTick = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent> (TEXT ("Root"));
    SetRootComponent (SceneRoot);

    ButtonMeshComponent = CreateDefaultSubobject<UStaticMeshComponent> (TEXT ("ButtonMesh"));
    ButtonMeshComponent->SetupAttachment (SceneRoot);
    ButtonMeshComponent->SetMobility (EComponentMobility::Movable);
    ButtonMeshComponent->SetCollisionEnabled (ECollisionEnabled::NoCollision);
}

void AGridButtonActor::BeginPlay ()
{
    Super::BeginPlay ();
}

void AGridButtonActor::Tick (float DeltaSeconds)
{
    Super::Tick (DeltaSeconds);
    UpdateAnimation (DeltaSeconds);
}

void AGridButtonActor::InitializeButton (
    UStaticMesh* InButtonMesh,
    UMaterialInterface* InMaterial,
    const FVector& InWorldLocation,
    const FRotator& InWorldRotation,
    int32 InCellX,
    int32 InCellY,
    EGridEdge InEdge)
{
    CellX = InCellX;
    CellY = InCellY;
    Edge = InEdge;

    if (ButtonMeshComponent)
    {
        ButtonMeshComponent->SetStaticMesh (InButtonMesh);

        if (InMaterial)
        {
            ButtonMeshComponent->SetMaterial (0, InMaterial);
        }
    }

    SetActorRotation (InWorldRotation);

    ReleasedLocation = InWorldLocation;
    PressedLocation = ReleasedLocation + (GetPressAxis () * PressDistance);

    SetActorLocation (ReleasedLocation);

    AnimState = EButtonAnimState::Idle;
    StateElapsed = 0.f;
}

void AGridButtonActor::TriggerPress ()
{
    AnimState = EButtonAnimState::Pressing;
    StateElapsed = 0.f;
}

bool AGridButtonActor::MatchesEdge (int32 InCellX, int32 InCellY, EGridEdge InEdge) const
{
    return CellX == InCellX && CellY == InCellY && Edge == InEdge;
}

FVector AGridButtonActor::GetPressAxis () const
{
    switch (Edge)
    {
        case EGridEdge::North: return FVector (0.f, 1.f, 0.f);
        case EGridEdge::East:  return FVector (1.f, 0.f, 0.f);
        case EGridEdge::South: return FVector (0.f, -1.f, 0.f);
        case EGridEdge::West:  return FVector (-1.f, 0.f, 0.f);
        default:               return FVector::ZeroVector;
    }
}

void AGridButtonActor::UpdateAnimation (float DeltaSeconds)
{
    switch (AnimState)
    {
        case EButtonAnimState::Idle:
            return;

        case EButtonAnimState::Pressing:
        {
            StateElapsed += DeltaSeconds;
            const float Alpha = FMath::Clamp (StateElapsed / FMath::Max (0.01f, PressDuration), 0.f, 1.f);
            SetActorLocation (FMath::Lerp (ReleasedLocation, PressedLocation, Alpha));

            if (Alpha >= 1.f)
            {
                AnimState = EButtonAnimState::Holding;
                StateElapsed = 0.f;
                SetActorLocation (PressedLocation);
            }
            break;
        }

        case EButtonAnimState::Holding:
        {
            StateElapsed += DeltaSeconds;

            if (StateElapsed >= HoldTime)
            {
                AnimState = EButtonAnimState::Releasing;
                StateElapsed = 0.f;
            }
            break;
        }

        case EButtonAnimState::Releasing:
        {
            StateElapsed += DeltaSeconds;
            const float Alpha = FMath::Clamp (StateElapsed / FMath::Max (0.01f, ReleaseDuration), 0.f, 1.f);
            SetActorLocation (FMath::Lerp (PressedLocation, ReleasedLocation, Alpha));

            if (Alpha >= 1.f)
            {
                AnimState = EButtonAnimState::Idle;
                StateElapsed = 0.f;
                SetActorLocation (ReleasedLocation);
            }
            break;
        }

        default:
            break;
    }
}