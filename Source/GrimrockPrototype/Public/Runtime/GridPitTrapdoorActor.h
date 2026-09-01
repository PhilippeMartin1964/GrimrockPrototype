#pragma once

#include "CoreMinimal.h"
#include "Runtime/GridMechanismActor.h"
#include "GridPitTrapdoorActor.generated.h"

/**
 * PIT03 presentation actor for a controlled pit.
 * Gameplay open/closed authority remains in AGridLevelRuntimeActor.
 * FixedMesh = permanent pit surround/open-pit geometry.
 * MovingMesh = optional closed trapdoor cover.
 */
UCLASS()
class GRIMROCKPROTOTYPE_API AGridPitTrapdoorActor : public AGridMechanismActor
{
	GENERATED_BODY()

public:
	AGridPitTrapdoorActor();

	virtual void InitializeMechanismVisuals(
		const FGridLevelObjectData& ObjectData, const UGridObjectArchetypeAsset* Archetype, const FTransform& WorldTransform) override;

	virtual void InitializeGridObject(
		const FGridLevelObjectData& ObjectData, UStaticMesh* Mesh, UMaterialInterface* Material, const FTransform& WorldTransform) override;

	UFUNCTION(BlueprintCallable, Category = "Pit")
	void SetPitOpenVisualState(bool bOpen, bool bPlayAudio = true);

	UFUNCTION(BlueprintPure, Category = "Pit")
	bool IsPitOpenVisualState() const
	{
		return bIsOpen;
	}

private:
	bool bIsOpen = true;
};
