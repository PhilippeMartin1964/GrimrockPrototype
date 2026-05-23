#include "Runtime/GrimrockPlayerController.h"

#include "Components/InputComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/EngineTypes.h"
#include "InputCoreTypes.h"

AGrimrockPlayerController::AGrimrockPlayerController ()
{
    bShowMouseCursor = true;
    bEnableClickEvents = true;
    bEnableMouseOverEvents = true;
    DefaultMouseCursor = EMouseCursor::Default;
    CurrentMouseCursor = EMouseCursor::Default;
}

void AGrimrockPlayerController::BeginPlay ()
{
    Super::BeginPlay ();

    bShowMouseCursor = true;
    bEnableClickEvents = true;
    bEnableMouseOverEvents = true;

    FInputModeGameAndUI InputMode;
    InputMode.SetHideCursorDuringCapture (false);
    SetInputMode (InputMode);
}

void AGrimrockPlayerController::PlayerTick (float DeltaTime)
{
    Super::PlayerTick (DeltaTime);

    UpdateHoveredInteractable ();
}

void AGrimrockPlayerController::SetupInputComponent ()
{
    Super::SetupInputComponent ();

    if (!InputComponent)
    {
        return;
    }

    InputComponent->BindKey (EKeys::LeftMouseButton, IE_Pressed, this, &AGrimrockPlayerController::HandleLeftMousePressed);
}

void AGrimrockPlayerController::HandleLeftMousePressed ()
{
    FHitResult HitResult;
    AActor* InteractableActor = nullptr;
    if (!TryGetInteractableUnderCursor (HitResult, InteractableActor) || !IsHitWithinInteractionDistance (HitResult))
    {
        return;
    }

    APawn* ControlledPawn = GetPawn ();
    UPrimitiveComponent* HitComponent = HitResult.GetComponent ();
    if (IGridInteractableInterface::Execute_CanInteract (InteractableActor, ControlledPawn, HitComponent))
    {
        IGridInteractableInterface::Execute_Interact (InteractableActor, ControlledPawn, HitComponent);
    }
}

void AGrimrockPlayerController::UpdateHoveredInteractable ()
{
    FHitResult HitResult;
    AActor* InteractableActor = nullptr;
    if (!TryGetInteractableUnderCursor (HitResult, InteractableActor))
    {
        CurrentMouseCursor = EMouseCursor::Default;
        return;
    }

    const EGridInteractionCursor InteractionCursor =
        IGridInteractableInterface::Execute_GetInteractionCursor (InteractableActor, HitResult.GetComponent ());
    CurrentMouseCursor = ToMouseCursor (InteractionCursor);
}

bool AGrimrockPlayerController::TryGetInteractableUnderCursor (FHitResult& OutHitResult, AActor*& OutInteractableActor) const
{
    OutInteractableActor = nullptr;
    if (!GetHitResultUnderCursorByChannel (UEngineTypes::ConvertToTraceType (ECC_Visibility), false, OutHitResult))
    {
        return false;
    }

    AActor* HitActor = OutHitResult.GetActor ();
    if (!HitActor || !HitActor->GetClass ()->ImplementsInterface (UGridInteractableInterface::StaticClass ()))
    {
        return false;
    }

    OutInteractableActor = HitActor;
    return true;
}

bool AGrimrockPlayerController::IsHitWithinInteractionDistance (const FHitResult& HitResult) const
{
    const APawn* ControlledPawn = GetPawn ();
    if (!ControlledPawn)
    {
        return false;
    }

    return FVector::DistSquared (ControlledPawn->GetActorLocation (), HitResult.ImpactPoint) <=
        FMath::Square (MaxInteractionDistance);
}

EMouseCursor::Type AGrimrockPlayerController::ToMouseCursor (EGridInteractionCursor InteractionCursor) const
{
    switch (InteractionCursor)
    {
        case EGridInteractionCursor::Use:
        case EGridInteractionCursor::Push:
        case EGridInteractionCursor::Pull:
        case EGridInteractionCursor::Take:
        case EGridInteractionCursor::Read:
            return EMouseCursor::Hand;

        case EGridInteractionCursor::Locked:
        case EGridInteractionCursor::Forbidden:
            return EMouseCursor::SlashedCircle;

        case EGridInteractionCursor::None:
        case EGridInteractionCursor::Default:
        default:
            return EMouseCursor::Default;
    }
}
