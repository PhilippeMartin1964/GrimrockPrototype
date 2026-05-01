#include "Runtime/GridPressurePlateActor.h"

AGridPressurePlateActor::AGridPressurePlateActor ()
{
    PrimaryActorTick.bCanEverTick = true;
    SetActorTickEnabled (false);
}

void AGridPressurePlateActor::Tick (float DeltaSeconds)
{
    Super::Tick (DeltaSeconds);

    if (bIsAnimating)
    {
        UpdateAnimation (DeltaSeconds);
    }
}

void AGridPressurePlateActor::InitializePlate (const FGridLevelObjectData& ObjectData, UStaticMesh* InPlateMesh, UMaterialInterface* InMaterial,
    const FVector& InWorldLocation, bool bStartPressed)
{
    InitializeGridObjectBase (ObjectData, InPlateMesh, InMaterial, InWorldLocation, FRotator::ZeroRotator);

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
    SetActorTickEnabled (true);
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
		SetActorTickEnabled (false);
    }
}

void AGridPressurePlateActor::InitializeGridObject (const FGridLevelObjectData& ObjectData, UStaticMesh* Mesh, UMaterialInterface* Material,
    const FTransform& WorldTransform)
{
	InitializePlate (ObjectData, Mesh, Material, WorldTransform.GetLocation (), ObjectData.bInitiallyActive);
}