#pragma once

#include "CoreMinimal.h"
#include "Runtime/GridRuntimeObjectActor.h"
#include "GridTriggerActor.generated.h"

UCLASS()
class GRIMROCKPROTOTYPE_API AGridTriggerActor : public AGridRuntimeObjectActor
{
	GENERATED_BODY()

public:
	AGridTriggerActor();

	virtual void InitializeGridObject(
		const FGridLevelObjectData& ObjectData, UStaticMesh* Mesh, UMaterialInterface* Material, const FTransform& WorldTransform) override;
};
