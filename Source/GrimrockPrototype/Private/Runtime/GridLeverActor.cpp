#include "Runtime/GridLeverActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GrimrockPartyPawn.h"

AGridLeverActor::AGridLeverActor ()
{
    PrimaryActorTick.bCanEverTick = true;
    SetActorTickEnabled (false);

    if (FixedMeshComponent)
    {
        FixedMeshComponent->SetCollisionEnabled (ECollisionEnabled::NoCollision);
        FixedMeshComponent->SetGenerateOverlapEvents (false);
    }

    if (MovingMeshComponent)
    {
        MovingMeshComponent->SetCollisionEnabled (ECollisionEnabled::QueryOnly);
        MovingMeshComponent->SetCollisionResponseToAllChannels (ECR_Ignore);
        MovingMeshComponent->SetCollisionResponseToChannel (ECC_Visibility, ECR_Block);
        MovingMeshComponent->SetGenerateOverlapEvents (false);
    }
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
    AGridRuntimeObjectActor::InitializeGridObject (ObjectData, nullptr, nullptr, FTransform (InWorldRotation, InWorldLocation));

    OffRelativeRotation = FRotator (LeverOffPitch, 0.f, 0.f);
    OnRelativeRotation = FRotator (LeverOnPitch, 0.f, 0.f);

    bIsOn = bStartOn;
    bIsAnimating = false;
    AnimElapsed = 0.f;

    MovingMeshComponent->SetRelativeRotation (bIsOn ? OnRelativeRotation : OffRelativeRotation);
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

    AnimStartRotation = MovingMeshComponent->GetRelativeRotation ();
    AnimTargetRotation = bIsOn ? OnRelativeRotation : OffRelativeRotation;
    SetActorTickEnabled (true);
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

    MovingMeshComponent->SetRelativeRotation (FMath::Lerp (AnimStartRotation, AnimTargetRotation, Alpha));

    if (Alpha >= 1.f)
    {
        MovingMeshComponent->SetRelativeRotation (AnimTargetRotation);
        bIsAnimating = false;
        AnimElapsed = 0.f;
        SetActorTickEnabled (false);
    }
}

void AGridLeverActor::InitializeGridObject (const FGridLevelObjectData& ObjectData, UStaticMesh* Mesh, UMaterialInterface* Material,
    const FTransform& WorldTransform)
{
    InitializeLever (ObjectData, Mesh, Material, WorldTransform.GetLocation (), WorldTransform.Rotator (), ObjectData.bInitiallyActive);
}

bool AGridLeverActor::CanInteract_Implementation (APawn* InstigatorPawn, UPrimitiveComponent* HitComponent) const
{
    if (!InstigatorPawn || !HitComponent)
    {
        return false;
    }

    if (HitComponent != MovingMeshComponent)
    {
        return false;
    }

    return !bIsAnimating;
}

void AGridLeverActor::Interact_Implementation (APawn* InstigatorPawn, UPrimitiveComponent* HitComponent)
{
    if (!CanInteract_Implementation (InstigatorPawn, HitComponent))
    {
        return;
    }

    AGrimrockPartyPawn* PartyPawn = Cast<AGrimrockPartyPawn> (InstigatorPawn);
    if (!PartyPawn)
    {
        return;
    }

    AGridLevelRuntimeActor* RuntimeActor = PartyPawn->LevelRuntimeActor;
    if (!RuntimeActor)
    {
        UWorld* World = GetWorld ();
        if (World)
        {
            for (TActorIterator<AGridLevelRuntimeActor> It (World); It; ++It)
            {
                RuntimeActor = *It;
                break;
            }
        }
    }

    if (RuntimeActor)
    {
        RuntimeActor->TryInteractAtEdge (CellX, CellY, Edge, PartyPawn);
    }
}

EGridInteractionCursor AGridLeverActor::GetInteractionCursor_Implementation (UPrimitiveComponent* HitComponent) const
{
    if (HitComponent == MovingMeshComponent && !bIsAnimating)
    {
        return EGridInteractionCursor::Pull;
    }

    return EGridInteractionCursor::Default;
}

FText AGridLeverActor::GetInteractionText_Implementation (UPrimitiveComponent* HitComponent) const
{
    if (HitComponent == MovingMeshComponent)
    {
        return FText::FromString (bIsOn ? TEXT ("Push lever") : TEXT ("Pull lever"));
    }

    return FText::GetEmpty ();
}
