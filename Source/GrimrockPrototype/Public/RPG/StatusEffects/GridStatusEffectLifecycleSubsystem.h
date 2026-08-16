#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Runtime/Combat/GridCombatTypes.h"
#include "GridStatusEffectLifecycleSubsystem.generated.h"

class AActor;
class UGridTurnManagerComponent;

UCLASS ()
class GRIMROCKPROTOTYPE_API UGridStatusEffectLifecycleSubsystem : public UWorldSubsystem
{
    GENERATED_BODY ()

public:
    virtual void Initialize (FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize () override;
    virtual void OnWorldBeginPlay (UWorld& InWorld) override;
    virtual bool DoesSupportWorldType (EWorldType::Type WorldType) const override;

    void BindToTurnManager (UGridTurnManagerComponent* TurnManager);
    void UnbindFromTurnManager ();

    UGridTurnManagerComponent* GetBoundTurnManager () const
    {
        return BoundTurnManager.Get ();
    }

private:
    TWeakObjectPtr<UGridTurnManagerComponent> BoundTurnManager;
    FDelegateHandle ActorSpawnedDelegateHandle;
    int32 LastObservedRoundNumber = 0;

    void TryBindFromActor (AActor* Actor);
    void AdvanceAllRoundEffects (int32 BoundaryCount);

    UFUNCTION ()
    void HandleActorSpawned (AActor* Actor);

    UFUNCTION ()
    void HandleCombatantStateChanged (FGridCombatantInitiativeEntry Combatant);

    UFUNCTION ()
    void HandleRoundStarted (int32 RoundNumber);

    UFUNCTION ()
    void HandleCombatEnded (EGridCombatPhase ResultPhase);
};
