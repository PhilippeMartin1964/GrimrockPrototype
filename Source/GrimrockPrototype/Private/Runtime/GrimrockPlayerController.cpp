#include "Runtime/GrimrockPlayerController.h"

#include "Blueprint/UserWidget.h"
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

    InitializeCustomCursor ();
    bShowMouseCursor = !(bUseCustomMouseCursor && CustomCursorWidgetClass);
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

void AGrimrockPlayerController::SetInventoryUiOpen (bool bOpen)
{
    bInventoryUiOpen = bOpen;

    if (bInventoryUiOpen)
    {
        SetGridInteractionCursor (EGridInteractionCursor::Default);
        if (CustomCursorWidget)
        {
            CustomCursorWidget->SetVisibility (ESlateVisibility::Collapsed);
        }
    }
    else if (bUseCustomMouseCursor && CustomCursorWidget)
    {
        CustomCursorWidget->SetVisibility (ESlateVisibility::HitTestInvisible);
    }

    UE_LOG (LogTemp, Log, TEXT ("GridInventory UI State Open=%s"), bInventoryUiOpen ? TEXT ("true") : TEXT ("false"));
}

void AGrimrockPlayerController::HandleLeftMousePressed ()
{
    if (bInventoryUiOpen)
    {
        if (bDebugMouseInteraction)
        {
            UE_LOG (LogTemp, Verbose, TEXT ("Mouse interaction ignored: inventory UI open."));
        }
        return;
    }

    FHitResult HitResult;
    AActor* InteractableActor = nullptr;
    if (!TryGetInteractableUnderCursor (HitResult, InteractableActor))
    {
        if (bDebugMouseInteraction)
        {
            UE_LOG (LogTemp, Verbose, TEXT ("Mouse interaction: no interactable under cursor."));
        }
        return;
    }

    if (!IsHitWithinInteractionDistance (HitResult))
    {
        if (bDebugMouseInteraction)
        {
            UE_LOG (LogTemp, Verbose, TEXT ("Mouse interaction: %s is outside interaction distance."),
                *GetNameSafe (InteractableActor));
        }
        return;
    }

    APawn* ControlledPawn = GetPawn ();
    UPrimitiveComponent* HitComponent = HitResult.GetComponent ();
    if (!ControlledPawn || !HitComponent)
    {
        if (bDebugMouseInteraction)
        {
            UE_LOG (LogTemp, Verbose, TEXT ("Mouse interaction: invalid pawn or hit component for %s."),
                *GetNameSafe (InteractableActor));
        }
        return;
    }

    if (!IGridInteractableInterface::Execute_CanInteract (InteractableActor, ControlledPawn, HitComponent))
    {
        if (bDebugMouseInteraction)
        {
            UE_LOG (LogTemp, Verbose, TEXT ("Mouse interaction: CanInteract rejected %s on component %s."),
                *GetNameSafe (InteractableActor),
                *GetNameSafe (HitComponent));
        }
        return;
    }

    if (bDebugMouseInteraction)
    {
        UE_LOG (LogTemp, Log, TEXT ("Mouse interaction: Interact %s on component %s."),
            *GetNameSafe (InteractableActor),
            *GetNameSafe (HitComponent));
    }

    IGridInteractableInterface::Execute_InteractWithHit (InteractableActor, ControlledPawn, HitComponent, HitResult);
}

void AGrimrockPlayerController::UpdateHoveredInteractable ()
{
    if (bInventoryUiOpen)
    {
        SetGridInteractionCursor (EGridInteractionCursor::Default);
        return;
    }

    FHitResult HitResult;
    AActor* InteractableActor = nullptr;
    if (!TryGetInteractableUnderCursor (HitResult, InteractableActor))
    {
        SetGridInteractionCursor (EGridInteractionCursor::Default);
        return;
    }

    if (!IsHitWithinInteractionDistance (HitResult))
    {
        SetGridInteractionCursor (EGridInteractionCursor::Forbidden);
        return;
    }

    APawn* ControlledPawn = GetPawn ();
    UPrimitiveComponent* HitComponent = HitResult.GetComponent ();
    if (!ControlledPawn || !HitComponent)
    {
        SetGridInteractionCursor (EGridInteractionCursor::Default);
        return;
    }

    if (!IGridInteractableInterface::Execute_CanInteract (InteractableActor, ControlledPawn, HitComponent))
    {
        SetGridInteractionCursor (EGridInteractionCursor::Default);
        return;
    }

    const EGridInteractionCursor InteractionCursor =
        IGridInteractableInterface::Execute_GetInteractionCursor (InteractableActor, HitComponent);
    SetGridInteractionCursor (InteractionCursor);
}

void AGrimrockPlayerController::InitializeCustomCursor ()
{
    if (!bUseCustomMouseCursor || !CustomCursorWidgetClass)
    {
        return;
    }

    if (CustomCursorWidget)
    {
        return;
    }

    CustomCursorWidget = CreateWidget<UUserWidget> (this, CustomCursorWidgetClass);
    if (CustomCursorWidget)
    {
        CustomCursorWidget->SetVisibility (ESlateVisibility::HitTestInvisible);
        CustomCursorWidget->SetIsEnabled (false);
        CustomCursorWidget->AddToViewport (9999);
    }
}

void AGrimrockPlayerController::SetGridInteractionCursor (EGridInteractionCursor NewCursor)
{
    CurrentGridInteractionCursor = NewCursor;

    if (!bUseCustomMouseCursor)
    {
        CurrentMouseCursor = ToMouseCursor (NewCursor);
        return;
    }

    CurrentMouseCursor = EMouseCursor::None;

    if (!CustomCursorWidget)
    {
        return;
    }

    static const FName SetCursorStateFunctionName = TEXT ("SetCursorState");
    UFunction* SetCursorStateFunction = CustomCursorWidget->FindFunction (SetCursorStateFunctionName);
    if (!SetCursorStateFunction)
    {
        return;
    }

    struct FSetCursorStateParams
    {
        EGridInteractionCursor Cursor;
    };

    FSetCursorStateParams Params;
    Params.Cursor = NewCursor;
    CustomCursorWidget->ProcessEvent (SetCursorStateFunction, &Params);
}

bool AGrimrockPlayerController::TryGetInteractableUnderCursor (FHitResult& OutHitResult, AActor*& OutInteractableActor) const
{
    OutInteractableActor = nullptr;
    OutHitResult = FHitResult ();
    FVector WorldOrigin = FVector::ZeroVector;
    FVector WorldDirection = FVector::ZeroVector;
    if (!DeprojectMousePositionToWorld (WorldOrigin, WorldDirection))
    {
        return false;
    }
    UWorld* World = GetWorld ();
    if (!World)
    {
        return false;
    }
    APawn* ControlledPawn = GetPawn ();
    if (!ControlledPawn)
    {
        return false;
    }
    const FVector Start = WorldOrigin;
    const FVector End = Start + WorldDirection * 10000.f;
    FCollisionQueryParams QueryParams (SCENE_QUERY_STAT (GridMouseInteractionTrace), true);

    QueryParams.AddIgnoredActor (ControlledPawn);
    TArray<FHitResult> Hits;
    if (!World->LineTraceMultiByChannel (Hits, Start, End, ECC_Visibility, QueryParams))
    {
        return false;
    }
    for (const FHitResult& Hit : Hits)
    {
        AActor* HitActor = Hit.GetActor ();
        if (!HitActor)
        {
            continue;
        }

        if (!HitActor->GetClass ()->ImplementsInterface (UGridInteractableInterface::StaticClass ()))
        {
            continue;
        }

        UPrimitiveComponent* HitComponent = Hit.GetComponent ();
        if (!HitComponent ||
            !IGridInteractableInterface::Execute_CanInteract (HitActor, ControlledPawn, HitComponent))
        {
            continue;
        }

        OutHitResult = Hit;
        OutInteractableActor = HitActor;
        return true;
    }
    return false;
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
