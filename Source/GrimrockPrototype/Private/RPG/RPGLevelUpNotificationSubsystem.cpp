#include "RPG/RPGLevelUpNotificationSubsystem.h"

#include "Blueprint/UserWidget.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "RPG/RPGCharacterRulesLibrary.h"
#include "RPG/RPGClassProgressionTransactionService.h"
#include "RPG/RPGLevelUpService.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "UI/RPGLevelUpWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogGridLevelUpUI, Log, All);

namespace GridLevelUpNotificationPrivate
{
	int32 FindCharacterIndexById(const FGridPartyInventoryState& PartyState, const FGuid& CharacterId)
	{
		for (int32 CharacterIndex = 0; CharacterIndex < PartyState.ActiveCharacters.Num(); ++CharacterIndex)
		{
			if (PartyState.ActiveCharacters[CharacterIndex].CharacterId == CharacterId)
			{
				return CharacterIndex;
			}
		}
		return INDEX_NONE;
	}

	UGridTurnManagerComponent* ResolveTurnManagerForParty(AGrimrockPartyPawn* PartyPawn)
	{
		if (!IsValid(PartyPawn))
		{
			return nullptr;
		}

		if (IsValid(PartyPawn->LevelRuntimeActor))
		{
			if (UGridTurnManagerComponent* TurnManager = PartyPawn->LevelRuntimeActor->FindComponentByClass<UGridTurnManagerComponent>())
			{
				if (!IsValid(TurnManager->PartyPawn) || TurnManager->PartyPawn == PartyPawn)
				{
					return TurnManager;
				}
			}
		}

		UWorld* World = PartyPawn->GetWorld();
		if (!World)
		{
			return nullptr;
		}

		for (TActorIterator<AGridLevelRuntimeActor> It(World); It; ++It)
		{
			AGridLevelRuntimeActor* RuntimeActor = *It;
			UGridTurnManagerComponent* TurnManager = RuntimeActor ? RuntimeActor->FindComponentByClass<UGridTurnManagerComponent>() : nullptr;
			if (IsValid(TurnManager) && (!IsValid(TurnManager->PartyPawn) || TurnManager->PartyPawn == PartyPawn))
			{
				return TurnManager;
			}
		}

		return nullptr;
	}
}

using namespace GridLevelUpNotificationPrivate;

void URPGLevelUpNotificationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LevelUpDelegateHandle =
		FRPGLevelUpService::OnCharacterLevelUpAppliedWithSource().AddUObject(this, &URPGLevelUpNotificationSubsystem::HandleCharacterLevelUpApplied);
}

void URPGLevelUpNotificationSubsystem::Deinitialize()
{
	if (LevelUpDelegateHandle.IsValid())
	{
		FRPGLevelUpService::OnCharacterLevelUpAppliedWithSource().Remove(LevelUpDelegateHandle);
		LevelUpDelegateHandle.Reset();
	}

	if (UGridPartyInventoryComponent* Inventory = ObservedPartyInventory.Get())
	{
		Inventory->OnPartyInventoryChanged.RemoveDynamic(this, &URPGLevelUpNotificationSubsystem::HandlePartyInventoryChanged);
	}
	ObservedPartyInventory.Reset();

	ClearDeferredCombatTurnManager();

	if (IsValid(ActiveWidget))
	{
		if (WidgetClosedDelegateHandle.IsValid())
		{
			ActiveWidget->OnClosed().Remove(WidgetClosedDelegateHandle);
			WidgetClosedDelegateHandle.Reset();
		}
		ActiveWidget->CancelSelection();
		ActiveWidget = nullptr;
	}

	ActiveNotification.Reset();
	PendingNotifications.Reset();
	Super::Deinitialize();
}

int32 URPGLevelUpNotificationSubsystem::GetPendingLevelUpNotificationCount() const
{
	return PendingNotifications.Num() + (IsValid(ActiveWidget) ? 1 : 0);
}

bool URPGLevelUpNotificationSubsystem::IsLevelUpModalOpen() const
{
	return IsValid(ActiveWidget) && ActiveWidget->IsInViewport();
}

void URPGLevelUpNotificationSubsystem::RefreshFromPartyState(UGridPartyInventoryComponent* PartyInventoryComponent)
{
	if (!IsValid(PartyInventoryComponent))
	{
		return;
	}

	if (UWorld* SourceWorld = PartyInventoryComponent->GetWorld())
	{
		if (SourceWorld->GetGameInstance() != GetGameInstance())
		{
			return;
		}
	}

	BindPartyInventory(PartyInventoryComponent);
	RebuildPendingNotificationsFromPartyState(PartyInventoryComponent);
	TryPresentNextNotification();
}

void URPGLevelUpNotificationSubsystem::BindPartyInventory(UGridPartyInventoryComponent* PartyInventoryComponent)
{
	if (ObservedPartyInventory.Get() == PartyInventoryComponent)
	{
		return;
	}

	if (UGridPartyInventoryComponent* PreviousInventory = ObservedPartyInventory.Get())
	{
		PreviousInventory->OnPartyInventoryChanged.RemoveDynamic(this, &URPGLevelUpNotificationSubsystem::HandlePartyInventoryChanged);
	}

	ObservedPartyInventory = PartyInventoryComponent;
	PendingNotifications.Reset();

	if (IsValid(PartyInventoryComponent))
	{
		PartyInventoryComponent->OnPartyInventoryChanged.AddUniqueDynamic(this, &URPGLevelUpNotificationSubsystem::HandlePartyInventoryChanged);
	}
}

void URPGLevelUpNotificationSubsystem::RebuildPendingNotificationsFromPartyState(UGridPartyInventoryComponent* PartyInventoryComponent)
{
	if (!IsValid(PartyInventoryComponent))
	{
		PendingNotifications.Reset();
		return;
	}

	TArray<FPendingNotification> Candidate;
	const FGridPartyInventoryState& PartyState = PartyInventoryComponent->PartyInventoryState;
	for (int32 CharacterIndex = 0; CharacterIndex < PartyState.ActiveCharacters.Num(); ++CharacterIndex)
	{
		const FGridCharacterInventoryState& Character = PartyState.ActiveCharacters[CharacterIndex];
		if (!Character.CharacterId.IsValid())
		{
			continue;
		}

		int32 EffectiveAcknowledgedLevel = Character.LastAcknowledgedLevel;
		if (ActiveNotification.IsSet() && ActiveNotification->InventoryComponent.Get() == PartyInventoryComponent &&
			ActiveNotification->CharacterId == Character.CharacterId)
		{
			EffectiveAcknowledgedLevel = FMath::Max(EffectiveAcknowledgedLevel, ActiveNotification->NewLevel);
		}

		if (EffectiveAcknowledgedLevel >= Character.Level)
		{
			continue;
		}

		FPendingNotification Notification;
		Notification.InventoryComponent = PartyInventoryComponent;
		Notification.CharacterIndex = CharacterIndex;
		Notification.CharacterId = Character.CharacterId;
		Notification.PreviousLevel = EffectiveAcknowledgedLevel;
		Notification.NewLevel = Character.Level;
		Notification.LevelsGained = Character.Level - EffectiveAcknowledgedLevel;
		Candidate.Add(MoveTemp(Notification));
	}

	Candidate.Sort(
		[](const FPendingNotification& Left, const FPendingNotification& Right)
		{
			return Left.CharacterIndex < Right.CharacterIndex;
		});
	PendingNotifications = MoveTemp(Candidate);
}

void URPGLevelUpNotificationSubsystem::HandleCharacterLevelUpApplied(
	UGridPartyInventoryComponent* PartyInventoryComponent, int32 CharacterIndex, int32 PreviousLevel, int32 NewLevel, int32 LevelsGained)
{
	if (!IsValid(PartyInventoryComponent) || !PartyInventoryComponent->IsValidCharacterIndex(CharacterIndex))
	{
		return;
	}

	UWorld* SourceWorld = PartyInventoryComponent->GetWorld();
	if (!SourceWorld || SourceWorld->GetGameInstance() != GetGameInstance())
	{
		return;
	}

	const FGridCharacterInventoryState& Character = PartyInventoryComponent->PartyInventoryState.ActiveCharacters[CharacterIndex];
	if (!Character.CharacterId.IsValid())
	{
		return;
	}

	FRPGClassProgressionTransactionService::RefreshCharacterProjection(PartyInventoryComponent, CharacterIndex);
	BindPartyInventory(PartyInventoryComponent);
	RebuildPendingNotificationsFromPartyState(PartyInventoryComponent);

	UE_LOG(LogGridLevelUpUI, Log,
		TEXT("[GridLevelUpUI] Derived Character=%d Previous=%d New=%d Gained=%d Acknowledged=%d Pending=%d"),
		CharacterIndex, PreviousLevel, NewLevel, LevelsGained, Character.LastAcknowledgedLevel, PendingNotifications.Num());

	TryPresentNextNotification();
}

void URPGLevelUpNotificationSubsystem::HandlePartyInventoryChanged(int32 CharacterIndex)
{
	(void)CharacterIndex;
	if (UGridPartyInventoryComponent* Inventory = ObservedPartyInventory.Get())
	{
		RebuildPendingNotificationsFromPartyState(Inventory);
		TryPresentNextNotification();
	}
}

void URPGLevelUpNotificationSubsystem::AcknowledgeNotification(const FPendingNotification& Notification)
{
	UGridPartyInventoryComponent* InventoryComponent = Notification.InventoryComponent.Get();
	if (!IsValid(InventoryComponent))
	{
		return;
	}

	const int32 CharacterIndex = FindCharacterIndexById(InventoryComponent->PartyInventoryState, Notification.CharacterId);
	if (!InventoryComponent->PartyInventoryState.ActiveCharacters.IsValidIndex(CharacterIndex))
	{
		return;
	}

	FGridCharacterInventoryState& Character = InventoryComponent->PartyInventoryState.ActiveCharacters[CharacterIndex];
	const int32 MinimumLevel = URPGCharacterRulesLibrary::GetMinimumLevel();
	const int32 AcknowledgedLevel = FMath::Clamp(Notification.NewLevel, MinimumLevel, Character.Level);
	Character.LastAcknowledgedLevel = FMath::Max(Character.LastAcknowledgedLevel, AcknowledgedLevel);

	UE_LOG(LogGridLevelUpUI, Log, TEXT("[GridLevelUpUI] Acknowledged Character=%d Level=%d Current=%d"), CharacterIndex,
		Character.LastAcknowledgedLevel, Character.Level);
}

void URPGLevelUpNotificationSubsystem::HandleWidgetClosed(URPGLevelUpWidget* ClosedWidget)
{
	if (ClosedWidget != ActiveWidget)
	{
		return;
	}

	const TOptional<FPendingNotification> ClosedNotification = ActiveNotification;

	if (WidgetClosedDelegateHandle.IsValid())
	{
		ClosedWidget->OnClosed().Remove(WidgetClosedDelegateHandle);
		WidgetClosedDelegateHandle.Reset();
	}
	ActiveWidget = nullptr;
	ActiveNotification.Reset();

	if (ClosedNotification.IsSet())
	{
		AcknowledgeNotification(*ClosedNotification);
	}

	if (UGridPartyInventoryComponent* Inventory = ObservedPartyInventory.Get())
	{
		RebuildPendingNotificationsFromPartyState(Inventory);
	}
	TryPresentNextNotification();
}

void URPGLevelUpNotificationSubsystem::HandleDeferredCombatEnded(EGridCombatPhase ResultPhase)
{
	UE_LOG(
		LogGridLevelUpUI, Log, TEXT("[GridLevelUpUI] CombatSafePoint Result=%s Pending=%d"), *UEnum::GetValueAsString(ResultPhase), PendingNotifications.Num());

	ClearDeferredCombatTurnManager();
	TryPresentNextNotification();
}

void URPGLevelUpNotificationSubsystem::SetDeferredCombatTurnManager(UGridTurnManagerComponent* TurnManager)
{
	if (DeferredCombatTurnManager == TurnManager)
	{
		return;
	}

	ClearDeferredCombatTurnManager();
	if (!IsValid(TurnManager))
	{
		return;
	}

	DeferredCombatTurnManager = TurnManager;
	TurnManager->OnCombatEnded.AddUniqueDynamic(this, &URPGLevelUpNotificationSubsystem::HandleDeferredCombatEnded);
}

void URPGLevelUpNotificationSubsystem::ClearDeferredCombatTurnManager()
{
	if (IsValid(DeferredCombatTurnManager))
	{
		DeferredCombatTurnManager->OnCombatEnded.RemoveDynamic(this, &URPGLevelUpNotificationSubsystem::HandleDeferredCombatEnded);
	}
	DeferredCombatTurnManager = nullptr;
}

void URPGLevelUpNotificationSubsystem::TryPresentNextNotification()
{
	if (IsValid(ActiveWidget))
	{
		return;
	}

	while (!PendingNotifications.IsEmpty())
	{
		const FPendingNotification Notification = PendingNotifications[0];
		UGridPartyInventoryComponent* InventoryComponent = Notification.InventoryComponent.Get();
		if (!IsValid(InventoryComponent) || !InventoryComponent->IsValidCharacterIndex(Notification.CharacterIndex) ||
			InventoryComponent->PartyInventoryState.ActiveCharacters[Notification.CharacterIndex].CharacterId != Notification.CharacterId)
		{
			PendingNotifications.RemoveAt(0);
			continue;
		}

		AGrimrockPartyPawn* PartyPawn = Cast<AGrimrockPartyPawn>(InventoryComponent->GetOwner());
		APlayerController* PlayerController = PartyPawn ? Cast<APlayerController>(PartyPawn->GetController()) : nullptr;
		if (!IsValid(PlayerController) || !PlayerController->IsLocalController())
		{
			PendingNotifications.RemoveAt(0);
			AcknowledgeNotification(Notification);
			UE_LOG(LogGridLevelUpUI, Verbose, TEXT("[GridLevelUpUI] Presentation skipped Character=%d Reason=NoLocalPlayerController"),
				Notification.CharacterIndex);
			continue;
		}

		UGridTurnManagerComponent* TurnManager = ResolveTurnManagerForParty(PartyPawn);
		if (IsValid(TurnManager) && TurnManager->bCombatActive)
		{
			const bool bNewDeferredManager = DeferredCombatTurnManager != TurnManager;
			SetDeferredCombatTurnManager(TurnManager);
			if (bNewDeferredManager)
			{
				UE_LOG(LogGridLevelUpUI, Log, TEXT("[GridLevelUpUI] Deferred Character=%d Previous=%d New=%d Reason=CombatActive Phase=%s"),
					Notification.CharacterIndex, Notification.PreviousLevel, Notification.NewLevel, *UEnum::GetValueAsString(TurnManager->CurrentPhase));
			}
			return;
		}

		ClearDeferredCombatTurnManager();
		PendingNotifications.RemoveAt(0);

		ActiveWidget = CreateWidget<URPGLevelUpWidget>(PlayerController, URPGLevelUpWidget::StaticClass());
		if (!IsValid(ActiveWidget) ||
			!ActiveWidget->InitializeLevelUpWidget(InventoryComponent, Notification.CharacterIndex, Notification.PreviousLevel, Notification.NewLevel))
		{
			ActiveWidget = nullptr;
			ActiveNotification.Reset();
			AcknowledgeNotification(Notification);
			continue;
		}

		ActiveNotification = Notification;
		WidgetClosedDelegateHandle = ActiveWidget->OnClosed().AddUObject(this, &URPGLevelUpNotificationSubsystem::HandleWidgetClosed);
		ActiveWidget->AddToViewport(200);

		UE_LOG(LogGridLevelUpUI, Log, TEXT("[GridLevelUpUI] Opened Character=%d Previous=%d New=%d"), Notification.CharacterIndex,
			Notification.PreviousLevel, Notification.NewLevel);
		return;
	}

	ClearDeferredCombatTurnManager();
}
