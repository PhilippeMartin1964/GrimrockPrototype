#include "Runtime/GridSecretDoorActor.h"

#include "Components/StaticMeshComponent.h"

AGridSecretDoorActor::AGridSecretDoorActor ()
{
    PrimaryActorTick.bCanEverTick = true;
    SetActorTickEnabled (false);

    FixedMeshComponent = CreateDefaultSubobject<UStaticMeshComponent> (TEXT ("FixedMesh"));
    FixedMeshComponent->SetupAttachment (SceneRoot);
    FixedMeshComponent->SetMobility (EComponentMobility::Movable);
    FixedMeshComponent->SetCollisionEnabled (ECollisionEnabled::NoCollision);

    MovingMeshComponent = CreateDefaultSubobject<UStaticMeshComponent> (TEXT ("MovingMesh"));
    MovingMeshComponent->SetupAttachment (SceneRoot);
    MovingMeshComponent->SetMobility (EComponentMobility::Movable);
    MovingMeshComponent->SetCollisionEnabled (ECollisionEnabled::NoCollision);

    if (MeshComponent)
    {
        MeshComponent->SetHiddenInGame (true);
        MeshComponent->SetVisibility (false);
        MeshComponent->SetCollisionEnabled (ECollisionEnabled::NoCollision);
    }
}

void AGridSecretDoorActor::InitializeGridObject (const FGridLevelObjectData& ObjectData, UStaticMesh* Mesh, UMaterialInterface* Material,
    const FTransform& WorldTransform)
{
    InitializeGridObjectBase (ObjectData, nullptr, nullptr, WorldTransform.GetLocation (), WorldTransform.GetRotation ().Rotator ());
    if (FixedMeshComponent)
    {
        FixedMeshComponent->SetStaticMesh (FixedMesh);

        if (FixedMaterial)
        {
            FixedMeshComponent->SetMaterial (0, FixedMaterial);
        } else if (Material)
        {
            FixedMeshComponent->SetMaterial (0, Material);
        }
    }
    if (MovingMeshComponent)
    {
        MovingMeshComponent->SetStaticMesh (Mesh);

        if (Material)
        {
            MovingMeshComponent->SetMaterial (0, Material);
        }
    }
    MovingClosedRelativeLocation = MovingMeshComponent
        ? MovingMeshComponent->GetRelativeLocation ()
        : FVector::ZeroVector;
    MovingOpenRelativeLocation = MovingClosedRelativeLocation + FVector (0.f, 0.f, OpenHeight);
    bIsOpen = ObjectData.bInitiallyActive;
    bIsAnimating = false;
    MoveElapsed = 0.f;
    CurrentMoveDuration = 0.f;

    if (MovingMeshComponent)
    {
        MovingMeshComponent->SetRelativeLocation (
            bIsOpen ? MovingOpenRelativeLocation : MovingClosedRelativeLocation
        );
    }
}

void AGridSecretDoorActor::SetDoorOpenState (bool bOpen)
{
    if (!MovingMeshComponent)
    {
        return;
    }

    const FVector DesiredTarget =
        bOpen ? MovingOpenRelativeLocation : MovingClosedRelativeLocation;

    if (!bIsAnimating && bIsOpen == bOpen)
    {
        return;
    }

    const FVector CurrentLocation = MovingMeshComponent->GetRelativeLocation ();

    if (CurrentLocation.Equals (DesiredTarget, 0.1f))
    {
        bIsOpen = bOpen;
        bIsAnimating = false;
        MoveElapsed = 0.f;
        CurrentMoveDuration = 0.f;
        MovingMeshComponent->SetRelativeLocation (DesiredTarget);
        SetActorTickEnabled (false);
        OnDoorAnimationFinished.Broadcast (CellX, CellY, Edge);
        return;
    }

    const float FullTravelDistance =
        FVector::Dist (MovingClosedRelativeLocation, MovingOpenRelativeLocation);

    const float RemainingDistance =
        FVector::Dist (CurrentLocation, DesiredTarget);

    if (FullTravelDistance <= KINDA_SMALL_NUMBER || RemainingDistance <= KINDA_SMALL_NUMBER)
    {
        bIsOpen = bOpen;
        bIsAnimating = false;
        MoveElapsed = 0.f;
        CurrentMoveDuration = 0.f;
        MovingMeshComponent->SetRelativeLocation (DesiredTarget);
        SetActorTickEnabled (false);
        OnDoorAnimationFinished.Broadcast (CellX, CellY, Edge);
        return;
    }

    MoveStartRelativeLocation = CurrentLocation;
    MoveTargetRelativeLocation = DesiredTarget;
    MoveElapsed = 0.f;

    const float TravelRatio =
        FMath::Clamp (RemainingDistance / FullTravelDistance, 0.f, 1.f);

    CurrentMoveDuration = FMath::Max (0.01f, MoveDuration * TravelRatio);

    bIsOpen = bOpen;
    bIsAnimating = true;

    SetActorTickEnabled (true);
}

void AGridSecretDoorActor::UpdateAnimation (float DeltaSeconds)
{
    if (!MovingMeshComponent)
    {
        return;
    }
    const float SafeDuration = FMath::Max (0.01f, CurrentMoveDuration);
    MoveElapsed += DeltaSeconds;
    const float Alpha = FMath::Clamp (MoveElapsed / SafeDuration, 0.f, 1.f);
    MovingMeshComponent->SetRelativeLocation (FMath::Lerp (MoveStartRelativeLocation, MoveTargetRelativeLocation, Alpha));
    if (Alpha >= 1.f)
    {
        MovingMeshComponent->SetRelativeLocation (MoveTargetRelativeLocation);

        bIsAnimating = false;
        MoveElapsed = 0.f;
        CurrentMoveDuration = 0.f;
        SetActorTickEnabled (false);
        OnDoorAnimationFinished.Broadcast (CellX, CellY, Edge);
    }
}

void AGridSecretDoorActor::OnConstruction (const FTransform& Transform)
{
    Super::OnConstruction (Transform);

    if (FixedMeshComponent && FixedMesh)
    {
        FixedMeshComponent->SetStaticMesh (FixedMesh);
    }

    if (FixedMeshComponent && FixedMaterial)
    {
        FixedMeshComponent->SetMaterial (0, FixedMaterial);
    }
}