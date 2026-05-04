#include "Runtime/GridDoorActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"

AGridDoorActor::AGridDoorActor ()
{
    PrimaryActorTick.bCanEverTick = true;
    SetActorTickEnabled (false);
}

void AGridDoorActor::Tick (float DeltaSeconds)
{
    Super::Tick (DeltaSeconds);

    if (bIsAnimating)
    {
        UpdateAnimation (DeltaSeconds);
    }
}

void AGridDoorActor::InitializeDoor (const FGridLevelObjectData& ObjectData, UStaticMesh* InMovingMesh, UMaterialInterface* InMovingMaterial,
    UStaticMesh* InFixedMesh, UMaterialInterface* InFixedMaterial, const FVector& ClosedWorldLocation, const FRotator& WorldRotation,
    bool bStartOpen)
{
    ObjectId = ObjectData.ObjectId;
    CellX = ObjectData.CellX;
    CellY = ObjectData.CellY;
    Edge = ObjectData.Edge;

    SetActorLocation (ClosedWorldLocation);
    SetActorRotation (WorldRotation);

    SetFixedMesh (InFixedMesh, InFixedMaterial);
    SetMovingMesh (InMovingMesh, InMovingMaterial);

    MovingClosedRelativeLocation = FVector::ZeroVector;
    MovingOpenRelativeLocation = FVector (0.f, 0.f, OpenHeight);

    MoveStartRelativeLocation = FVector::ZeroVector;
    MoveTargetRelativeLocation = FVector::ZeroVector;
    MoveElapsed = 0.f;
    CurrentMoveDuration = 0.f;

    bIsOpen = bStartOpen;
    bIsAnimating = false;

    SetMovingRelativeLocation (bIsOpen ? MovingOpenRelativeLocation : MovingClosedRelativeLocation);

    SetActorTickEnabled (false);
}

void AGridDoorActor::SetDoorOpenState (bool bOpen)
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
        return;
    }

    MoveStartRelativeLocation = CurrentLocation;
    MoveTargetRelativeLocation = DesiredTarget;
    MoveElapsed = 0.f;

    const float TravelRatio = FMath::Clamp (RemainingDistance / FullTravelDistance, 0.f, 1.f);
    CurrentMoveDuration = FMath::Max (0.01f, MoveDuration * TravelRatio);

    bIsAnimating = true;
    SetActorTickEnabled (true);
}

void AGridDoorActor::OpenDoor ()
{
    SetDoorOpenState (true);
}

void AGridDoorActor::CloseDoor ()
{
    SetDoorOpenState (false);
}

void AGridDoorActor::UpdateAnimation (float DeltaSeconds)
{
    if (!MovingMeshComponent)
    {
        return;
    }

    const float SafeDuration = FMath::Max (0.01f, CurrentMoveDuration);

    MoveElapsed += DeltaSeconds;
    const float Alpha = FMath::Clamp (MoveElapsed / SafeDuration, 0.f, 1.f);

    MovingMeshComponent->SetRelativeLocation (
        FMath::Lerp (MoveStartRelativeLocation, MoveTargetRelativeLocation, Alpha)
    );

    if (Alpha >= 1.f)
    {
        MovingMeshComponent->SetRelativeLocation (MoveTargetRelativeLocation);

        bIsOpen = MoveTargetRelativeLocation.Equals (MovingOpenRelativeLocation, 0.1f);
        bIsAnimating = false;
        MoveElapsed = 0.f;
        CurrentMoveDuration = 0.f;

        SetActorTickEnabled (false);
        OnDoorAnimationFinished.Broadcast (CellX, CellY, Edge);
    }
}

void AGridDoorActor::InitializeGridObject (const FGridLevelObjectData& ObjectData, UStaticMesh* Mesh, UMaterialInterface* Material,
    const FTransform& WorldTransform)
{
    InitializeDoor (ObjectData, Mesh, Material, nullptr, nullptr, WorldTransform.GetLocation (), WorldTransform.GetRotation ().Rotator (),
        ObjectData.bInitiallyActive);
}