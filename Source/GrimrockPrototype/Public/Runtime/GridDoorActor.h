#pragma once

#include "CoreMinimal.h"
#include "Core/GridTypes.h"
#include "Runtime/GridMechanismActor.h"
#include "GridDoorActor.generated.h"

class UBoxComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams (
    FOnGridDoorAnimationFinished,
    int32, CellX,
    int32, CellY,
    EGridEdge, Edge
);

UCLASS ()
class GRIMROCKPROTOTYPE_API AGridDoorActor : public AGridMechanismActor
{
    GENERATED_BODY ()

public:
    AGridDoorActor ();

    virtual void Tick (float DeltaSeconds) override;

public:
    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Door")
    float OpenHeight = 180.f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Door")
    float MoveDuration = 2.5f;

    UPROPERTY (BlueprintReadOnly, Category = "Door")
    bool bIsOpen = false;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Door|Chain")
    TObjectPtr<USceneComponent> ChainRootComponent;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Door|Chain")
    TObjectPtr<UStaticMeshComponent> ChainSupportMeshComponent;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Door|Chain")
    TObjectPtr<UStaticMeshComponent> ChainMovingMeshComponent;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Door|Chain")
    TObjectPtr<UBoxComponent> ChainInteractionBox;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Door|Chain")
    TObjectPtr<UStaticMesh> ChainSupportMesh;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Door|Chain")
    TObjectPtr<UStaticMesh> ChainMovingMesh;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Door|Chain")
    TObjectPtr<UMaterialInterface> ChainMaterial;

    UFUNCTION (BlueprintCallable, Category = "Door")
    void InitializeDoor (const FGridLevelObjectData& ObjectData, UStaticMesh* InMovingMesh, UMaterialInterface* InMovingMaterial,
        UStaticMesh* InFixedMesh, UMaterialInterface* InFixedMaterial, const FVector& ClosedWorldLocation, const FRotator& WorldRotation,
        bool bStartOpen);

    UFUNCTION (BlueprintCallable, Category = "Door")
    virtual void SetDoorOpenState (bool bOpen);

    UFUNCTION (BlueprintCallable, Category = "Door")
    virtual void SnapDoorOpenState (bool bOpen);

    UFUNCTION (BlueprintCallable, Category = "Door")
    virtual void OpenDoor ();

    UFUNCTION (BlueprintCallable, Category = "Door")
    void CloseDoor ();

    UFUNCTION (BlueprintCallable, Category = "Door|Chain")
    void PullChain ();

    UFUNCTION (BlueprintPure, Category = "Door|Chain")
    UBoxComponent* GetChainInteractionComponent () const;

    UFUNCTION (BlueprintCallable, Category = "Door")
    bool IsFullyOpen () const { return bIsOpen && !bIsAnimating; }

    UFUNCTION (BlueprintCallable, Category = "Door")
    bool IsFullyClosed () const { return !bIsOpen && !bIsAnimating; }

    UFUNCTION (BlueprintCallable, Category = "Door")
    bool IsAnimating () const { return bIsAnimating; }

    virtual void InitializeGridObject (const FGridLevelObjectData& ObjectData, UStaticMesh* Mesh, UMaterialInterface* Material,
        const FTransform& WorldTransform) override;

public:
    UPROPERTY (BlueprintAssignable, Category = "Door")
    FOnGridDoorAnimationFinished OnDoorAnimationFinished;
    
protected:
    virtual void UpdateAnimation (float DeltaSeconds);
    void InitializeChainMechanism (const FGridDoorAnimationParams& ChainParams);
    void UpdateChainAnimation (float DeltaSeconds);
    void RefreshTickEnabled ();

    FVector MovingClosedRelativeLocation = FVector::ZeroVector;
    FVector MovingOpenRelativeLocation = FVector::ZeroVector;
    FVector MoveStartRelativeLocation = FVector::ZeroVector;
    FVector MoveTargetRelativeLocation = FVector::ZeroVector;

    bool bIsAnimating = false;
    float MoveElapsed = 0.f;
    float CurrentMoveDuration = 0.f;

private:
    FVector ChainRestRelativeLocation = FVector::ZeroVector;
    FVector ChainPulledRelativeLocation = FVector::ZeroVector;
    float ChainAnimationElapsed = 0.f;
    float CurrentChainPullDuration = 0.25f;
    bool bIsChainAnimating = false;
};
