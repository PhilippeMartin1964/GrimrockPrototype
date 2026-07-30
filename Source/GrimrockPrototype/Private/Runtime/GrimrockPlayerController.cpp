#include "Runtime/GrimrockPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/Engine.h"
#include "Engine/EngineTypes.h"
#include "EngineUtils.h"
#include "InputCoreTypes.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridReceptacleActor.h"
#include "Runtime/GridWallLockActor.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "UI/GridInventoryWidget.h"

DEFINE_LOG_CATEGORY_STATIC (LogGridMouse, Log, All);
DEFINE_LOG_CATEGORY_STATIC(LogGridTurnManagerInput, Log, All);

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

    const TCHAR* GetReceptacleRejectReasonName (EGridReceptacleRejectReason Reason)
    {
        switch (Reason)
        {
        case EGridReceptacleRejectReason::InvalidItem:
            return TEXT ("InvalidItem");
        case EGridReceptacleRejectReason::Full:
            return TEXT ("Full");
        case EGridReceptacleRejectReason::ExplicitlyRejected:
            return TEXT ("ExplicitlyRejected");
        case EGridReceptacleRejectReason::NoMatchingAcceptanceRule:
            return TEXT ("NoMatchingAcceptanceRule");
        case EGridReceptacleRejectReason::None:
        default:
            return TEXT ("Unknown");
        }
    }

    bool IsHoverCursorReason (const TCHAR* Reason)
    {
        return Reason && FCString::Strncmp (Reason, TEXT ("Hover"), 5) == 0;
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

#if !UE_BUILD_SHIPPING
    FInputKeyBinding& StartPerceptionBinding = InputComponent->BindKey (
        EKeys::NumPadOne,
        IE_Pressed,
        this,
        &AGrimrockPlayerController::HandleMON5StartCombatFromPerception);
    StartPerceptionBinding.bConsumeInput = true;
    StartPerceptionBinding.bExecuteWhenPaused = false;

    FInputKeyBinding& EndPlayerPhaseBinding = InputComponent->BindKey (
        EKeys::NumPadTwo,
        IE_Pressed,
        this,
        &AGrimrockPlayerController::HandleMON5EndPlayerPhase);
    EndPlayerPhaseBinding.bConsumeInput = true;
    EndPlayerPhaseBinding.bExecuteWhenPaused = false;

    FInputKeyBinding& AbortCombatBinding = InputComponent->BindKey (
        EKeys::NumPadThree,
        IE_Pressed,
        this,
        &AGrimrockPlayerController::HandleMON5AbortCombat);
    AbortCombatBinding.bConsumeInput = true;
    AbortCombatBinding.bExecuteWhenPaused = false;

    FInputKeyBinding& LogTurnStateBinding = InputComponent->BindKey (
        EKeys::NumPadFour,
        IE_Pressed,
        this,
        &AGrimrockPlayerController::HandleMON5LogTurnState);
    LogTurnStateBinding.bConsumeInput = true;
    LogTurnStateBinding.bExecuteWhenPaused = false;

    FInputKeyBinding& StartAllBinding = InputComponent->BindKey (
        EKeys::NumPadFive,
        IE_Pressed,
        this,
        &AGrimrockPlayerController::HandleMON5StartCombatWithAllMonsters);
    StartAllBinding.bConsumeInput = true;
    StartAllBinding.bExecuteWhenPaused = false;

    FInputKeyBinding& ForceVictoryBinding = InputComponent->BindKey (
        EKeys::NumPadSix,
        IE_Pressed,
        this,
        &AGrimrockPlayerController::HandleMON5ForceVictory);
    ForceVictoryBinding.bConsumeInput = true;
    ForceVictoryBinding.bExecuteWhenPaused = false;

    FInputKeyBinding& RequestPlayerAttackBinding = InputComponent->BindKey (
        EKeys::NumPadSeven,
        IE_Pressed,
        this,
        &AGrimrockPlayerController::HandleMON11RequestSelectedCharacterAttack);
    RequestPlayerAttackBinding.bConsumeInput = true;
    RequestPlayerAttackBinding.bExecuteWhenPaused = false;

    UE_LOG (LogGridTurnManagerInput, Log,
        TEXT ("[GridTurnManagerInput] Bound NumPad 1-7 PlayerController=%s InputComponent=%s"),
        *GetNameSafe (this),
        *GetNameSafe (InputComponent));
#endif
}

#if !UE_BUILD_SHIPPING
UGridTurnManagerComponent* AGrimrockPlayerController::ResolveMON5TurnManager () const
{
    UWorld* World = GetWorld ();
    if (!World)
    {
        UE_LOG (LogGridTurnManagerInput, Warning,
            TEXT ("[GridTurnManagerInput] Cannot resolve TurnManager: World is null. PlayerController=%s"),
            *GetNameSafe (this));
        return nullptr;
    }

    const APawn* ControlledPawn = GetPawn ();
    const AGrimrockPartyPawn* PartyPawn = Cast<AGrimrockPartyPawn> (ControlledPawn);
    AGridLevelRuntimeActor* RuntimeActor = PartyPawn
        ? PartyPawn->LevelRuntimeActor.Get ()
        : nullptr;

    if (!PartyPawn)
    {
        UE_LOG (LogGridTurnManagerInput, Warning,
            TEXT ("[GridTurnManagerInput] Controlled pawn is not AGrimrockPartyPawn. Pawn=%s PlayerController=%s"),
            *GetNameSafe (ControlledPawn),
            *GetNameSafe (this));
    }

    if (!RuntimeActor)
    {
        for (TActorIterator<AGridLevelRuntimeActor> It (World); It; ++It)
        {
            RuntimeActor = *It;
            break;
        }
    }

    if (!RuntimeActor)
    {
        UE_LOG (LogGridTurnManagerInput, Warning,
            TEXT ("[GridTurnManagerInput] No GridLevelRuntimeActor could be resolved. Pawn=%s PlayerController=%s"),
            *GetNameSafe (ControlledPawn),
            *GetNameSafe (this));
        return nullptr;
    }

    UGridTurnManagerComponent* TurnManager =
        RuntimeActor->FindComponentByClass<UGridTurnManagerComponent> ();
    if (!TurnManager)
    {
        UE_LOG (LogGridTurnManagerInput, Warning,
            TEXT ("[GridTurnManagerInput] RuntimeActor=%s has no GridTurnManagerComponent."),
            *GetNameSafe (RuntimeActor));
    }
    return TurnManager;
}

void AGrimrockPlayerController::LogMON5CommandResult (
    const TCHAR* CommandName,
    bool bSucceeded) const
{
    const FString Message = FString::Printf (
        TEXT ("[GridTurnManagerInput] %s=%s"),
        CommandName ? CommandName : TEXT ("UnknownCommand"),
        bSucceeded ? TEXT ("true") : TEXT ("false"));

    UE_LOG (LogGridTurnManagerInput, Log, TEXT ("%s"), *Message);
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage (
            INDEX_NONE,
            1.50f,
            bSucceeded ? FColor::Green : FColor::Red,
            Message);
    }
}

void AGrimrockPlayerController::HandleMON5StartCombatFromPerception ()
{
    UGridTurnManagerComponent* TurnManager = ResolveMON5TurnManager ();
    LogMON5CommandResult (
        TEXT ("StartCombatFromPerception"),
        TurnManager && TurnManager->StartCombatFromPerception ());
}

void AGrimrockPlayerController::HandleMON5EndPlayerPhase ()
{
    UGridTurnManagerComponent* TurnManager = ResolveMON5TurnManager ();
    LogMON5CommandResult (
        TEXT ("EndPlayerPhase"),
        TurnManager && TurnManager->EndPlayerPhase ());
}

void AGrimrockPlayerController::HandleMON5AbortCombat ()
{
    UGridTurnManagerComponent* TurnManager = ResolveMON5TurnManager ();
    if (TurnManager)
    {
        TurnManager->AbortCombat ();
    }
    LogMON5CommandResult (
        TEXT ("AbortCombat"),
        TurnManager && TurnManager->CurrentPhase == EGridCombatPhase::Exploration);
}

void AGrimrockPlayerController::HandleMON5LogTurnState ()
{
    UGridTurnManagerComponent* TurnManager = ResolveMON5TurnManager ();
    if (TurnManager)
    {
        TurnManager->LogCurrentTurnState ();
    }
    LogMON5CommandResult (TEXT ("LogCurrentTurnState"), TurnManager != nullptr);
}

void AGrimrockPlayerController::HandleMON5StartCombatWithAllMonsters ()
{
    UGridTurnManagerComponent* TurnManager = ResolveMON5TurnManager ();
    LogMON5CommandResult (
        TEXT ("StartCombatWithAllMonsters"),
        TurnManager && TurnManager->StartCombatWithAllMonsters ());
}

void AGrimrockPlayerController::HandleMON5ForceVictory ()
{
    UGridTurnManagerComponent* TurnManager = ResolveMON5TurnManager ();
    if (TurnManager)
    {
        TurnManager->ForceVictory ();
    }
    LogMON5CommandResult (
        TEXT ("ForceVictory"),
        TurnManager && TurnManager->CurrentPhase == EGridCombatPhase::Victory);
}

void AGrimrockPlayerController::HandleMON11RequestSelectedCharacterAttack ()
{
    UGridTurnManagerComponent* TurnManager = ResolveMON5TurnManager ();
    FGridPlayerAttackRequest Request;
    FGridAttackResult Result;
    EGridPlayerAttackRejectReason RejectReason =
        EGridPlayerAttackRejectReason::TurnManagerNotInitialized;
    const bool bAccepted = TurnManager &&
        TurnManager->RequestSelectedCharacterAttack (
            Request,
            Result,
            RejectReason);
    const FString ReasonText = StaticEnum<EGridPlayerAttackRejectReason> ()
        ? StaticEnum<EGridPlayerAttackRejectReason> ()->GetNameStringByValue (
            static_cast<int64> (RejectReason))
        : TEXT ("Unknown");
    const FString Message = FString::Printf (
        TEXT ("[GridPlayerAttack] Accepted=%s Reason=%s Attacker=%d Attack=%s Item=%s Slot=%s Range=%d Target=%s TargetCell=(%d,%d) DamageType=%s PhysicalSubtype=%s Natural=%d Roll=%d Defense=%d Hit=%s Critical=%s RawDamage=%d ArmorPhysical=%d ArmorMagical=%d HealthDamage=%d Health=%d->%d TargetDefeated=%s"),
        bAccepted ? TEXT ("true") : TEXT ("false"),
        *ReasonText,
        Request.AttackerCharacterIndex,
        *Request.AttackId.ToString (),
        *Request.OffensiveItemDefinitionId.ToString (),
        *UEnum::GetValueAsString (Request.OffensiveEquipmentSlot),
        Request.RangeCells,
        *Request.TargetMonsterId.ToString (EGuidFormats::Digits),
        Request.TargetCell.X,
        Request.TargetCell.Y,
        *UEnum::GetValueAsString (Result.DamageType),
        *UEnum::GetValueAsString (Result.PhysicalSubtype),
        Result.NaturalAttackRoll,
        Result.AttackRoll,
        Result.DefenseValue,
        Result.bHit ? TEXT ("true") : TEXT ("false"),
        Result.bCriticalHit ? TEXT ("true") : TEXT ("false"),
        Result.RawDamage,
        Result.PhysicalArmorDamage,
        Result.MagicalArmorDamage,
        Result.HealthDamage,
        Result.TargetHealthBefore,
        Result.TargetHealthAfter,
        Result.TargetHealthBefore > 0 &&
            Result.TargetHealthAfter <= 0
                ? TEXT ("true")
                : TEXT ("false"));

    UE_LOG (LogGridTurnManagerInput, Log, TEXT ("%s"), *Message);
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage (
            INDEX_NONE,
            2.0f,
            bAccepted ? FColor::Green : FColor::Red,
            Message);
    }
}
#endif

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

    const UGridInventoryWidget* InventoryWidget = Resolution.PartyPawn
        ? Resolution.PartyPawn->GetInventoryWidget ()
        : nullptr;
    Resolution.bItemActionMenuOpen = InventoryWidget && InventoryWidget->IsItemActionMenuOpen ();
    if (bInventoryUiOpen && !Resolution.bHasCursorItem && Resolution.bItemActionMenuOpen)
    {
        Resolution.Intent = EGridMouseInteractionIntent::IgnoreModalUi;
        Resolution.DiagnosticReason = TEXT ("ItemActionMenuOpen");
        return Resolution;
    }

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
        if (Resolution.ReceptacleActor)
        {
            if (Resolution.bWithinInteractionDistance)
            {
                Resolution.bReceptacleAccessible = RuntimeActor && RuntimeActor->CanPartyInteractWithEdgeObject (
                    Resolution.ReceptacleActor->CellX,
                    Resolution.ReceptacleActor->CellY,
                    Resolution.ReceptacleActor->Edge,
                    Resolution.PartyPawn);
            }
            Resolution.Intent = Resolution.WallLockActor
                ? EGridMouseInteractionIntent::CursorItemWallLock
                : EGridMouseInteractionIntent::CursorItemReceptacle;
            Resolution.DiagnosticReason = Resolution.bReceptacleAccessible
                ? (Resolution.WallLockActor ? TEXT ("WallLockCandidate") : TEXT ("ReceptacleCandidate"))
                : (Resolution.bWithinInteractionDistance ? TEXT ("ReceptacleInaccessible") : TEXT ("ReceptacleOutOfRange"));
            return Resolution;
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
            UE_LOG (LogGridMouse, Log,
                TEXT ("GridMouse Click Priority=ReadableMessage Result=Dismissed"));
        }
        return;
    }

    if (MouseResolution.Intent == EGridMouseInteractionIntent::IgnoreModalUi)
    {
        UE_LOG (LogGridMouse, Log,
            TEXT ("GridMouse Click Priority=ModalUI Result=Ignored Reason=ItemActionMenuOpen"));
        if (bDebugMouseInteraction)
        {
            UE_LOG (LogTemp, Verbose, TEXT ("Mouse interaction ignored: item action menu open."));
        }
        return;
    }

    if (MouseResolution.Intent == EGridMouseInteractionIntent::IgnoreInventoryUiWithoutCursorItem)
    {
        UE_LOG (LogGridMouse, Log,
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
        UE_LOG (LogGridMouse, Log,
            TEXT ("GridMouse Click Priority=CursorItem Item=%s RuntimeId=%s"),
            *CursorItem.ItemDefinitionId.ToString (),
            *CursorItem.RuntimeObjectId.ToString ());

        if (MouseResolution.Intent == EGridMouseInteractionIntent::CursorItemNoWorldHit)
        {
            UE_LOG (LogGridMouse, Log,
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

        if (ReceptacleActor)
        {
            UE_LOG (LogGridMouse, Log,
                TEXT ("GridMouse Click Priority=CursorItem Branch=ReceptacleCandidate Item=%s Target=%s HitActor=%s WithinDistance=%s Accessible=%s"),
                *CursorItem.ItemDefinitionId.ToString (),
                *GetNameSafe (ReceptacleActor),
                *GetNameSafe (WorldHitResult.GetActor ()),
                bWithinInteractionDistance ? TEXT ("true") : TEXT ("false"),
                MouseResolution.bReceptacleAccessible ? TEXT ("true") : TEXT ("false"));

            if (MouseResolution.bReceptacleAccessible)
            {
                if (MouseResolution.Intent == EGridMouseInteractionIntent::CursorItemWallLock)
                {
                    const bool bWasUnlocked = WallLockActor && WallLockActor->bIsUnlocked;
                    const bool bAcceptedKey = WallLockActor && WallLockActor->CanAcceptKeyDefinition (CursorItem.ItemDefinitionId);
                    UE_LOG (LogGridMouse, Log,
                        TEXT ("GridMouse Click Priority=CursorItem Branch=WallLockAttempt Item=%s Target=%s Result=Attempt AcceptedKey=%s AlreadyUnlocked=%s"),
                        *CursorItem.ItemDefinitionId.ToString (),
                        *GetNameSafe (WallLockActor),
                        bAcceptedKey ? TEXT ("true") : TEXT ("false"),
                        bWasUnlocked ? TEXT ("true") : TEXT ("false"));
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
                    const bool bNowUnlocked = WallLockActor && WallLockActor->bIsUnlocked;
                    if (!bWasUnlocked && bNowUnlocked)
                    {
                        UE_LOG (LogGridMouse, Log,
                            TEXT ("GridMouse Click Priority=CursorItem Branch=WallLockAttempt Item=%s Target=%s Result=Inserted"),
                            *CursorItem.ItemDefinitionId.ToString (),
                            *GetNameSafe (WallLockActor));
                    }
                    else
                    {
                        UGridItemDefinitionAsset* CursorItemDefinition = RuntimeActor
                            ? RuntimeActor->ResolveRuntimeItemDefinition (CursorItem.ItemDefinitionId)
                            : nullptr;
                        const TCHAR* RejectReason = bWasUnlocked
                            ? TEXT ("AlreadyUnlocked")
                            : (bAcceptedKey
                                ? TEXT ("InsertFailed")
                                : (CursorItemDefinition && CursorItemDefinition->ItemType != EGridItemType::Key
                                    ? TEXT ("NonKeyItem")
                                    : TEXT ("IncompatibleKey")));
                        UE_LOG (LogGridMouse, Log,
                            TEXT ("GridMouse Click Priority=CursorItem Branch=WallLockAttempt Item=%s Target=%s Result=Rejected Reason=%s"),
                            *CursorItem.ItemDefinitionId.ToString (),
                            *GetNameSafe (WallLockActor),
                            RejectReason);
                    }
                    SetGridInteractionCursor (EGridInteractionCursor::Default, TEXT ("ClickWallLockAttemptComplete"));
                    return;
                }

                UE_LOG (LogGridMouse, Log,
                    TEXT ("GridMouse Click Priority=CursorItem Branch=ReceptacleAttempt Item=%s Target=%s Result=Validate"),
                    *CursorItem.ItemDefinitionId.ToString (),
                    *GetNameSafe (ReceptacleActor));
                FGridReceptacleAcceptanceResult AcceptanceResult;
                if (!ReceptacleActor->EvaluateItemAcceptance (CursorItem, AcceptanceResult, true))
                {
                    UE_LOG (LogGridMouse, Log,
                        TEXT ("GridMouse Click Priority=CursorItem Branch=ReceptacleAttempt Item=%s Target=%s Result=Rejected Reason=%s"),
                        *CursorItem.ItemDefinitionId.ToString (),
                        *GetNameSafe (ReceptacleActor),
                        GetReceptacleRejectReasonName (AcceptanceResult.RejectReason));
                    ShowInteractionFeedback (
                        GetReceptacleRejectFeedbackText (AcceptanceResult.RejectReason));
                    SetGridInteractionCursor (EGridInteractionCursor::CannotPlaceItem, TEXT ("ClickReceptacleRejected"));
                    return;
                }

                UE_LOG (LogTemp, Log, TEXT ("GridInventory WorldDrop Attempt Item=%s Target=%s"),
                    *CursorItem.ItemDefinitionId.ToString (),
                    *GetNameSafe (ReceptacleActor));

                const bool bPlaced = ReceptacleActor->TryPlaceCursorItemFromHit (PartyPawn, WorldHitResult);
                UE_LOG (LogGridMouse, Log,
                    TEXT ("GridMouse Click Priority=CursorItem Branch=ReceptacleAttempt Item=%s Target=%s Result=%s%s"),
                    *CursorItem.ItemDefinitionId.ToString (),
                    *GetNameSafe (ReceptacleActor),
                    bPlaced ? TEXT ("Placed") : TEXT ("Rejected"),
                    bPlaced ? TEXT ("") : TEXT (" Reason=PlaceFailed"));
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

            if (WallLockActor)
            {
                UE_LOG (LogGridMouse, Log,
                    TEXT ("GridMouse Click Priority=CursorItem Branch=WallLockAttempt Item=%s Target=%s Result=Rejected Reason=%s"),
                    *CursorItem.ItemDefinitionId.ToString (),
                    *GetNameSafe (WallLockActor),
                    bWithinInteractionDistance ? TEXT ("EdgeInaccessible") : TEXT ("OutOfRange"));
            }
            else
            {
                UE_LOG (LogGridMouse, Log,
                    TEXT ("GridMouse Click Priority=CursorItem Branch=ReceptacleAttempt Item=%s Target=%s Result=Rejected Reason=%s"),
                    *CursorItem.ItemDefinitionId.ToString (),
                    *GetNameSafe (ReceptacleActor),
                    bWithinInteractionDistance ? TEXT ("EdgeInaccessible") : TEXT ("OutOfRange"));
            }
            ShowInteractionFeedback (FText::FromString (TEXT ("Cible hors de port\u00E9e.")));
            SetGridInteractionCursor (EGridInteractionCursor::CannotPlaceItem, TEXT ("ClickReceptacleInaccessible"));
            return;
        }

        UE_LOG (LogGridMouse, Log,
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
            UE_LOG (LogGridMouse, Log,
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
        UE_LOG (LogGridMouse, Log,
            TEXT ("GridMouse Click Priority=CursorItem Branch=ThrowAttempt Item=%s Throwable=%s"),
            *CursorItem.ItemDefinitionId.ToString (),
            ItemDefinition && ItemDefinition->bThrowable ? TEXT ("true") : TEXT ("false"));
        if (!ItemDefinition || !ItemDefinition->bThrowable)
        {
            UE_LOG (LogGridMouse, Log,
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
            UE_LOG (LogGridMouse, Log,
                TEXT ("GridMouse Click Priority=CursorItem Branch=ThrowAttempt Item=%s Result=InvalidTargetOffset"),
                *CursorItem.ItemDefinitionId.ToString ());
            ShowInteractionFeedback (FText::FromString (TEXT ("Impossible de d\u00E9poser ou lancer ici.")));
            SetGridInteractionCursor (EGridInteractionCursor::CannotPlaceItem, TEXT ("ClickThrowInvalidOffset"));
            return;
        }

        if (MaxThrowTargetDistance > 0.f && TargetDistance > MaxThrowTargetDistance)
        {
            UE_LOG (LogGridMouse, Log,
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
        UE_LOG (LogGridMouse, Log,
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
        UE_LOG (LogGridMouse, Log,
            TEXT ("GridMouse Click Priority=WorldInteractable Result=None Fallback=NoInteractable"));
        if (bDebugMouseInteraction)
        {
            UE_LOG (LogTemp, Verbose, TEXT ("Mouse interaction: no interactable under cursor."));
        }
        return;
    }

    if (MouseResolution.Intent == EGridMouseInteractionIntent::WorldInteractableOutOfRange)
    {
        UE_LOG (LogGridMouse, Log,
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
        UE_LOG (LogGridMouse, Log,
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
        UE_LOG (LogGridMouse, Log,
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

    UE_LOG (LogGridMouse, Log,
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
    const AGrimrockPartyPawn* PartyPawn = Cast<AGrimrockPartyPawn> (GetPawn ());
    FGridItemInstance CursorItem;
    const bool bHasCursorItem = PartyPawn && PartyPawn->GetCursorItem (CursorItem);
    if (bInventoryUiOpen && !bHasCursorItem)
    {
        SetGridInteractionCursor (EGridInteractionCursor::Default, TEXT ("HoverInventoryUiOpen"));
        return;
    }

    if (bHasCursorItem)
    {
        const FGridMouseInteractionResolution MouseResolution = ResolveLeftMouseInteraction ();
        EGridInteractionCursor HoverCursor = EGridInteractionCursor::Default;
        const TCHAR* HoverReason = TEXT ("HoverCursorItemCannotPlace");
        if (ResolveCursorItemHoverCursor (MouseResolution, HoverCursor, HoverReason))
        {
            SetGridInteractionCursor (HoverCursor, HoverReason);
        }
        else
        {
            SetGridInteractionCursor (EGridInteractionCursor::Default, TEXT ("HoverCursorItemCannotPlace"));
        }
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

bool AGrimrockPlayerController::ResolveCursorItemHoverCursor (
    const FGridMouseInteractionResolution& MouseResolution,
    EGridInteractionCursor& OutCursor,
    const TCHAR*& OutReason) const
{
    if (!MouseResolution.bHasCursorItem)
    {
        return false;
    }

    switch (MouseResolution.Intent)
    {
    case EGridMouseInteractionIntent::CursorItemNoWorldHit:
        OutCursor = EGridInteractionCursor::Default;
        OutReason = TEXT ("HoverCursorItemNoWorldHit");
        return true;

    case EGridMouseInteractionIntent::CursorItemWallLock:
        OutCursor = MouseResolution.bReceptacleAccessible
            ? EGridInteractionCursor::PlaceItem
            : EGridInteractionCursor::CannotPlaceItem;
        OutReason = MouseResolution.bReceptacleAccessible
            ? TEXT ("HoverCursorItemWallLock")
            : TEXT ("HoverCursorItemWallLockInaccessible");
        return true;

    case EGridMouseInteractionIntent::CursorItemReceptacle:
        if (MouseResolution.bReceptacleAccessible &&
            MouseResolution.ReceptacleActor &&
            MouseResolution.ReceptacleActor->CanAcceptItemInstance (MouseResolution.CursorItem))
        {
            OutCursor = EGridInteractionCursor::PlaceItem;
            OutReason = TEXT ("HoverCursorItemReceptacle");
        }
        else
        {
            OutCursor = EGridInteractionCursor::CannotPlaceItem;
            OutReason = TEXT ("HoverCursorItemCannotPlace");
        }
        return true;

    case EGridMouseInteractionIntent::CursorItemWorldDrop:
        OutCursor = EGridInteractionCursor::PlaceItem;
        OutReason = TEXT ("HoverCursorItemWorldDrop");
        return true;

    case EGridMouseInteractionIntent::CursorItemThrow:
        {
            const AGrimrockPartyPawn* PartyPawn = MouseResolution.PartyPawn;
            const AGridLevelRuntimeActor* RuntimeActor = PartyPawn
                ? PartyPawn->LevelRuntimeActor.Get ()
                : nullptr;
            const UGridItemDefinitionAsset* ItemDefinition = RuntimeActor
                ? RuntimeActor->ResolveRuntimeItemDefinition (MouseResolution.CursorItem.ItemDefinitionId)
                : nullptr;
            const FVector ThrowStartLocation = PartyPawn && PartyPawn->Camera
                ? PartyPawn->Camera->GetComponentLocation ()
                : (PartyPawn ? PartyPawn->GetActorLocation () : FVector::ZeroVector);
            const FVector TargetOffset = MouseResolution.HitResult.ImpactPoint - ThrowStartLocation;
            const float TargetDistance = TargetOffset.Size ();
            const bool bCanAimThrow =
                MouseResolution.bHasWorldHit &&
                ItemDefinition &&
                ItemDefinition->bThrowable &&
                !TargetOffset.IsNearlyZero () &&
                (MaxThrowTargetDistance <= 0.f || TargetDistance <= MaxThrowTargetDistance);

            OutCursor = bCanAimThrow
                ? EGridInteractionCursor::AimThrow
                : EGridInteractionCursor::CannotPlaceItem;
            OutReason = bCanAimThrow
                ? TEXT ("HoverCursorItemThrow")
                : TEXT ("HoverCursorItemCannotPlace");
            return true;
        }

    default:
        OutCursor = EGridInteractionCursor::CannotPlaceItem;
        OutReason = TEXT ("HoverCursorItemCannotPlace");
        return true;
    }
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
        if (IsHoverCursorReason (Reason))
        {
            UE_LOG (LogGridMouse, Verbose,
                TEXT ("GridMouse Hover CursorChanged From=%s To=%s Reason=%s"),
                *UEnum::GetValueAsString (PreviousCursor),
                *UEnum::GetValueAsString (NewCursor),
                Reason ? Reason : TEXT ("None"));
        }
        else
        {
            UE_LOG (LogGridMouse, Log,
                TEXT ("GridMouse Hover CursorChanged From=%s To=%s Reason=%s"),
                *UEnum::GetValueAsString (PreviousCursor),
                *UEnum::GetValueAsString (NewCursor),
                Reason ? Reason : TEXT ("None"));
        }
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
