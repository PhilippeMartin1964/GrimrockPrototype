#include "Runtime/GrimrockPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/EngineTypes.h"
#include "InputCoreTypes.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridReceptacleActor.h"
#include "Runtime/GridWallLockActor.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "UI/GridInventoryWidget.h"

namespace
{
    AGridReceptacleActor* ResolveReceptacleFromHitActor (AActor* HitActor)
    {
        if (AGridReceptacleActor* ReceptacleActor = Cast<AGridReceptacleActor> (HitActor))
        {
            return ReceptacleActor;
        }

        return HitActor ? Cast<AGridReceptacleActor> (HitActor->GetOwner ()) : nullptr;
    }

    AGridWallLockActor* ResolveWallLockFromHitActor (AActor* HitActor)
    {
        if (AGridWallLockActor* WallLockActor = Cast<AGridWallLockActor> (HitActor))
        {
            return WallLockActor;
        }

        return HitActor ? Cast<AGridWallLockActor> (HitActor->GetOwner ()) : nullptr;
    }

    FText GetReceptacleRejectFeedbackText (EGridReceptacleRejectReason Reason)
    {
        switch (Reason)
        {
        case EGridReceptacleRejectReason::InvalidItem:
            return FText::FromString (TEXT ("Objet invalide."));
        case EGridReceptacleRejectReason::Full:
            return FText::FromString (TEXT ("Ce réceptacle est plein."));
        case EGridReceptacleRejectReason::ExplicitlyRejected:
        case EGridReceptacleRejectReason::NoMatchingAcceptanceRule:
            return FText::FromString (TEXT ("Cet objet ne convient pas."));
        case EGridReceptacleRejectReason::None:
        default:
            return FText::FromString (TEXT ("Impossible de placer cet objet ici."));
        }
    }

}

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

AGrimrockPlayerController::FGridMouseInteractionResolution AGrimrockPlayerController::ResolveLeftMouseInteraction ()
{
    FGridMouseInteractionResolution Resolution;
    Resolution.PartyPawn = Cast<AGrimrockPartyPawn> (GetPawn ());

    if (Resolution.PartyPawn &&
        Resolution.PartyPawn->LevelRuntimeActor &&
        Resolution.PartyPawn->LevelRuntimeActor->HasActiveReadableMessage ())
    {
        Resolution.Intent = EGridMouseInteractionIntent::DismissReadableMessage;
        Resolution.DiagnosticReason = TEXT ("ReadableMessageActive");
        return Resolution;
    }

    Resolution.bHasCursorItem = Resolution.PartyPawn &&
        Resolution.PartyPawn->GetCursorItem (Resolution.CursorItem);
    if (bInventoryUiOpen && !Resolution.bHasCursorItem)
    {
        Resolution.Intent = EGridMouseInteractionIntent::IgnoreInventoryUiWithoutCursorItem;
        Resolution.DiagnosticReason = TEXT ("OpenWithoutCursorItem");
        return Resolution;
    }

    if (Resolution.bHasCursorItem)
    {
        Resolution.bHasWorldHit = TryGetWorldHitUnderCursor (Resolution.HitResult);
        if (!Resolution.bHasWorldHit)
        {
            Resolution.Intent = EGridMouseInteractionIntent::CursorItemNoWorldHit;
            Resolution.DiagnosticReason = TEXT ("NoWorldHit");
            return Resolution;
        }

        Resolution.bWithinInteractionDistance = IsHitWithinInteractionDistance (Resolution.HitResult);
        Resolution.ReceptacleActor = ResolveReceptacleFromHitActor (Resolution.HitResult.GetActor ());
        Resolution.WallLockActor = ResolveWallLockFromHitActor (Resolution.HitResult.GetActor ());

        AGridLevelRuntimeActor* RuntimeActor = Resolution.PartyPawn
            ? Resolution.PartyPawn->LevelRuntimeActor.Get ()
            : nullptr;
        if (Resolution.ReceptacleActor && Resolution.bWithinInteractionDistance)
        {
            Resolution.bReceptacleAccessible = RuntimeActor && RuntimeActor->CanPartyInteractWithEdgeObject (
                Resolution.ReceptacleActor->CellX,
                Resolution.ReceptacleActor->CellY,
                Resolution.ReceptacleActor->Edge,
                Resolution.PartyPawn);
            if (Resolution.bReceptacleAccessible)
            {
                Resolution.Intent = Resolution.WallLockActor
                    ? EGridMouseInteractionIntent::CursorItemWallLock
                    : EGridMouseInteractionIntent::CursorItemReceptacle;
                Resolution.DiagnosticReason = Resolution.WallLockActor
                    ? TEXT ("WallLockCandidate")
                    : TEXT ("ReceptacleCandidate");
                return Resolution;
            }
        }

        if (Resolution.bWithinInteractionDistance &&
            TryResolveWorldDropFromHit (
                Resolution.HitResult,
                Resolution.PartyPawn,
                Resolution.DropCellX,
                Resolution.DropCellY,
                Resolution.DropLocalOffset))
        {
            Resolution.Intent = EGridMouseInteractionIntent::CursorItemWorldDrop;
            Resolution.DiagnosticReason = TEXT ("WorldDropCandidate");
            return Resolution;
        }

        Resolution.Intent = EGridMouseInteractionIntent::CursorItemThrow;
        Resolution.DiagnosticReason = TEXT ("ThrowCandidate");
        return Resolution;
    }

    if (!TryGetInteractableUnderCursor (Resolution.HitResult, Resolution.InteractableActor))
    {
        Resolution.Intent = EGridMouseInteractionIntent::FallbackNoInteractable;
        Resolution.DiagnosticReason = TEXT ("NoInteractable");
        return Resolution;
    }

    Resolution.bWithinInteractionDistance = IsHitWithinInteractionDistance (Resolution.HitResult);
    if (!Resolution.bWithinInteractionDistance)
    {
        Resolution.Intent = EGridMouseInteractionIntent::WorldInteractableOutOfRange;
        Resolution.DiagnosticReason = TEXT ("OutOfRange");
        return Resolution;
    }

    Resolution.HitComponent = Resolution.HitResult.GetComponent ();
    APawn* ControlledPawn = GetPawn ();
    if (!ControlledPawn || !Resolution.HitComponent)
    {
        Resolution.Intent = EGridMouseInteractionIntent::WorldInteractableInvalidPawnOrComponent;
        Resolution.DiagnosticReason = TEXT ("InvalidPawnOrComponent");
        return Resolution;
    }

    if (!IGridInteractableInterface::Execute_CanInteract (
        Resolution.InteractableActor,
        ControlledPawn,
        Resolution.HitComponent))
    {
        Resolution.Intent = EGridMouseInteractionIntent::WorldInteractableCanInteractRejected;
        Resolution.DiagnosticReason = TEXT ("CanInteractRejected");
        return Resolution;
    }

    Resolution.Intent = EGridMouseInteractionIntent::WorldInteractable;
    Resolution.DiagnosticReason = TEXT ("Interactable");
    return Resolution;
}

void AGrimrockPlayerController::HandleLeftMousePressed ()
{
    const FGridMouseInteractionResolution MouseResolution = ResolveLeftMouseInteraction ();
    AGrimrockPartyPawn* PartyPawn = MouseResolution.PartyPawn;

    if (MouseResolution.Intent == EGridMouseInteractionIntent::DismissReadableMessage)
    {
        if (PartyPawn &&
            PartyPawn->LevelRuntimeActor &&
            PartyPawn->LevelRuntimeActor->DismissReadableMessage ())
        {
            UE_LOG (LogTemp, Log,
                TEXT ("GridMouse Click Priority=ReadableMessage Result=Dismissed"));
        }
        return;
    }

    if (MouseResolution.Intent == EGridMouseInteractionIntent::IgnoreInventoryUiWithoutCursorItem)
    {
        UE_LOG (LogTemp, Log,
            TEXT ("GridMouse Click Priority=InventoryUI Result=Ignored Reason=OpenWithoutCursorItem"));
        if (bDebugMouseInteraction)
        {
            UE_LOG (LogTemp, Verbose, TEXT ("Mouse interaction ignored: inventory UI open."));
        }
        return;
    }

    if (MouseResolution.bHasCursorItem)
    {
        const FGridItemInstance& CursorItem = MouseResolution.CursorItem;
        UE_LOG (LogTemp, Log,
            TEXT ("GridMouse Click Priority=CursorItem Item=%s RuntimeId=%s"),
            *CursorItem.ItemDefinitionId.ToString (),
            *CursorItem.RuntimeObjectId.ToString ());

        if (MouseResolution.Intent == EGridMouseInteractionIntent::CursorItemNoWorldHit)
        {
            UE_LOG (LogTemp, Log,
                TEXT ("GridMouse Click Priority=CursorItem Branch=WorldHit Result=NoTarget"));
            UE_LOG (LogTemp, Warning, TEXT ("GridInventory WorldDrop Failed Reason=NoTarget"));
            ShowInteractionFeedback (FText::FromString (TEXT ("Impossible de d\u00E9poser ou lancer ici.")));
            SetGridInteractionCursor (EGridInteractionCursor::CannotPlaceItem, TEXT ("ClickCursorNoWorldHit"));
            return;
        }

        const FHitResult& WorldHitResult = MouseResolution.HitResult;
        const bool bWithinInteractionDistance = MouseResolution.bWithinInteractionDistance;
        AGridReceptacleActor* ReceptacleActor = MouseResolution.ReceptacleActor;
        AGridWallLockActor* WallLockActor = MouseResolution.WallLockActor;
        AGridLevelRuntimeActor* RuntimeActor = PartyPawn ? PartyPawn->LevelRuntimeActor.Get () : nullptr;

        if (ReceptacleActor && bWithinInteractionDistance)
        {
            UE_LOG (LogTemp, Log,
                TEXT ("GridMouse Click Priority=CursorItem Branch=ReceptacleCandidate Item=%s Target=%s HitActor=%s WithinDistance=%s"),
                *CursorItem.ItemDefinitionId.ToString (),
                *GetNameSafe (ReceptacleActor),
                *GetNameSafe (WorldHitResult.GetActor ()),
                bWithinInteractionDistance ? TEXT ("true") : TEXT ("false"));

            if (MouseResolution.bReceptacleAccessible)
            {
                if (MouseResolution.Intent == EGridMouseInteractionIntent::CursorItemWallLock)
                {
                    UE_LOG (LogTemp, Log,
                        TEXT ("GridMouse Click Priority=CursorItem Branch=WallLockAttempt Item=%s Target=%s Result=Attempt"),
                        *CursorItem.ItemDefinitionId.ToString (),
                        *GetNameSafe (WallLockActor));
                    UE_LOG (LogTemp, Log,
                        TEXT ("GridInventory WorldDrop RoutedToWallLock Item=%s Target=%s"),
                        *CursorItem.ItemDefinitionId.ToString (),
                        *GetNameSafe (WallLockActor));
                    IGridInteractableInterface::Execute_InteractWithHit (
                        WallLockActor,
                        PartyPawn,
                        WorldHitResult.GetComponent (),
                        WorldHitResult);

                    if (UGridInventoryWidget* InventoryWidget = PartyPawn->GetInventoryWidget ())
                    {
                        InventoryWidget->RefreshInventory ();
                    }
                    SetGridInteractionCursor (EGridInteractionCursor::Default, TEXT ("ClickWallLockAttemptComplete"));
                    return;
                }

                UE_LOG (LogTemp, Log,
                    TEXT ("GridMouse Click Priority=CursorItem Branch=ReceptacleAttempt Item=%s Target=%s Result=Validate"),
                    *CursorItem.ItemDefinitionId.ToString (),
                    *GetNameSafe (ReceptacleActor));
                FGridReceptacleAcceptanceResult AcceptanceResult;
                if (!ReceptacleActor->EvaluateItemAcceptance (CursorItem, AcceptanceResult, true))
                {
                    UE_LOG (LogTemp, Log,
                        TEXT ("GridMouse Click Priority=CursorItem Branch=ReceptacleAttempt Item=%s Target=%s Result=Rejected Reason=%d"),
                        *CursorItem.ItemDefinitionId.ToString (),
                        *GetNameSafe (ReceptacleActor),
                        static_cast<int32> (AcceptanceResult.RejectReason));
                    ShowInteractionFeedback (
                        GetReceptacleRejectFeedbackText (AcceptanceResult.RejectReason));
                    SetGridInteractionCursor (EGridInteractionCursor::CannotPlaceItem, TEXT ("ClickReceptacleRejected"));
                    return;
                }

                UE_LOG (LogTemp, Log, TEXT ("GridInventory WorldDrop Attempt Item=%s Target=%s"),
                    *CursorItem.ItemDefinitionId.ToString (),
                    *GetNameSafe (ReceptacleActor));

                const bool bPlaced = ReceptacleActor->TryPlaceCursorItemFromHit (PartyPawn, WorldHitResult);
                UE_LOG (LogTemp, Log,
                    TEXT ("GridMouse Click Priority=CursorItem Branch=ReceptacleAttempt Item=%s Target=%s Result=%s"),
                    *CursorItem.ItemDefinitionId.ToString (),
                    *GetNameSafe (ReceptacleActor),
                    bPlaced ? TEXT ("Placed") : TEXT ("Failed"));
                UE_LOG (LogTemp, Log, TEXT ("GridInventory WorldDrop Result=%s"),
                    bPlaced ? TEXT ("true") : TEXT ("false"));

                if (UGridInventoryWidget* InventoryWidget = PartyPawn->GetInventoryWidget ())
                {
                    InventoryWidget->RefreshInventory ();
                }
                if (!bPlaced)
                {
                    ShowInteractionFeedback (
                        FText::FromString (TEXT ("Impossible de placer cet objet ici.")));
                }
                SetGridInteractionCursor (bPlaced
                    ? EGridInteractionCursor::Default
                    : EGridInteractionCursor::CannotPlaceItem,
                    TEXT ("ClickReceptacleAttemptComplete"));
                return;
            }

            UE_LOG (LogTemp, Log,
                TEXT ("GridMouse Click Priority=CursorItem Branch=ReceptacleAttempt Item=%s Target=%s Result=Inaccessible"),
                *CursorItem.ItemDefinitionId.ToString (),
                *GetNameSafe (ReceptacleActor));
        }

        UE_LOG (LogTemp, Log,
            TEXT ("GridMouse Click Priority=CursorItem Branch=WorldDropAttempt Item=%s WithinDistance=%s"),
            *CursorItem.ItemDefinitionId.ToString (),
            bWithinInteractionDistance ? TEXT ("true") : TEXT ("false"));
        if (MouseResolution.Intent == EGridMouseInteractionIntent::CursorItemWorldDrop &&
            PartyPawn->TryDropCursorItemAtCell (
                MouseResolution.DropCellX,
                MouseResolution.DropCellY,
                EGridEdge::None,
                MouseResolution.DropLocalOffset))
        {
            if (UGridInventoryWidget* InventoryWidget = PartyPawn->GetInventoryWidget ())
            {
                InventoryWidget->RefreshInventory ();
            }
            UE_LOG (LogTemp, Log,
                TEXT ("GridMouse Click Priority=CursorItem Branch=WorldDropAttempt Item=%s Result=Dropped Cell=(%d,%d)"),
                *CursorItem.ItemDefinitionId.ToString (),
                MouseResolution.DropCellX,
                MouseResolution.DropCellY);
            SetGridInteractionCursor (EGridInteractionCursor::Default, TEXT ("ClickWorldDropSuccess"));
            return;
        }

        UGridItemDefinitionAsset* ItemDefinition = RuntimeActor
            ? RuntimeActor->ResolveRuntimeItemDefinition (CursorItem.ItemDefinitionId)
            : nullptr;
        UE_LOG (LogTemp, Log,
            TEXT ("GridMouse Click Priority=CursorItem Branch=ThrowAttempt Item=%s Throwable=%s"),
            *CursorItem.ItemDefinitionId.ToString (),
            ItemDefinition && ItemDefinition->bThrowable ? TEXT ("true") : TEXT ("false"));
        if (!ItemDefinition || !ItemDefinition->bThrowable)
        {
            UE_LOG (LogTemp, Log,
                TEXT ("GridMouse Click Priority=CursorItem Branch=ThrowAttempt Item=%s Result=NotThrowable"),
                *CursorItem.ItemDefinitionId.ToString ());
            ShowInteractionFeedback (FText::FromString (TEXT ("Cet objet ne peut pas \u00EAtre lanc\u00E9.")));
            SetGridInteractionCursor (EGridInteractionCursor::CannotPlaceItem, TEXT ("ClickThrowNotThrowable"));
            return;
        }

        const FVector ThrowStartLocation = PartyPawn->Camera
            ? PartyPawn->Camera->GetComponentLocation ()
            : PartyPawn->GetActorLocation ();
        FVector TargetOffset = WorldHitResult.ImpactPoint - ThrowStartLocation;
        const float TargetDistance = TargetOffset.Size ();
        if (TargetOffset.IsNearlyZero ())
        {
            UE_LOG (LogTemp, Log,
                TEXT ("GridMouse Click Priority=CursorItem Branch=ThrowAttempt Item=%s Result=InvalidTargetOffset"),
                *CursorItem.ItemDefinitionId.ToString ());
            ShowInteractionFeedback (FText::FromString (TEXT ("Impossible de d\u00E9poser ou lancer ici.")));
            SetGridInteractionCursor (EGridInteractionCursor::CannotPlaceItem, TEXT ("ClickThrowInvalidOffset"));
            return;
        }

        if (MaxThrowTargetDistance > 0.f && TargetDistance > MaxThrowTargetDistance)
        {
            UE_LOG (LogTemp, Log,
                TEXT ("GridMouse Click Priority=CursorItem Branch=ThrowAttempt Item=%s Result=TooFar Distance=%.2f Max=%.2f"),
                *CursorItem.ItemDefinitionId.ToString (),
                TargetDistance,
                MaxThrowTargetDistance);
            ShowInteractionFeedback (FText::FromString (TEXT ("Cible trop \u00E9loign\u00E9e.")));
            SetGridInteractionCursor (EGridInteractionCursor::CannotPlaceItem, TEXT ("ClickThrowTooFar"));
            return;
        }

        const EGridItemThrowMode ThrowMode = TargetDistance < ThrowDistanceThreshold
            ? EGridItemThrowMode::ShortToss
            : EGridItemThrowMode::Throw;
        const bool bThrown = PartyPawn->TryThrowOneCursorItem (TargetOffset, ThrowMode);
        UE_LOG (LogTemp, Log,
            TEXT ("GridMouse Click Priority=CursorItem Branch=ThrowAttempt Item=%s Result=%s Distance=%.2f Mode=%d"),
            *CursorItem.ItemDefinitionId.ToString (),
            bThrown ? TEXT ("Thrown") : TEXT ("Failed"),
            TargetDistance,
            static_cast<int32> (ThrowMode));
        if (UGridInventoryWidget* InventoryWidget = PartyPawn->GetInventoryWidget ())
        {
            InventoryWidget->RefreshInventory ();
        }
        if (!bThrown)
        {
            ShowInteractionFeedback (FText::FromString (TEXT ("Lancer impossible.")));
            SetGridInteractionCursor (EGridInteractionCursor::CannotPlaceItem, TEXT ("ClickThrowFailed"));
            return;
        }

        SetGridInteractionCursor (EGridInteractionCursor::Default, TEXT ("ClickThrowSuccess"));
        return;
    }

    AActor* InteractableActor = MouseResolution.InteractableActor;
    if (MouseResolution.Intent == EGridMouseInteractionIntent::FallbackNoInteractable)
    {
        UE_LOG (LogTemp, Log,
            TEXT ("GridMouse Click Priority=WorldInteractable Result=None Fallback=NoInteractable"));
        if (bDebugMouseInteraction)
        {
            UE_LOG (LogTemp, Verbose, TEXT ("Mouse interaction: no interactable under cursor."));
        }
        return;
    }

    if (MouseResolution.Intent == EGridMouseInteractionIntent::WorldInteractableOutOfRange)
    {
        UE_LOG (LogTemp, Log,
            TEXT ("GridMouse Click Priority=WorldInteractable Actor=%s Result=OutOfRange"),
            *GetNameSafe (InteractableActor));
        ShowInteractionFeedback (FText::FromString (TEXT ("Hors de port\u00E9e.")));
        if (bDebugMouseInteraction)
        {
            UE_LOG (LogTemp, Verbose, TEXT ("Mouse interaction: %s is outside interaction distance."),
                *GetNameSafe (InteractableActor));
        }
        return;
    }

    APawn* ControlledPawn = GetPawn ();
    UPrimitiveComponent* HitComponent = MouseResolution.HitComponent;
    if (MouseResolution.Intent == EGridMouseInteractionIntent::WorldInteractableInvalidPawnOrComponent)
    {
        UE_LOG (LogTemp, Log,
            TEXT ("GridMouse Click Priority=WorldInteractable Actor=%s Result=InvalidPawnOrComponent Component=%s"),
            *GetNameSafe (InteractableActor),
            *GetNameSafe (HitComponent));
        if (bDebugMouseInteraction)
        {
            UE_LOG (LogTemp, Verbose, TEXT ("Mouse interaction: invalid pawn or hit component for %s."),
                *GetNameSafe (InteractableActor));
        }
        return;
    }

    if (MouseResolution.Intent == EGridMouseInteractionIntent::WorldInteractableCanInteractRejected)
    {
        UE_LOG (LogTemp, Log,
            TEXT ("GridMouse Click Priority=WorldInteractable Actor=%s Component=%s Result=CanInteractRejected"),
            *GetNameSafe (InteractableActor),
            *GetNameSafe (HitComponent));
        ShowInteractionFeedback (FText::FromString (TEXT ("Action impossible.")));
        if (bDebugMouseInteraction)
        {
            UE_LOG (LogTemp, Verbose, TEXT ("Mouse interaction: CanInteract rejected %s on component %s."),
                *GetNameSafe (InteractableActor),
                *GetNameSafe (HitComponent));
        }
        return;
    }

    UE_LOG (LogTemp, Log,
        TEXT ("GridMouse Click Priority=WorldInteractable Actor=%s Component=%s Result=Interact"),
        *GetNameSafe (InteractableActor),
        *GetNameSafe (HitComponent));
    if (bDebugMouseInteraction)
    {
        UE_LOG (LogTemp, Log, TEXT ("Mouse interaction: Interact %s on component %s."),
            *GetNameSafe (InteractableActor),
            *GetNameSafe (HitComponent));
    }

    IGridInteractableInterface::Execute_InteractWithHit (InteractableActor, ControlledPawn, HitComponent, MouseResolution.HitResult);
}

void AGrimrockPlayerController::ShowInteractionFeedback (const FText& MessageText) const
{
    const AGrimrockPartyPawn* PartyPawn = Cast<AGrimrockPartyPawn> (GetPawn ());
    if (PartyPawn && PartyPawn->LevelRuntimeActor)
    {
        PartyPawn->LevelRuntimeActor->ShowInteractionFeedback (MessageText);
    }
}

void AGrimrockPlayerController::UpdateHoveredInteractable ()
{
    if (bInventoryUiOpen)
    {
        SetGridInteractionCursor (EGridInteractionCursor::Default, TEXT ("HoverInventoryUiOpen"));
        return;
    }

    const AGrimrockPartyPawn* PartyPawn = Cast<AGrimrockPartyPawn> (GetPawn ());
    FGridItemInstance CursorItem;
    if (PartyPawn && PartyPawn->GetCursorItem (CursorItem))
    {
        FHitResult WorldHitResult;
        if (!TryGetWorldHitUnderCursor (WorldHitResult))
        {
            SetGridInteractionCursor (EGridInteractionCursor::Default, TEXT ("HoverCursorItemNoWorldHit"));
            return;
        }

        AGridReceptacleActor* ReceptacleActor =
            ResolveReceptacleFromHitActor (WorldHitResult.GetActor ());

        const bool bWithinInteractionDistance = IsHitWithinInteractionDistance (WorldHitResult);
        if (bWithinInteractionDistance && ReceptacleActor)
        {
            const bool bCanPlace =
                PartyPawn->LevelRuntimeActor &&
                PartyPawn->LevelRuntimeActor->CanPartyInteractWithEdgeObject (
                    ReceptacleActor->CellX,
                    ReceptacleActor->CellY,
                    ReceptacleActor->Edge,
                    PartyPawn) &&
                ReceptacleActor->CanAcceptItemInstance (CursorItem);
            SetGridInteractionCursor (bCanPlace
                ? EGridInteractionCursor::PlaceItem
                : EGridInteractionCursor::CannotPlaceItem,
                TEXT ("HoverCursorItemReceptacle"));
            return;
        }

        if (bWithinInteractionDistance)
        {
            int32 DropCellX = INDEX_NONE;
            int32 DropCellY = INDEX_NONE;
            FVector DropLocalOffset = FVector::ZeroVector;
            if (TryResolveWorldDropFromHit (
                WorldHitResult,
                PartyPawn,
                DropCellX,
                DropCellY,
                DropLocalOffset))
            {
                SetGridInteractionCursor (EGridInteractionCursor::PlaceItem, TEXT ("HoverCursorItemWorldDrop"));
                return;
            }
        }

        UGridItemDefinitionAsset* ItemDefinition = PartyPawn->LevelRuntimeActor
            ? PartyPawn->LevelRuntimeActor->ResolveRuntimeItemDefinition (CursorItem.ItemDefinitionId)
            : nullptr;
        const FVector ThrowStartLocation = PartyPawn->Camera
            ? PartyPawn->Camera->GetComponentLocation ()
            : PartyPawn->GetActorLocation ();
        const float TargetDistance = FVector::Distance (ThrowStartLocation, WorldHitResult.ImpactPoint);
        const bool bCanAimThrow =
            ItemDefinition &&
            ItemDefinition->bThrowable &&
            TargetDistance > KINDA_SMALL_NUMBER &&
            (MaxThrowTargetDistance <= 0.f || TargetDistance <= MaxThrowTargetDistance);
        SetGridInteractionCursor (bCanAimThrow
            ? EGridInteractionCursor::AimThrow
            : (bWithinInteractionDistance
                ? EGridInteractionCursor::CannotPlaceItem
                : EGridInteractionCursor::Default),
            TEXT ("HoverCursorItemThrow"));
        return;
    }

    FHitResult HitResult;
    AActor* InteractableActor = nullptr;
    if (!TryGetInteractableUnderCursor (HitResult, InteractableActor))
    {
        SetGridInteractionCursor (EGridInteractionCursor::Default, TEXT ("HoverNoInteractable"));
        return;
    }

    if (!IsHitWithinInteractionDistance (HitResult))
    {
        SetGridInteractionCursor (EGridInteractionCursor::Default, TEXT ("HoverOutOfRange"));
        return;
    }

    APawn* ControlledPawn = GetPawn ();
    UPrimitiveComponent* HitComponent = HitResult.GetComponent ();
    if (!ControlledPawn || !HitComponent)
    {
        SetGridInteractionCursor (EGridInteractionCursor::Default, TEXT ("HoverInvalidPawnOrComponent"));
        return;
    }

    if (!IGridInteractableInterface::Execute_CanInteract (InteractableActor, ControlledPawn, HitComponent))
    {
        SetGridInteractionCursor (EGridInteractionCursor::Default, TEXT ("HoverCanInteractRejected"));
        return;
    }

    const EGridInteractionCursor InteractionCursor =
        IGridInteractableInterface::Execute_GetInteractionCursor (InteractableActor, HitComponent);
    SetGridInteractionCursor (InteractionCursor, TEXT ("HoverInteractable"));
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

void AGrimrockPlayerController::SetGridInteractionCursor (EGridInteractionCursor NewCursor, const TCHAR* Reason)
{
    const EGridInteractionCursor PreviousCursor = CurrentGridInteractionCursor;
    CurrentGridInteractionCursor = NewCursor;
    if (PreviousCursor != NewCursor)
    {
        UE_LOG (LogTemp, Log,
            TEXT ("GridMouse Hover CursorChanged From=%s To=%s Reason=%s"),
            *UEnum::GetValueAsString (PreviousCursor),
            *UEnum::GetValueAsString (NewCursor),
            Reason ? Reason : TEXT ("None"));
    }

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
    if (!HitActor)
    {
        return false;
    }

    AActor* InteractableActor = ResolveReceptacleFromHitActor (HitActor);
    if (!InteractableActor)
    {
        InteractableActor = HitActor;
    }
    if (!InteractableActor->GetClass ()->ImplementsInterface (UGridInteractableInterface::StaticClass ()))
    {
        return false;
    }

    OutHitResult = Hit;
    OutInteractableActor = InteractableActor;
    return true;
}

bool AGrimrockPlayerController::TryGetWorldHitUnderCursor (FHitResult& OutHitResult) const
{
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

    FCollisionQueryParams QueryParams (SCENE_QUERY_STAT (GridMouseWorldDropTrace), true);
    QueryParams.AddIgnoredActor (ControlledPawn);
    return World->LineTraceSingleByChannel (
        OutHitResult,
        WorldOrigin,
        WorldOrigin + WorldDirection * 10000.f,
        ECC_Visibility,
        QueryParams);
}

bool AGrimrockPlayerController::TryResolveWorldDropFromHit (
    const FHitResult& HitResult,
    const AGrimrockPartyPawn* PartyPawn,
    int32& OutCellX,
    int32& OutCellY,
    FVector& OutLocalOffset) const
{
    OutCellX = INDEX_NONE;
    OutCellY = INDEX_NONE;
    OutLocalOffset = FVector::ZeroVector;
    const AGridLevelRuntimeActor* RuntimeActor = PartyPawn ? PartyPawn->LevelRuntimeActor.Get () : nullptr;
    if (!RuntimeActor || !RuntimeActor->LevelAsset)
    {
        return false;
    }

    const float CellSize = RuntimeActor->LevelAsset->CellSize;
    if (CellSize <= KINDA_SMALL_NUMBER)
    {
        return false;
    }

    const FVector GridLocalPoint =
        HitResult.ImpactPoint - RuntimeActor->GetActorLocation () - RuntimeActor->GridOrigin;
    OutCellX = FMath::FloorToInt (GridLocalPoint.X / CellSize);
    OutCellY = FMath::FloorToInt (GridLocalPoint.Y / CellSize);
    if (!RuntimeActor->IsWalkableCell (OutCellX, OutCellY))
    {
        return false;
    }

    int32 FrontCellX = INDEX_NONE;
    int32 FrontCellY = INDEX_NONE;
    const bool bHasFrontCell = RuntimeActor->TryGetNeighborCell (
        PartyPawn->CurrentCellX,
        PartyPawn->CurrentCellY,
        PartyPawn->Facing,
        FrontCellX,
        FrontCellY);
    const bool bAllowedCell =
        (OutCellX == PartyPawn->CurrentCellX && OutCellY == PartyPawn->CurrentCellY) ||
        (bHasFrontCell && OutCellX == FrontCellX && OutCellY == FrontCellY);
    if (!bAllowedCell)
    {
        return false;
    }

    const FVector CellCenter = RuntimeActor->GetCellCenterWorld (OutCellX, OutCellY, 12.f);
    const FVector RawOffset = HitResult.ImpactPoint - CellCenter;
    const float MaxOffset = CellSize * 0.35f;
    OutLocalOffset = FVector (
        FMath::Clamp (RawOffset.X, -MaxOffset, MaxOffset),
        FMath::Clamp (RawOffset.Y, -MaxOffset, MaxOffset),
        0.f);
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

    AGridReceptacleActor* ReceptacleActor = ResolveReceptacleFromHitActor (HitActor);
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
