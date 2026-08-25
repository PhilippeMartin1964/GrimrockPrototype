#pragma once

#include "CoreMinimal.h"
#include "Runtime/GridRuntimeObjectActor.h"
#include "Runtime/GridInteractableInterface.h"
#include "GridGenericObjectActor.generated.h"

class UGridObjectArchetypeAsset;
class UPointLightComponent;

/**
 * Generic runtime actor for data-driven decorative objects, props, readable objects
 * and simple light sources.
 *
 * The level runtime actor still computes placement. This actor applies generic
 * archetype-driven options when InitializeGenericObject is used. If it is spawned
 * through the base InitializeGridObject path, it still behaves as a safe static
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

	UPROPERTY(BlueprintReadOnly, Category = "Grid|Archetype")
	TObjectPtr<const UGridObjectArchetypeAsset> SourceArchetype;

	UPROPERTY(BlueprintReadOnly, Category = "Grid|Readable")
	FText RuntimeReadableText;

	UPROPERTY(BlueprintReadOnly, Category = "Grid|Readable")
	bool bRuntimeReadableOnlyOnce = false;

	UPROPERTY(BlueprintReadOnly, Category = "Grid|Readable")
	bool bRuntimeHasBeenRead = false;

public:
	UFUNCTION(BlueprintCallable, Category = "Grid")
	void InitializeGenericObject(const FGridLevelObjectData& ObjectData, const UGridObjectArchetypeAsset* Archetype, UStaticMesh* Mesh,
		UMaterialInterface* Material, const FTransform& WorldTransform);

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
	void ApplyArchetypeOptions(const UGridObjectArchetypeAsset* Archetype);
};
