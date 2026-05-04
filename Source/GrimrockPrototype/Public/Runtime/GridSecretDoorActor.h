#pragma once

#include "CoreMinimal.h"
#include "Runtime/GridDoorActor.h"
#include "GridSecretDoorActor.generated.h"

class UStaticMeshComponent;

UCLASS ()
class GRIMROCKPROTOTYPE_API AGridSecretDoorActor : public AGridDoorActor
{
    GENERATED_BODY ()

public:
    AGridSecretDoorActor ();

    virtual void InitializeGridObject (const FGridLevelObjectData& ObjectData, UStaticMesh* Mesh, UMaterialInterface* Material,
        const FTransform& WorldTransform) override;
    virtual void SetDoorOpenState (bool bOpen) override;
    virtual void UpdateAnimation (float DeltaSeconds) override;
    virtual void OnConstruction (const FTransform& Transform) override;

public:
    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Secret Door")
    TObjectPtr<UStaticMesh> FixedMesh;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Secret Door")
    TObjectPtr<UMaterialInterface> FixedMaterial;

private:
    FVector MovingClosedRelativeLocation = FVector::ZeroVector;
    FVector MovingOpenRelativeLocation = FVector::ZeroVector;
    FVector MoveStartRelativeLocation = FVector::ZeroVector;
    FVector MoveTargetRelativeLocation = FVector::ZeroVector;

    float MoveElapsed = 0.f;
    float CurrentMoveDuration = 0.f;
};