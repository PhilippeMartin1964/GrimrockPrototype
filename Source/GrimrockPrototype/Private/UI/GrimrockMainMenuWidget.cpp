#include "UI/GrimrockMainMenuWidget.h"

#include "Components/Button.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "Runtime/GrimrockGameInstance.h"
#include "UI/RPGCharacterCreationWidget.h"
#include "UObject/UObjectGlobals.h"

DEFINE_LOG_CATEGORY_STATIC(LogGrimrockFrontendUI, Log, All);

void UGrimrockMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BindMainMenuButtons();
	RefreshSaveAvailabilityFromGameInstance();
	RefreshButtonStates();
}

void UGrimrockMainMenuWidget::SetHasValidSaveGame(bool bInHasValidSaveGame)
{
	bHasValidSaveGame = bInHasValidSaveGame;
	bHasLoadableSaveSlot = bInHasValidSaveGame;
	RefreshButtonStates();
}

bool UGrimrockMainMenuWidget::HasValidSaveGame() const
{
	return bHasValidSaveGame;
}

void UGrimrockMainMenuWidget::RefreshButtonStates()
{
	if (Button_Continue)
	{
		Button_Continue->SetIsEnabled(bHasValidSaveGame);
	}

	if (Button_LoadGame)
	{
		Button_LoadGame->SetIsEnabled(bHasLoadableSaveSlot);
	}
}

bool UGrimrockMainMenuWidget::OpenOptionsMenu()
{
	return OpenMainMenuModal(OptionsMenuWidgetClass, TEXT("NoOptionsMenuWidgetClass"));
}

bool UGrimrockMainMenuWidget::OpenCreditsMenu()
{
	return OpenMainMenuModal(CreditsMenuWidgetClass, TEXT("NoCreditsMenuWidgetClass"));
}

bool UGrimrockMainMenuWidget::OpenLicenseMenu()
{
	return OpenMainMenuModal(LicenseMenuWidgetClass, TEXT("NoLicenseMenuWidgetClass"));
}

void UGrimrockMainMenuWidget::QuitMainMenu()
{
	APlayerController* PlayerController = GetOwningPlayer();
	if (!PlayerController && GetWorld())
	{
		PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	}

	UE_LOG(LogGrimrockFrontendUI, Verbose, TEXT("MainMenu Quit Requested Widget=%s"), *GetName());
	UKismetSystemLibrary::QuitGame(this, PlayerController, EQuitPreference::Quit, false);
}

void UGrimrockMainMenuWidget::BindMainMenuButtons()
{
	if (Button_Continue)
	{
		Button_Continue->OnClicked.RemoveDynamic(this, &UGrimrockMainMenuWidget::HandleContinueClicked);
		Button_Continue->OnClicked.AddDynamic(this, &UGrimrockMainMenuWidget::HandleContinueClicked);
	}

	if (Button_NewGame)
	{
		Button_NewGame->OnClicked.RemoveDynamic(this, &UGrimrockMainMenuWidget::HandleNewGameClicked);
		Button_NewGame->OnClicked.AddDynamic(this, &UGrimrockMainMenuWidget::HandleNewGameClicked);
	}

	if (Button_LoadGame)
	{
		Button_LoadGame->OnClicked.RemoveDynamic(this, &UGrimrockMainMenuWidget::HandleLoadGameClicked);
		Button_LoadGame->OnClicked.AddDynamic(this, &UGrimrockMainMenuWidget::HandleLoadGameClicked);
	}

	if (Button_Options)
	{
		Button_Options->OnClicked.RemoveDynamic(this, &UGrimrockMainMenuWidget::HandleOptionsClicked);
		Button_Options->OnClicked.AddDynamic(this, &UGrimrockMainMenuWidget::HandleOptionsClicked);
	}

	if (Button_Credits)
	{
		Button_Credits->OnClicked.RemoveDynamic(this, &UGrimrockMainMenuWidget::HandleCreditsClicked);
		Button_Credits->OnClicked.AddDynamic(this, &UGrimrockMainMenuWidget::HandleCreditsClicked);
	}

	if (Button_License)
	{
		Button_License->OnClicked.RemoveDynamic(this, &UGrimrockMainMenuWidget::HandleLicenseClicked);
		Button_License->OnClicked.AddDynamic(this, &UGrimrockMainMenuWidget::HandleLicenseClicked);
	}

	if (Button_Quit)
	{
		Button_Quit->OnClicked.RemoveDynamic(this, &UGrimrockMainMenuWidget::HandleQuitClicked);
		Button_Quit->OnClicked.AddDynamic(this, &UGrimrockMainMenuWidget::HandleQuitClicked);
	}
}

void UGrimrockMainMenuWidget::RefreshSaveAvailabilityFromGameInstance()
{
	const UGrimrockGameInstance* GrimrockGameInstance = GetWorld() ? GetWorld()->GetGameInstance<UGrimrockGameInstance>() : nullptr;

	if (!GrimrockGameInstance)
	{
		bHasValidSaveGame = false;
		bHasLoadableSaveSlot = false;
		return;
	}

	bHasValidSaveGame = GrimrockGameInstance->HasDefaultPartySaveGame();
	bHasLoadableSaveSlot = GrimrockGameInstance->GetExistingPartySaveSlotInfos().Num() > 0;
}

bool UGrimrockMainMenuWidget::OpenMainMenuModal(TSubclassOf<UUserWidget> WidgetClass, const TCHAR* MissingClassReason)
{
	if (!WidgetClass)
	{
		UE_LOG(LogGrimrockFrontendUI, Warning, TEXT("MainMenu Modal Open Failed Widget=%s Reason=%s"), *GetName(), MissingClassReason);
		return false;
	}

	APlayerController* PlayerController = GetOwningPlayer();
	if (!PlayerController && GetWorld())
	{
		PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	}

	if (!PlayerController)
	{
		UE_LOG(LogGrimrockFrontendUI, Warning, TEXT("MainMenu Modal Open Failed Widget=%s Reason=NoPlayerController"), *GetName());
		return false;
	}

	UUserWidget* ModalWidget = CreateWidget<UUserWidget>(PlayerController, WidgetClass);
	if (!ModalWidget)
	{
		UE_LOG(LogGrimrockFrontendUI, Warning, TEXT("MainMenu Modal Open Failed Widget=%s Reason=CreateWidgetFailed Class=%s"), *GetName(), *GetNameSafe(WidgetClass));
		return false;
	}

	ModalWidget->AddToViewport(ModalZOrder);
	ModalWidget->SetVisibility(ESlateVisibility::Visible);

	UE_LOG(LogGrimrockFrontendUI, Verbose, TEXT("MainMenu Modal Opened Widget=%s Class=%s ZOrder=%d"), *GetName(), *GetNameSafe(WidgetClass), ModalZOrder);
	return true;
}

void UGrimrockMainMenuWidget::HandleContinueClicked()
{
	if (!bHasValidSaveGame)
	{
		return;
	}

	UGrimrockGameInstance* GrimrockGameInstance = GetWorld() ? GetWorld()->GetGameInstance<UGrimrockGameInstance>() : nullptr;
	if (!GrimrockGameInstance || !GrimrockGameInstance->RequestContinueDefaultPartySaveSlot() || !GrimrockGameInstance->OpenDungeonLevel(this))
	{
		UE_LOG(LogGrimrockFrontendUI, Warning, TEXT("MainMenu Continue Failed Widget=%s"), *GetName());
	}
}

void UGrimrockMainMenuWidget::HandleNewGameClicked()
{
	if (!OpenNewGameCharacterCreation())
	{
		UE_LOG(LogGrimrockFrontendUI, Error, TEXT("MainMenu NewGame Failed Widget=%s Reason=CharacterCreationUnavailable"), *GetName());
	}
}

bool UGrimrockMainMenuWidget::OpenNewGameCharacterCreation()
{
	APlayerController* PlayerController = GetOwningPlayer();
	if (!PlayerController && GetWorld())
	{
		PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	}
	if (!PlayerController)
	{
		UE_LOG(LogGrimrockFrontendUI, Error, TEXT("MainMenu NewGame Failed Widget=%s Reason=NoPlayerController"), *GetName());
		return false;
	}

	TSubclassOf<URPGCharacterCreationWidget> WidgetClass = CharacterCreationWidgetClass;
	if (!WidgetClass)
	{
		UE_LOG(LogGrimrockFrontendUI, Error, TEXT("MainMenu NewGame Failed Widget=%s Reason=NoCharacterCreationWidgetClass"), *GetName());
		return false;
	}

	// The menu is a UUserWidget, not an AActor. A component outered to
	// the widget has no game World and cannot resolve the GameInstance's
	// canonical ItemSystem when CreateInitialCharacter builds the starter kit.
	const UWorld* FrontendWorld = PlayerController->GetWorld();
	UGameInstance* FrontendGameInstance = FrontendWorld ? FrontendWorld->GetGameInstance() : nullptr;
	const UGridItemSubsystem* ItemRegistry = FrontendGameInstance
		? FrontendGameInstance->GetSubsystem<UGridItemSubsystem>() : nullptr;
	if (!ItemRegistry || !ItemRegistry->IsConfigured())
	{
		UE_LOG(LogGrimrockFrontendUI, Error, TEXT("MainMenu NewGame Failed Widget=%s Reason=ItemSystemNotConfigured"), *GetName());
		return false;
	}

	// A world-owned transient component shares the real frontend GI registry
	// with the later dungeon pawn. No legacy definitions, saved items or
	// character actors are created in the menu.
	NewGamePartyInventory = NewObject<UGridPartyInventoryComponent>(PlayerController);
	if (!NewGamePartyInventory)
	{
		UE_LOG(LogGrimrockFrontendUI, Error, TEXT("MainMenu NewGame Failed Widget=%s Reason=NoFrontendPartyInventory"), *GetName());
		return false;
	}
	PlayerController->AddInstanceComponent(NewGamePartyInventory);
	NewGamePartyInventory->RegisterComponent();
	if (!NewGamePartyInventory->IsRegistered() || NewGamePartyInventory->GetWorld() != FrontendWorld)
	{
		UE_LOG(LogGrimrockFrontendUI, Error, TEXT("MainMenu NewGame Failed Widget=%s Reason=FrontendInventoryWorldMismatch"), *GetName());
		NewGamePartyInventory->DestroyComponent();
		NewGamePartyInventory = nullptr;
		return false;
	}
	NewGamePartyInventory->ResetPartyForNewGame();

	NewGameCharacterCreationWidget = CreateWidget<URPGCharacterCreationWidget>(PlayerController, WidgetClass);
	if (!NewGameCharacterCreationWidget)
	{
		NewGamePartyInventory->DestroyComponent();
		NewGamePartyInventory = nullptr;
		UE_LOG(LogGrimrockFrontendUI, Error, TEXT("MainMenu NewGame Failed Widget=%s Reason=CreateCharacterCreationWidgetFailed"), *GetName());
		return false;
	}

	NewGameCharacterCreationWidget->InitializeCharacterCreationWidgetForInventory(
		NewGamePartyInventory, ERPGCharacterCreationContext::NewGameMainHero);
	NewGameCharacterCreationWidget->OnInitialCharacterCreationCommitted().AddUObject(
		this, &UGrimrockMainMenuWidget::HandleInitialCharacterCreationCommitted);
	NewGameCharacterCreationWidget->OnInitialCharacterCreationCancelled().AddUObject(
		this, &UGrimrockMainMenuWidget::HandleInitialCharacterCreationCancelled);
	NewGameCharacterCreationWidget->AddToViewport(CharacterCreationZOrder);
	NewGameCharacterCreationWidget->SetVisibility(ESlateVisibility::Visible);
	SetVisibility(ESlateVisibility::Collapsed);

	FInputModeUIOnly InputMode;
	// The wizard root is not focusable; Slate must not be asked to focus it.
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PlayerController->SetInputMode(InputMode);
	PlayerController->bShowMouseCursor = true;

	UE_LOG(LogGrimrockFrontendUI, Verbose, TEXT("MainMenu NewGame CharacterCreationShown Widget=%s Class=%s"), *GetName(), *GetNameSafe(WidgetClass));
	return true;
}

void UGrimrockMainMenuWidget::CloseNewGameCharacterCreation(bool bRestoreMainMenu)
{
	if (NewGameCharacterCreationWidget)
	{
		NewGameCharacterCreationWidget->OnInitialCharacterCreationCommitted().RemoveAll(this);
		NewGameCharacterCreationWidget->OnInitialCharacterCreationCancelled().RemoveAll(this);
		NewGameCharacterCreationWidget->RemoveFromParent();
		NewGameCharacterCreationWidget = nullptr;
	}
	if (NewGamePartyInventory)
	{
		NewGamePartyInventory->DestroyComponent();
		NewGamePartyInventory = nullptr;
	}

	if (bRestoreMainMenu)
	{
		SetVisibility(ESlateVisibility::Visible);
		RestoreMainMenuInput();
	}
}

void UGrimrockMainMenuWidget::RestoreMainMenuInput()
{
	APlayerController* PlayerController = GetOwningPlayer();
	if (!PlayerController && GetWorld())
	{
		PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	}
	if (!PlayerController)
	{
		return;
	}

	FInputModeUIOnly InputMode;
	// Main-menu root is also non-focusable; mouse UI needs no forced root focus.
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PlayerController->SetInputMode(InputMode);
	PlayerController->bShowMouseCursor = true;
}
void UGrimrockMainMenuWidget::HandleInitialCharacterCreationCommitted(URPGCharacterCreationWidget* SourceWidget)
{
	if (SourceWidget != NewGameCharacterCreationWidget.Get() || !NewGamePartyInventory ||
		!NewGamePartyInventory->HasCompletedInitialCharacterCreation())
	{
		UE_LOG(LogGrimrockFrontendUI, Error, TEXT("MainMenu NewGame Commit Rejected Widget=%s Reason=InvalidFrontendPartyState"), *GetName());
		return;
	}

	UGrimrockGameInstance* GrimrockGameInstance = GetWorld() ? GetWorld()->GetGameInstance<UGrimrockGameInstance>() : nullptr;
	if (!GrimrockGameInstance || !GrimrockGameInstance->SetPendingNewPartyState(NewGamePartyInventory->PartyInventoryState))
	{
		UE_LOG(LogGrimrockFrontendUI, Error, TEXT("MainMenu NewGame Commit Failed Widget=%s Reason=PendingPartyRejected"), *GetName());
		CloseNewGameCharacterCreation(true);
		return;
	}

	GrimrockGameInstance->SetPendingStartupMode(EGrimrockPartyStartupMode::NewGame);
	CloseNewGameCharacterCreation(false);

	if (!GrimrockGameInstance->OpenDungeonLevel(this))
	{
		GrimrockGameInstance->ClearPendingNewPartyState();
		GrimrockGameInstance->ClearPendingStartupMode();
		SetVisibility(ESlateVisibility::Visible);
		RestoreMainMenuInput();
		UE_LOG(LogGrimrockFrontendUI, Error, TEXT("MainMenu NewGame Commit Failed Widget=%s Reason=OpenDungeonFailed"), *GetName());
		return;
	}

	UE_LOG(LogGrimrockFrontendUI, Verbose, TEXT("MainMenu NewGame CharacterCreationCommitted Widget=%s"), *GetName());
}

void UGrimrockMainMenuWidget::HandleInitialCharacterCreationCancelled(URPGCharacterCreationWidget* SourceWidget)
{
	if (SourceWidget != NewGameCharacterCreationWidget.Get())
	{
		return;
	}

	UE_LOG(LogGrimrockFrontendUI, Verbose, TEXT("MainMenu NewGame CharacterCreationCancelled Widget=%s"), *GetName());
	CloseNewGameCharacterCreation(true);
}

void UGrimrockMainMenuWidget::HandleLoadGameClicked()
{
	if (!bHasLoadableSaveSlot)
	{
		return;
	}

	OnLoadGameRequested();
}

void UGrimrockMainMenuWidget::HandleOptionsClicked()
{
	if (OpenOptionsMenu())
	{
		return;
	}

	OnOptionsRequested();
}

void UGrimrockMainMenuWidget::HandleCreditsClicked()
{
	if (OpenCreditsMenu())
	{
		return;
	}

	OnCreditsRequested();
}

void UGrimrockMainMenuWidget::HandleLicenseClicked()
{
	if (OpenLicenseMenu())
	{
		return;
	}

	OnLicenseRequested();
}

void UGrimrockMainMenuWidget::HandleQuitClicked()
{
	if (bQuitDirectlyFromMainMenu)
	{
		QuitMainMenu();
		return;
	}

	OnQuitRequested();
}
