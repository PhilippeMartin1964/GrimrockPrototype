#include "Runtime/GridLeverActor.h"

#include "Components/SceneComponent.h"

AGridLeverActor::AGridLeverActor ()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AGridLeverActor::Tick (float DeltaSeconds)
{
    Super::Tick (DeltaSeconds);

    if (bIsAnimating)
    {
        UpdateAnimation (DeltaSeconds);
    }
}

void AGridLeverActor::InitializeLever (const FGridLevelObjectData& ObjectData, UStaticMesh* InLeverMesh, UMaterialInterface* InMaterial,
    const FVector& InWorldLocation, const FRotator& InWorldRotation, bool bStartOn)
{
    InitializeGridObjectBase (ObjectData, InLeverMesh, InMaterial, InWorldLocation, InWorldRotation);

    OffRelativeRotation = FRotator (LeverOffPitch, 0.f, 0.f);
    OnRelativeRotation = FRotator (LeverOnPitch, 0.f, 0.f);

    bIsOn = bStartOn;
    bIsAnimating = false;
    AnimElapsed = 0.f;

    MeshComponent->SetRelativeRotation (bIsOn ? OnRelativeRotation : OffRelativeRotation);
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

    AnimStartRotation = MeshComponent->GetRelativeRotation ();
    AnimTargetRotation = bIsOn ? OnRelativeRotation : OffRelativeRotation;
}

void AGridLeverActor::ToggleLever ()
{
    SetLeverState (!bIsOn);
}

void AGridLeverActor::UpdateAnimation (float DeltaSeconds)
{
    const float SafeDuration = FMath::Max (0.01f, ToggleDuration);

    AnimElapsed += DeltaSeconds;
    const float Alpha = FMath::Clamp (AnimElapsed / SafeDuration, 0.f, 1.f);

    MeshComponent->SetRelativeRotation (
        FMath::Lerp (AnimStartRotation, AnimTargetRotation, Alpha));

    if (Alpha >= 1.f)
    {
        MeshComponent->SetRelativeRotation (AnimTargetRotation);
        bIsAnimating = false;
        AnimElapsed = 0.f;
    }
}