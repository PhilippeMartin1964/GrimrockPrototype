#include "Runtime/GridButtonActor.h"

#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GrimrockPartyPawn.h"

AGridButtonActor::AGridButtonActor ()
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

void AGridButtonActor::Tick (float DeltaSeconds)
{
    Super::Tick (DeltaSeconds);
    UpdateAnimation (DeltaSeconds);
}

void AGridButtonActor::InitializeButton (const FGridLevelObjectData& ObjectData, UStaticMesh* InButtonMesh, UMaterialInterface* InMaterial,
    const FVector& InWorldLocation, const FRotator& InWorldRotation)
{
    AGridRuntimeObjectActor::InitializeGridObject (ObjectData, nullptr, nullptr, FTransform (InWorldRotation, InWorldLocation));
    PressDistance = ObjectData.Behavior.ButtonAnimation.ButtonPressDistance;
    PressDuration = ObjectData.Behavior.ButtonAnimation.ButtonPressDuration;
    ReleaseDuration = ObjectData.Behavior.ButtonAnimation.ButtonReleaseDuration;
    HoldTime = ObjectData.Behavior.ButtonAnimation.ButtonHoldTime;
    
    ReleasedLocation = FVector::ZeroVector;
    PressedLocation = FVector (PressDistance, 0.f, 0.f);

	SetMovingRelativeLocation (ReleasedLocation);

    AnimState = EButtonAnimState::Idle;
    StateElapsed = 0.f;
    SetActorTickEnabled (false);
}

void AGridButtonActor::TriggerPress ()
{
    AnimState = EButtonAnimState::Pressing;
    StateElapsed = 0.f;
    SetActorTickEnabled (true);
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
            SetMovingRelativeLocation (FMath::Lerp (ReleasedLocation, PressedLocation, Alpha));

            if (Alpha >= 1.f)
            {
                AnimState = EButtonAnimState::Holding;
                StateElapsed = 0.f;
                SetMovingRelativeLocation (PressedLocation);
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
            SetMovingRelativeLocation (FMath::Lerp (PressedLocation, ReleasedLocation, Alpha));

            if (Alpha >= 1.f)
            {
                AnimState = EButtonAnimState::Idle;
                StateElapsed = 0.f;
                SetMovingRelativeLocation (ReleasedLocation);
				SetActorTickEnabled (false);
            }
            break;
        }

        default:
            break;
    }
}

void AGridButtonActor::InitializeGridObject (const FGridLevelObjectData& ObjectData, UStaticMesh* Mesh, UMaterialInterface* Material,
    const FTransform& WorldTransform)
{
    InitializeButton (ObjectData, Mesh, Material, WorldTransform.GetLocation (), WorldTransform.GetRotation ().Rotator ());
}

bool AGridButtonActor::CanInteract_Implementation (APawn* InstigatorPawn, UPrimitiveComponent* HitComponent) const
{
    if (!InstigatorPawn || !HitComponent)
    {
        return false;
    }

    if (HitComponent != MovingMeshComponent)
    {
        return false;
    }

    return AnimState == EButtonAnimState::Idle;
}

void AGridButtonActor::Interact_Implementation (APawn* InstigatorPawn, UPrimitiveComponent* HitComponent)
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

EGridInteractionCursor AGridButtonActor::GetInteractionCursor_Implementation (UPrimitiveComponent* HitComponent) const
{
    if (HitComponent == MovingMeshComponent && AnimState == EButtonAnimState::Idle)
    {
        return EGridInteractionCursor::Push;
    }

    return EGridInteractionCursor::Default;
}

FText AGridButtonActor::GetInteractionText_Implementation (UPrimitiveComponent* HitComponent) const
{
    if (HitComponent == MovingMeshComponent)
    {
        return FText::FromString (TEXT ("Push button"));
    }

    return FText::GetEmpty ();
}
