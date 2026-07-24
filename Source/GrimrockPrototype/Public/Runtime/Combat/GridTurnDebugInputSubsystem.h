#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GridTurnDebugInputSubsystem.generated.h"

class APlayerController;
class UInputComponent;
class UGridTurnManagerComponent;

/**
 * Native MON5 diagnostic input router.
 *
 * It binds NumPad 1-6 directly on the active PlayerController InputComponent,
 * so BP_GridLevelRuntimeActor no longer needs Auto Receive Input or Enable Input.
 * The bindings are intentionally excluded from Shipping builds.
 */
UCLASS ()
class GRIMROCKPROTOTYPE_API UGridTurnDebugInputSubsystem : public UWorldSubsystem
{
    GENERATED_BODY ()

public:
    virtual void Initialize (FSubsystemCollectionBase& Collection) override;
    virtual void OnWorldBeginPlay (UWorld& InWorld) override;
    virtual void Deinitialize () override;

private:
    FTimerHandle InputBindingTimerHandle;
    TWeakObjectPtr<APlayerController> BoundPlayerController;
    TWeakObjectPtr<UInputComponent> BoundInputComponent;

    void RefreshInputBinding ();
    bool TryBindInput ();
    UGridTurnManagerComponent* ResolveTurnManager () const;
    void LogCommandResult (const TCHAR* CommandName, bool bSucceeded) const;

    void HandleStartCombatFromPerception ();
    void HandleEndPlayerPhase ();
    void HandleAbortCombat ();
    void HandleLogTurnState ();
    void HandleStartCombatWithAllMonsters ();
    void HandleForceVictory ();
};
