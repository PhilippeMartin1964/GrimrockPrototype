#include "Runtime/GrimrockStartupModeComponent.h"

#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPIEPlaytestRequest.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockGameInstance.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "UI/GridDungeonBuildProgressWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogGrimrockStartupMode, Log, All);

#define LOCTEXT_NAMESPACE "GrimrockStartupModeComponent"

UGrimrockStartupModeComponent::UGrimrockStartupModeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UGrimrockStartupModeComponent::BeginPlay()
{
	Super::BeginPlay();

	AGrimrockPartyPawn* PartyPawn = Cast<AGrimrockPartyPawn>(GetOwner());
	if (!PartyPawn)
	{
		UE_LOG(LogGrimrockStartupMode, Warning, TEXT("GrimrockStartupMode Apply Failed Owner=%s Reason=OwnerIsNotGrimrockPartyPawn"), *GetNameSafe(GetOwner()));
		return;
	}

	CachedPartyPawn = PartyPawn;

	UGrimrockGameInstance* GrimrockGameInstance = GetWorld() ? GetWorld()->GetGameInstance<UGrimrockGameInstance>() : nullptr;
	if (!GrimrockGameInstance)
	{
		UE_LOG(LogGrimrockStartupMode, Verbose, TEXT("GrimrockStartupMode Apply Skipped Pawn=%s Reason=NoGrimrockGameInstance"), *GetNameSafe(PartyPawn));
		return;
	}

	PartyPawn->PartyStartupMode = GrimrockGameInstance->ConsumePendingStartupMode();

	bool bHasPendingLoadRequest = false;
	FString PendingLoadSlotName;
	int32 PendingLoadSlotUserIndex = 0;
	if (GrimrockGameInstance->ConsumePendingLoadSlot(PendingLoadSlotName, PendingLoadSlotUserIndex))
	{
		bHasPendingLoadRequest = true;
		PartyPawn->PartySaveSlotName = PendingLoadSlotName;
		PartyPawn->PartySaveUserIndex = PendingLoadSlotUserIndex;
		UE_LOG(LogGrimrockStartupMode, Log, TEXT("GrimrockStartupMode AppliedSaveSlot Pawn=%s Slot=%s UserIndex=%d"), *GetNameSafe(PartyPawn), *PartyPawn->PartySaveSlotName,
			PartyPawn->PartySaveUserIndex);
	}

	if (PartyPawn->PartyStartupMode == EGrimrockPartyStartupMode::NewGame)
	{
		DeferNewGameRuntimeActivation(PartyPawn);
	}
	else if (bHasPendingLoadRequest || PartyPawn->HasCurrentSave())
	{
		bWaitingForLoadedGameRuntime = true;
		ShowBuildProgress(LOCTEXT("LoadGameProgressTitle", "Chargement de la partie"), LOCTEXT("LoadGameProgressStart", "Lecture de la sauvegarde..."), 0.10f);
		SetWaitingTickEnabled();
	}

	UE_LOG(LogGrimrockStartupMode, Log, TEXT("GrimrockStartupMode Applied Pawn=%s Mode=%d Slot=%s UserIndex=%d"), *GetNameSafe(PartyPawn),
		static_cast<int32>(PartyPawn->PartyStartupMode), *PartyPawn->PartySaveSlotName, PartyPawn->PartySaveUserIndex);
}

void UGrimrockStartupModeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	TryActivateDeferredNewGameRuntime();
	TryCompleteLoadedGameProgress();
	SetWaitingTickEnabled();
}

void UGrimrockStartupModeComponent::DeferNewGameRuntimeActivation(AGrimrockPartyPawn* PartyPawn)
{
	if (!PartyPawn)
		return;

	AGridLevelRuntimeActor* RuntimeActor = PartyPawn->LevelRuntimeActor.Get();
	if (!RuntimeActor)
	{
		RuntimeActor = GridPIEPlaytestRequest::IsActiveForWorld(GetWorld())
			? GridPIEPlaytestRequest::ResolveMatchingRuntimeActor(GetWorld())
			: Cast<AGridLevelRuntimeActor>(UGameplayStatics::GetActorOfClass(GetWorld(), AGridLevelRuntimeActor::StaticClass()));
	}

	if (RuntimeActor)
	{
		PartyPawn->LevelRuntimeActor = RuntimeActor;
		RuntimeActor->DungeonRuntimeState = FGridDungeonRuntimeState();
		RuntimeActor->ClearVisuals(EGridRuntimeRebuildMode::Full);
		UE_LOG(LogGrimrockStartupMode, Log, TEXT("GrimrockStartupMode DeferredRuntimeActivation Pawn=%s Runtime=%s Reason=InitialCharacterCreationPending"),
			*GetNameSafe(PartyPawn), *GetNameSafe(RuntimeActor));
	}
	else
	{
		UE_LOG(LogGrimrockStartupMode, Warning, TEXT("GrimrockStartupMode DeferredRuntimeActivation MissingRuntimeActor Pawn=%s"), *GetNameSafe(PartyPawn));
	}

	DeferredRuntimeActor = RuntimeActor;
	bWaitingForInitialCharacterCreation = true;
	SetWaitingTickEnabled();
}

void UGrimrockStartupModeComponent::TryActivateDeferredNewGameRuntime()
{
	if (!bWaitingForInitialCharacterCreation)
		return;

	AGrimrockPartyPawn* PartyPawn = CachedPartyPawn.Get();
	if (!PartyPawn)
	{
		PartyPawn = Cast<AGrimrockPartyPawn>(GetOwner());
		CachedPartyPawn = PartyPawn;
	}
	if (!PartyPawn || !PartyPawn->PartyInventoryComponent)
		return;
	if (!PartyPawn->PartyInventoryComponent->HasCompletedInitialCharacterCreation())
		return;

	AGridLevelRuntimeActor* RuntimeActor = DeferredRuntimeActor.Get();
	if (!RuntimeActor)
		RuntimeActor = PartyPawn->LevelRuntimeActor.Get();
	if (!RuntimeActor)
	{
		RuntimeActor = GridPIEPlaytestRequest::IsActiveForWorld(GetWorld())
			? GridPIEPlaytestRequest::ResolveMatchingRuntimeActor(GetWorld())
			: Cast<AGridLevelRuntimeActor>(UGameplayStatics::GetActorOfClass(GetWorld(), AGridLevelRuntimeActor::StaticClass()));
	}
	if (!RuntimeActor)
	{
		UE_LOG(LogGrimrockStartupMode, Error, TEXT("GrimrockStartupMode ActivateDeferredRuntime Failed Pawn=%s Reason=NoRuntimeActor"), *GetNameSafe(PartyPawn));
		return;
	}

	ShowBuildProgress(LOCTEXT("NewGameProgressTitle", "Construction du donjon"), LOCTEXT("NewGameProgressPrepare", "Préparation du niveau..."), 0.05f);
	PartyPawn->LevelRuntimeActor = RuntimeActor;
	RuntimeActor->DungeonRuntimeState = FGridDungeonRuntimeState();
	UpdateBuildProgress(LOCTEXT("NewGameProgressGeometry", "Construction de la géométrie..."), 0.25f);
	RuntimeActor->RebuildLevel(EGridRuntimeRebuildMode::Full);
	UpdateBuildProgress(LOCTEXT("NewGameProgressParty", "Placement du groupe..."), 0.85f);
	PartyPawn->SnapToCurrentCell();
	RuntimeActor->HandlePartyCellChanged(PartyPawn->CurrentCellX, PartyPawn->CurrentCellY, PartyPawn->CurrentCellX, PartyPawn->CurrentCellY);

	bWaitingForInitialCharacterCreation = false;
	DeferredRuntimeActor = nullptr;
	CompleteBuildProgress(LOCTEXT("NewGameProgressReady", "Donjon prêt."));
	UE_LOG(LogGrimrockStartupMode, Log, TEXT("GrimrockStartupMode ActivatedDeferredRuntime Pawn=%s Runtime=%s"), *GetNameSafe(PartyPawn), *GetNameSafe(RuntimeActor));
}

void UGrimrockStartupModeComponent::TryCompleteLoadedGameProgress()
{
	if (!bWaitingForLoadedGameRuntime)
		return;

	AGrimrockPartyPawn* PartyPawn = CachedPartyPawn.Get();
	if (!PartyPawn)
	{
		PartyPawn = Cast<AGrimrockPartyPawn>(GetOwner());
		CachedPartyPawn = PartyPawn;
	}
	if (!PartyPawn || !PartyPawn->PartyInventoryComponent)
		return;

	if (PartyPawn->bCharacterCreationModalActive && !PartyPawn->PartyInventoryComponent->HasCompletedInitialCharacterCreation())
	{
		bWaitingForLoadedGameRuntime = false;
		CompleteBuildProgress(LOCTEXT("LoadGameProgressAborted", "Chargement interrompu."));
		return;
	}
	if (!PartyPawn->PartyInventoryComponent->HasCompletedInitialCharacterCreation())
		return;

	UpdateBuildProgress(LOCTEXT("LoadGameProgressRuntime", "Application de l'état du donjon..."), 0.85f);
	UpdateBuildProgress(LOCTEXT("LoadGameProgressReady", "Partie chargée."), 1.0f);
	bWaitingForLoadedGameRuntime = false;

	// A restored Level Up can pause the world on the next tick. Hiding the load
	// overlay synchronously avoids freezing its delayed hide timer above the modal.
	HideBuildProgress();
	UE_LOG(LogGrimrockStartupMode, Log, TEXT("GrimrockStartupMode LoadProgress HiddenImmediately Pawn=%s Reason=LoadedGameReady"), *GetNameSafe(PartyPawn));
}

void UGrimrockStartupModeComponent::SetWaitingTickEnabled()
{
	SetComponentTickEnabled(bWaitingForInitialCharacterCreation || bWaitingForLoadedGameRuntime);
}

void UGrimrockStartupModeComponent::ShowBuildProgress(const FText& Title, const FText& StatusText, float Progress)
{
	UWorld* World = GetWorld();
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	if (!PlayerController)
		return;
	if (World)
		World->GetTimerManager().ClearTimer(HideBuildProgressTimerHandle);

	TSubclassOf<UGridDungeonBuildProgressWidget> WidgetClass = BuildProgressWidgetClass;
	if (!WidgetClass)
	{
		WidgetClass = UGridDungeonBuildProgressWidget::StaticClass();
	}

	if (!BuildProgressWidgetInstance)
	{
		BuildProgressWidgetInstance = CreateWidget<UGridDungeonBuildProgressWidget>(PlayerController, WidgetClass);
	}
	if (!BuildProgressWidgetInstance)
	{
		UE_LOG(LogGrimrockStartupMode, Warning, TEXT("DungeonBuildProgress Show Failed Reason=CreateWidgetFailed"));
		return;
	}
	if (!BuildProgressWidgetInstance->IsInViewport())
		BuildProgressWidgetInstance->AddToViewport(5000);

	// Progress overlays are informational only and must never consume input from
	// a modal that may appear during startup restoration.
	BuildProgressWidgetInstance->SetVisibility(ESlateVisibility::HitTestInvisible);
	BuildProgressWidgetInstance->SetBuildTitle(Title);
	BuildProgressWidgetInstance->SetBuildProgress(Progress, StatusText);
}

void UGrimrockStartupModeComponent::UpdateBuildProgress(const FText& StatusText, float Progress)
{
	if (BuildProgressWidgetInstance)
		BuildProgressWidgetInstance->SetBuildProgress(Progress, StatusText);
}

void UGrimrockStartupModeComponent::CompleteBuildProgress(const FText& StatusText)
{
	UpdateBuildProgress(StatusText, 1.0f);
	UWorld* World = GetWorld();
	if (!World)
	{
		HideBuildProgress();
		return;
	}
	World->GetTimerManager().ClearTimer(HideBuildProgressTimerHandle);
	World->GetTimerManager().SetTimer(
		HideBuildProgressTimerHandle, this, &UGrimrockStartupModeComponent::HideBuildProgress, BuildProgressMinimumVisibleSeconds, false);
}

void UGrimrockStartupModeComponent::HideBuildProgress()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HideBuildProgressTimerHandle);
	}

	if (BuildProgressWidgetInstance)
	{
		BuildProgressWidgetInstance->RemoveFromParent();
		BuildProgressWidgetInstance = nullptr;
	}
}

#undef LOCTEXT_NAMESPACE
