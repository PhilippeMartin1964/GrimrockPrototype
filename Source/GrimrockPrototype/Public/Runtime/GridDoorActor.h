#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/GridTypes.h"
#include "GridDoorActor.generated.h"

class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams (
    FOnGridDoorAnimationFinished,
    int32, CellX,
    int32, CellY,
    EGridEdge, Edge
);

UCLASS ()
class GRIMROCKPROTOTYPE_API AGridDoorActor : public AActor
{
    GENERATED_BODY ()

public:
    AGridDoorActor ();

    virtual void BeginPlay () override;
    virtual void Tick (float DeltaSeconds) override;

public:
    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* SceneRoot;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* DoorMeshComponent;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Door")
    float OpenHeight = 180.f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Door")
    float MoveDuration = 0.25f;

    UPROPERTY (BlueprintReadOnly, Category = "Door")
    bool bIsOpen = false;

    UPROPERTY (BlueprintReadOnly, Category = "Door")
    int32 CellX = INDEX_NONE;

    UPROPERTY (BlueprintReadOnly, Category = "Door")
    int32 CellY = INDEX_NONE;

    UPROPERTY (BlueprintReadOnly, Category = "Door")
    EGridEdge Edge = EGridEdge::None;

    UFUNCTION (BlueprintCallable, Category = "Door")
    void InitializeDoor (
        UStaticMesh* InDoorMesh,
        UMaterialInterface* InMaterial,
        const FVector& ClosedWorldLocation,
        const FRotator& WorldRotation,
        int32 InCellX,
        int32 InCellY,
        EGridEdge InEdge,
        bool bStartOpen);

    UFUNCTION (BlueprintCallable, Category = "Door")
    void SetDoorOpenState (bool bOpen);

    UFUNCTION (BlueprintCallable, Category = "Door")
    void OpenDoor ();

    UFUNCTION (BlueprintCallable, Category = "Door")
    void CloseDoor ();

    UFUNCTION (BlueprintCallable, Category = "Door")
    bool MatchesEdge (int32 InCellX, int32 InCellY, EGridEdge InEdge) const;

    UFUNCTION (BlueprintCallable, Category = "Door")
    bool IsFullyOpen () const { return bIsOpen && !bIsAnimating; }

    UFUNCTION (BlueprintCallable, Category = "Door")
    bool IsFullyClosed () const { return !bIsOpen && !bIsAnimating; }

    UFUNCTION (BlueprintCallable, Category = "Door")
    bool IsAnimating () const { return bIsAnimating; }

public:
    UPROPERTY (BlueprintAssignable, Category = "Door")
    FOnGridDoorAnimationFinished OnDoorAnimationFinished;

protected:
    void UpdateAnimation (float DeltaSeconds);

private:
    FVector ClosedLocation = FVector::ZeroVector;
    FVector OpenLocation = FVector::ZeroVector;
    FVector MoveStartLocation = FVector::ZeroVector;
    FVector MoveTargetLocation = FVector::ZeroVector;

    bool bIsAnimating = false;
    float MoveElapsed = 0.f;
    float CurrentMoveDuration = 0.f;
};