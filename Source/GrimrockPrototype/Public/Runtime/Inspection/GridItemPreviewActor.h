#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Runtime/Inspection/GridItemInspectionTypes.h"
#include "GridItemPreviewActor.generated.h"

class UGridItemVisualEffectsComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;

/** Presentation only, intentionally not a pickup or interactable.
 * Never applies a selection in an ordinary gameplay/editor world.
 */
UCLASS(Transient, NotPlaceable, NotBlueprintable)
class GRIMROCKPROTOTYPE_API AGridItemPreviewActor : public AActor
{
	GENERATED_BODY()

public:
	AGridItemPreviewActor();

	EGridItemPreviewResult ApplyPreview(const FGridItemInspectionView& View);
	void ClearPreview();

	UStaticMeshComponent* GetPreviewMesh() const { return MeshComponent; }
	UGridItemVisualEffectsComponent* GetPreviewEffects() const { return EffectsComponent; }

	virtual void Destroyed() override;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(Transient)
	TObjectPtr<UGridItemVisualEffectsComponent> EffectsComponent;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> FinishMaterials;
};
