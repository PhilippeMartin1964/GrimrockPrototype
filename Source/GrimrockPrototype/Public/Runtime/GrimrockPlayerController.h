#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Runtime/GridInteractableInterface.h"
#include "GrimrockPlayerController.generated.h"

class UPrimitiveComponent;
class UUserWidget;
class AGridReceptacleActor;
class AGrimrockPartyPawn;

UCLASS ()
class GRIMROCKPROTOTYPE_API AGrimrockPlayerController : public APlayerController
{
    GENERATED_BODY ()

public:
    AGrimrockPlayerController ();

    virtual void BeginPlay () override;
    virtual void PlayerTick (float DeltaTime) override;
    virtual void SetupInputComponent () override;

    UPROPERTY (BlueprintReadOnly, Category = "UI")
    bool bInventoryUiOpen = false;

    UFUNCTION (BlueprintCallable, Category = "UI")
    void SetInventoryUiOpen (bool bOpen);

protected:
    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Grid Interaction", meta = (ClampMin = "0.0"))
    float MaxInteractionDistance = 300.f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Grid Interaction|Debug")
    bool bDebugMouseInteraction = false;

    //UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Grid Interaction|Cursor")
    //bool bUseCustomMouseCursor = true;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Grid Interaction|Cursor")
    TSubclassOf<UUserWidget> CustomCursorWidgetClass;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Grid Interaction|Cursor")
    TObjectPtr<UUserWidget> CustomCursorWidget;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Grid Interaction|Cursor")
    EGridInteractionCursor CurrentGridInteractionCursor = EGridInteractionCursor::Default;

    void HandleLeftMousePressed ();
    void UpdateHoveredInteractable ();
    void InitializeCustomCursor ();
    void SetGridInteractionCursor (EGridInteractionCursor NewCursor);
    bool TryGetInteractableUnderCursor (FHitResult& OutHitResult, AActor*& OutInteractableActor) const;
    bool TryGetReceptacleUnderCursor (FHitResult& OutHitResult, AGridReceptacleActor*& OutReceptacleActor) const;
    bool TryGetWorldHitUnderCursor (FHitResult& OutHitResult) const;
    bool TryResolveWorldDropFromHit (
        const FHitResult& HitResult,
        const AGrimrockPartyPawn* PartyPawn,
        int32& OutCellX,
        int32& OutCellY,
        FVector& OutLocalOffset) const;
    bool IsHitWithinInteractionDistance (const FHitResult& HitResult) const;
    void ShowInteractionFeedback (const FText& MessageText) const;
};
