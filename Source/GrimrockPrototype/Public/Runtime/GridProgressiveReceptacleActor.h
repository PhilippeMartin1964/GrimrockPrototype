#pragma once

#include "CoreMinimal.h"
#include "Runtime/GridReceptacleActor.h"
#include "GridProgressiveReceptacleActor.generated.h"

class UMaterialInterface;

/**
 * PUZZLE01 generic progressive consumable receptacle.
 *
 * Accepted items are removed from player ownership by the existing transfer path, but the receptacle keeps
 * their logical contained-item records as durable progress charges. Their individual AGridItemActor visuals
 * are suppressed. Progress is therefore already persisted by FGridRuntimeReceptacleState without introducing
 * a parallel counter or a new SaveGame schema field.
 *
 * MaxContainedItems is the completion threshold. Reaching it applies the final material step and emits
 * EGridObjectEvent::Activated exactly on the successful insertion that completes the mechanism.
 */
UCLASS(Blueprintable)
class GRIMROCKPROTOTYPE_API AGridProgressiveReceptacleActor : public AGridReceptacleActor
{
	GENERATED_BODY()

public:
	virtual void InitializeRuntimeWorldObject(
		const FGridRuntimeWorldObjectData& ObjectData, UStaticMesh* Mesh, const FTransform& WorldTransform) override;

	virtual bool TryInsertItem(FName ItemDefinitionId, UGridItemDefinitionAsset* ItemDefinition, AGrimrockPartyPawn* PartyPawn) override;
	virtual bool TryInsertItemInstanceFromCursor(const FGridItemInstance& CursorItem, FGridItemInstance& OutAcceptedItem) override;

	virtual int32 ForceClearRuntimeContents(bool bMarkInitialItemsRemoved) override;
	virtual bool RestoreRuntimeContainedItem(const FGridRuntimeItemState& ItemState, AGridItemActor* ItemActor) override;

	UFUNCTION(BlueprintPure, Category = "Receptacle|Progressive Consume")
	bool IsProgressiveConsumeEnabled() const
	{
		return ProgressiveConsume.bEnabled;
	}

	UFUNCTION(BlueprintPure, Category = "Receptacle|Progressive Consume")
	int32 GetProgressiveItemCount() const
	{
		return GetContainedItemCount();
	}

	UFUNCTION(BlueprintPure, Category = "Receptacle|Progressive Consume")
	int32 GetProgressiveRequiredItemCount() const
	{
		return MaxContainedItems;
	}

protected:
	void CaptureProgressiveBaseMaterials();
	void ApplyProgressivePresentation();
	void SuppressContainedItemVisuals();
	void HandleSuccessfulProgressiveInsertion(int32 PreviousItemCount);
	void EmitProgressiveCompletion();

protected:
	/** Runtime snapshot resolved from Definition.DefaultBehavior.Receptacle.ProgressiveConsume. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Receptacle|Progressive Consume")
	FGridReceptacleProgressiveConsumeParams ProgressiveConsume;

	/** Original materials for every configured progressive material slot, used when progress is cleared/restored. */
	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UMaterialInterface>> ProgressiveBaseMaterials;
};