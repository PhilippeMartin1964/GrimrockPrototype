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

	virtual void InitializeRuntimeWorldObject(
		const FGridRuntimeWorldObjectData& ObjectData, UStaticMesh* Mesh, const FTransform& WorldTransform) override;
};
