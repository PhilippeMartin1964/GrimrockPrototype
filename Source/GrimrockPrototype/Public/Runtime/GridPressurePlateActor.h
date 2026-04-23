#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridPressurePlateActor.generated.h"

class UStaticMeshComponent;

UCLASS ()
class GRIMROCKPROTOTYPE_API AGridPressurePlateActor : public AActor
{
    GENERATED_BODY ()

public:
    AGridPressurePlateActor ();

    virtual void BeginPlay () override;
    virtual void Tick (float DeltaSeconds) override;

public:
    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* SceneRoot;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* PlateMeshComponent;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Plate")
    float ReleasedHeightAboveFloor = 4.f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Plate")
    float PressedHeightAboveFloor = 1.f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Plate")
    float MoveDuration = 0.08f;

    UPROPERTY (BlueprintReadOnly, Category = "Plate")
    bool bIsPressed = false;

    UPROPERTY (BlueprintReadOnly, Category = "Plate")
    int32 CellX = INDEX_NONE;

    UPROPERTY (BlueprintReadOnly, Category = "Plate")
    int32 CellY = INDEX_NONE;

    UFUNCTION (BlueprintCallable, Category = "Plate")
    void InitializePlate (
        UStaticMesh* InPlateMesh,
        UMaterialInterface* InMaterial,
        const FVector& InWorldLocation,
        int32 InCellX,
        int32 InCellY,
        bool bStartPressed);

    UFUNCTION (BlueprintCallable, Category = "Plate")
    void SetPressed (bool bNewPressed);

    UFUNCTION (BlueprintCallable, Category = "Plate")
    bool MatchesCell (int32 InCellX, int32 InCellY) const;

protected:
    void UpdateAnimation (float DeltaSeconds);

private:
    FVector ReleasedLocation = FVector::ZeroVector;
    FVector PressedLocation = FVector::ZeroVector;
    FVector AnimStartLocation = FVector::ZeroVector;
    FVector AnimTargetLocation = FVector::ZeroVector;

    bool bIsAnimating = false;
    float AnimElapsed = 0.f;
};