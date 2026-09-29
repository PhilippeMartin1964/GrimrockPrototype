#include "Runtime/GrimrockPartyPawn.h"

#include "Components/Button.h"
#include "GameFramework/PlayerController.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPlayerController.h"
#include "UI/GridCombatHudWidget.h"
#include "UI/GridCharacterSheetWidget.h"
#include "UI/GridInventoryBagWidget.h"
#include "UI/GridMapWidget.h"
#include "UI/GridPersistentHudWidget.h"
#include "UI/GrimrockMenuWidget.h"
#include "UI/RPGCharacterCreationWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogGrimrockPartyUI, Log, All);

void AGrimrockPartyPawn::ToggleInventoryWidget()
{
	if (IsMajorGameplayUiBlockedByCombat())
	{
		return;
	}

	const bool bCharacterSheetVisible =
		CharacterSheetWidgetInstance && CharacterSheetWidgetInstance->GetVisibility() != ESlateVisibility::Collapsed &&
		CharacterSheetWidgetInstance->GetVisibility() != ESlateVisibility::Hidden;
	const bool bInventoryBagVisible =
		InventoryBagWidgetInstance && InventoryBagWidgetInstance->GetVisibility() != ESlateVisibility::Collapsed &&
		InventoryBagWidgetInstance->GetVisibility() != ESlateVisibility::Hidden;

	// Both visible: I closes the workspace. One missing/closed: I restores both.
	if (bInventoryWorkspaceVisible && bCharacterSheetVisible && bInventoryBagVisible)
	{
		HideInventoryWidget();
		return;
	}

	ShowInventoryWorkspace();
}

void AGrimrockPartyPawn::ToggleSkillsWidget()
{
	ToggleMenuPage(EInventoryTopTab::Skills);
}

void AGrimrockPartyPawn::ToggleCraftingWidget()
{
	ToggleMenuPage(EInventoryTopTab::Recipes);
}

void AGrimrockPartyPawn::ToggleMapWidget()
{
	if (IsMajorGameplayUiBlockedByCombat() || bCharacterCreationModalActive || bIsPitFalling)
	{
		return;
	}

	if (IsMapWidgetVisible())
	{
		HideMapWidget();
		return;
	}

	ShowMapWidget();
}

bool AGrimrockPartyPawn::IsMapWidgetVisible() const
{
	return IsValid(MapWidgetInstance) &&
		MapWidgetInstance->GetVisibility() != ESlateVisibility::Collapsed &&
		MapWidgetInstance->GetVisibility() != ESlateVisibility::Hidden;
}

void AGrimrockPartyPawn::ShowMapWidget()
{
	if (IsMajorGameplayUiBlockedByCombat() || bCharacterCreationModalActive || bIsPitFalling)
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		UE_LOG(LogGrimrockPartyUI, Warning, TEXT("GridMap Standalone Show Failed Pawn=%s Reason=NoPlayerController"), *GetName());
		return;
	}

	if (!MapWidgetClass)
	{
		UE_LOG(LogGrimrockPartyUI, Warning, TEXT("GridMap Standalone Show Failed Pawn=%s Reason=WidgetClassUnset"), *GetName());
		return;
	}

	CollapseInventoryWorkspaceForMenuPage();
	if (MenuWidgetInstance)
	{
		MenuWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (!MapWidgetInstance)
	{
		MapWidgetInstance = CreateWidget<UGridMapWidget>(PlayerController, MapWidgetClass);
		if (MapWidgetInstance)
		{
			// InitializeMapWidget already selects/builds the party floor once.
			MapWidgetInstance->InitializeMapWidget(this);
			if (MapWidgetInstance->Button_CloseMap)
			{
				MapWidgetInstance->Button_CloseMap->OnClicked.AddUniqueDynamic(
					this, &AGrimrockPartyPawn::HandleMapWindowCloseClicked);
			}
		}
	}
	else
	{
		// Reopening an existing map deliberately returns to the party's current floor.
		MapWidgetInstance->SelectPartyFloor();
	}

	if (!MapWidgetInstance)
	{
		UE_LOG(LogGrimrockPartyUI, Warning, TEXT("GridMap Standalone Show Failed Pawn=%s Reason=CreateWidgetFailed"), *GetName());
		return;
	}

	if (!MapWidgetInstance->IsInViewport())
	{
		MapWidgetInstance->AddToViewport(100);
	}
	MapWidgetInstance->SetVisibility(ESlateVisibility::Visible);

	bInventoryWorkspaceVisible = false;
	bInventoryWidgetVisible = true;
	RefreshPersistentHudWidget();
	ApplyMajorUiInputMode(true);

	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(MapWidgetInstance->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	PlayerController->SetInputMode(InputMode);

	UE_LOG(LogGrimrockPartyUI, Verbose, TEXT("GridMap Standalone Shown Pawn=%s Widget=%s"), *GetName(), *GetNameSafe(MapWidgetInstance));
}

void AGrimrockPartyPawn::ToggleJournalWidget()
{
	ToggleMenuPage(EInventoryTopTab::Journal);
}

void AGrimrockPartyPawn::ToggleHelpWidget()
{
	ToggleMenuPage(EInventoryTopTab::Codex);
}

bool AGrimrockPartyPawn::IsSplitInventoryWorkspaceConfigured() const
{
	return CharacterSheetWidgetClass && InventoryBagWidgetClass;
}

bool AGrimrockPartyPawn::IsInventoryWorkspaceVisible() const
{
	return bInventoryWorkspaceVisible;
}

void AGrimrockPartyPawn::ToggleMenuPage(EInventoryTopTab TopTab)
{
	if (IsMajorGameplayUiBlockedByCombat())
	{
		return;
	}

	if (bCharacterCreationModalActive || bIsPitFalling)
	{
		return;
	}

	if (TopTab == EInventoryTopTab::Inventory)
	{
		ToggleInventoryWidget();
		return;
	}
	if (TopTab == EInventoryTopTab::Map)
	{
		ToggleMapWidget();
		return;
	}

	const bool bPageShellVisible = MenuWidgetInstance && MenuWidgetInstance->GetVisibility() != ESlateVisibility::Collapsed &&
		MenuWidgetInstance->GetVisibility() != ESlateVisibility::Hidden;
	if (bInventoryWidgetVisible && bPageShellVisible && MenuWidgetInstance->CurrentTopTab == TopTab)
	{
		HideInventoryWidget();
		return;
	}

	ShowMenuPage(TopTab);
}

void AGrimrockPartyPawn::ShowInventoryWidget()
{
	ShowInventoryWorkspace();
}

bool AGrimrockPartyPawn::EnsureSplitInventoryWorkspaceWidgets(APlayerController* PlayerController)
{
	if (!PlayerController || !IsSplitInventoryWorkspaceConfigured())
	{
		return false;
	}

	if (!CharacterSheetWidgetInstance)
	{
		CharacterSheetWidgetInstance = CreateWidget<UGridCharacterSheetWidget>(PlayerController, CharacterSheetWidgetClass);
		if (CharacterSheetWidgetInstance)
		{
			CharacterSheetWidgetInstance->InitializeInventoryWidget(this);
			if (CharacterSheetWidgetInstance->Button_CloseCharacterSheet)
			{
				CharacterSheetWidgetInstance->Button_CloseCharacterSheet->OnClicked.AddUniqueDynamic(
					this, &AGrimrockPartyPawn::HandleCharacterSheetWindowCloseClicked);
			}
		}
	}

	if (!InventoryBagWidgetInstance)
	{
		InventoryBagWidgetInstance = CreateWidget<UGridInventoryBagWidget>(PlayerController, InventoryBagWidgetClass);
		if (InventoryBagWidgetInstance)
		{
			InventoryBagWidgetInstance->InitializeInventoryWidget(this);
			if (InventoryBagWidgetInstance->Button_CloseInventoryBag)
			{
				InventoryBagWidgetInstance->Button_CloseInventoryBag->OnClicked.AddUniqueDynamic(
					this, &AGrimrockPartyPawn::HandleInventoryBagWindowCloseClicked);
			}
		}
	}

	return CharacterSheetWidgetInstance && InventoryBagWidgetInstance;
}

void AGrimrockPartyPawn::ShowInventoryWorkspace()
{
	if (IsMajorGameplayUiBlockedByCombat())
	{
		return;
	}

	if (bCharacterCreationModalActive || bIsPitFalling)
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		UE_LOG(LogGrimrockPartyUI, Warning, TEXT("GridInventory SplitWorkspace Show Failed Pawn=%s Reason=NoPlayerController"), *GetName());
		return;
	}

	if (!IsSplitInventoryWorkspaceConfigured())
	{
		UE_LOG(LogGrimrockPartyUI, Warning, TEXT("GridInventory SplitWorkspace Show Failed Pawn=%s Reason=WidgetClassesUnset"), *GetName());
		return;
	}

	if (!EnsureSplitInventoryWorkspaceWidgets(PlayerController))
	{
		UE_LOG(LogGrimrockPartyUI, Warning, TEXT("GridInventory SplitWorkspace Show Failed Pawn=%s Reason=CreateWidgetFailed"), *GetName());
		return;
	}

	// Inventory owns independent viewport windows; hide any other major page surface.
	if (MenuWidgetInstance)
	{
		MenuWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (MapWidgetInstance)
	{
		MapWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (!CharacterSheetWidgetInstance->IsInViewport())
	{
		CharacterSheetWidgetInstance->AddToViewport(100);
	}
	CharacterSheetWidgetInstance->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	CharacterSheetWidgetInstance->RefreshInventory();

	if (!InventoryBagWidgetInstance->IsInViewport())
	{
		InventoryBagWidgetInstance->AddToViewport(101);
	}
	InventoryBagWidgetInstance->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	InventoryBagWidgetInstance->RefreshInventory();

	bInventoryWorkspaceVisible = true;
	bInventoryWidgetVisible = true;

	RefreshPersistentHudWidget();

	ApplyMajorUiInputMode(true);
	UE_LOG(LogGrimrockPartyUI, Verbose, TEXT("GridInventory SplitWorkspace Shown Pawn=%s Sheet=%s Bag=%s"), *GetName(),
		*GetNameSafe(CharacterSheetWidgetInstance), *GetNameSafe(InventoryBagWidgetInstance));
}

void AGrimrockPartyPawn::CollapseInventoryWorkspaceForMenuPage()
{
	if (CharacterSheetWidgetInstance)
	{
		CharacterSheetWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (InventoryBagWidgetInstance)
	{
		InventoryBagWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);
	}
	bInventoryWorkspaceVisible = false;
}

void AGrimrockPartyPawn::ShowMenuPage(EInventoryTopTab TopTab)
{
	if (IsMajorGameplayUiBlockedByCombat())
	{
		return;
	}

	if (bCharacterCreationModalActive || bIsPitFalling)
	{
		return;
	}

	if (TopTab == EInventoryTopTab::Inventory)
	{
		ShowInventoryWorkspace();
		return;
	}
	if (TopTab == EInventoryTopTab::Map)
	{
		ShowMapWidget();
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		UE_LOG(LogGrimrockPartyUI, Warning, TEXT("GrimrockMenu UI Show Failed Pawn=%s Reason=NoPlayerController"), *GetName());
		return;
	}

	if (!MenuWidgetClass)
	{
		UE_LOG(LogGrimrockPartyUI, Warning, TEXT("GrimrockMenu UI Show Failed Pawn=%s Reason=NoMenuWidgetClass"), *GetName());
		return;
	}

	CollapseInventoryWorkspaceForMenuPage();
	if (MapWidgetInstance)
	{
		MapWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);
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
		UE_LOG(LogGrimrockPartyUI, Warning, TEXT("GrimrockMenu UI Show Failed Pawn=%s Reason=CreateWidgetFailed"), *GetName());
		return;
	}

	if (!MenuWidgetInstance->IsInViewport())
	{
		MenuWidgetInstance->AddToViewport(100);
	}
	MenuWidgetInstance->SetVisibility(ESlateVisibility::Visible);
	MenuWidgetInstance->SetActiveTopTab(TopTab);
	bInventoryWidgetVisible = true;

	RefreshPersistentHudWidget();

	ApplyMajorUiInputMode(true);
	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(MenuWidgetInstance->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	PlayerController->SetInputMode(InputMode);

	UE_LOG(LogGrimrockPartyUI, Verbose, TEXT("GrimrockMenu UI Shown Pawn=%s TopTab=%d"), *GetName(), static_cast<int32>(TopTab));
}

void AGrimrockPartyPawn::ApplyMajorUiInputMode(bool bOpen)
{
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		return;
	}

	if (AGrimrockPlayerController* GrimrockPlayerController = Cast<AGrimrockPlayerController>(PlayerController))
	{
		GrimrockPlayerController->SetInventoryUiOpen(bOpen);
	}

	PlayerController->bEnableClickEvents = true;
	PlayerController->bEnableMouseOverEvents = true;
	PlayerController->bShowMouseCursor = true;
	PlayerController->DefaultMouseCursor = EMouseCursor::Default;
	PlayerController->CurrentMouseCursor = EMouseCursor::Default;

	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	PlayerController->SetInputMode(InputMode);
}

void AGrimrockPartyPawn::HideMapWidget()
{
	CollapseMajorGameplayUi();
	UE_LOG(LogGrimrockPartyUI, Verbose, TEXT("GridMap Standalone Hidden Pawn=%s"), *GetName());
}

void AGrimrockPartyPawn::HandleMapWindowCloseClicked()
{
	HideMapWidget();
}

void AGrimrockPartyPawn::HandleCharacterSheetWindowCloseClicked()
{
	if (CharacterSheetWidgetInstance)
	{
		CharacterSheetWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);
	}
	RefreshMajorUiVisibilityAfterSplitClose();
}

void AGrimrockPartyPawn::HandleInventoryBagWindowCloseClicked()
{
	if (InventoryBagWidgetInstance)
	{
		InventoryBagWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);
	}
	RefreshMajorUiVisibilityAfterSplitClose();
}

void AGrimrockPartyPawn::RefreshMajorUiVisibilityAfterSplitClose()
{
	const bool bCharacterSheetVisible =
		CharacterSheetWidgetInstance && CharacterSheetWidgetInstance->GetVisibility() != ESlateVisibility::Collapsed &&
		CharacterSheetWidgetInstance->GetVisibility() != ESlateVisibility::Hidden;
	const bool bInventoryBagVisible =
		InventoryBagWidgetInstance && InventoryBagWidgetInstance->GetVisibility() != ESlateVisibility::Collapsed &&
		InventoryBagWidgetInstance->GetVisibility() != ESlateVisibility::Hidden;

	bInventoryWorkspaceVisible = bCharacterSheetVisible || bInventoryBagVisible;
	if (!bInventoryWorkspaceVisible)
	{
		HideInventoryWidget();
		return;
	}

	bInventoryWidgetVisible = true;
	RefreshPersistentHudWidget();
}

void AGrimrockPartyPawn::HandleGlobalEscape()
{
	if (bCharacterCreationModalActive || bIsPitFalling)
	{
		return;
	}

	if (CharacterSheetWidgetInstance && CharacterSheetWidgetInstance->IsItemActionMenuOpen())
	{
		CharacterSheetWidgetInstance->CloseItemActionMenu(FName(TEXT("Escape")));
		return;
	}
	if (InventoryBagWidgetInstance && InventoryBagWidgetInstance->IsItemActionMenuOpen())
	{
		InventoryBagWidgetInstance->CloseItemActionMenu(FName(TEXT("Escape")));
		return;
	}
	if (IsMapWidgetVisible())
	{
		HideMapWidget();
		return;
	}
	if (bInventoryWidgetVisible)
	{
		HideInventoryWidget();
		return;
	}

	OnInGameMainMenuRequested();
}

void AGrimrockPartyPawn::CollapseMajorGameplayUi()
{
	if (MenuWidgetInstance)
	{
		MenuWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (MapWidgetInstance)
	{
		MapWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (CharacterSheetWidgetInstance)
	{
		CharacterSheetWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (InventoryBagWidgetInstance)
	{
		InventoryBagWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);
	}

	bInventoryWorkspaceVisible = false;
	bInventoryWidgetVisible = false;

	RefreshPersistentHudWidget();

	ApplyMajorUiInputMode(false);
}

void AGrimrockPartyPawn::HideInventoryWidget()
{
	CollapseMajorGameplayUi();

	if (bAutoSaveOnInventoryClose && PartyInventoryComponent && PartyInventoryComponent->HasCompletedInitialCharacterCreation())
	{
		FText SaveError;
		if (!SaveCurrentGame(SaveError))
		{
			UE_LOG(LogGrimrockPartyUI, Warning, TEXT("PartySave InventoryClose Failed Slot=%s Reason=%s"), *PartySaveSlotName, *SaveError.ToString());
		}
	}

	UE_LOG(LogGrimrockPartyUI, Verbose, TEXT("Grimrock UI Hidden Pawn=%s"), *GetName());
}

void AGrimrockPartyPawn::HandleCombatStarted()
{
	if (CharacterSheetWidgetInstance && CharacterSheetWidgetInstance->IsItemActionMenuOpen())
	{
		CharacterSheetWidgetInstance->CloseItemActionMenu(FName(TEXT("CombatStarted")));
	}
	if (InventoryBagWidgetInstance && InventoryBagWidgetInstance->IsItemActionMenuOpen())
	{
		InventoryBagWidgetInstance->CloseItemActionMenu(FName(TEXT("CombatStarted")));
	}

	if (IsCustomRecruitCharacterCreationModalActive())
	{
		CloseCustomRecruitCharacterCreationWidget();
	}
	if (IsStoryCompanionRecruitmentModalActive())
	{
		CloseStoryCompanionRecruitmentWidget();
	}

	DismissReadableMessageIfVisible();
	CollapseMajorGameplayUi();
	UE_LOG(LogGrimrockPartyUI, Verbose, TEXT("UI-COMBAT01 CombatUiTransition Pawn=%s"), *GetName());
}

bool AGrimrockPartyPawn::IsMajorGameplayUiBlockedByCombat() const
{
	const UGridTurnManagerComponent* TurnManager = FindTurnManager();
	return IsValid(TurnManager) && TurnManager->bCombatActive;
}

UGridInventoryWidget* AGrimrockPartyPawn::GetInventoryWidget() const
{
	// Context actions can originate from equipment on the sheet or items in
	// the bag. Prefer the view that currently owns the modal item menu.
	if (CharacterSheetWidgetInstance && CharacterSheetWidgetInstance->IsItemActionMenuOpen())
	{
		return CharacterSheetWidgetInstance;
	}
	if (InventoryBagWidgetInstance && InventoryBagWidgetInstance->IsItemActionMenuOpen())
	{
		return InventoryBagWidgetInstance;
	}
	return InventoryBagWidgetInstance;
}

bool AGrimrockPartyPawn::ShowCombatActionPanelWidget()
{
	UGridTurnManagerComponent* TurnManager = FindTurnManager();

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		UE_LOG(LogGrimrockPartyUI, Warning, TEXT("GridCombatHud Show Failed Pawn=%s Reason=NoPlayerController"), *GetName());
		return false;
	}

	if (!CombatHudWidgetClass)
	{
		UE_LOG(LogGrimrockPartyUI, Warning, TEXT("GridCombatHud Show Failed Pawn=%s Reason=WidgetClassUnset"), *GetName());
		return false;
	}

	if (!CombatHudWidgetInstance)
	{
		CombatHudWidgetInstance = CreateWidget<UGridCombatHudWidget>(PlayerController, CombatHudWidgetClass);
	}
	if (!CombatHudWidgetInstance)
	{
		UE_LOG(LogGrimrockPartyUI, Warning, TEXT("GridCombatHud Show Failed Pawn=%s Reason=CreateWidgetFailed"), *GetName());
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

bool AGrimrockPartyPawn::ShowPersistentHudWidget()
{
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController || !PersistentHudWidgetClass)
	{
		return false;
	}

	if (!PersistentHudWidgetInstance)
	{
		PersistentHudWidgetInstance = CreateWidget<UGridPersistentHudWidget>(PlayerController, PersistentHudWidgetClass);
	}
	if (!PersistentHudWidgetInstance)
	{
		UE_LOG(LogGrimrockPartyUI, Warning, TEXT("GridPersistentHud Show Failed Pawn=%s Reason=CreateWidgetFailed"), *GetName());
		return false;
	}

	PersistentHudWidgetInstance->InitializePersistentHud(this);
	if (!PersistentHudWidgetInstance->IsInViewport())
	{
		PersistentHudWidgetInstance->AddToViewport(PersistentHudZOrder);
	}
	PersistentHudWidgetInstance->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	if (CombatHudWidgetInstance)
	{
		CombatHudWidgetInstance->RefreshFromSources();
	}
	return true;
}

void AGrimrockPartyPawn::HidePersistentHudWidget()
{
	if (PersistentHudWidgetInstance)
	{
		PersistentHudWidgetInstance->RemoveFromParent();
		PersistentHudWidgetInstance = nullptr;
	}
}

void AGrimrockPartyPawn::RefreshPersistentHudWidget()
{
	if (PersistentHudWidgetInstance)
	{
		PersistentHudWidgetInstance->RefreshFromSources();
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
	if (IsCombatHotbarExecutionBlocked() || !PartyInventoryComponent || SlotIndex < 0 || SlotIndex >= PartyInventoryComponent->GetCombatHotbarSlotCount())
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
		UE_LOG(LogGrimrockPartyUI, Error, TEXT("CharacterCreation UI Show Failed Pawn=%s Reason=NoPlayerController"), *GetName());
		return;
	}

	ApplyCharacterCreationInputMode(true);

	if (!CharacterCreationWidgetClass)
	{
		UE_LOG(LogGrimrockPartyUI, Error, TEXT("CharacterCreation UI Show Failed Pawn=%s Reason=NoWidgetClass"), *GetName());
		return;
	}

	if (!CharacterCreationWidgetInstance)
	{
		CharacterCreationWidgetInstance = CreateWidget<URPGCharacterCreationWidget>(PlayerController, CharacterCreationWidgetClass);
	}

	if (!CharacterCreationWidgetInstance)
	{
		UE_LOG(LogGrimrockPartyUI, Error, TEXT("CharacterCreation UI Show Failed Pawn=%s Reason=CreateWidgetFailed"), *GetName());
		return;
	}

	CharacterCreationWidgetInstance->InitializeCharacterCreationWidget(this);
	if (!CharacterCreationWidgetInstance->IsInViewport())
	{
		CharacterCreationWidgetInstance->AddToViewport(1000);
	}
	CharacterCreationWidgetInstance->SetVisibility(ESlateVisibility::Visible);
	CharacterCreationWidgetInstance->FocusNameInput();

	UE_LOG(LogGrimrockPartyUI, Verbose, TEXT("CharacterCreation UI Shown Pawn=%s"), *GetName());
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
		UE_LOG(LogGrimrockPartyUI, Warning, TEXT("PartySave InitialCharacter Failed Slot=%s Reason=%s"), *PartySaveSlotName, *SaveError.ToString());
	}

	UE_LOG(LogGrimrockPartyUI, Verbose, TEXT("CharacterCreation Completed Pawn=%s"), *GetName());
}

bool AGrimrockPartyPawn::IsCharacterCreationModalActive() const
{
	return bCharacterCreationModalActive;
}
