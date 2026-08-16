#include "RPG/RPGLevelUpNotificationSubsystem.h"

#include "Blueprint/UserWidget.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "RPG/RPGClassProgressionTransactionService.h"
#include "RPG/RPGLevelUpService.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "UI/RPGLevelUpWidget.h"

DEFINE_LOG_CATEGORY_STATIC (LogGridLevelUpUI, Log, All);

void URPGLevelUpNotificationSubsystem::Initialize (
    FSubsystemCollectionBase& Collection)
{
    Super::Initialize (Collection);
    LevelUpDelegateHandle =
        FRPGLevelUpService::OnCharacterLevelUpAppliedWithSource ()
            .AddUObject (
                this,
                &URPGLevelUpNotificationSubsystem::
                    HandleCharacterLevelUpApplied);
}

void URPGLevelUpNotificationSubsystem::Deinitialize ()
{
    if (LevelUpDelegateHandle.IsValid ())
    {
        FRPGLevelUpService::OnCharacterLevelUpAppliedWithSource ()
            .Remove (LevelUpDelegateHandle);
        LevelUpDelegateHandle.Reset ();
    }

    ClearDeferredCombatTurnManager ();

    if (IsValid (ActiveWidget))
    {
        if (WidgetClosedDelegateHandle.IsValid ())
        {
            ActiveWidget->OnClosed ().Remove (WidgetClosedDelegateHandle);
            WidgetClosedDelegateHandle.Reset ();
        }
        ActiveWidget->CancelSelection ();
        ActiveWidget = nullptr;
    }
    PendingNotifications.Reset ();
    Super::Deinitialize ();
}

int32 URPGLevelUpNotificationSubsystem::GetPendingLevelUpNotificationCount () const
{
    return PendingNotifications.Num () + (IsValid (ActiveWidget) ? 1 : 0);
}

bool URPGLevelUpNotificationSubsystem::IsLevelUpModalOpen () const
{
    return IsValid (ActiveWidget) && ActiveWidget->IsInViewport ();
}

void URPGLevelUpNotificationSubsystem::HandleCharacterLevelUpApplied (
    UGridPartyInventoryComponent* PartyInventoryComponent,
    int32 CharacterIndex,
    int32 PreviousLevel,
    int32 NewLevel,
    int32 LevelsGained)
{
    if (!IsValid (PartyInventoryComponent) ||
        !PartyInventoryComponent->IsValidCharacterIndex (CharacterIndex))
    {
        return;
    }

    UWorld* SourceWorld = PartyInventoryComponent->GetWorld ();
    if (!SourceWorld ||
        SourceWorld->GetGameInstance () != GetGameInstance ())
    {
        return;
    }

    FRPGClassProgressionTransactionService::RefreshCharacterProjection (
        PartyInventoryComponent,
        CharacterIndex);

    FPendingNotification Notification;
    Notification.InventoryComponent = PartyInventoryComponent;
    Notification.CharacterIndex = CharacterIndex;
    Notification.PreviousLevel = PreviousLevel;
    Notification.NewLevel = NewLevel;
    Notification.LevelsGained = LevelsGained;
    PendingNotifications.Add (Notification);

    UE_LOG (
        LogGridLevelUpUI,
        Log,
        TEXT ("[GridLevelUpUI] Queued Character=%d Previous=%d New=%d Gained=%d Pending=%d"),
        CharacterIndex,
        PreviousLevel,
        NewLevel,
        LevelsGained,
        PendingNotifications.Num ());
    TryPresentNextNotification ();
}

void URPGLevelUpNotificationSubsystem::HandleWidgetClosed (
    URPGLevelUpWidget* ClosedWidget)
{
    if (ClosedWidget != ActiveWidget)
    {
        return;
    }
    if (WidgetClosedDelegateHandle.IsValid ())
    {
        ClosedWidget->OnClosed ().Remove (WidgetClosedDelegateHandle);
        WidgetClosedDelegateHandle.Reset ();
    }
    ActiveWidget = nullptr;
    TryPresentNextNotification ();
}

void URPGLevelUpNotificationSubsystem::HandleDeferredCombatEnded (
    EGridCombatPhase ResultPhase)
{
    UE_LOG (
        LogGridLevelUpUI,
        Log,
        TEXT ("[GridLevelUpUI] CombatSafePoint Result=%s Pending=%d"),
        *UEnum::GetValueAsString (ResultPhase),
        PendingNotifications.Num ());

    ClearDeferredCombatTurnManager ();
    TryPresentNextNotification ();
}

void URPGLevelUpNotificationSubsystem::SetDeferredCombatTurnManager (
    UGridTurnManagerComponent* TurnManager)
{
    if (DeferredCombatTurnManager == TurnManager)
    {
        return;
    }

    ClearDeferredCombatTurnManager ();
    if (!IsValid (TurnManager))
    {
        return;
    }

    DeferredCombatTurnManager = TurnManager;
    TurnManager->OnCombatEnded.AddUniqueDynamic (
        this,
        &URPGLevelUpNotificationSubsystem::HandleDeferredCombatEnded);
}

void URPGLevelUpNotificationSubsystem::ClearDeferredCombatTurnManager ()
{
    if (IsValid (DeferredCombatTurnManager))
    {
        DeferredCombatTurnManager->OnCombatEnded.RemoveDynamic (
            this,
            &URPGLevelUpNotificationSubsystem::HandleDeferredCombatEnded);
    }
    DeferredCombatTurnManager = nullptr;
}

void URPGLevelUpNotificationSubsystem::TryPresentNextNotification ()
{
    if (IsValid (ActiveWidget))
    {
        return;
    }

    while (!PendingNotifications.IsEmpty ())
    {
        const FPendingNotification Notification = PendingNotifications[0];
        UGridPartyInventoryComponent* InventoryComponent =
            Notification.InventoryComponent.Get ();
        if (!IsValid (InventoryComponent) ||
            !InventoryComponent->IsValidCharacterIndex (
                Notification.CharacterIndex))
        {
            PendingNotifications.RemoveAt (0);
            continue;
        }

        APawn* PartyPawn = Cast<APawn> (InventoryComponent->GetOwner ());
        APlayerController* PlayerController = PartyPawn
            ? Cast<APlayerController> (PartyPawn->GetController ())
            : nullptr;
        if (!IsValid (PlayerController) ||
            !PlayerController->IsLocalController ())
        {
            PendingNotifications.RemoveAt (0);
            UE_LOG (
                LogGridLevelUpUI,
                Verbose,
                TEXT ("[GridLevelUpUI] Presentation skipped Character=%d Reason=NoLocalPlayerController"),
                Notification.CharacterIndex);
            continue;
        }

        UGridTurnManagerComponent* TurnManager = PartyPawn
            ? PartyPawn->FindComponentByClass<UGridTurnManagerComponent> ()
            : nullptr;
        if (IsValid (TurnManager) && TurnManager->bCombatActive)
        {
            const bool bNewDeferredManager =
                DeferredCombatTurnManager != TurnManager;
            SetDeferredCombatTurnManager (TurnManager);
            if (bNewDeferredManager)
            {
                UE_LOG (
                    LogGridLevelUpUI,
                    Log,
                    TEXT ("[GridLevelUpUI] Deferred Character=%d Previous=%d New=%d Reason=CombatActive Phase=%s"),
                    Notification.CharacterIndex,
                    Notification.PreviousLevel,
                    Notification.NewLevel,
                    *UEnum::GetValueAsString (TurnManager->CurrentPhase));
            }
            return;
        }

        ClearDeferredCombatTurnManager ();
        PendingNotifications.RemoveAt (0);

        ActiveWidget = CreateWidget<URPGLevelUpWidget> (
            PlayerController,
            URPGLevelUpWidget::StaticClass ());
        if (!IsValid (ActiveWidget) ||
            !ActiveWidget->InitializeLevelUpWidget (
                InventoryComponent,
                Notification.CharacterIndex,
                Notification.PreviousLevel,
                Notification.NewLevel))
        {
            ActiveWidget = nullptr;
            continue;
        }

        WidgetClosedDelegateHandle = ActiveWidget->OnClosed ().AddUObject (
            this,
            &URPGLevelUpNotificationSubsystem::HandleWidgetClosed);
        ActiveWidget->AddToViewport (200);

        UE_LOG (
            LogGridLevelUpUI,
            Log,
            TEXT ("[GridLevelUpUI] Opened Character=%d Previous=%d New=%d"),
            Notification.CharacterIndex,
            Notification.PreviousLevel,
            Notification.NewLevel);
        return;
    }

    ClearDeferredCombatTurnManager ();
}