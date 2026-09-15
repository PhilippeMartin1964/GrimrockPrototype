#include "Runtime/GridConsumingSlotReceptacleActor.h"

#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Materials/MaterialInterface.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridInteractionUtils.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"

void AGridConsumingSlotReceptacleActor::InitializeRuntimeWorldObject(
	const FGridRuntimeWorldObjectData& ObjectData, UStaticMesh* Mesh, const FTransform& WorldTransform)
{
	ClearRuntimeConsumingSlots();
	Super::InitializeRuntimeWorldObject(ObjectData, Mesh, WorldTransform);

	RuntimeDefinitionId = ObjectData.WorldObjectDefinitionId;
	FilledConsumingSlots.Reset();
	bCompletionEmitted = false;

	const FGridObjectBehaviorParams EffectiveBehavior = ResolveEffectiveBehavior(ObjectData);
	const AGridLevelRuntimeActor* RuntimeActor = Cast<AGridLevelRuntimeActor>(GetOwner());
	const UGridWorldObjectDefinitionAsset* Definition = RuntimeActor ? RuntimeActor->FindWorldObjectDefinition(RuntimeDefinitionId) : nullptr;

	for (const FGridReceptacleConsumingSlotConfig& Config : EffectiveBehavior.Receptacle.ConsumingSlots)
	{
		if (Config.SlotId.IsNone() || RuntimeConsumingSlots.ContainsByPredicate(
			[&Config](const FRuntimeConsumingSlot& Existing) { return Existing.Config.SlotId == Config.SlotId; }))
		{
			UE_LOG(LogGridReceptacle, Warning, TEXT("Consuming slot skipped: ObjectId=%s SlotId=%s Reason=invalid or duplicate id"),
				*ObjectId.ToString(), *Config.SlotId.ToString());
			continue;
		}

		FRuntimeConsumingSlot& Slot = RuntimeConsumingSlots.AddDefaulted_GetRef();
		Slot.Config = Config;

		const FName InteractionName(*FString::Printf(TEXT("ConsumingSlot_%s"), *Config.SlotId.ToString()));
		Slot.InteractionComponent = NewObject<UBoxComponent>(this, InteractionName);
		Slot.InteractionComponent->SetupAttachment(SceneRoot);
		Slot.InteractionComponent->SetRelativeLocation(Config.InteractionRelativeLocation);
		Slot.InteractionComponent->SetRelativeRotation(Config.InteractionRelativeRotation);
		Slot.InteractionComponent->SetBoxExtent(Config.InteractionBoxExtent.ComponentMax(FVector::OneVector));
		Slot.InteractionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Slot.InteractionComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
		Slot.InteractionComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		Slot.InteractionComponent->SetGenerateOverlapEvents(false);
		AddInstanceComponent(Slot.InteractionComponent);
		Slot.InteractionComponent->RegisterComponent();

		if (Config.bEnableFilledLight)
		{
			const FName LightName(*FString::Printf(TEXT("ConsumingSlotLight_%s"), *Config.SlotId.ToString()));
			Slot.LightComponent = NewObject<UPointLightComponent>(this, LightName);
			Slot.LightComponent->SetupAttachment(SceneRoot);
			Slot.LightComponent->SetRelativeLocation(Config.LightRelativeLocation);
			Slot.LightComponent->SetLightColor(Config.LightColor);
			Slot.LightComponent->SetAttenuationRadius(FMath::Max(0.f, Config.LightRadius));
			Slot.LightComponent->SetIntensity(0.f);
			Slot.LightComponent->SetVisibility(false);
			AddInstanceComponent(Slot.LightComponent);
			Slot.LightComponent->RegisterComponent();
		}

		if (!Config.FilledMaterialSlot.IsNone() && !Config.FilledMaterialAlias.IsNone() && Definition && MeshComponent)
		{
			Slot.FilledMaterialIndex = MeshComponent->GetMaterialIndex(Config.FilledMaterialSlot);
			if (const TObjectPtr<UMaterialInterface>* Material = Definition->RuntimeMaterialAliases.Find(Config.FilledMaterialAlias))
			{
				Slot.FilledMaterial = Material->Get();
			}
		}
	}
}

bool AGridConsumingSlotReceptacleActor::TryPlaceCursorItemFromHit(AGrimrockPartyPawn* PartyPawn, const FHitResult& HitResult)
{
	const int32 SlotIndex = FindSlotIndexFromHit(HitResult);
	if (!RuntimeConsumingSlots.IsValidIndex(SlotIndex) || !PartyPawn || !PartyPawn->PartyInventoryComponent)
	{
		return false;
	}
	AGridLevelRuntimeActor* RuntimeActor = GridInteractionUtils::ResolveRuntimeActor(PartyPawn, this);
	if (!RuntimeActor || !RuntimeActor->CanPartyInteractWithEdgeObject(CellX, CellY, Edge, PartyPawn))
	{
		return false;
	}

	const FRuntimeConsumingSlot& Slot = RuntimeConsumingSlots[SlotIndex];
	if (FilledConsumingSlots.Contains(Slot.Config.SlotId))
	{
		return false;
	}

	FGridItemInstance CursorItem;
	if (!PartyPawn->GetCursorItem(CursorItem) || CursorItem.Quantity < 1)
	{
		return false;
	}
	FGridItemInstance SingleItem = CursorItem;
	SingleItem.Quantity = 1;
	if (!CanAcceptItemInstance(SingleItem))
	{
		return false;
	}

	FString PresentationError;
	if (!ValidateSlotPresentation(SlotIndex, PresentationError))
	{
		UE_LOG(LogGridReceptacle, Warning, TEXT("Consuming slot refused: ObjectId=%s Slot=%s Reason=%s"),
			*ObjectId.ToString(), *Slot.Config.SlotId.ToString(), *PresentationError);
		return false;
	}

	if (!ObjectId.IsValid() || !RuntimeActor || !RuntimeActor->GetOrCreateRuntimeStateForCurrentLevel())
	{
		return false;
	}

	UGridPartyInventoryComponent* Inventory = PartyPawn->PartyInventoryComponent;
	const bool bCursorConsumed = CursorItem.Quantity > 1
		? ([Inventory, &CursorItem]()
			{
				FGridItemInstance Remaining = CursorItem;
				--Remaining.Quantity;
				return Inventory->SetCursorItem(Remaining);
			})()
		: Inventory->ClearCursorItem();
	if (!bCursorConsumed)
	{
		return false;
	}

	UMaterialInterface* PreviousMaterial = nullptr;
	if (Slot.FilledMaterialIndex != INDEX_NONE)
	{
		PreviousMaterial = MeshComponent->GetMaterial(Slot.FilledMaterialIndex);
	}

	FilledConsumingSlots.Add(Slot.Config.SlotId);
	FString MaterialError;
	if (Slot.FilledMaterialIndex != INDEX_NONE &&
		!SetRuntimeMaterialAlias(Slot.Config.FilledMaterialSlot, Slot.Config.FilledMaterialAlias, true, MaterialError))
	{
		FilledConsumingSlots.Remove(Slot.Config.SlotId);
		MeshComponent->SetMaterial(Slot.FilledMaterialIndex, PreviousMaterial);
		Inventory->SetCursorItem(CursorItem);
		return false;
	}

	ApplySlotLight(SlotIndex, true);
	PersistConsumingSlotState();
	ExecuteInsertionLinks();

	if (!bCompletionEmitted && AreAllRequiredSlotsFilled())
	{
		bCompletionEmitted = true;
		PersistConsumingSlotState();
		RuntimeActor->ExecuteLinksFromRuntimeObject(ObjectId, EGridObjectEvent::Activated);
	}
	return true;
}

void AGridConsumingSlotReceptacleActor::CaptureRuntimeReceptacleState(FGridRuntimeReceptacleState& OutState) const
{
	Super::CaptureRuntimeReceptacleState(OutState);
	OutState.FilledConsumingSlots = FilledConsumingSlots;
	OutState.bConsumingSlotsCompletionEmitted = bCompletionEmitted;
}

bool AGridConsumingSlotReceptacleActor::CanInteract_Implementation(APawn* InstigatorPawn, UPrimitiveComponent* HitComponent) const
{
	const AGrimrockPartyPawn* PartyPawn = GridInteractionUtils::ResolvePartyPawn(InstigatorPawn);
	const AGridLevelRuntimeActor* RuntimeActor = GridInteractionUtils::ResolveRuntimeActor(InstigatorPawn, this);
	if (!PartyPawn || !RuntimeActor || !RuntimeActor->CanPartyInteractWithEdgeObject(CellX, CellY, Edge, PartyPawn))
	{
		return false;
	}
	const FRuntimeConsumingSlot* Slot = RuntimeConsumingSlots.FindByPredicate(
		[HitComponent](const FRuntimeConsumingSlot& Candidate) { return Candidate.InteractionComponent == HitComponent; });
	return Slot && !FilledConsumingSlots.Contains(Slot->Config.SlotId) && CanAcceptCursorItemFromParty(PartyPawn);
}

void AGridConsumingSlotReceptacleActor::Interact_Implementation(APawn* InstigatorPawn, UPrimitiveComponent* HitComponent)
{
	AGrimrockPartyPawn* PartyPawn = GridInteractionUtils::ResolvePartyPawn(InstigatorPawn);
	if (!PartyPawn)
	{
		return;
	}
	FHitResult HitResult;
	HitResult.Component = HitComponent;
	TryPlaceCursorItemFromHit(PartyPawn, HitResult);
}

void AGridConsumingSlotReceptacleActor::InteractWithHit_Implementation(
	APawn* InstigatorPawn, UPrimitiveComponent* HitComponent, const FHitResult& HitResult)
{
	AGrimrockPartyPawn* PartyPawn = GridInteractionUtils::ResolvePartyPawn(InstigatorPawn);
	if (!PartyPawn || HitResult.GetComponent() != HitComponent)
	{
		return;
	}
	TryPlaceCursorItemFromHit(PartyPawn, HitResult);
}

void AGridConsumingSlotReceptacleActor::RestoreConsumingSlotState(const FGridRuntimeReceptacleState& State)
{
	FilledConsumingSlots.Reset();
	for (int32 Index = 0; Index < RuntimeConsumingSlots.Num(); ++Index)
	{
		const FName SlotId = RuntimeConsumingSlots[Index].Config.SlotId;
		const bool bFilled = State.FilledConsumingSlots.Contains(SlotId);
		if (bFilled)
		{
			FilledConsumingSlots.Add(SlotId);
			FString Error;
			if (!RuntimeConsumingSlots[Index].Config.FilledMaterialSlot.IsNone())
			{
				SetRuntimeMaterialAlias(RuntimeConsumingSlots[Index].Config.FilledMaterialSlot,
					RuntimeConsumingSlots[Index].Config.FilledMaterialAlias, false, Error);
			}
		}
		ApplySlotLight(Index, bFilled);
	}
	bCompletionEmitted = State.bConsumingSlotsCompletionEmitted;
}

bool AGridConsumingSlotReceptacleActor::IsConsumingSlotFilled(FName SlotId) const
{
	return FilledConsumingSlots.Contains(SlotId);
}

UBoxComponent* AGridConsumingSlotReceptacleActor::GetConsumingSlotInteractionComponent(FName SlotId) const
{
	const FRuntimeConsumingSlot* Slot = RuntimeConsumingSlots.FindByPredicate(
		[SlotId](const FRuntimeConsumingSlot& Candidate) { return Candidate.Config.SlotId == SlotId; });
	return Slot ? Slot->InteractionComponent.Get() : nullptr;
}

UPointLightComponent* AGridConsumingSlotReceptacleActor::GetConsumingSlotLightComponent(FName SlotId) const
{
	const FRuntimeConsumingSlot* Slot = RuntimeConsumingSlots.FindByPredicate(
		[SlotId](const FRuntimeConsumingSlot& Candidate) { return Candidate.Config.SlotId == SlotId; });
	return Slot ? Slot->LightComponent.Get() : nullptr;
}

void AGridConsumingSlotReceptacleActor::ClearRuntimeConsumingSlots()
{
	for (FRuntimeConsumingSlot& Slot : RuntimeConsumingSlots)
	{
		if (Slot.InteractionComponent)
		{
			Slot.InteractionComponent->DestroyComponent();
		}
		if (Slot.LightComponent)
		{
			Slot.LightComponent->DestroyComponent();
		}
	}
	RuntimeConsumingSlots.Reset();
}

int32 AGridConsumingSlotReceptacleActor::FindSlotIndexFromHit(const FHitResult& HitResult) const
{
	const UPrimitiveComponent* HitComponent = HitResult.GetComponent();
	for (int32 Index = 0; Index < RuntimeConsumingSlots.Num(); ++Index)
	{
		if (RuntimeConsumingSlots[Index].InteractionComponent == HitComponent)
		{
			return Index;
		}
	}
	return INDEX_NONE;
}

bool AGridConsumingSlotReceptacleActor::ValidateSlotPresentation(int32 SlotIndex, FString& OutError) const
{
	OutError.Reset();
	if (!RuntimeConsumingSlots.IsValidIndex(SlotIndex))
	{
		OutError = TEXT("invalid slot");
		return false;
	}
	const FRuntimeConsumingSlot& Slot = RuntimeConsumingSlots[SlotIndex];
	if (!Slot.Config.FilledMaterialSlot.IsNone() || !Slot.Config.FilledMaterialAlias.IsNone())
	{
		if (Slot.Config.FilledMaterialSlot.IsNone() || Slot.Config.FilledMaterialAlias.IsNone() ||
			Slot.FilledMaterialIndex == INDEX_NONE || !Slot.FilledMaterial)
		{
			OutError = TEXT("filled material slot or alias cannot be resolved");
			return false;
		}
	}
	if (Slot.Config.bEnableFilledLight && !Slot.LightComponent)
	{
		OutError = TEXT("filled light component is unavailable");
		return false;
	}
	return true;
}

void AGridConsumingSlotReceptacleActor::ApplySlotLight(int32 SlotIndex, bool bFilled)
{
	if (!RuntimeConsumingSlots.IsValidIndex(SlotIndex))
	{
		return;
	}
	FRuntimeConsumingSlot& Slot = RuntimeConsumingSlots[SlotIndex];
	if (!Slot.LightComponent)
	{
		return;
	}
	Slot.LightComponent->SetLightColor(Slot.Config.LightColor);
	Slot.LightComponent->SetAttenuationRadius(FMath::Max(0.f, Slot.Config.LightRadius));
	Slot.LightComponent->SetIntensity(bFilled ? FMath::Max(0.f, Slot.Config.LightIntensity) : 0.f);
	Slot.LightComponent->SetVisibility(bFilled);
}

bool AGridConsumingSlotReceptacleActor::AreAllRequiredSlotsFilled() const
{
	bool bHasRequiredSlot = false;
	for (const FRuntimeConsumingSlot& Slot : RuntimeConsumingSlots)
	{
		if (!Slot.Config.bRequiredForActivation)
		{
			continue;
		}
		bHasRequiredSlot = true;
		if (!FilledConsumingSlots.Contains(Slot.Config.SlotId))
		{
			return false;
		}
	}
	return bHasRequiredSlot;
}

void AGridConsumingSlotReceptacleActor::PersistConsumingSlotState()
{
	AGridLevelRuntimeActor* RuntimeActor = Cast<AGridLevelRuntimeActor>(GetOwner());
	FGridLevelRuntimeState* LevelState = RuntimeActor ? RuntimeActor->GetOrCreateRuntimeStateForCurrentLevel() : nullptr;
	if (!LevelState)
	{
		return;
	}
	FGridRuntimeReceptacleState& State = LevelState->Receptacles.FindOrAdd(ObjectId);
	CaptureRuntimeReceptacleState(State);
}
