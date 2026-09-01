#include "Runtime/GrimrockPartyPawn.h"

#include "GameFramework/PlayerController.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPlayerController.h"
#include "UI/GridCombatHudWidget.h"
#include "UI/GridInventoryWidget.h"
#include "UI/GrimrockMenuWidget.h"
#include "UI/RPGCharacterCreationWidget.h"

void AGrimrockPartyPawn::ToggleInventoryWidget()
{
	if (bCharacterCreationModalActive || bIsPitFalling)
	{
		return;
	}

	if (bInventoryWidgetVisible)
	{
		HideInventoryWidget();
	}
	else
	{
		ShowInventoryWidget();
	}
}

void AGrimrockPartyPawn::ShowInventoryWidget()
{
	if (bCharacterCreationModalActive || bIsPitFalling)
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridInventory UI Show Failed Pawn=%s Reason=NoPlayerController"), *GetName());
		return;
	}

	if (!MenuWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("GrimrockMenu UI Show Failed Pawn=%s Reason=NoMenuWidgetClass"), *GetName());
		return;
	}

	if (!MenuWidgetInstance)
	{
		MenuWidgetInstance = CreateWidget<UGrimrockMenuWidget>(PlayerController, MenuWidgetClass);
		if (MenuWidgetInstance)
		{
			MenuWidgetInstance->InitializeMenuWidget(this);
		}
	}

	if (!MenuWidgetInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("GrimrockMenu UI Show Failed Pawn=%s Reason=CreateWidgetFailed"), *GetName());
		return;
	}

	if (!MenuWidgetInstance->IsInViewport())
	{
		MenuWidgetInstance->AddToViewport(100);
	}
	MenuWidgetInstance->SetVisibility(ESlateVisibility::Visible);
	MenuWidgetInstance->RefreshInventory();
	bInventoryWidgetVisible = true;

	if (CombatHudWidgetInstance && CombatHudWidgetInstance->IsInViewport())
	{
		CombatHudWidgetInstance->RemoveFromParent();
		CombatHudWidgetInstance->AddToViewport(CombatHotbarConfigurationZOrder);
		CombatHudWidgetInstance->RefreshFromSources();
	}

	PlayerController->bEnableClickEvents = true;
	PlayerController->bEnableMouseOverEvents = true;
	PlayerController->bShowMouseCursor = true;
	PlayerController->DefaultMouseCursor = EMouseCursor::Default;
	PlayerController->CurrentMouseCursor = EMouseCursor::Default;
	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(MenuWidgetInstance->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	PlayerController->SetInputMode(InputMode);
	PlayerController->bShowMouseCursor = true;
	PlayerController->DefaultMouseCursor = EMouseCursor::Default;
	PlayerController->CurrentMouseCursor = EMouseCursor::Default;

	if (AGrimrockPlayerController* GrimrockPlayerController = Cast<AGrimrockPlayerController>(PlayerController))
	{
		GrimrockPlayerController->SetInventoryUiOpen(true);
	}

	UE_LOG(LogTemp, Log, TEXT("GrimrockMenu UI Shown Pawn=%s"), *GetName());
}

void AGrimrockPartyPawn::HideInventoryWidget()
{
	if (MenuWidgetInstance)
	{
		MenuWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);
	}
	bInventoryWidgetVisible = false;

	if (CombatHudWidgetInstance && CombatHudWidgetInstance->IsInViewport())
	{
		CombatHudWidgetInstance->RemoveFromParent();
		CombatHudWidgetInstance->AddToViewport(CombatActionPanelZOrder);
		CombatHudWidgetInstance->RefreshFromSources();
	}

	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (AGrimrockPlayerController* GrimrockPlayerController = Cast<AGrimrockPlayerController>(PlayerController))
		{
			GrimrockPlayerController->SetInventoryUiOpen(false);
		}
		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		PlayerController->SetInputMode(InputMode);
		PlayerController->bShowMouseCursor = true;
		PlayerController->DefaultMouseCursor = EMouseCursor::Default;
		PlayerController->CurrentMouseCursor = EMouseCursor::Default;
	}

	if (bAutoSaveOnInventoryClose && PartyInventoryComponent && PartyInventoryComponent->HasCompletedInitialCharacterCreation())
	{
		FText SaveError;
		if (!SaveCurrentGame(SaveError))
		{
			UE_LOG(LogTemp, Warning, TEXT("PartySave InventoryClose Failed Slot=%s Reason=%s"), *PartySaveSlotName, *SaveError.ToString());
		}
	}

	UE_LOG(LogTemp, Log, TEXT("GrimrockMenu UI Hidden Pawn=%s"), *GetName());
}

UGridInventoryWidget* AGrimrockPartyPawn::GetInventoryWidget() const
{
	return MenuWidgetInstance ? MenuWidgetInstance->GetInventoryWidget() : nullptr;
}

bool AGrimrockPartyPawn::ShowCombatActionPanelWidget()
{
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridCombatHud Show Failed Pawn=%s Reason=NoPlayerController"), *GetName());
		return false;
	}

	UGridTurnManagerComponent* TurnManager = IsValid(LevelRuntimeActor) ? LevelRuntimeActor->FindComponentByClass<UGridTurnManagerComponent>() : nullptr;
	if (!CombatHudWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridCombatHud Show Failed Pawn=%s Reason=WidgetClassUnset"), *GetName());
		return false;
	}

	if (!CombatHudWidgetInstance)
	{
		CombatHudWidgetInstance = CreateWidget<UGridCombatHudWidget>(PlayerController, CombatHudWidgetClass);
	}
	if (!CombatHudWidgetInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridCombatHud Show Failed Pawn=%s Reason=CreateWidgetFailed"), *GetName());
		return false;
	}

	CombatHudWidgetInstance->InitializeCombatHud(this, TurnManager);
	if (!CombatHudWidgetInstance->IsInViewport())
	{
		CombatHudWidgetInstance->AddToViewport(CombatActionPanelZOrder);
	}
	return true;
}

void AGrimrockPartyPawn::HideCombatActionPanelWidget()
{
	if (CombatHudWidgetInstance)
	{
		CombatHudWidgetInstance->RemoveFromParent();
		CombatHudWidgetInstance = nullptr;
	}
}

void AGrimrockPartyPawn::RefreshCombatActionPanelWidget()
{
	if (CombatHudWidgetInstance)
	{
		CombatHudWidgetInstance->RefreshFromSources();
	}
}

bool AGrimrockPartyPawn::BeginSelectedCharacterMainHandThrowAiming()
{
	AGrimrockPlayerController* PlayerController = Cast<AGrimrockPlayerController>(GetController());
	if (!PartyInventoryComponent || !LevelRuntimeActor || !PlayerController)
	{
		return false;
	}

	if (bInventoryWidgetVisible || PlayerController->bInventoryUiOpen)
	{
		HideInventoryWidget();
	}
	return PlayerController->BeginPhysicalThrowAiming();
}


bool AGrimrockPartyPawn::BeginSelectedCharacterInventoryItemThrowAiming(FName ItemDefinitionId)
{
	AGrimrockPlayerController* PlayerController = Cast<AGrimrockPlayerController>(GetController());
	if (!PartyInventoryComponent || !LevelRuntimeActor || !PlayerController || ItemDefinitionId.IsNone())
	{
		return false;
	}
	if (bInventoryWidgetVisible || PlayerController->bInventoryUiOpen)
	{
		HideInventoryWidget();
	}
	return PlayerController->BeginPhysicalInventoryThrowAiming(ItemDefinitionId);
}

bool AGrimrockPartyPawn::TryExecuteCombatHotbarSlot(int32 SlotIndex)
{
	if (IsCombatHotbarExecutionBlocked() || SlotIndex < 0 || SlotIndex >= FGridCombatHotbarBinding::SlotCount)
	{
		return false;
	}

	if (!IsValid(CombatHudWidgetInstance))
	{
		return false;
	}

	FGridCombatActionRequestResult Result;
	return CombatHudWidgetInstance->RequestHotbarSlot(SlotIndex, Result);
}

bool AGrimrockPartyPawn::IsCombatHotbarExecutionBlocked() const
{
	const AGrimrockPlayerController* PlayerController = Cast<AGrimrockPlayerController>(GetController());
	return bInventoryWidgetVisible || bCharacterCreationModalActive || bIsPitFalling || (PlayerController && PlayerController->bInventoryUiOpen);
}

void AGrimrockPartyPawn::CloseCharacterCreationWidget()
{
	if (CharacterCreationWidgetInstance)
	{
		CharacterCreationWidgetInstance->RemoveFromParent();
		CharacterCreationWidgetInstance = nullptr;
	}
}

void AGrimrockPartyPawn::ApplyCharacterCreationInputMode(bool bIsActive)
{
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		return;
	}

	if (AGrimrockPlayerController* GrimrockPlayerController = Cast<AGrimrockPlayerController>(PlayerController))
	{
		GrimrockPlayerController->SetInventoryUiOpen(bIsActive);
	}

	PlayerController->bEnableClickEvents = true;
	PlayerController->bEnableMouseOverEvents = true;
	PlayerController->bShowMouseCursor = true;
	PlayerController->DefaultMouseCursor = EMouseCursor::Default;
	PlayerController->CurrentMouseCursor = EMouseCursor::Default;

	if (bIsActive)
	{
		FInputModeUIOnly InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PlayerController->SetInputMode(InputMode);

		if (CharacterCreationWidgetInstance)
		{
			CharacterCreationWidgetInstance->FocusNameInput();
		}
		return;
	}

	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	PlayerController->SetInputMode(InputMode);
}

void AGrimrockPartyPawn::ShowInitialCharacterCreationWidget()
{
	if (!PartyInventoryComponent || PartyInventoryComponent->HasCompletedInitialCharacterCreation())
	{
		bCharacterCreationModalActive = false;
		return;
	}

	bCharacterCreationModalActive = true;
	ClearBufferedCommand();

	if (bInventoryWidgetVisible)
	{
		HideInventoryWidget();
	}

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		UE_LOG(LogTemp, Error, TEXT("CharacterCreation UI Show Failed Pawn=%s Reason=NoPlayerController"), *GetName());
		return;
	}

	ApplyCharacterCreationInputMode(true);

	if (!CharacterCreationWidgetClass)
	{
		UE_LOG(LogTemp, Error, TEXT("CharacterCreation UI Show Failed Pawn=%s Reason=NoWidgetClass"), *GetName());
		return;
	}

	if (!CharacterCreationWidgetInstance)
	{
		CharacterCreationWidgetInstance = CreateWidget<URPGCharacterCreationWidget>(PlayerController, CharacterCreationWidgetClass);
	}

	if (!CharacterCreationWidgetInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("CharacterCreation UI Show Failed Pawn=%s Reason=CreateWidgetFailed"), *GetName());
		return;
	}

	CharacterCreationWidgetInstance->InitializeCharacterCreationWidget(this);
	if (!CharacterCreationWidgetInstance->IsInViewport())
	{
		CharacterCreationWidgetInstance->AddToViewport(1000);
	}
	CharacterCreationWidgetInstance->SetVisibility(ESlateVisibility::Visible);
	CharacterCreationWidgetInstance->FocusNameInput();

	UE_LOG(LogTemp, Log, TEXT("CharacterCreation UI Shown Pawn=%s"), *GetName());
}

void AGrimrockPartyPawn::HandleInitialCharacterCreated()
{
	if (!PartyInventoryComponent || !PartyInventoryComponent->HasCompletedInitialCharacterCreation())
	{
		return;
	}

	CloseCharacterCreationWidget();

	bCharacterCreationModalActive = false;
	ClearBufferedCommand();
	SyncHeldVisualFromSelectedCharacterEquipment();

	ApplyCharacterCreationInputMode(false);

	FText SaveError;
	if (!SaveCurrentGame(SaveError))
	{
		UE_LOG(LogTemp, Warning, TEXT("PartySave InitialCharacter Failed Slot=%s Reason=%s"), *PartySaveSlotName, *SaveError.ToString());
	}

	UE_LOG(LogTemp, Log, TEXT("CharacterCreation Completed Pawn=%s"), *GetName());
}

bool AGrimrockPartyPawn::IsCharacterCreationModalActive() const
{
	return bCharacterCreationModalActive;
}
