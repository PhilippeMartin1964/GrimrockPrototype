#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Runtime/Inspection/GridItemInspectionTypes.h"
#include "GridItemPreviewActor.generated.h"

class UGridItemVisualEffectsComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UTextureCube;

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
	UStaticMeshComponent* GetPreviewOrnament() const { return OrnamentComponent; }
	FBoxSphereBounds GetPresentationBounds() const;
	UTextureCube* GetStudioCubemap() const { return StudioCubemap.Get(); }
	UGridItemVisualEffectsComponent* GetPreviewEffects() const { return EffectsComponent; }

	virtual void Destroyed() override;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Runtime environment lighting only; hard reference for cook. */
	UPROPERTY()
	TObjectPtr<UTextureCube> StudioCubemap;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> OrnamentComponent;

	UPROPERTY(Transient)
	TObjectPtr<UGridItemVisualEffectsComponent> EffectsComponent;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> FinishMaterials;
};
