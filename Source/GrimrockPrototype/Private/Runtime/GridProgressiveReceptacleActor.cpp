#include "Runtime/GridProgressiveReceptacleActor.h"

#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "Materials/MaterialInterface.h"
#include "Runtime/GridLevelRuntimeActor.h"

void AGridProgressiveReceptacleActor::InitializeRuntimeWorldObject(
	const FGridRuntimeWorldObjectData& ObjectData, UStaticMesh* Mesh, const FTransform& WorldTransform)
{
	Super::InitializeRuntimeWorldObject(ObjectData, Mesh, WorldTransform);

	const FGridObjectBehaviorParams EffectiveBehavior = ResolveEffectiveBehavior(ObjectData);
	ProgressiveConsume = EffectiveBehavior.Receptacle.ProgressiveConsume;
	ProgressiveBaseMaterials.Reset();

	if (!ProgressiveConsume.bEnabled)
	{
		return;
	}

	if (MaxContainedItems <= 0)
	{
		UE_LOG(LogGridReceptacle, Warning,
			TEXT("GridProgressiveReceptacle invalid unlimited capacity: ObjectId=%s MaxContainedItems=%d. Clamping completion threshold to 1."),
			*ObjectId.ToString(), MaxContainedItems);
		MaxContainedItems = 1;
	}

	// PUZZLE01 contract: progress charges are never removable by the player.
	SetCanRemoveItem(false);
	CaptureProgressiveBaseMaterials();
	SuppressContainedItemVisuals();
	ApplyProgressivePresentation();

	if (ProgressiveConsume.MaterialSteps.Num() != MaxContainedItems)
	{
		UE_LOG(LogGridReceptacle, Warning,
			TEXT("GridProgressiveReceptacle presentation step count differs from completion threshold: ObjectId=%s Steps=%d Required=%d."),
			*ObjectId.ToString(), ProgressiveConsume.MaterialSteps.Num(), MaxContainedItems);
	}
}

bool AGridProgressiveReceptacleActor::TryInsertItem(
	FName ItemDefinitionId, UGridItemDefinitionAsset* ItemDefinition, AGrimrockPartyPawn* PartyPawn)
{
	const int32 PreviousItemCount = GetContainedItemCount();
	const bool bInserted = Super::TryInsertItem(ItemDefinitionId, ItemDefinition, PartyPawn);
	if (bInserted && ProgressiveConsume.bEnabled)
	{
		HandleSuccessfulProgressiveInsertion(PreviousItemCount);
	}
	return bInserted;
}

bool AGridProgressiveReceptacleActor::TryInsertItemInstanceFromCursor(
	const FGridItemInstance& CursorItem, FGridItemInstance& OutAcceptedItem)
{
	const int32 PreviousItemCount = GetContainedItemCount();
	const bool bInserted = Super::TryInsertItemInstanceFromCursor(CursorItem, OutAcceptedItem);
	if (bInserted && ProgressiveConsume.bEnabled)
	{
		HandleSuccessfulProgressiveInsertion(PreviousItemCount);
	}
	return bInserted;
}

int32 AGridProgressiveReceptacleActor::ForceClearRuntimeContents(bool bMarkInitialItemsRemoved)
{
	const int32 RemovedCount = Super::ForceClearRuntimeContents(bMarkInitialItemsRemoved);
	if (ProgressiveConsume.bEnabled)
	{
		ApplyProgressivePresentation();
	}
	return RemovedCount;
}

bool AGridProgressiveReceptacleActor::RestoreRuntimeContainedItem(const FGridRuntimeItemState& ItemState, AGridItemActor* ItemActor)
{
	const bool bRestored = Super::RestoreRuntimeContainedItem(ItemState, ItemActor);
	if (!bRestored || !ProgressiveConsume.bEnabled)
	{
		return bRestored;
	}

	// A progressive charge is durable state, not a removable world item.
	SetCanRemoveItem(false);
	SuppressContainedItemVisuals();
	ApplyProgressivePresentation();
	return true;
}

void AGridProgressiveReceptacleActor::CaptureProgressiveBaseMaterials()
{
	ProgressiveBaseMaterials.Reset();
	if (!MeshComponent)
	{
		return;
	}

	for (const FGridReceptacleProgressMaterialStep& Step : ProgressiveConsume.MaterialSteps)
	{
		if (Step.MaterialSlotName.IsNone())
		{
			UE_LOG(LogGridReceptacle, Warning,
				TEXT("GridProgressiveReceptacle material step ignored: ObjectId=%s Reason=EmptyMaterialSlotName."), *ObjectId.ToString());
			continue;
		}

		const int32 MaterialIndex = MeshComponent->GetMaterialIndex(Step.MaterialSlotName);
		if (MaterialIndex == INDEX_NONE)
		{
			UE_LOG(LogGridReceptacle, Warning,
				TEXT("GridProgressiveReceptacle material step ignored: ObjectId=%s Slot=%s Reason=MaterialSlotNotFound."), *ObjectId.ToString(),
				*Step.MaterialSlotName.ToString());
			continue;
		}

		if (!Step.Material)
		{
			UE_LOG(LogGridReceptacle, Warning,
				TEXT("GridProgressiveReceptacle material step ignored: ObjectId=%s Slot=%s Reason=NoMaterial."), *ObjectId.ToString(),
				*Step.MaterialSlotName.ToString());
			continue;
		}

		if (!ProgressiveBaseMaterials.Contains(Step.MaterialSlotName))
		{
			ProgressiveBaseMaterials.Add(Step.MaterialSlotName, MeshComponent->GetMaterial(MaterialIndex));
		}
	}
}

void AGridProgressiveReceptacleActor::ApplyProgressivePresentation()
{
	if (!ProgressiveConsume.bEnabled || !MeshComponent)
	{
		return;
	}

	// Always rebuild from the authored baseline so save/restore and runtime clears are deterministic.
	for (const TPair<FName, TObjectPtr<UMaterialInterface>>& Pair : ProgressiveBaseMaterials)
	{
		const int32 MaterialIndex = MeshComponent->GetMaterialIndex(Pair.Key);
		if (MaterialIndex != INDEX_NONE)
		{
			MeshComponent->SetMaterial(MaterialIndex, Pair.Value.Get());
		}
	}

	const int32 AppliedStepCount = FMath::Min(GetContainedItemCount(), ProgressiveConsume.MaterialSteps.Num());
	for (int32 StepIndex = 0; StepIndex < AppliedStepCount; ++StepIndex)
	{
		const FGridReceptacleProgressMaterialStep& Step = ProgressiveConsume.MaterialSteps[StepIndex];
		if (Step.MaterialSlotName.IsNone() || !Step.Material)
		{
			continue;
		}

		const int32 MaterialIndex = MeshComponent->GetMaterialIndex(Step.MaterialSlotName);
		if (MaterialIndex != INDEX_NONE)
		{
			MeshComponent->SetMaterial(MaterialIndex, Step.Material.Get());
		}
	}
}

void AGridProgressiveReceptacleActor::SuppressContainedItemVisuals()
{
	for (FGridContainedReceptacleItem& Item : ContainedItems)
	{
		if (IsValid(Item.ItemActor.Get()))
		{
			ClearContainedActor(Item);
		}
	}
	UpdateContainedItemInteractionCollision();
}

void AGridProgressiveReceptacleActor::HandleSuccessfulProgressiveInsertion(int32 PreviousItemCount)
{
	SuppressContainedItemVisuals();
	ApplyProgressivePresentation();

	const int32 CurrentItemCount = GetContainedItemCount();
	UE_LOG(LogGridReceptacle, Log,
		TEXT("GridProgressiveReceptacle progress: ObjectId=%s Previous=%d Current=%d Required=%d Complete=%s"), *ObjectId.ToString(), PreviousItemCount,
		CurrentItemCount, MaxContainedItems, CurrentItemCount >= MaxContainedItems ? TEXT("true") : TEXT("false"));

	if (MaxContainedItems > 0 && PreviousItemCount < MaxContainedItems && CurrentItemCount >= MaxContainedItems)
	{
		EmitProgressiveCompletion();
	}
}

void AGridProgressiveReceptacleActor::EmitProgressiveCompletion()
{
	AGridLevelRuntimeActor* RuntimeActor = Cast<AGridLevelRuntimeActor>(GetOwner());
	if (!RuntimeActor)
	{
		if (UWorld* World = GetWorld())
		{
			for (TActorIterator<AGridLevelRuntimeActor> It(World); It; ++It)
			{
				RuntimeActor = *It;
				break;
			}
		}
	}

	const bool bLinkExecuted = RuntimeActor && RuntimeActor->ExecuteLinksFromRuntimeObject(ObjectId, EGridObjectEvent::Activated);
	UE_LOG(LogGridReceptacle, Log, TEXT("GridProgressiveReceptacle completed: ObjectId=%s ActivatedLinkExecuted=%s"), *ObjectId.ToString(),
		bLinkExecuted ? TEXT("true") : TEXT("false"));
}