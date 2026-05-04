#pragma once

#include "CoreMinimal.h"
#include "Runtime/GridRuntimeObjectActor.h"
#include "Core/GridObjectArchetypeAsset.h"
#include "GridMechanismActor.generated.h"

UCLASS ()
class GRIMROCKPROTOTYPE_API AGridMechanismActor : public AGridRuntimeObjectActor
{
    GENERATED_BODY ()

public:
    AGridMechanismActor ();
    virtual void InitializeMechanism (const FGridLevelObjectData& ObjectData,
        const UGridObjectArchetypeAsset* Archetype, const FTransform& WorldTransform);

protected:
    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Mechanism")
    TObjectPtr<UStaticMeshComponent> FixedMeshComponent = nullptr;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Mechanism")
    TObjectPtr<UStaticMeshComponent> MovingMeshComponent = nullptr;

protected:
    void SetFixedMesh (UStaticMesh* Mesh, UMaterialInterface* Material);
    void SetMovingMesh (UStaticMesh* Mesh, UMaterialInterface* Material);

    void SetMovingRelativeLocation (const FVector& RelativeLocation);
    FVector GetMovingRelativeLocation () const;
};