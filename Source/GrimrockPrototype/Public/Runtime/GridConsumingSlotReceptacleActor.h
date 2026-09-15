#pragma once

#include "CoreMinimal.h"
#include "Runtime/GridReceptacleActor.h"
#include "GridConsumingSlotReceptacleActor.generated.h"

class UBoxComponent;
class UMaterialInterface;
class UPointLightComponent;

/** Generic receptacle whose hit-targeted slots consume cursor items without creating contained or world items. */
UCLASS()
class GRIMROCKPROTOTYPE_API AGridConsumingSlotReceptacleActor : public AGridReceptacleActor
{
	GENERATED_BODY()

public:
	virtual void InitializeRuntimeWorldObject(
		const FGridRuntimeWorldObjectData& ObjectData, UStaticMesh* Mesh, const FTransform& WorldTransform) override;
	virtual bool TryPlaceCursorItemFromHit(AGrimrockPartyPawn* PartyPawn, const FHitResult& HitResult) override;
	virtual void CaptureRuntimeReceptacleState(FGridRuntimeReceptacleState& OutState) const override;
	virtual bool CanInteract_Implementation(APawn* InstigatorPawn, UPrimitiveComponent* HitComponent) const override;
	virtual void Interact_Implementation(APawn* InstigatorPawn, UPrimitiveComponent* HitComponent) override;
	virtual void InteractWithHit_Implementation(
		APawn* InstigatorPawn, UPrimitiveComponent* HitComponent, const FHitResult& HitResult) override;

	void RestoreConsumingSlotState(const FGridRuntimeReceptacleState& State);

	bool IsConsumingSlotFilled(FName SlotId) const;
	bool WasConsumingSlotsCompletionEmitted() const { return bCompletionEmitted; }
	UBoxComponent* GetConsumingSlotInteractionComponent(FName SlotId) const;
	UPointLightComponent* GetConsumingSlotLightComponent(FName SlotId) const;

private:
	struct FRuntimeConsumingSlot
	{
		FGridReceptacleConsumingSlotConfig Config;
		TObjectPtr<UBoxComponent> InteractionComponent = nullptr;
		TObjectPtr<UPointLightComponent> LightComponent = nullptr;
		TObjectPtr<UMaterialInterface> FilledMaterial = nullptr;
		int32 FilledMaterialIndex = INDEX_NONE;
	};

	void ClearRuntimeConsumingSlots();
	int32 FindSlotIndexFromHit(const FHitResult& HitResult) const;
	bool ValidateSlotPresentation(int32 SlotIndex, FString& OutError) const;
	void ApplySlotLight(int32 SlotIndex, bool bFilled);
	bool AreAllRequiredSlotsFilled() const;
	void PersistConsumingSlotState();

	TArray<FRuntimeConsumingSlot> RuntimeConsumingSlots;
	TSet<FName> FilledConsumingSlots;
	bool bCompletionEmitted = false;
	FName RuntimeDefinitionId = NAME_None;
};
