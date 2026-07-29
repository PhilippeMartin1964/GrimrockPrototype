#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/GridObjectBehavior.h"
#include "GridMonsterMovementComponent.generated.h"

class AGridLevelRuntimeActor;
class AGridMonsterActor;
class UGridMonsterOccupancySubsystem;

DECLARE_LOG_CATEGORY_EXTERN (LogGridMonsterMovement, Log, All);

UENUM (BlueprintType)
enum class EGridMonsterMotionType : uint8
{
    None,
    Move,
    Turn
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams (
    FGridMonsterMoveCompletedSignature,
    FIntPoint, FromCell,
    FIntPoint, ToCell);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams (
    FGridMonsterTurnCompletedSignature,
    EGridEdge, FromFacing,
    EGridEdge, ToFacing);

/**
 * Visual interpolation and cell reservation for one grid monster.
 * Decisions remain discrete; Tick is enabled only while a move or turn is shown.
 */
UCLASS (ClassGroup = (Grid), meta = (BlueprintSpawnableComponent))
class GRIMROCKPROTOTYPE_API UGridMonsterMovementComponent : public UActorComponent
{
    GENERATED_BODY ()

public:
    UGridMonsterMovementComponent ();

    virtual void BeginPlay () override;
    virtual void EndPlay (const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent (
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Monster|Movement")
    bool bAutoInitialize = true;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Monster|Movement")
    bool bSnapToCellOnInitialize = true;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Monster|Movement")
    bool bInferCellFromActorLocation = true;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Monster|Movement")
    bool bUseEaseInOut = true;

    UPROPERTY (VisibleInstanceOnly, BlueprintReadOnly, Category = "Monster|Movement")
    EGridMonsterMotionType ActiveMotion = EGridMonsterMotionType::None;

    UPROPERTY (VisibleInstanceOnly, BlueprintReadOnly, Category = "Monster|Movement")
    FIntPoint ReservedCell = FIntPoint::ZeroValue;

    UPROPERTY (VisibleInstanceOnly, BlueprintReadOnly, Category = "Monster|Movement")
    TObjectPtr<AGridLevelRuntimeActor> RuntimeActor = nullptr;

    UPROPERTY (BlueprintAssignable, Category = "Monster|Movement")
    FGridMonsterMoveCompletedSignature OnMoveCompleted;

    UPROPERTY (BlueprintAssignable, Category = "Monster|Movement")
    FGridMonsterTurnCompletedSignature OnTurnCompleted;

    UFUNCTION (BlueprintCallable, Category = "Monster|Movement")
    bool InitializeMovement (AGridLevelRuntimeActor* InRuntimeActor = nullptr);

    UFUNCTION (BlueprintCallable, Category = "Monster|Movement")
    bool TryMove (EGridEdge Direction);

    UFUNCTION (BlueprintCallable, Category = "Monster|Movement")
    bool TryMoveForward ();

    UFUNCTION (BlueprintCallable, Category = "Monster|Movement")
    bool TryTurnLeft ();

    UFUNCTION (BlueprintCallable, Category = "Monster|Movement")
    bool TryTurnRight ();

    UFUNCTION (BlueprintCallable, Category = "Monster|Movement")
    bool TeleportToGridPose (FIntPoint Cell, EGridEdge Facing);

    UFUNCTION (BlueprintCallable, Category = "Monster|Movement")
    void CancelCurrentAction ();

    UFUNCTION (BlueprintCallable, Category = "Monster|Movement")
    void ReleaseOccupancy ();

    UFUNCTION (BlueprintCallable, Category = "Monster|Movement")
    void HandleOwnerDeath ();

    UFUNCTION (BlueprintPure, Category = "Monster|Movement")
    bool IsInitialized () const { return bInitialized; }

    UFUNCTION (BlueprintPure, Category = "Monster|Movement")
    bool IsBusy () const { return ActiveMotion != EGridMonsterMotionType::None; }

    UFUNCTION (BlueprintPure, Category = "Monster|Movement")
    AGridMonsterActor* GetMonsterOwner () const;

    UFUNCTION (BlueprintPure, Category = "Monster|Movement")
    UGridMonsterOccupancySubsystem* GetOccupancySubsystem () const { return OccupancySubsystem; }

private:
    UPROPERTY (Transient)
    TObjectPtr<UGridMonsterOccupancySubsystem> OccupancySubsystem = nullptr;

    bool bInitialized = false;
    float MotionElapsed = 0.0f;
    float MotionDuration = 0.0f;

    FIntPoint MotionStartCell = FIntPoint::ZeroValue;
    FIntPoint MotionTargetCell = FIntPoint::ZeroValue;
    EGridEdge MotionStartFacing = EGridEdge::North;
    EGridEdge MotionTargetFacing = EGridEdge::North;

    FVector MotionStartLocation = FVector::ZeroVector;
    FVector MotionTargetLocation = FVector::ZeroVector;
    FQuat MotionStartRotation = FQuat::Identity;
    FQuat MotionTargetRotation = FQuat::Identity;

    AGridLevelRuntimeActor* FindRuntimeActor () const;
    bool ValidateInitialization (AGridMonsterActor* Monster, AGridLevelRuntimeActor* CandidateRuntime) const;
    bool StartTurn (EGridEdge TargetFacing, int32 DirectionSign);
    void CompleteMove ();
    void CompleteTurn ();
    void ResetMotionState ();
    float GetMoveDuration () const;
    float GetTurnDuration () const;
    static bool IsCardinalDirection (EGridEdge Direction);
};
