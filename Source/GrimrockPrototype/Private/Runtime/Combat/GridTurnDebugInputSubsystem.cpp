#include "Runtime/Combat/GridTurnDebugInputSubsystem.h"

#include "Components/InputComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC (LogGridTurnDebugInput, Log, All);

void UGridTurnDebugInputSubsystem::Initialize (FSubsystemCollectionBase& Collection)
{
    Super::Initialize (Collection);
    BoundPlayerController.Reset ();
    BoundInputComponent.Reset ();
}

void UGridTurnDebugInputSubsystem::OnWorldBeginPlay (UWorld& InWorld)
{
    Super::OnWorldBeginPlay (InWorld);

#if !UE_BUILD_SHIPPING
    const bool bSupportedWorld =
        InWorld.WorldType == EWorldType::Game ||
        InWorld.WorldType == EWorldType::PIE ||
        InWorld.WorldType == EWorldType::GamePreview;
    if (!bSupportedWorld)
    {
        return;
    }

    RefreshInputBinding ();
    InWorld.GetTimerManager ().SetTimer (
        InputBindingTimerHandle,
        this,
        &UGridTurnDebugInputSubsystem::RefreshInputBinding,
        0.50f,
        true);
#endif
}

void UGridTurnDebugInputSubsystem::Deinitialize ()
{
    if (UWorld* World = GetWorld ())
    {
        World->GetTimerManager ().ClearTimer (InputBindingTimerHandle);
    }

    BoundPlayerController.Reset ();
    BoundInputComponent.Reset ();
    Super::Deinitialize ();
}

void UGridTurnDebugInputSubsystem::RefreshInputBinding ()
{
#if !UE_BUILD_SHIPPING
    UWorld* World = GetWorld ();
    APlayerController* PlayerController = World ? World->GetFirstPlayerController () : nullptr;
    UInputComponent* InputComponent = PlayerController ? PlayerController->InputComponent.Get () : nullptr;

    if (PlayerController &&
        InputComponent &&
        BoundPlayerController.Get () == PlayerController &&
        BoundInputComponent.Get () == InputComponent)
    {
        return;
    }

    TryBindInput ();
#endif
}

bool UGridTurnDebugInputSubsystem::TryBindInput ()
{
#if UE_BUILD_SHIPPING
    return false;
#else
    UWorld* World = GetWorld ();
    APlayerController* PlayerController = World ? World->GetFirstPlayerController () : nullptr;
    UInputComponent* InputComponent = PlayerController ? PlayerController->InputComponent.Get () : nullptr;
    if (!PlayerController || !InputComponent)
    {
        return false;
    }

    FInputKeyBinding& StartPerceptionBinding = InputComponent->BindKey (
        EKeys::NumPadOne,
        IE_Pressed,
        this,
        &UGridTurnDebugInputSubsystem::HandleStartCombatFromPerception);
    StartPerceptionBinding.bConsumeInput = true;

    FInputKeyBinding& EndPlayerPhaseBinding = InputComponent->BindKey (
        EKeys::NumPadTwo,
        IE_Pressed,
        this,
        &UGridTurnDebugInputSubsystem::HandleEndPlayerPhase);
    EndPlayerPhaseBinding.bConsumeInput = true;

    FInputKeyBinding& AbortCombatBinding = InputComponent->BindKey (
        EKeys::NumPadThree,
        IE_Pressed,
        this,
        &UGridTurnDebugInputSubsystem::HandleAbortCombat);
    AbortCombatBinding.bConsumeInput = true;

    FInputKeyBinding& LogTurnStateBinding = InputComponent->BindKey (
        EKeys::NumPadFour,
        IE_Pressed,
        this,
        &UGridTurnDebugInputSubsystem::HandleLogTurnState);
    LogTurnStateBinding.bConsumeInput = true;

    FInputKeyBinding& StartAllBinding = InputComponent->BindKey (
        EKeys::NumPadFive,
        IE_Pressed,
        this,
        &UGridTurnDebugInputSubsystem::HandleStartCombatWithAllMonsters);
    StartAllBinding.bConsumeInput = true;

    FInputKeyBinding& ForceVictoryBinding = InputComponent->BindKey (
        EKeys::NumPadSix,
        IE_Pressed,
        this,
        &UGridTurnDebugInputSubsystem::HandleForceVictory);
    ForceVictoryBinding.bConsumeInput = true;

    BoundPlayerController = PlayerController;
    BoundInputComponent = InputComponent;

    UE_LOG (LogGridTurnDebugInput, Log,
        TEXT ("[GridTurnDebugInput] Bound NumPad 1-6 to PlayerController=%s InputComponent=%s"),
        *GetNameSafe (PlayerController),
        *GetNameSafe (InputComponent));
    return true;
#endif
}

UGridTurnManagerComponent* UGridTurnDebugInputSubsystem::ResolveTurnManager () const
{
    UWorld* World = GetWorld ();
    if (!World)
    {
        return nullptr;
    }

    APlayerController* PlayerController = BoundPlayerController.Get ();
    if (!PlayerController)
    {
        PlayerController = World->GetFirstPlayerController ();
    }

    const AGrimrockPartyPawn* PartyPawn = PlayerController
        ? Cast<AGrimrockPartyPawn> (PlayerController->GetPawn ())
        : nullptr;
    AGridLevelRuntimeActor* RuntimeActor = PartyPawn
        ? PartyPawn->LevelRuntimeActor.Get ()
        : nullptr;

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
        UE_LOG (LogGridTurnDebugInput, Warning,
            TEXT ("[GridTurnDebugInput] No GridLevelRuntimeActor could be resolved."));
        return nullptr;
    }

    UGridTurnManagerComponent* TurnManager =
        RuntimeActor->FindComponentByClass<UGridTurnManagerComponent> ();
    if (!TurnManager)
    {
        UE_LOG (LogGridTurnDebugInput, Warning,
            TEXT ("[GridTurnDebugInput] RuntimeActor=%s has no GridTurnManagerComponent."),
            *GetNameSafe (RuntimeActor));
    }
    return TurnManager;
}

void UGridTurnDebugInputSubsystem::LogCommandResult (
    const TCHAR* CommandName,
    bool bSucceeded) const
{
    const FString Message = FString::Printf (
        TEXT ("[GridTurnDebugInput] %s=%s"),
        CommandName ? CommandName : TEXT ("UnknownCommand"),
        bSucceeded ? TEXT ("true") : TEXT ("false"));

    UE_LOG (LogGridTurnDebugInput, Log, TEXT ("%s"), *Message);
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage (
            INDEX_NONE,
            1.50f,
            bSucceeded ? FColor::Green : FColor::Red,
            Message);
    }
}

void UGridTurnDebugInputSubsystem::HandleStartCombatFromPerception ()
{
    UGridTurnManagerComponent* TurnManager = ResolveTurnManager ();
    LogCommandResult (
        TEXT ("StartCombatFromPerception"),
        TurnManager && TurnManager->StartCombatFromPerception ());
}

void UGridTurnDebugInputSubsystem::HandleEndPlayerPhase ()
{
    UGridTurnManagerComponent* TurnManager = ResolveTurnManager ();
    LogCommandResult (
        TEXT ("EndPlayerPhase"),
        TurnManager && TurnManager->EndPlayerPhase ());
}

void UGridTurnDebugInputSubsystem::HandleAbortCombat ()
{
    UGridTurnManagerComponent* TurnManager = ResolveTurnManager ();
    if (TurnManager)
    {
        TurnManager->AbortCombat ();
    }
    LogCommandResult (
        TEXT ("AbortCombat"),
        TurnManager && TurnManager->CurrentPhase == EGridCombatPhase::Exploration);
}

void UGridTurnDebugInputSubsystem::HandleLogTurnState ()
{
    UGridTurnManagerComponent* TurnManager = ResolveTurnManager ();
    if (TurnManager)
    {
        TurnManager->LogCurrentTurnState ();
    }
    LogCommandResult (TEXT ("LogCurrentTurnState"), TurnManager != nullptr);
}

void UGridTurnDebugInputSubsystem::HandleStartCombatWithAllMonsters ()
{
    UGridTurnManagerComponent* TurnManager = ResolveTurnManager ();
    LogCommandResult (
        TEXT ("StartCombatWithAllMonsters"),
        TurnManager && TurnManager->StartCombatWithAllMonsters ());
}

void UGridTurnDebugInputSubsystem::HandleForceVictory ()
{
    UGridTurnManagerComponent* TurnManager = ResolveTurnManager ();
    if (TurnManager)
    {
        TurnManager->ForceVictory ();
    }
    LogCommandResult (
        TEXT ("ForceVictory"),
        TurnManager && TurnManager->CurrentPhase == EGridCombatPhase::Victory);
}
