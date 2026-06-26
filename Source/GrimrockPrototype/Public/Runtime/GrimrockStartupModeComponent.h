#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GrimrockStartupModeComponent.generated.h"

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
};
