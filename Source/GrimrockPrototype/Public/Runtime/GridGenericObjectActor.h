#pragma once

#include "CoreMinimal.h"
#include "Runtime/GridRuntimeObjectActor.h"
#include "Runtime/GridInteractableInterface.h"
#include "GridGenericObjectActor.generated.h"

class UGridWorldObjectDefinitionAsset;
class UNiagaraComponent;
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

	/** Optional definition-driven Niagara presentation. It mirrors runtime active state and owns no gameplay state. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Components")
	TObjectPtr<UNiagaraComponent> ActiveNiagaraComponent;

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

	/** Applies presentation only; runtime activation remains owned by UGridActivationComponent. */
	void SetRuntimeActivePresentation(bool bActive);

	UFUNCTION(BlueprintCallable, Category = "Grid|Readable")
	bool HasReadableText() const;

	UFUNCTION(BlueprintCallable, Category = "Grid|Readable")
	FText GetReadableText() const;

	UFUNCTION(BlueprintCallable, Category = "Grid|Readable")
	void MarkAsRead();

	// TRAP01.5.4: register concrete BlueprintNativeEvent entry points so
	// ProcessEvent/Execute_* can reach this class's native implementations.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Grid Interaction")
	bool CanInteract(APawn* InstigatorPawn, UPrimitiveComponent* HitComponent) const;
	virtual bool CanInteract_Implementation(APawn* InstigatorPawn, UPrimitiveComponent* HitComponent) const override;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Grid Interaction")
	void Interact(APawn* InstigatorPawn, UPrimitiveComponent* HitComponent);
	virtual void Interact_Implementation(APawn* InstigatorPawn, UPrimitiveComponent* HitComponent) override;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Grid Interaction")
	EGridInteractionCursor GetInteractionCursor(UPrimitiveComponent* HitComponent) const;
	virtual EGridInteractionCursor GetInteractionCursor_Implementation(UPrimitiveComponent* HitComponent) const override;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Grid Interaction")
	FText GetInteractionText(UPrimitiveComponent* HitComponent) const;
	virtual FText GetInteractionText_Implementation(UPrimitiveComponent* HitComponent) const override;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Grid Interaction")
	void InteractWithHit(APawn* InstigatorPawn, UPrimitiveComponent* HitComponent, const FHitResult& HitResult);
	virtual void InteractWithHit_Implementation(APawn* InstigatorPawn, UPrimitiveComponent* HitComponent, const FHitResult& HitResult) override
	{
		(void)HitResult;
		Interact_Implementation(InstigatorPawn, HitComponent);
	}

protected:
	void ApplyDefinitionOptions(const UGridWorldObjectDefinitionAsset* Definition);
};
