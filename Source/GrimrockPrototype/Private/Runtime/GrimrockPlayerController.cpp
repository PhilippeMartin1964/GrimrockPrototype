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
#include "Runtime/GridPartyInventoryComponent.h"
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

    FText GetReceptacleRejectFeedbackText (EGridReceptacleRejectReason Reason)
    {
        switch (Reason)
        {
        case EGridReceptacleRejectReason::InvalidItem:
            return FText::FromString (TEXT ("Objet invalide."));
        case EGridReceptacleRejectReason::Full:
            return FText::FromString (TEXT ("Ce réceptacle est plein."));
        case EGridReceptacleRejectReason::InsertDisabled:
            return FText::FromString (TEXT ("Impossible d'insérer un objet ici."));
        case EGridReceptacleRejectReason::ExplicitlyRejected:
        case EGridReceptacleRejectReason::NoMatchingAcceptanceRule:
            return FText::FromString (TEXT ("Cet objet ne convient pas."));
        case EGridReceptacleRejectReason::None:
        default:
            return FText::FromString (TEXT ("Impossible de placer cet objet ici."));
        }
    }

    bool ResolveHeldEquipmentItem (
        const AGrimrockPartyPawn* PartyPawn,
        FGridItemInstance& OutItem)
    {
        OutItem = FGridItemInstance ();
        if (!PartyPawn || !PartyPawn->PartyInventoryComponent)
        {
            return false;
        }

        const UGridPartyInventoryComponent* Inventory = PartyPawn->PartyInventoryComponent;
        const int32 CharacterIndex = Inventory->GetSelectedCharacterIndex ();
        const FName HeldItemDefinitionId = PartyPawn->GetHeldItemDefinitionId ();

        FGridItemInstance EquippedItem;
        if (Inventory->GetEquippedItem (CharacterIndex, EGridEquipmentSlot::MainHand, EquippedItem) &&
            (HeldItemDefinitionId.IsNone () || EquippedItem.ItemDefinitionId == HeldItemDefinitionId))
        {
            OutItem = EquippedItem;
            return true;
        }
        if (Inventory->GetEquippedItem (CharacterIndex, EGridEquipmentSlot::OffHand, EquippedItem) &&
            (HeldItemDefinitionId.IsNone () || EquippedItem.ItemDefinitionId == HeldItemDefinitionId))
        {
            OutItem = EquippedItem;
            return true;
        }

        return false;
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
        FHitResult WorldHitResult;
        if (!TryGetWorldHitUnderCursor (WorldHitResult))
        {
            UE_LOG (LogTemp, Warning, TEXT ("GridInventory WorldDrop Failed Reason=NoTarget"));
            ShowInteractionFeedback (FText::FromString (TEXT ("Impossible de déposer ou lancer ici.")));
            SetGridInteractionCursor (EGridInteractionCursor::CannotPlaceItem);
            return;
        }

        const bool bWithinInteractionDistance = IsHitWithinInteractionDistance (WorldHitResult);

        AGridReceptacleActor* ReceptacleActor =
            ResolveReceptacleFromHitActor (WorldHitResult.GetActor ());

        AGridLevelRuntimeActor* RuntimeActor = PartyPawn ? PartyPawn->LevelRuntimeActor.Get () : nullptr;
        if (ReceptacleActor && bWithinInteractionDistance)
        {
            const bool bAccessible = RuntimeActor && RuntimeActor->CanPartyInteractWithEdgeObject (
                ReceptacleActor->CellX,
                ReceptacleActor->CellY,
                ReceptacleActor->Edge,
                PartyPawn);
            if (bAccessible)
            {
                FGridReceptacleAcceptanceResult AcceptanceResult;
                if (!ReceptacleActor->EvaluateItemAcceptance (CursorItem, AcceptanceResult, true))
                {
                    ShowInteractionFeedback (
                        GetReceptacleRejectFeedbackText (AcceptanceResult.RejectReason));
                    SetGridInteractionCursor (EGridInteractionCursor::CannotPlaceItem);
                    return;
                }

                UE_LOG (LogTemp, Log, TEXT ("GridInventory WorldDrop Attempt Item=%s Target=%s"),
                    *CursorItem.ItemDefinitionId.ToString (),
                    *GetNameSafe (ReceptacleActor));

                const bool bPlaced = ReceptacleActor->TryPlaceCursorItemFromHit (PartyPawn, WorldHitResult);
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
                    : EGridInteractionCursor::CannotPlaceItem);
                return;
            }
        }

        int32 DropCellX = INDEX_NONE;
        int32 DropCellY = INDEX_NONE;
        FVector DropLocalOffset = FVector::ZeroVector;
        if (bWithinInteractionDistance &&
            TryResolveWorldDropFromHit (
                WorldHitResult,
                PartyPawn,
                DropCellX,
                DropCellY,
                DropLocalOffset) &&
            PartyPawn->TryDropCursorItemAtCell (
                DropCellX,
                DropCellY,
                EGridEdge::None,
                DropLocalOffset))
        {
            if (UGridInventoryWidget* InventoryWidget = PartyPawn->GetInventoryWidget ())
            {
                InventoryWidget->RefreshInventory ();
            }
            SetGridInteractionCursor (EGridInteractionCursor::Default);
            return;
        }

        UGridItemDefinitionAsset* ItemDefinition = RuntimeActor
            ? RuntimeActor->ResolveRuntimeItemDefinition (CursorItem.ItemDefinitionId)
            : nullptr;
        if (!ItemDefinition || !ItemDefinition->bThrowable)
        {
            ShowInteractionFeedback (FText::FromString (TEXT ("Cet objet ne peut pas être lancé.")));
            SetGridInteractionCursor (EGridInteractionCursor::CannotPlaceItem);
            return;
        }

        const FVector ThrowStartLocation = PartyPawn->Camera
            ? PartyPawn->Camera->GetComponentLocation ()
            : PartyPawn->GetActorLocation ();
        FVector TargetOffset = WorldHitResult.ImpactPoint - ThrowStartLocation;
        const float TargetDistance = TargetOffset.Size ();
        if (TargetOffset.IsNearlyZero ())
        {
            ShowInteractionFeedback (FText::FromString (TEXT ("Impossible de déposer ou lancer ici.")));
            SetGridInteractionCursor (EGridInteractionCursor::CannotPlaceItem);
            return;
        }

        if (MaxThrowTargetDistance > 0.f && TargetDistance > MaxThrowTargetDistance)
        {
            ShowInteractionFeedback (FText::FromString (TEXT ("Cible trop éloignée.")));
            SetGridInteractionCursor (EGridInteractionCursor::CannotPlaceItem);
            return;
        }

        const EGridItemThrowMode ThrowMode = TargetDistance < ThrowDistanceThreshold
            ? EGridItemThrowMode::ShortToss
            : EGridItemThrowMode::Throw;
        const bool bThrown = PartyPawn->TryThrowOneCursorItem (TargetOffset, ThrowMode);
        if (UGridInventoryWidget* InventoryWidget = PartyPawn->GetInventoryWidget ())
        {
            InventoryWidget->RefreshInventory ();
        }
        if (!bThrown)
        {
            ShowInteractionFeedback (FText::FromString (TEXT ("Lancer impossible.")));
            SetGridInteractionCursor (EGridInteractionCursor::CannotPlaceItem);
            return;
        }

        SetGridInteractionCursor (EGridInteractionCursor::Default);
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
        ShowInteractionFeedback (FText::FromString (TEXT ("Hors de portée.")));
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
        AGridReceptacleActor* ReceptacleActor =
            ResolveReceptacleFromHitActor (InteractableActor);
        FGridItemInstance HeldItem;
        if (ReceptacleActor && ResolveHeldEquipmentItem (PartyPawn, HeldItem))
        {
            FGridReceptacleAcceptanceResult AcceptanceResult;
            ReceptacleActor->EvaluateItemAcceptance (HeldItem, AcceptanceResult, true);
            ShowInteractionFeedback (
                GetReceptacleRejectFeedbackText (AcceptanceResult.RejectReason));
            SetGridInteractionCursor (EGridInteractionCursor::CannotPlaceItem);
        }
        else
        {
            ShowInteractionFeedback (FText::FromString (TEXT ("Action impossible.")));
        }
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
        SetGridInteractionCursor (EGridInteractionCursor::Default);
        return;
    }

    const AGrimrockPartyPawn* PartyPawn = Cast<AGrimrockPartyPawn> (GetPawn ());
    FGridItemInstance CursorItem;
    if (PartyPawn && PartyPawn->GetCursorItem (CursorItem))
    {
        FHitResult WorldHitResult;
        if (!TryGetWorldHitUnderCursor (WorldHitResult))
        {
            SetGridInteractionCursor (EGridInteractionCursor::Default);
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
                : EGridInteractionCursor::CannotPlaceItem);
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
                SetGridInteractionCursor (EGridInteractionCursor::PlaceItem);
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
                : EGridInteractionCursor::Default));
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
        SetGridInteractionCursor (EGridInteractionCursor::Default);
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
