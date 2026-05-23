#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Runtime/GridInteractableInterface.h"
#include "GrimrockPlayerController.generated.h"

class UPrimitiveComponent;

UCLASS ()
class GRIMROCKPROTOTYPE_API AGrimrockPlayerController : public APlayerController
{
    GENERATED_BODY ()

public:
    AGrimrockPlayerController ();

    virtual void BeginPlay () override;
    virtual void PlayerTick (float DeltaTime) override;
    virtual void SetupInputComponent () override;

protected:
    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Grid Interaction", meta = (ClampMin = "0.0"))
    float MaxInteractionDistance = 300.f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Grid Interaction|Debug")
    bool bDebugMouseInteraction = false;

    void HandleLeftMousePressed ();
    void UpdateHoveredInteractable ();
    bool TryGetInteractableUnderCursor (FHitResult& OutHitResult, AActor*& OutInteractableActor) const;
    bool IsHitWithinInteractionDistance (const FHitResult& HitResult) const;
    EMouseCursor::Type ToMouseCursor (EGridInteractionCursor InteractionCursor) const;
};
