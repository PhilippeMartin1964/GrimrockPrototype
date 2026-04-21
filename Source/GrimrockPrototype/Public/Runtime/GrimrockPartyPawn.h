#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Core/GridTypes.h"
#include "GrimrockPartyPawn.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;
class AGridLevelRuntimeActor;

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

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Grid")
    TObjectPtr<AGridLevelRuntimeActor> LevelRuntimeActor;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Grid")
    int32 CurrentCellX = 0;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Grid")
    int32 CurrentCellY = 0;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Grid")
    EGridEdge Facing = EGridEdge::North;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Movement")
    float MoveDuration = 0.18f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Movement")
    float TurnDuration = 0.12f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Movement")
    float EyeHeight = 90.f;

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

public:
    UFUNCTION (BlueprintCallable, Category = "Grid")
    void SnapToCurrentCell ();

    UFUNCTION (BlueprintCallable, Category = "Grid")
    void SetGridStart (
        AGridLevelRuntimeActor* InLevelRuntimeActor,
        int32 StartX,
        int32 StartY,
        EGridEdge StartFacing);

protected:
    void HandleMoveForward (const FInputActionValue& Value);
    void HandleMoveBackward (const FInputActionValue& Value);
    void HandleTurnLeft (const FInputActionValue& Value);
    void HandleTurnRight (const FInputActionValue& Value);

    bool TryStartMove (EGridEdge MoveDirection);
    bool TryStartTurn (bool bTurnRight);

    void UpdateMove (float DeltaSeconds);
    void UpdateTurn (float DeltaSeconds);

    EGridEdge GetRelativeDirectionForward () const;
    EGridEdge GetRelativeDirectionBackward () const;

    static EGridEdge RotateLeft (EGridEdge Dir);
    static EGridEdge RotateRight (EGridEdge Dir);
    static float FacingToYaw (EGridEdge Dir);

    bool HasLevelRuntimeActor () const;
    bool CanMoveOnLevel (int32 FromX, int32 FromY, EGridEdge Direction) const;
    bool TryGetNeighborOnLevel (int32 X, int32 Y, EGridEdge Direction, int32& OutX, int32& OutY) const;
    FVector GetCellCenterOnLevel (int32 X, int32 Y, float ZOffset) const;

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
};
