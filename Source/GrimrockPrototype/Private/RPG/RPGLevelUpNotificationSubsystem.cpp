#include "RPG/RPGLevelUpNotificationSubsystem.h"

#include "Blueprint/UserWidget.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "RPG/RPGClassProgressionTransactionService.h"
#include "RPG/RPGLevelUpService.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "TimerManager.h"
#include "UI/RPGLevelUpWidget.h"

DEFINE_LOG_CATEGORY_STATIC (LogGridLevelUpUI, Log, All);

namespace
{
    TArray<FRPGPendingLevelUpSaveState> PersistentNotificationMirror;
    TArray<TWeakObjectPtr<URPGLevelUpNotificationSubsystem>> SubsystemInstances;

    int32 FindCharacterIndexById (
        const FGridPartyInventoryState& PartyState,
        const FGuid& CharacterId)
    {
        for (int32 CharacterIndex = 0;
            CharacterIndex < PartyState.ActiveCharacters.Num ();
            ++CharacterIndex)
        {
            if (PartyState.ActiveCharacters[CharacterIndex].CharacterId ==
                CharacterId)
            {
                return CharacterIndex;
            }
        }
        return INDEX_NONE;
    }

    void UpsertPersistentMirror (
        const FRPGPendingLevelUpSaveState& Incoming)
    {
        FRPGPendingLevelUpSaveState* Existing =
            PersistentNotificationMirror.FindByPredicate (
                [&Incoming] (const FRPGPendingLevelUpSaveState& Candidate)
                {
                    return Candidate.CharacterId == Incoming.CharacterId;
                });
        if (!Existing)
        {
            PersistentNotificationMirror.Add (Incoming);
            return;
        }

        Existing->PreviousLevel = FMath::Min (
            Existing->PreviousLevel,
            Incoming.PreviousLevel);
        Existing->NewLevel = FMath::Max (
            Existing->NewLevel,
            Incoming.NewLevel);
        Existing->LevelsGained = FMath::Max (
            0,
            Existing->NewLevel - Existing->PreviousLevel);
    }

    void RemovePersistentMirror (const FGuid& CharacterId)
    {
        PersistentNotificationMirror.RemoveAll (
            [&CharacterId] (const FRPGPendingLevelUpSaveState& Candidate)
            {
                return Candidate.CharacterId == CharacterId;
            });
    }

    UGridTurnManagerComponent* ResolveTurnManagerForParty (
        AGrimrockPartyPawn* PartyPawn)
    {
        if (!IsValid (PartyPawn))
        {
            return nullptr;
        }

        if (IsValid (PartyPawn->LevelRuntimeActor))
        {
            if (UGridTurnManagerComponent* TurnManager =
                    PartyPawn->LevelRuntimeActor
                        ->FindComponentByClass<UGridTurnManagerComponent> ())
            {
                if (!IsValid (TurnManager->PartyPawn) ||
                    TurnManager->PartyPawn == PartyPawn)
                {
                    return TurnManager;
                }
            }
        }

        UWorld* World = PartyPawn->GetWorld ();
        if (!World)
        {
            return nullptr;
        }

        for (TActorIterator<AGridLevelRuntimeActor> It (World); It; ++It)
        {
            AGridLevelRuntimeActor* RuntimeActor = *It;
            UGridTurnManagerComponent* TurnManager = RuntimeActor
                ? RuntimeActor
                    ->FindComponentByClass<UGridTurnManagerComponent> ()
                : nullptr;
            if (IsValid (TurnManager) &&
                (!IsValid (TurnManager->PartyPawn) ||
                    TurnManager->PartyPawn == PartyPawn))
            {
                return TurnManager;
            }
        }

        return nullptr;
    }
}

void URPGLevelUpNotificationSubsystem::Initialize (
    FSubsystemCollectionBase& Collection)
{
    Super::Initialize (Collection);
    SubsystemInstances.AddUnique (this);

    LevelUpDelegateHandle =
        FRPGLevelUpService::OnCharacterLevelUpAppliedWithSource ()
            .AddUObject (
                this,
                &URPGLevelUpNotificationSubsystem::
                    HandleCharacterLevelUpApplied);

    if (!PersistentNotificationMirror.IsEmpty ())
    {
        HandlePersistentStateRestored (PersistentNotificationMirror);
    }
}

void URPGLevelUpNotificationSubsystem::Deinitialize ()
{
    SubsystemInstances.RemoveAll (
        [this] (const TWeakObjectPtr<URPGLevelUpNotificationSubsystem>& Entry)
        {
            return !Entry.IsValid () || Entry.Get () == this;
        });

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
    ActiveNotification.Reset ();
    PendingNotifications.Reset ();
    PendingPersistentRestoreStates.Reset ();
    PersistentRestoreRetryCount = 0;
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

bool URPGLevelUpNotificationSubsystem::CapturePersistentState (
    const FGridPartyInventoryState& PartyState,
    TArray<FRPGPendingLevelUpSaveState>& OutStates,
    FText& OutError)
{
    OutStates.Reset ();
    OutError = FText::GetEmpty ();

    TSet<FGuid> ActiveCharacterIds;
    for (const FGridCharacterInventoryState& Character :
        PartyState.ActiveCharacters)
    {
        if (!Character.CharacterId.IsValid () ||
            ActiveCharacterIds.Contains (Character.CharacterId))
        {
            OutError = FText::FromString (
                TEXT ("Impossible de capturer les notifications Level Up : CharacterId actif invalide ou dupliqué."));
            return false;
        }
        ActiveCharacterIds.Add (Character.CharacterId);
    }

    TSet<FGuid> CapturedIds;
    for (const FRPGPendingLevelUpSaveState& PersistentState :
        PersistentNotificationMirror)
    {
        if (!ActiveCharacterIds.Contains (PersistentState.CharacterId))
        {
            continue;
        }
        if (!PersistentState.CharacterId.IsValid () ||
            CapturedIds.Contains (PersistentState.CharacterId))
        {
            OutError = FText::FromString (
                TEXT ("Le miroir des notifications Level Up contient un CharacterId invalide ou dupliqué."));
            OutStates.Reset ();
            return false;
        }
        CapturedIds.Add (PersistentState.CharacterId);
        OutStates.Add (PersistentState);
    }
    return true;
}

void URPGLevelUpNotificationSubsystem::RestorePersistentState (
    const TArray<FRPGPendingLevelUpSaveState>& SavedStates)
{
    PersistentNotificationMirror = SavedStates;

    for (auto It = SubsystemInstances.CreateIterator (); It; ++It)
    {
        if (!It->IsValid ())
        {
            It.RemoveCurrent ();
            continue;
        }
        It->Get ()->HandlePersistentStateRestored (SavedStates);
    }
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

    const FGridCharacterInventoryState& Character =
        PartyInventoryComponent->PartyInventoryState.ActiveCharacters[
            CharacterIndex];
    if (!Character.CharacterId.IsValid ())
    {
        return;
    }

    FRPGClassProgressionTransactionService::RefreshCharacterProjection (
        PartyInventoryComponent,
        CharacterIndex);

    FPendingNotification* ExistingNotification =
        PendingNotifications.FindByPredicate (
            [PartyInventoryComponent, CharacterIndex] (
                const FPendingNotification& Pending)
            {
                return Pending.InventoryComponent.Get () ==
                        PartyInventoryComponent &&
                    Pending.CharacterIndex == CharacterIndex;
            });

    if (ExistingNotification)
    {
        ExistingNotification->PreviousLevel = FMath::Min (
            ExistingNotification->PreviousLevel,
            PreviousLevel);
        ExistingNotification->NewLevel = FMath::Max (
            ExistingNotification->NewLevel,
            NewLevel);
        ExistingNotification->LevelsGained = FMath::Max (
            0,
            ExistingNotification->NewLevel -
                ExistingNotification->PreviousLevel);

        UE_LOG (
            LogGridLevelUpUI,
            Log,
            TEXT ("[GridLevelUpUI] Coalesced Character=%d Previous=%d New=%d Gained=%d Pending=%d"),
            CharacterIndex,
            ExistingNotification->PreviousLevel,
            ExistingNotification->NewLevel,
            ExistingNotification->LevelsGained,
            PendingNotifications.Num ());
    }
    else
    {
        FPendingNotification Notification;
        Notification.InventoryComponent = PartyInventoryComponent;
        Notification.CharacterIndex = CharacterIndex;
        Notification.CharacterId = Character.CharacterId;
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
    }

    SyncPersistentMirrorForCharacter (Character.CharacterId);
    TryPresentNextNotification ();
}

void URPGLevelUpNotificationSubsystem::HandleWidgetClosed (
    URPGLevelUpWidget* ClosedWidget)
{
    if (ClosedWidget != ActiveWidget)
    {
        return;
    }

    const FGuid ClosedCharacterId = ActiveNotification.IsSet ()
        ? ActiveNotification->CharacterId
        : FGuid ();

    if (WidgetClosedDelegateHandle.IsValid ())
    {
        ClosedWidget->OnClosed ().Remove (WidgetClosedDelegateHandle);
        WidgetClosedDelegateHandle.Reset ();
    }
    ActiveWidget = nullptr;
    ActiveNotification.Reset ();

    if (ClosedCharacterId.IsValid ())
    {
        SyncPersistentMirrorForCharacter (ClosedCharacterId);
    }
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

    if (!PendingPersistentRestoreStates.IsEmpty () &&
        !TryAdoptPersistentRestoreState ())
    {
        SchedulePersistentRestoreRetry ();
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
            SyncPersistentMirrorForCharacter (Notification.CharacterId);
            continue;
        }

        AGrimrockPartyPawn* PartyPawn = Cast<AGrimrockPartyPawn> (
            InventoryComponent->GetOwner ());
        APlayerController* PlayerController = PartyPawn
            ? Cast<APlayerController> (PartyPawn->GetController ())
            : nullptr;
        if (!IsValid (PlayerController) ||
            !PlayerController->IsLocalController ())
        {
            PendingNotifications.RemoveAt (0);
            SyncPersistentMirrorForCharacter (Notification.CharacterId);
            UE_LOG (
                LogGridLevelUpUI,
                Verbose,
                TEXT ("[GridLevelUpUI] Presentation skipped Character=%d Reason=NoLocalPlayerController"),
                Notification.CharacterIndex);
            continue;
        }

        UGridTurnManagerComponent* TurnManager =
            ResolveTurnManagerForParty (PartyPawn);
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
            ActiveNotification.Reset ();
            SyncPersistentMirrorForCharacter (Notification.CharacterId);
            continue;
        }

        ActiveNotification = Notification;
        SyncPersistentMirrorForCharacter (Notification.CharacterId);
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

void URPGLevelUpNotificationSubsystem::HandlePersistentStateRestored (
    const TArray<FRPGPendingLevelUpSaveState>& SavedStates)
{
    PendingPersistentRestoreStates = SavedStates;
    PersistentRestoreRetryCount = 0;

    if (SavedStates.IsEmpty ())
    {
        PendingNotifications.Reset ();
        return;
    }
    SchedulePersistentRestoreRetry ();
}

bool URPGLevelUpNotificationSubsystem::TryAdoptPersistentRestoreState ()
{
    if (PendingPersistentRestoreStates.IsEmpty ())
    {
        return true;
    }

    UWorld* World = GetWorld ();
    if (!World)
    {
        return false;
    }

    UGridPartyInventoryComponent* MatchingInventory = nullptr;
    for (TActorIterator<AGrimrockPartyPawn> It (World); It; ++It)
    {
        AGrimrockPartyPawn* PartyPawn = *It;
        UGridPartyInventoryComponent* Inventory = PartyPawn
            ? PartyPawn->PartyInventoryComponent.Get ()
            : nullptr;
        if (!IsValid (Inventory))
        {
            continue;
        }

        bool bAllCharactersMatch = true;
        for (const FRPGPendingLevelUpSaveState& SavedState :
            PendingPersistentRestoreStates)
        {
            if (FindCharacterIndexById (
                    Inventory->PartyInventoryState,
                    SavedState.CharacterId) == INDEX_NONE)
            {
                bAllCharactersMatch = false;
                break;
            }
        }
        if (bAllCharactersMatch)
        {
            MatchingInventory = Inventory;
            break;
        }
    }

    if (!MatchingInventory)
    {
        return false;
    }

    PendingNotifications.Reset ();
    for (const FRPGPendingLevelUpSaveState& SavedState :
        PendingPersistentRestoreStates)
    {
        const int32 CharacterIndex = FindCharacterIndexById (
            MatchingInventory->PartyInventoryState,
            SavedState.CharacterId);
        if (CharacterIndex == INDEX_NONE)
        {
            return false;
        }

        FPendingNotification Notification;
        Notification.InventoryComponent = MatchingInventory;
        Notification.CharacterIndex = CharacterIndex;
        Notification.CharacterId = SavedState.CharacterId;
        Notification.PreviousLevel = SavedState.PreviousLevel;
        Notification.NewLevel = SavedState.NewLevel;
        Notification.LevelsGained = SavedState.LevelsGained;
        PendingNotifications.Add (MoveTemp (Notification));
    }

    UE_LOG (
        LogGridLevelUpUI,
        Log,
        TEXT ("[GridLevelUpUI] Restored Pending=%d"),
        PendingNotifications.Num ());

    PendingPersistentRestoreStates.Reset ();
    PersistentRestoreRetryCount = 0;
    return true;
}

void URPGLevelUpNotificationSubsystem::SchedulePersistentRestoreRetry ()
{
    if (PendingPersistentRestoreStates.IsEmpty () ||
        PersistentRestoreRetryCount >= 60)
    {
        return;
    }

    UWorld* World = GetWorld ();
    if (!World)
    {
        return;
    }

    World->GetTimerManager ().SetTimerForNextTick (
        FTimerDelegate::CreateUObject (
            this,
            &URPGLevelUpNotificationSubsystem::HandlePersistentRestoreRetryTick));
}

void URPGLevelUpNotificationSubsystem::HandlePersistentRestoreRetryTick ()
{
    if (PendingPersistentRestoreStates.IsEmpty ())
    {
        return;
    }

    if (TryAdoptPersistentRestoreState ())
    {
        TryPresentNextNotification ();
        return;
    }

    ++PersistentRestoreRetryCount;
    if (PersistentRestoreRetryCount >= 60)
    {
        UE_LOG (
            LogGridLevelUpUI,
            Warning,
            TEXT ("[GridLevelUpUI] PersistentRestore Deferred Result=Abandoned Pending=%d Reason=PartyNotReady"),
            PendingPersistentRestoreStates.Num ());
        return;
    }
    SchedulePersistentRestoreRetry ();
}

void URPGLevelUpNotificationSubsystem::SyncPersistentMirrorForCharacter (
    const FGuid& CharacterId)
{
    if (!CharacterId.IsValid ())
    {
        return;
    }

    bool bFound = false;
    FRPGPendingLevelUpSaveState Merged;
    Merged.CharacterId = CharacterId;

    auto MergeNotification = [&Merged, &bFound] (
        const FPendingNotification& Notification)
    {
        if (!bFound)
        {
            Merged.PreviousLevel = Notification.PreviousLevel;
            Merged.NewLevel = Notification.NewLevel;
            bFound = true;
        }
        else
        {
            Merged.PreviousLevel = FMath::Min (
                Merged.PreviousLevel,
                Notification.PreviousLevel);
            Merged.NewLevel = FMath::Max (
                Merged.NewLevel,
                Notification.NewLevel);
        }
        Merged.LevelsGained = FMath::Max (
            0,
            Merged.NewLevel - Merged.PreviousLevel);
    };

    if (ActiveNotification.IsSet () &&
        ActiveNotification->CharacterId == CharacterId)
    {
        MergeNotification (*ActiveNotification);
    }

    for (const FPendingNotification& Pending : PendingNotifications)
    {
        if (Pending.CharacterId == CharacterId)
        {
            MergeNotification (Pending);
        }
    }

    if (bFound)
    {
        RemovePersistentMirror (CharacterId);
        UpsertPersistentMirror (Merged);
    }
    else
    {
        RemovePersistentMirror (CharacterId);
    }
}
