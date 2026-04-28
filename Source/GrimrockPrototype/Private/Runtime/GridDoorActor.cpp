#include "Runtime/GridDoorActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"

AGridDoorActor::AGridDoorActor ()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AGridDoorActor::Tick (float DeltaSeconds)
{
    Super::Tick (DeltaSeconds);

    if (bIsAnimating)
    {
        UpdateAnimation (DeltaSeconds);
    }
}

void AGridDoorActor::InitializeDoor (const FGridLevelObjectData& ObjectData, UStaticMesh* InDoorMesh, UMaterialInterface* InMaterial,
    const FVector& ClosedWorldLocation, const FRotator& WorldRotation, bool bStartOpen)
{
    InitializeGridObjectBase (ObjectData, InDoorMesh, InMaterial, ClosedWorldLocation, WorldRotation);

    ClosedLocation = ClosedWorldLocation;
    OpenLocation = ClosedLocation + FVector (0.f, 0.f, OpenHeight);

    bIsOpen = bStartOpen;
    bIsAnimating = false;
    MoveElapsed = 0.f;
    CurrentMoveDuration = 0.f;

    SetActorLocation (bIsOpen ? OpenLocation : ClosedLocation);
}

void AGridDoorActor::SetDoorOpenState (bool bOpen)
{
    const FVector DesiredTarget = bOpen ? OpenLocation : ClosedLocation;

    if (!bIsAnimating && bIsOpen == bOpen)
    {
        return;
    }

    const FVector CurrentLocation = GetActorLocation ();

    if (CurrentLocation.Equals (DesiredTarget, 0.1f))
    {
        bIsOpen = bOpen;
        bIsAnimating = false;
        MoveElapsed = 0.f;
        CurrentMoveDuration = 0.f;
        SetActorLocation (DesiredTarget);
        return;
    }

    const float FullTravelDistance = FVector::Dist (ClosedLocation, OpenLocation);
    const float RemainingDistance = FVector::Dist (CurrentLocation, DesiredTarget);

    if (FullTravelDistance <= KINDA_SMALL_NUMBER || RemainingDistance <= KINDA_SMALL_NUMBER)
    {
        bIsOpen = bOpen;
        bIsAnimating = false;
        MoveElapsed = 0.f;
        CurrentMoveDuration = 0.f;
        SetActorLocation (DesiredTarget);
        return;
    }

    MoveStartLocation = CurrentLocation;
    MoveTargetLocation = DesiredTarget;
    MoveElapsed = 0.f;

    const float TravelRatio = FMath::Clamp (RemainingDistance / FullTravelDistance, 0.f, 1.f);
    CurrentMoveDuration = FMath::Max (0.01f, MoveDuration * TravelRatio);

    bIsAnimating = true;
    bIsOpen = bOpen;
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
    const float SafeDuration = FMath::Max (0.01f, CurrentMoveDuration);

    MoveElapsed += DeltaSeconds;
    const float Alpha = FMath::Clamp (MoveElapsed / SafeDuration, 0.f, 1.f);

    const FVector NewLocation = FMath::Lerp (MoveStartLocation, MoveTargetLocation, Alpha);
    SetActorLocation (NewLocation);

    if (Alpha >= 1.f)
    {
        SetActorLocation (MoveTargetLocation);
        bIsAnimating = false;
        MoveElapsed = 0.f;
        CurrentMoveDuration = 0.f;

        OnDoorAnimationFinished.Broadcast (CellX, CellY, Edge);
    }
}