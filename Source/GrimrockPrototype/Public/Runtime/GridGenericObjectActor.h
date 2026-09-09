#pragma once

#include "CoreMinimal.h"
#include "Runtime/GridRuntimeObjectActor.h"
#include "Runtime/GridInteractableInterface.h"
#include "GridGenericObjectActor.generated.h"

class UGridWorldObjectDefinitionAsset;
class UPointLightComponent;

/**
 * Generic runtime actor for data-driven decorative objects, props, readable objects
 * and simple light sources.
 *
 * The level runtime actor still computes placement. This actor applies generic
 * definition-driven options when InitializeRuntimeGenericObject is used. If it is spawned
 * through the base InitializeRuntimeWorldObject path, it still behaves as a safe static
 * visual actor.
 */
UCLASS()
class GRIMROCKPROTOTYPE_API AGridGenericObjectActor : public AGridRuntimeObjectActor, public IGridInteractableInterface
{
	GENERATED_BODY()

public:
	AGridGenericObjectActor();

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPointLightComponent> PointLightComponent;

	UPROPERTY(BlueprintReadOnly, Category = "Grid|Definition")
	TObjectPtr<const UGridWorldObjectDefinitionAsset> SourceWorldObjectDefinition;

	UPROPERTY(BlueprintReadOnly, Category = "Grid|Readable")
	FText RuntimeReadableText;

	UPROPERTY(BlueprintReadOnly, Category = "Grid|Readable")
	bool bRuntimeReadableOnlyOnce = false;

	UPROPERTY(BlueprintReadOnly, Category = "Grid|Readable")
	bool bRuntimeHasBeenRead = false;

public:
	/** Runtime-native generic world-object initializer. */
	void InitializeRuntimeGenericObject(const FGridRuntimeWorldObjectData& ObjectData, const UGridWorldObjectDefinitionAsset* Definition, UStaticMesh* Mesh,
		const FTransform& WorldTransform);

	UFUNCTION(BlueprintCallable, Category = "Grid|Readable")
	bool HasReadableText() const;

	UFUNCTION(BlueprintCallable, Category = "Grid|Readable")
	FText GetReadableText() const;

	UFUNCTION(BlueprintCallable, Category = "Grid|Readable")
	void MarkAsRead();

	virtual bool CanInteract_Implementation(APawn* InstigatorPawn, UPrimitiveComponent* HitComponent) const override;
	virtual void Interact_Implementation(APawn* InstigatorPawn, UPrimitiveComponent* HitComponent) override;
	virtual EGridInteractionCursor GetInteractionCursor_Implementation(UPrimitiveComponent* HitComponent) const override;
	virtual FText GetInteractionText_Implementation(UPrimitiveComponent* HitComponent) const override;

protected:
	void ApplyDefinitionOptions(const UGridWorldObjectDefinitionAsset* Definition);
};
