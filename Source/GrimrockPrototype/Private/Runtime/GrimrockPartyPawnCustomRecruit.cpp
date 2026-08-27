#include "Runtime/GrimrockPartyPawn.h"

#include "GameFramework/PlayerController.h"
#include "RPG/RPGCharacterTypes.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "UI/RPGCharacterCreationWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogGridCustomRecruitRuntime, Log, All);

bool AGrimrockPartyPawn::ShowCustomRecruitCharacterCreationWidget()
{
	if (!IsValid(PartyInventoryComponent))
	{
		UE_LOG(LogGridCustomRecruitRuntime, Warning, TEXT("[GridCustomRecruitRuntime] Show Failed Pawn=%s Reason=NoPartyInventoryComponent"), *GetName());
		return false;
	}

	const FGridPartyInventoryState& State = PartyInventoryComponent->PartyInventoryState;
	if (!State.bInitialCharacterCreationCompleted || State.ActiveCharacters.IsEmpty())
	{
		UE_LOG(LogGridCustomRecruitRuntime, Verbose, TEXT("[GridCustomRecruitRuntime] Show Rejected Pawn=%s Reason=InitialCharacterMissing"), *GetName());
		return false;
	}

	if (State.ActiveCharacters.Num() >= State.MaxActiveCharacters)
	{
		UE_LOG(LogGridCustomRecruitRuntime, Verbose, TEXT("[GridCustomRecruitRuntime] Show Rejected Pawn=%s Reason=PartyFull Active=%d Max=%d"), *GetName(),
			State.ActiveCharacters.Num(), State.MaxActiveCharacters);
		return false;
	}

	if (const UGridTurnManagerComponent* TurnManager = FindTurnManager(); IsValid(TurnManager) && TurnManager->bCombatActive)
	{
		UE_LOG(LogGridCustomRecruitRuntime, Log, TEXT("[GridCustomRecruitRuntime] Show Rejected Pawn=%s Reason=CombatActive Phase=%d Round=%d"), *GetName(),
			static_cast<int32>(TurnManager->CurrentPhase), TurnManager->RoundNumber);
		return false;
	}

	if (bCharacterCreationModalActive)
	{
		UE_LOG(LogGridCustomRecruitRuntime, Verbose, TEXT("[GridCustomRecruitRuntime] Show Rejected Pawn=%s Reason=CharacterCreationModalAlreadyActive"),
			*GetName());
		return false;
	}

	if (IsStoryCompanionRecruitmentModalActive())
	{
		UE_LOG(LogGridCustomRecruitRuntime, Verbose, TEXT("[GridCustomRecruitRuntime] Show Rejected Pawn=%s Reason=StoryRecruitmentModalActive"), *GetName());
		return false;
	}

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		UE_LOG(LogGridCustomRecruitRuntime, Warning, TEXT("[GridCustomRecruitRuntime] Show Failed Pawn=%s Reason=NoPlayerController"), *GetName());
		return false;
	}

	if (!CharacterCreationWidgetClass)
	{
		UE_LOG(LogGridCustomRecruitRuntime, Warning, TEXT("[GridCustomRecruitRuntime] Show Failed Pawn=%s Reason=NoCharacterCreationWidgetClass"), *GetName());
		return false;
	}

	if (CharacterCreationWidgetInstance)
	{
		if (CharacterCreationWidgetInstance->IsInViewport())
		{
			CharacterCreationWidgetInstance->RemoveFromParent();
		}
		CharacterCreationWidgetInstance = nullptr;
	}

	URPGCharacterCreationWidget* Widget = CreateWidget<URPGCharacterCreationWidget>(PlayerController, CharacterCreationWidgetClass);
	if (!Widget)
	{
		UE_LOG(LogGridCustomRecruitRuntime, Warning, TEXT("[GridCustomRecruitRuntime] Show Failed Pawn=%s Reason=CreateWidgetFailed"), *GetName());
		return false;
	}

	Widget->InitializeCharacterCreationWidgetForContext(this, ERPGCharacterCreationContext::CustomRecruit);
	Widget->OnCustomRecruitCommitted().AddUObject(this, &AGrimrockPartyPawn::HandleCustomRecruitCommitted);
	Widget->OnCustomRecruitCancelled().AddUObject(this, &AGrimrockPartyPawn::HandleCustomRecruitCancelled);

	if (bInventoryWidgetVisible)
	{
		HideInventoryWidget();
	}

	CharacterCreationWidgetInstance = Widget;
	bCharacterCreationModalActive = true;
	ClearBufferedCommand();

	Widget->AddToViewport(1000);
	Widget->SetVisibility(ESlateVisibility::Visible);
	ApplyCharacterCreationInputMode(true);

	UE_LOG(LogGridCustomRecruitRuntime, Log, TEXT("[GridCustomRecruitRuntime] Shown Pawn=%s WidgetClass=%s Context=CustomRecruit"), *GetName(),
		*GetNameSafe(CharacterCreationWidgetClass.Get()));
	return true;
}

void AGrimrockPartyPawn::CloseCustomRecruitCharacterCreationWidget()
{
	URPGCharacterCreationWidget* Widget = GetCustomRecruitCharacterCreationWidget();
	if (!Widget)
	{
		if (bCharacterCreationModalActive && CharacterCreationWidgetInstance &&
			CharacterCreationWidgetInstance->GetCreationContext() == ERPGCharacterCreationContext::CustomRecruit)
		{
			CharacterCreationWidgetInstance = nullptr;
			bCharacterCreationModalActive = false;
			ClearBufferedCommand();
			ApplyCharacterCreationInputMode(false);
		}
		return;
	}

	FinishCustomRecruitCharacterCreationWidget(Widget, false, INDEX_NONE);
}

bool AGrimrockPartyPawn::IsCustomRecruitCharacterCreationModalActive() const
{
	return bCharacterCreationModalActive && IsValid(CharacterCreationWidgetInstance) && CharacterCreationWidgetInstance->IsInViewport() &&
		CharacterCreationWidgetInstance->GetCreationContext() == ERPGCharacterCreationContext::CustomRecruit;
}

URPGCharacterCreationWidget* AGrimrockPartyPawn::GetCustomRecruitCharacterCreationWidget() const
{
	return IsCustomRecruitCharacterCreationModalActive() ? CharacterCreationWidgetInstance.Get() : nullptr;
}

void AGrimrockPartyPawn::HandleCustomRecruitCommitted(URPGCharacterCreationWidget* SourceWidget, int32 CharacterIndex)
{
	FinishCustomRecruitCharacterCreationWidget(SourceWidget, true, CharacterIndex);
}

void AGrimrockPartyPawn::HandleCustomRecruitCancelled(URPGCharacterCreationWidget* SourceWidget)
{
	if (!IsValid(SourceWidget) || CharacterCreationWidgetInstance.Get() != SourceWidget ||
		SourceWidget->GetCreationContext() != ERPGCharacterCreationContext::CustomRecruit)
	{
		UE_LOG(LogGridCustomRecruitRuntime, Verbose, TEXT("[GridCustomRecruitRuntime] Cancel Ignored Pawn=%s Widget=%s Reason=StaleOrWrongContext"), *GetName(),
			*GetNameSafe(SourceWidget));
		return;
	}

	// CancelWizard() owns the actual RemoveFromParent() call after this
	// synchronous delegate returns. Only release Pawn-side modal/input state
	// here, otherwise the widget is removed twice and PIE logs a warning.
	CharacterCreationWidgetInstance = nullptr;
	bCharacterCreationModalActive = false;
	ClearBufferedCommand();
	ApplyCharacterCreationInputMode(false);
	SyncHeldVisualFromSelectedCharacterEquipment();
	RefreshCombatActionPanelWidget();

	UE_LOG(LogGridCustomRecruitRuntime, Log, TEXT("[GridCustomRecruitRuntime] Cancelled Pawn=%s"), *GetName());
}

void AGrimrockPartyPawn::FinishCustomRecruitCharacterCreationWidget(URPGCharacterCreationWidget* SourceWidget, bool bCommitted, int32 CharacterIndex)
{
	if (!IsValid(SourceWidget) || CharacterCreationWidgetInstance.Get() != SourceWidget ||
		SourceWidget->GetCreationContext() != ERPGCharacterCreationContext::CustomRecruit)
	{
		UE_LOG(LogGridCustomRecruitRuntime, Verbose, TEXT("[GridCustomRecruitRuntime] Finish Ignored Pawn=%s Widget=%s Reason=StaleOrWrongContext"), *GetName(),
			*GetNameSafe(SourceWidget));
		return;
	}

	FGuid RecruitedCharacterId;
	if (bCommitted && IsValid(PartyInventoryComponent) && PartyInventoryComponent->PartyInventoryState.ActiveCharacters.IsValidIndex(CharacterIndex))
	{
		RecruitedCharacterId = PartyInventoryComponent->PartyInventoryState.ActiveCharacters[CharacterIndex].CharacterId;
	}

	if (SourceWidget->IsInViewport())
	{
		SourceWidget->RemoveFromParent();
	}
	CharacterCreationWidgetInstance = nullptr;
	bCharacterCreationModalActive = false;
	ClearBufferedCommand();
	ApplyCharacterCreationInputMode(false);
	SyncHeldVisualFromSelectedCharacterEquipment();
	RefreshCombatActionPanelWidget();

	if (bCommitted)
	{
		UE_LOG(LogGridCustomRecruitRuntime, Log, TEXT("[GridCustomRecruitRuntime] Committed Pawn=%s CharacterIndex=%d CharacterId=%s"), *GetName(),
			CharacterIndex, RecruitedCharacterId.IsValid() ? *RecruitedCharacterId.ToString() : TEXT("Invalid"));
	}
	else
	{
		UE_LOG(LogGridCustomRecruitRuntime, Log, TEXT("[GridCustomRecruitRuntime] Cancelled Pawn=%s"), *GetName());
	}
}
