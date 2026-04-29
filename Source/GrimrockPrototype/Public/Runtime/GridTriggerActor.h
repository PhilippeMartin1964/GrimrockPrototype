#pragma once

#include "CoreMinimal.h"
#include "Runtime/GridRuntimeObjectActor.h"
#include "GridTriggerActor.generated.h"

UCLASS ()
class GRIMROCKPROTOTYPE_API AGridTriggerActor : public AGridRuntimeObjectActor
{
    GENERATED_BODY ()

public:
    AGridTriggerActor ();

    void InitializeTrigger (const FGridLevelObjectData& ObjectData, UStaticMesh* Mesh, UMaterialInterface* Material,
        const FVector& WorldLocation, const FRotator& WorldRotation);

    virtual void InitializeGridObject (const FGridLevelObjectData& ObjectData, UStaticMesh* Mesh, UMaterialInterface* Material,
        const FTransform& WorldTransform) override;
};