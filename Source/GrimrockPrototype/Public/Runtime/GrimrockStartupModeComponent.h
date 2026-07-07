#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GrimrockStartupModeComponent.generated.h"

class AGridLevelRuntimeActor;
class AGrimrockPartyPawn;

/**
 * Applies the pending startup mode stored in UGrimrockGameInstance to the owning party pawn.
 *
 * Add this component to BP_GrimrockPartyPawn. Actor components begin play during
 * AActor::BeginPlay(), which is called from AGrimrockPartyPawn::Super::BeginPlay().
 * This lets the component set PartyStartupMode before the pawn executes its runtime
 * load/new-game logic.
 */
UCLASS(ClassGroup = (Grimrock), meta = (BlueprintSpawnableComponent))
class GRIMROCKPROTOTYPE_API UGrimrockStartupModeComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UGrimrockStartupModeComponent();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    void DeferNewGameRuntimeActivation(AGrimrockPartyPawn* PartyPawn);
    void TryActivateDeferredNewGameRuntime();

private:
    UPROPERTY(Transient)
    TObjectPtr<AGrimrockPartyPawn> CachedPartyPawn;

    UPROPERTY(Transient)
    TObjectPtr<AGridLevelRuntimeActor> DeferredRuntimeActor;

    UPROPERTY(Transient)
    bool bWaitingForInitialCharacterCreation = false;
};
