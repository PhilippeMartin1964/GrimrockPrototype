#include "Runtime/GridButtonActor.h"

AGridButtonActor::AGridButtonActor ()
{
    PrimaryActorTick.bCanEverTick = true;
    SetActorTickEnabled (false);
}

void AGridButtonActor::Tick (float DeltaSeconds)
{
    Super::Tick (DeltaSeconds);
    UpdateAnimation (DeltaSeconds);
}

void AGridButtonActor::InitializeButton (const FGridLevelObjectData& ObjectData, UStaticMesh* InButtonMesh, UMaterialInterface* InMaterial,
    const FVector& InWorldLocation, const FRotator& InWorldRotation)
{
	InitializeGridObjectBase (ObjectData, InButtonMesh, InMaterial, InWorldLocation, InWorldRotation);
    PressDistance = ObjectData.Behavior.ButtonPressDistance;
    PressDuration = ObjectData.Behavior.ButtonPressDuration;
    ReleaseDuration = ObjectData.Behavior.ButtonReleaseDuration;
    HoldTime = ObjectData.Behavior.ButtonHoldTime;
    ReleasedLocation = InWorldLocation;
    PressedLocation = ReleasedLocation + (GetPressAxis () * PressDistance);

    AnimState = EButtonAnimState::Idle;
    StateElapsed = 0.f;
    UE_LOG (
        LogTemp,
        Warning,
        TEXT ("Button %s | Archetype=%s | PressDistance=%f"),
        *ObjectData.ObjectId.ToString (),
        *ObjectData.ArchetypeId.ToString (),
        ObjectData.Behavior.ButtonPressDistance
    );
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