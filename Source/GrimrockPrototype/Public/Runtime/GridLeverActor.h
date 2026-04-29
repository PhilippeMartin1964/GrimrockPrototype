#pragma once

#include "CoreMinimal.h"
#include "Runtime/GridRuntimeObjectActor.h"
#include "Core/GridTypes.h"
#include "GridLeverActor.generated.h"

UCLASS ()
class GRIMROCKPROTOTYPE_API AGridLeverActor : public AGridRuntimeObjectActor
{
    GENERATED_BODY ()

public:
    AGridLeverActor ();

    virtual void Tick (float DeltaSeconds) override;

public:
    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Lever")
    float LeverOffPitch = 45.f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Lever")
    float LeverOnPitch = 135.f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Lever")
    float ToggleDuration = 0.10f;

    UPROPERTY (BlueprintReadOnly, Category = "Lever")
    bool bIsOn = false;

    UFUNCTION (BlueprintCallable, Category = "Lever")
    void InitializeLever (const FGridLevelObjectData& ObjectData, UStaticMesh* InLeverMesh, UMaterialInterface* InMaterial,
        const FVector& InWorldLocation, const FRotator& InWorldRotation, bool bStartOn);

    UFUNCTION (BlueprintCallable, Category = "Lever")
    void SetLeverState (bool bNewOn);

    UFUNCTION (BlueprintCallable, Category = "Lever")
    void ToggleLever ();

    virtual void InitializeGridObject (const FGridLevelObjectData& ObjectData, UStaticMesh* Mesh, UMaterialInterface* Material,
        const FTransform& WorldTransform) override;

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