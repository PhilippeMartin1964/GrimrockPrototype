#pragma once

#include "CoreMinimal.h"
#include "Runtime/GridRuntimeObjectActor.h"
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
class GRIMROCKPROTOTYPE_API AGridDoorActor : public AGridRuntimeObjectActor
{
    GENERATED_BODY ()

public:
    AGridDoorActor ();

    virtual void Tick (float DeltaSeconds) override;

public:
    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Door")
    float OpenHeight = 180.f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Door")
    float MoveDuration = 0.25f;

    UPROPERTY (BlueprintReadOnly, Category = "Door")
    bool bIsOpen = false;

    UFUNCTION (BlueprintCallable, Category = "Door")
    void InitializeDoor (const FGridLevelObjectData& ObjectData, UStaticMesh* InDoorMesh, UMaterialInterface* InMaterial,
        const FVector& ClosedWorldLocation, const FRotator& WorldRotation, bool bStartOpen);

    UFUNCTION (BlueprintCallable, Category = "Door")
    void SetDoorOpenState (bool bOpen);

    UFUNCTION (BlueprintCallable, Category = "Door")
    void OpenDoor ();

    UFUNCTION (BlueprintCallable, Category = "Door")
    void CloseDoor ();

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