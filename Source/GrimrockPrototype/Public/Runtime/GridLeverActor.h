#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/GridTypes.h"
#include "GridLeverActor.generated.h"

class UStaticMeshComponent;

UCLASS ()
class GRIMROCKPROTOTYPE_API AGridLeverActor : public AActor
{
    GENERATED_BODY ()

public:
    AGridLeverActor ();

    virtual void Tick (float DeltaSeconds) override;

public:
    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* SceneRoot;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* LeverMeshComponent;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Lever")
    float LeverOffPitch = 45.f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Lever")
    float LeverOnPitch = 135.f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Lever")
    float ToggleDuration = 0.10f;
    UPROPERTY (BlueprintReadOnly, Category = "Lever")
    bool bIsOn = false;

    UPROPERTY (BlueprintReadOnly, Category = "Lever")
    int32 CellX = INDEX_NONE;

    UPROPERTY (BlueprintReadOnly, Category = "Lever")
    int32 CellY = INDEX_NONE;

    UPROPERTY (BlueprintReadOnly, Category = "Lever")
    EGridEdge Edge = EGridEdge::None;

    UFUNCTION (BlueprintCallable, Category = "Lever")
    void InitializeLever (
        UStaticMesh* InLeverMesh,
        UMaterialInterface* InMaterial,
        const FVector& InWorldLocation,
        const FRotator& InWorldRotation,
        int32 InCellX,
        int32 InCellY,
        EGridEdge InEdge,
        bool bStartOn);

    UFUNCTION (BlueprintCallable, Category = "Lever")
    void SetLeverState (bool bNewOn);

    UFUNCTION (BlueprintCallable, Category = "Lever")
    void ToggleLever ();

    UFUNCTION (BlueprintCallable, Category = "Lever")
    bool MatchesEdge (int32 InCellX, int32 InCellY, EGridEdge InEdge) const;

protected:
    void UpdateAnimation (float DeltaSeconds);

private:
    FRotator OffRelativeRotation = FRotator::ZeroRotator;
    FRotator OnRelativeRotation = FRotator::ZeroRotator;
    FRotator AnimStartRotation = FRotator::ZeroRotator;
    FRotator AnimTargetRotation = FRotator::ZeroRotator;

    bool bIsAnimating = false;
    float AnimElapsed = 0.f;
};