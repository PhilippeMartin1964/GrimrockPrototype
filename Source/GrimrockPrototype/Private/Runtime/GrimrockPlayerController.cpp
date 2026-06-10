#include "Runtime/GrimrockPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Components/InputComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/EngineTypes.h"
#include "InputCoreTypes.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridReceptacleActor.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "UI/GridInventoryWidget.h"

AGrimrockPlayerController::AGrimrockPlayerController ()
{
    bEnableClickEvents = true;
    bEnableMouseOverEvents = true;
    CurrentMouseCursor = EMouseCursor::Default;
    DefaultMouseCursor = EMouseCursor::Default;
    bShowMouseCursor = true;
}

void AGrimrockPlayerController::BeginPlay ()
{
    Super::BeginPlay ();
    bShowMouseCursor = true;
    bEnableClickEvents = true;
    bEnableMouseOverEvents = true;
    DefaultMouseCursor = EMouseCursor::Default;
    CurrentMouseCursor = EMouseCursor::Default;
    FInputModeGameAndUI InputMode;
    InputMode.SetHideCursorDuringCapture (false);
    SetInputMode (InputMode);
    InitializeCustomCursor ();
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
    DefaultMouseCursor = EMouseCursor::Default;
    CurrentMouseCursor = EMouseCursor::Default;
    bShowMouseCursor = true;
    if (CustomCursorWidget)
    {
        CustomCursorWidget->SetVisibility (ESlateVisibility::Collapsed);
    }
    UE_LOG (LogTemp, Log, TEXT ("GridInventory UI State Open=%s"), bInventoryUiOpen ? TEXT ("true") : TEXT ("false"));
    UE_LOG (LogTemp, Log, TEXT ("GridInventory UI CustomCursor Widget=%s Visibility=%s Enabled=%s"),
        *GetNameSafe (CustomCursorWidget),
        CustomCursorWidget && CustomCursorWidget->GetVisibility () == ESlateVisibility::HitTestInvisible
            ? TEXT ("HitTestInvisible")
            : TEXT ("Other"),
        CustomCursorWidget && CustomCursorWidget->GetIsEnabled () ? TEXT ("true") : TEXT ("false"));
}

void AGrimrockPlayerController::HandleLeftMousePressed ()
{
    AGrimrockPartyPawn* PartyPawn = Cast<AGrimrockPartyPawn> (GetPawn ());
    if (PartyPawn &&
        PartyPawn->LevelRuntimeActor &&
        PartyPawn->LevelRuntimeActor->DismissReadableMessage ())
    {
        return;
    }

    FGridItemInstance CursorItem;
    const bool bHasCursorItem = PartyPawn && PartyPawn->GetCursorItem (CursorItem);
    if (bInventoryUiOpen && !bHasCursorItem)
    {
        if (bDebugMouseInteraction)
        {
            UE_LOG (LogTemp, Verbose, TEXT ("Mouse interaction ignored: inventory UI open."));
        }
        return;
    }

    if (bHasCursorItem)
    {
        FHitResult ReceptacleHitResult;
        AGridReceptacleActor* ReceptacleActor = nullptr;
        if (!TryGetReceptacleUnderCursor (ReceptacleHitResult, ReceptacleActor))
        {
            UE_LOG (LogTemp, Warning, TEXT ("GridInventory WorldDrop Failed Reason=NoTarget"));
            SetGridInteractionCursor (EGridInteractionCursor::CannotPlaceItem);
            return;
        }

        if (!IsHitWithinInteractionDistance (ReceptacleHitResult))
        {
            UE_LOG (LogTemp, Warning, TEXT ("GridInventory WorldDrop Failed Reason=TargetOutOfRange Target=%s"),
                *GetNameSafe (ReceptacleActor));
            SetGridInteractionCursor (EGridInteractionCursor::CannotPlaceItem);
            return;
        }

        AGridLevelRuntimeActor* RuntimeActor = PartyPawn ? PartyPawn->LevelRuntimeActor.Get () : nullptr;
        if (!ReceptacleActor || !RuntimeActor || !RuntimeActor->CanPartyInteractWithEdgeObject (
            ReceptacleActor->CellX,
            ReceptacleActor->CellY,
            ReceptacleActor->Edge,
            PartyPawn))
        {
            UE_LOG (LogTemp, Warning,
                TEXT ("GridInventory WorldDrop Failed Reason=EdgeNotFacingParty Target=%s"),
                *GetNameSafe (ReceptacleActor));
            SetGridInteractionCursor (EGridInteractionCursor::CannotPlaceItem);
            return;
        }

        UE_LOG (LogTemp, Log, TEXT ("GridInventory WorldDrop Attempt Item=%s Target=%s"),
            *CursorItem.ItemDefinitionId.ToString (),
            *GetNameSafe (ReceptacleActor));

        if (!ReceptacleActor || !ReceptacleActor->CanAcceptItemInstance (CursorItem))
        {
            const TCHAR* FailureReason = !ReceptacleActor
                ? TEXT ("NoTarget")
                : (ReceptacleActor->IsFull ()
                    ? TEXT ("ReceptacleFull")
                    : (!CursorItem.IsValid ()
                        ? TEXT ("InvalidItem")
                        : TEXT ("IncompatibleTarget")));
            UE_LOG (LogTemp, Warning, TEXT ("GridInventory WorldDrop Failed Reason=%s"), FailureReason);
            SetGridInteractionCursor (EGridInteractionCursor::CannotPlaceItem);
            return;
        }

        const bool bPlaced = ReceptacleActor->TryPlaceCursorItemFromHit (PartyPawn, ReceptacleHitResult);
        UE_LOG (LogTemp, Log, TEXT ("GridInventory WorldDrop Result=%s"),
            bPlaced ? TEXT ("true") : TEXT ("false"));

        if (UGridInventoryWidget* InventoryWidget = PartyPawn->GetInventoryWidget ())
        {
            InventoryWidget->RefreshInventory ();
        }

        SetGridInteractionCursor (bPlaced ? EGridInteractionCursor::Default : EGridInteractionCursor::CannotPlaceItem);
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

    const AGrimrockPartyPawn* PartyPawn = Cast<AGrimrockPartyPawn> (GetPawn ());
    FGridItemInstance CursorItem;
    if (PartyPawn && PartyPawn->GetCursorItem (CursorItem))
    {
        FHitResult ReceptacleHitResult;
        AGridReceptacleActor* ReceptacleActor = nullptr;
        const bool bHasReceptacle = TryGetReceptacleUnderCursor (ReceptacleHitResult, ReceptacleActor);
        const bool bCanPlace = bHasReceptacle &&
            IsHitWithinInteractionDistance (ReceptacleHitResult) &&
            ReceptacleActor &&
            PartyPawn->LevelRuntimeActor &&
            PartyPawn->LevelRuntimeActor->CanPartyInteractWithEdgeObject (
                ReceptacleActor->CellX,
                ReceptacleActor->CellY,
                ReceptacleActor->Edge,
                PartyPawn) &&
            ReceptacleActor->CanAcceptItemInstance (CursorItem);

        SetGridInteractionCursor (bCanPlace
            ? EGridInteractionCursor::PlaceItem
            : EGridInteractionCursor::CannotPlaceItem);

        UE_LOG (LogTemp, Verbose, TEXT ("GridInventory CursorHover Item=%s Target=%s CanPlace=%s Cursor=%s"),
            *CursorItem.ItemDefinitionId.ToString (),
            *GetNameSafe (ReceptacleActor),
            bCanPlace ? TEXT ("true") : TEXT ("false"),
            bCanPlace ? TEXT ("PlaceItem") : TEXT ("CannotPlaceItem"));
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
    if (!CustomCursorWidgetClass)
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
        CustomCursorWidget->SetIsEnabled (true);
        CustomCursorWidget->SetVisibility (ESlateVisibility::HitTestInvisible);
        CustomCursorWidget->AddToViewport (9999);
        CustomCursorWidget->SetVisibility (ESlateVisibility::HitTestInvisible);
    }
}

void AGrimrockPlayerController::SetGridInteractionCursor (EGridInteractionCursor NewCursor)
{
    CurrentGridInteractionCursor = NewCursor;
    if (CustomCursorWidget)
    {
        DefaultMouseCursor = EMouseCursor::None;
        CurrentMouseCursor = EMouseCursor::None;
        bShowMouseCursor = false;
        CustomCursorWidget->SetIsEnabled (true);
        CustomCursorWidget->SetVisibility (ESlateVisibility::HitTestInvisible);

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
        return;
    }

    DefaultMouseCursor = EMouseCursor::Default;
    CurrentMouseCursor = NewCursor == EGridInteractionCursor::Take
        ? EMouseCursor::Hand
        : EMouseCursor::Default;
    bShowMouseCursor = true;
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
    FHitResult Hit;
    if (!World->LineTraceSingleByChannel (Hit, Start, End, ECC_Visibility, QueryParams))
    {
        return false;
    }

    // The first visibility blocker owns the click. Never search through geometry
    // or a rejected component for a deeper interactable.
    AActor* HitActor = Hit.GetActor ();
    if (!HitActor ||
        !HitActor->GetClass ()->ImplementsInterface (UGridInteractableInterface::StaticClass ()))
    {
        return false;
    }

    OutHitResult = Hit;
    OutInteractableActor = HitActor;
    return true;
}

bool AGrimrockPlayerController::TryGetReceptacleUnderCursor (
    FHitResult& OutHitResult,
    AGridReceptacleActor*& OutReceptacleActor) const
{
    OutReceptacleActor = nullptr;
    OutHitResult = FHitResult ();

    FVector WorldOrigin = FVector::ZeroVector;
    FVector WorldDirection = FVector::ZeroVector;
    if (!DeprojectMousePositionToWorld (WorldOrigin, WorldDirection))
    {
        return false;
    }

    UWorld* World = GetWorld ();
    APawn* ControlledPawn = GetPawn ();
    if (!World || !ControlledPawn)
    {
        return false;
    }

    const FVector Start = WorldOrigin;
    const FVector End = Start + WorldDirection * 10000.f;
    FCollisionQueryParams QueryParams (SCENE_QUERY_STAT (GridMouseReceptacleTrace), true);
    QueryParams.AddIgnoredActor (ControlledPawn);

    FHitResult Hit;
    if (!World->LineTraceSingleByChannel (Hit, Start, End, ECC_Visibility, QueryParams))
    {
        return false;
    }

    // Item placement follows the same direct-hit rule as ordinary interaction.
    AActor* HitActor = Hit.GetActor ();
    if (!HitActor)
    {
        return false;
    }

    AGridReceptacleActor* ReceptacleActor = Cast<AGridReceptacleActor> (HitActor);
    if (!ReceptacleActor)
    {
        ReceptacleActor = Cast<AGridReceptacleActor> (HitActor->GetOwner ());
    }
    if (!ReceptacleActor)
    {
        return false;
    }

    OutHitResult = Hit;
    OutReceptacleActor = ReceptacleActor;
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
