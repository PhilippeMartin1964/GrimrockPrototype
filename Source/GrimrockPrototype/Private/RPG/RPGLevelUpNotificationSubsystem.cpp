#include "RPG/RPGLevelUpNotificationSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "RPG/RPGAuthoringIdentityResolver.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGClassProgressionService.h"
#include "RPG/RPGLevelUpService.h"
#include "RPG/RPGSkillPointService.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "UI/GridPersistentHudWidget.h"
#include "UI/RPGProgressionFeedbackService.h"
#include "TimerManager.h"

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

	FText ResolveCharacterName(const FGridCharacterInventoryState& Character, int32 CharacterIndex)
	{
		return Character.DisplayName.IsEmpty()
			? FText::FromString(FString::Printf(TEXT("Personnage %d"), CharacterIndex + 1))
			: Character.DisplayName;
	}

	URPGClassAsset* ResolveClassDefinition(const FGridCharacterInventoryState& Character)
	{
		if (URPGClassAsset* Definition = Character.ClassDefinition.Get();
			IsValid(Definition) && Definition->IsValidDefinition() &&
			(Character.ClassId.IsNone() || Definition->ClassId == Character.ClassId))
		{
			return Definition;
		}
		return FRPGAuthoringIdentityResolver::ResolveClassById(Character.ClassId);
	}
}

using namespace GridLevelUpNotificationPrivate;

void URPGLevelUpNotificationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LevelUpDelegateHandle =
		FRPGLevelUpService::OnCharacterLevelUpAppliedWithSource().AddUObject(
			this,
			&URPGLevelUpNotificationSubsystem::HandleCharacterLevelUpApplied);
}

void URPGLevelUpNotificationSubsystem::Deinitialize()
{
	if (LevelUpDelegateHandle.IsValid())
	{
		FRPGLevelUpService::OnCharacterLevelUpAppliedWithSource().Remove(LevelUpDelegateHandle);
		LevelUpDelegateHandle.Reset();
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ActiveToastTimerHandle);
	}

	ObservedPartyInventory.Reset();
	PendingNotifications.Reset();
	ActiveNotification.Reset();
	Super::Deinitialize();
}

int32 URPGLevelUpNotificationSubsystem::GetPendingLevelUpNotificationCount() const
{
	return PendingNotifications.Num() + (ActiveNotification.IsSet() ? 1 : 0);
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

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ActiveToastTimerHandle);
	}
	PendingNotifications.Reset();
	ActiveNotification.Reset();

	FGridPartyInventoryState& PartyState = PartyInventoryComponent->PartyInventoryState;
	for (int32 CharacterIndex = 0; CharacterIndex < PartyState.ActiveCharacters.Num(); ++CharacterIndex)
	{
		FGridCharacterInventoryState& Character = PartyState.ActiveCharacters[CharacterIndex];
		if (!Character.CharacterId.IsValid() || Character.LastAcknowledgedLevel >= Character.Level)
		{
			continue;
		}

		const int32 PreviousLevel = Character.LastAcknowledgedLevel;
		const int32 NewLevel = Character.Level;
		EnqueueNotification(
			PartyInventoryComponent,
			CharacterIndex,
			PreviousLevel,
			NewLevel,
			NewLevel - PreviousLevel);

		// Legacy/current saves may still contain the old durable modal gap.
		// Consume it immediately: the toast is informational and never blocks play.
		Character.LastAcknowledgedLevel = Character.Level;
	}

	TryPresentNextNotification();
}

void URPGLevelUpNotificationSubsystem::BindPartyInventory(UGridPartyInventoryComponent* PartyInventoryComponent)
{
	ObservedPartyInventory = PartyInventoryComponent;
}

void URPGLevelUpNotificationSubsystem::EnqueueNotification(
	UGridPartyInventoryComponent* PartyInventoryComponent,
	int32 CharacterIndex,
	int32 PreviousLevel,
	int32 NewLevel,
	int32 LevelsGained)
{
	if (!IsValid(PartyInventoryComponent) ||
		!PartyInventoryComponent->IsValidCharacterIndex(CharacterIndex) ||
		PreviousLevel >= NewLevel ||
		LevelsGained <= 0)
	{
		return;
	}

	const FGridCharacterInventoryState& Character =
		PartyInventoryComponent->PartyInventoryState.ActiveCharacters[CharacterIndex];
	if (!Character.CharacterId.IsValid())
	{
		return;
	}

	FPendingNotification Notification;
	Notification.InventoryComponent = PartyInventoryComponent;
	Notification.CharacterIndex = CharacterIndex;
	Notification.CharacterId = Character.CharacterId;
	Notification.PreviousLevel = PreviousLevel;
	Notification.NewLevel = NewLevel;
	Notification.LevelsGained = LevelsGained;
	PendingNotifications.Add(MoveTemp(Notification));
}

void URPGLevelUpNotificationSubsystem::HandleCharacterLevelUpApplied(
	UGridPartyInventoryComponent* PartyInventoryComponent,
	int32 CharacterIndex,
	int32 PreviousLevel,
	int32 NewLevel,
	int32 LevelsGained)
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

	BindPartyInventory(PartyInventoryComponent);
	EnqueueNotification(PartyInventoryComponent, CharacterIndex, PreviousLevel, NewLevel, LevelsGained);

	const FGridCharacterInventoryState& Character =
		PartyInventoryComponent->PartyInventoryState.ActiveCharacters[CharacterIndex];
	UE_LOG(
		LogGridLevelUpUI,
		Log,
		TEXT("[GridLevelUpUI] QueuedNonModal Character=%d Previous=%d New=%d Gained=%d Acknowledged=%d Pending=%d"),
		CharacterIndex,
		PreviousLevel,
		NewLevel,
		LevelsGained,
		Character.LastAcknowledgedLevel,
		PendingNotifications.Num());

	TryPresentNextNotification();
}

void URPGLevelUpNotificationSubsystem::AcknowledgeNotification(const FPendingNotification& Notification)
{
	UGridPartyInventoryComponent* InventoryComponent = Notification.InventoryComponent.Get();
	if (!IsValid(InventoryComponent))
	{
		return;
	}

	const int32 CharacterIndex =
		FindCharacterIndexById(InventoryComponent->PartyInventoryState, Notification.CharacterId);
	if (!InventoryComponent->PartyInventoryState.ActiveCharacters.IsValidIndex(CharacterIndex))
	{
		return;
	}

	FGridCharacterInventoryState& Character =
		InventoryComponent->PartyInventoryState.ActiveCharacters[CharacterIndex];
	Character.LastAcknowledgedLevel = FMath::Max(
		Character.LastAcknowledgedLevel,
		FMath::Min(Notification.NewLevel, Character.Level));
}

bool URPGLevelUpNotificationSubsystem::PresentNotification(
	const FPendingNotification& Notification,
	float& OutDurationSeconds)
{
	OutDurationSeconds = 0.0f;

	UGridPartyInventoryComponent* InventoryComponent = Notification.InventoryComponent.Get();
	if (!IsValid(InventoryComponent))
	{
		return false;
	}

	const int32 CharacterIndex =
		FindCharacterIndexById(InventoryComponent->PartyInventoryState, Notification.CharacterId);
	if (!InventoryComponent->PartyInventoryState.ActiveCharacters.IsValidIndex(CharacterIndex))
	{
		return false;
	}

	const FGridCharacterInventoryState& Character =
		InventoryComponent->PartyInventoryState.ActiveCharacters[CharacterIndex];

	AGrimrockPartyPawn* PartyPawn = Cast<AGrimrockPartyPawn>(InventoryComponent->GetOwner());
	if (!IsValid(PartyPawn) || !IsValid(PartyPawn->PersistentHudWidgetInstance))
	{
		return false;
	}

	const int32 SkillPointsGained = FMath::Max(
		0,
		FRPGSkillPointService::GetTotalPointsGranted(Notification.NewLevel) -
			FRPGSkillPointService::GetTotalPointsGranted(Notification.PreviousLevel));

	int32 TalentPointsGained = 0;
	if (URPGClassAsset* ClassDefinition = ResolveClassDefinition(Character))
	{
		TalentPointsGained = FMath::Max(
			0,
			FRPGClassProgressionService::GetTotalChoicePointsGranted(ClassDefinition, Notification.NewLevel) -
				FRPGClassProgressionService::GetTotalChoicePointsGranted(ClassDefinition, Notification.PreviousLevel));
	}

	const int32 PreviousRankCap = FRPGSkillPointService::GetRankCapForLevel(Notification.PreviousLevel);
	const int32 NewRankCap = FRPGSkillPointService::GetRankCapForLevel(Notification.NewLevel);
	const int32 UnlockedRankCap = NewRankCap > PreviousRankCap ? NewRankCap : 0;

	const FRPGProgressionNotificationView View =
		FRPGProgressionFeedbackService::MakeLevelUpNotification(
			ResolveCharacterName(Character, CharacterIndex),
			Notification.PreviousLevel,
			Notification.NewLevel,
			SkillPointsGained,
			TalentPointsGained,
			UnlockedRankCap);

	OutDurationSeconds = View.DurationSeconds;
	return PartyPawn->PersistentHudWidgetInstance->ShowProgressionNotification(View);
}

void URPGLevelUpNotificationSubsystem::TryPresentNextNotification()
{
	if (ActiveNotification.IsSet())
	{
		return;
	}

	while (!PendingNotifications.IsEmpty())
	{
		const FPendingNotification Notification = PendingNotifications[0];
		PendingNotifications.RemoveAt(0);

		UGridPartyInventoryComponent* InventoryComponent = Notification.InventoryComponent.Get();
		if (!IsValid(InventoryComponent))
		{
			continue;
		}

		AcknowledgeNotification(Notification);

		float DurationSeconds = 0.0f;
		if (!PresentNotification(Notification, DurationSeconds))
		{
			UE_LOG(
				LogGridLevelUpUI,
				Verbose,
				TEXT("[GridLevelUpUI] NonModalPresentationSkipped Character=%d Previous=%d New=%d Reason=NoPersistentHudNotificationSurface"),
				Notification.CharacterIndex,
				Notification.PreviousLevel,
				Notification.NewLevel);
			continue;
		}

		ActiveNotification = Notification;
		UE_LOG(
			LogGridLevelUpUI,
			Log,
			TEXT("[GridLevelUpUI] NonModalToastShown Character=%d Previous=%d New=%d Remaining=%d"),
			Notification.CharacterIndex,
			Notification.PreviousLevel,
			Notification.NewLevel,
			PendingNotifications.Num());

		UWorld* World = GetWorld();
		if (!World || DurationSeconds <= 0.0f)
		{
			HandleActiveToastExpired();
			return;
		}

		World->GetTimerManager().SetTimer(
			ActiveToastTimerHandle,
			this,
			&URPGLevelUpNotificationSubsystem::HandleActiveToastExpired,
			DurationSeconds + 0.05f,
			false);
		return;
	}
}

void URPGLevelUpNotificationSubsystem::HandleActiveToastExpired()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ActiveToastTimerHandle);
	}
	ActiveNotification.Reset();
	TryPresentNextNotification();
}
