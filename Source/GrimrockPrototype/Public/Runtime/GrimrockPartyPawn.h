#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Core/GridTypes.h"
#include "Core/GridDirectionUtils.h"
#include "GrimrockPartyPawn.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;
class AGridLevelRuntimeActor;
class AGridItemActor;
class UGridPartyInventoryComponent;

UCLASS ()
class GRIMROCKPROTOTYPE_API AGrimrockPartyPawn : public APawn
{
    GENERATED_BODY ()

public:
    AGrimrockPartyPawn ();

    virtual void BeginPlay () override;
    virtual void Tick (float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent (UInputComponent* PlayerInputComponent) override;

public:
    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* SceneRoot;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USpringArmComponent* SpringArm;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UCameraComponent* Camera;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USceneComponent> HeldItemRoot;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
    TObjectPtr<UGridPartyInventoryComponent> PartyInventoryComponent;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Grid")
    TObjectPtr<AGridLevelRuntimeActor> LevelRuntimeActor;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Grid")
    int32 CurrentCellX = 0;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Grid")
    int32 CurrentCellY = 0;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Grid")
    EGridEdge Facing = EGridEdge::North;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Movement")
    float MoveDuration = 0.36f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Movement")
    float TurnDuration = 0.12f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Movement")
    float EyeHeight = 110.f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Camera|View")
    FVector CameraLocalOffset = FVector (-40.f, 0.f, 0.f);

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Camera|View")

    FRotator CameraLocalRotationOffset = FRotator (-4.f, 0.f, 0.f);

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputMappingContext> DefaultMappingContext;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> MoveForwardAction;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> MoveBackwardAction;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> TurnLeftAction;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> TurnRightAction;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> StrafeLeftAction;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> StrafeRightAction;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> UseAction;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Input|Legacy")
    bool bEnableLegacyKeyboardUseAction = false;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Input Buffer")
    bool bEnableInputBuffer = true;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Input Buffer", meta = (ClampMin = "0.0"))
    float InputBufferMaxAge = 0.25f;
    // Head Bob
    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Camera|Head Bob")
    bool bEnableHeadBob = true;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Camera|Head Bob", meta = (ClampMin = "0.0"))
    float HeadBobVerticalAmplitude = 6.f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Camera|Head Bob", meta = (ClampMin = "0.0"))
    float HeadBobHorizontalAmplitude = 1.5f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Camera|Head Bob", meta = (ClampMin = "0.0"))
    float HeadBobReturnSpeed = 10.f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Camera|Head Bob")
    bool bHeadBobStrafeSway = true;
    // Free look
    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Camera|Free Look")
    float FreeLookYawLimit = 60.f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Camera|Free Look")
    float FreeLookPitchUpLimit = 35.f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Camera|Free Look")
    float FreeLookPitchDownLimit = 45.f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Camera|Free Look")
    float FreeLookSensitivityYaw = 0.20f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Camera|Free Look")
    float FreeLookSensitivityPitch = 0.20f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Camera|Free Look")
    bool bEnableFreeLookRecentering = true;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Camera|Free Look", meta = (EditCondition = "bEnableFreeLookRecentering", ClampMin = "0.0"))
    float FreeLookRecenteringSpeed = 6.f;

    UPROPERTY (BlueprintReadOnly, Category = "Camera|Free Look")
    bool bIsFreeLooking = false;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Inventory")
    TMap<FName, int32> InventoryItems;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Inventory")
    FName DefaultInteractionItemId = TEXT ("Item_Torch");

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Held Item")
    FName DefaultHeldItemArchetypeId = TEXT ("Item_Torch");

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Held Item")
    TSubclassOf<AGridItemActor> HeldTorchActorClass;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Held Item")
    FVector HeldItemRelativeLocation = FVector (45.f, 22.f, -24.f);

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Held Item")
    FRotator HeldItemRelativeRotation = FRotator::ZeroRotator;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Held Item")
    FVector HeldItemRelativeScale = FVector::OneVector;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Held Item")
    bool bAutoEquipTorchOnPickup = true;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Held Item")
    TObjectPtr<AGridItemActor> HeldItemActor;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Held Item")
    bool bHasTorchInHand = false;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Held Item")
    FName HeldItemArchetypeId = NAME_None;

    UFUNCTION (BlueprintCallable, Category = "Inventory")
    bool HasInventoryItem (FName ItemId) const;

    UFUNCTION (BlueprintCallable, Category = "Inventory")
    bool AddInventoryItem (FName ItemId, int32 Count = 1);

    UFUNCTION (BlueprintCallable, Category = "Inventory")
    bool RemoveInventoryItem (FName ItemId, int32 Count = 1);

    UFUNCTION (BlueprintCallable, Category = "Held Item")
    bool EquipHeldItem (FName ItemArchetypeId);

    UFUNCTION (BlueprintCallable, Category = "Held Item")
    void ClearHeldItem ();

    UFUNCTION (BlueprintCallable, Category = "Held Item")
    FName GetHeldItemArchetypeId () const;

    UFUNCTION (BlueprintCallable, Category = "Held Item")
    bool IsHoldingItem (FName ItemArchetypeId) const;

public:
    UFUNCTION (BlueprintCallable, Category = "Grid")
    void SnapToCurrentCell ();

    UFUNCTION (BlueprintCallable, Category = "Grid")
    void SetGridStart (AGridLevelRuntimeActor* InLevelRuntimeActor, int32 StartX, int32 StartY, EGridEdge StartFacing);

protected:
    void HandleMoveForward (const FInputActionValue& Value);
    void HandleMoveBackward (const FInputActionValue& Value);
    void HandleTurnLeft (const FInputActionValue& Value);
    void HandleTurnRight (const FInputActionValue& Value);
    void HandleStrafeLeft (const FInputActionValue& Value);
    void HandleStrafeRight (const FInputActionValue& Value);
    void HandleUse (const FInputActionValue& Value);

    bool TryUseFrontInteraction ();
    bool TryStartMove (EGridEdge MoveDirection);
    bool TryStartTurn (bool bTurnRight);

    void UpdateMove (float DeltaSeconds);
    void UpdateTurn (float DeltaSeconds);

    bool HasLevelRuntimeActor () const;
    bool CanMoveOnLevel (int32 FromX, int32 FromY, EGridEdge Direction) const;
    bool TryGetNeighborOnLevel (int32 X, int32 Y, EGridEdge Direction, int32& OutX, int32& OutY) const;
    FVector GetCellCenterOnLevel (int32 X, int32 Y, float ZOffset) const;
    bool TryToggleDoorOnLevel (int32 X, int32 Y, EGridEdge Edge);
    // Head Bob
    void UpdateHeadBob (float DeltaSeconds);
    void ApplyCameraOffsets ();
    // Free look
    void BeginFreeLook ();
    void EndFreeLook ();
    void UpdateFreeLook (float DeltaSeconds);
    void ApplyFreeLookRotation ();
    void ApplyCameraLocalViewOffset ();

    bool TryInteractOnLevel (int32 X, int32 Y, EGridEdge Edge);

private:
    enum class EBufferedCommandType : uint8
    {
        None,
        Move,
        Turn,
        Use
    };

    void BufferMoveCommand (EGridEdge MoveDirection);
    void BufferTurnCommand (bool bTurnRight);
    void BufferUseCommand ();
    void ClearBufferedCommand ();
    bool TryConsumeBufferedCommand ();
    bool IsBusy () const;

private:
    FVector MoveStartLocation = FVector::ZeroVector;
    FVector MoveTargetLocation = FVector::ZeroVector;
    float MoveElapsed = 0.f;
    bool bIsMoving = false;

    float TurnStartYaw = 0.f;
    float TurnTargetYaw = 0.f;
    float TurnElapsed = 0.f;
    float TurnDeltaYaw = 0.f;
    bool bIsTurning = false;

    EBufferedCommandType BufferedCommandType = EBufferedCommandType::None;
    EGridEdge BufferedMoveDirection = EGridEdge::None;
    bool bBufferedTurnRight = false;
    float BufferedCommandAge = 0.f;

    // Head Bob
    FVector SpringArmBaseRelativeLocation = FVector::ZeroVector;

    float HeadBobAlpha = 0.f;
    FVector CurrentHeadBobOffset = FVector::ZeroVector;
    FVector TargetHeadBobOffset = FVector::ZeroVector;
    EGridEdge ActiveMoveDirection = EGridEdge::None;
    //Free look
    float FreeLookYaw = 0.f;
    float FreeLookPitch = 0.f;

    int32 MoveStartCellX = 0;
    int32 MoveStartCellY = 0;
};
