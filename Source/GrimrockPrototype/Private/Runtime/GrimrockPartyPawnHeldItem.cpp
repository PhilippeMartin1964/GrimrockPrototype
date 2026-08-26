#include "Runtime/GrimrockPartyPawn.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"

namespace
{
	const TCHAR* GridPartyPawnHeldItemGetEquipmentSlotName(EGridEquipmentSlot Slot)
	{
		switch (Slot)
		{
			case EGridEquipmentSlot::None:
				return TEXT("None");
			case EGridEquipmentSlot::MainHand:
				return TEXT("MainHand");
			case EGridEquipmentSlot::OffHand:
				return TEXT("OffHand");
			default:
				return TEXT("Unsupported");
		}
	}
}

bool AGrimrockPartyPawn::EquipHeldItem(FName ItemDefinitionId)
{
	if (ItemDefinitionId.IsNone())
	{
		return false;
	}

	const bool bUseHeldTorchClass = ItemDefinitionId == DefaultHeldItemDefinitionId && HeldTorchActorClass;

	if (!bUseHeldTorchClass && !LevelRuntimeActor)
	{
		LevelRuntimeActor = Cast<AGridLevelRuntimeActor>(UGameplayStatics::GetActorOfClass(GetWorld(), AGridLevelRuntimeActor::StaticClass()));
	}

	if (!bUseHeldTorchClass && !LevelRuntimeActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("Held item equip failed: no AGridLevelRuntimeActor found for %s."), *ItemDefinitionId.ToString());
		return false;
	}

	UGridItemDefinitionAsset* ItemDefinition = PartyInventoryComponent ? PartyInventoryComponent->FindItemDefinition(ItemDefinitionId) : nullptr;
	if (!ItemDefinition && LevelRuntimeActor)
	{
		ItemDefinition = LevelRuntimeActor->ResolveRuntimeItemDefinition(ItemDefinitionId);
	}
	if (!bUseHeldTorchClass && !ItemDefinition)
	{
		UE_LOG(LogTemp, Warning, TEXT("Held item equip failed: item definition %s could not be resolved."), *ItemDefinitionId.ToString());
		return false;
	}

	ClearHeldItem();

	USceneComponent* AttachParent = HeldItemRoot ? HeldItemRoot.Get() : GetRootComponent();
	if (bUseHeldTorchClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = GetInstigator();

		HeldItemActor = GetWorld()->SpawnActor<AGridItemActor>(HeldTorchActorClass, FTransform::Identity, SpawnParams);
		if (HeldItemActor)
		{
			HeldItemActor->ArchetypeId = ItemDefinitionId;
		}
	}
	else
	{
		HeldItemActor = LevelRuntimeActor->SpawnItemActorForDefinition(ItemDefinition, ItemDefinitionId, this, AttachParent);
	}

	if (!HeldItemActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("Held item equip failed: could not spawn item definition %s."), *ItemDefinitionId.ToString());
		return false;
	}

	if (!bUseHeldTorchClass && ItemDefinition && HeldItemActor->MeshComponent)
	{
		HeldItemActor->MeshComponent->SetStaticMesh(ItemDefinition->LoadHeldMesh());
	}
	HeldItemActor->AttachToComponent(AttachParent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	HeldItemActor->SetActorRelativeLocation(HeldItemRelativeLocation);
	HeldItemActor->SetActorRelativeRotation(HeldItemRelativeRotation);
	HeldItemActor->SetActorRelativeScale3D(HeldItemRelativeScale);
	HeldItemActor->OnPlacedInWorld();
	HeldItemDefinitionId = ItemDefinitionId;
	bHasTorchInHand = ItemDefinitionId == DefaultHeldItemDefinitionId;

	UE_LOG(LogTemp, Log, TEXT("Held item equipped: %s Mesh=%s"), *ItemDefinitionId.ToString(),
		HeldItemActor->MeshComponent ? *GetNameSafe(HeldItemActor->MeshComponent->GetStaticMesh()) : TEXT("None"));
	return true;
}

void AGrimrockPartyPawn::ClearHeldItem()
{
	if (HeldItemActor)
	{
		HeldItemActor->OnRemovedFromWorld();
		HeldItemActor->Destroy();
		HeldItemActor = nullptr;
	}

	HeldItemDefinitionId = NAME_None;
	bHasTorchInHand = false;
}

FName AGrimrockPartyPawn::GetHeldItemDefinitionId() const
{
	return HeldItemActor ? HeldItemDefinitionId : NAME_None;
}

bool AGrimrockPartyPawn::IsHoldingItem(FName ItemDefinitionId) const
{
	return !ItemDefinitionId.IsNone() && GetHeldItemDefinitionId() == ItemDefinitionId;
}

UGridItemDefinitionAsset* AGrimrockPartyPawn::ResolveEquippedItemDefinition(const FGridItemInstance& Item) const
{
	if (!Item.IsValid())
	{
		return nullptr;
	}

	if (PartyInventoryComponent)
	{
		if (UGridItemDefinitionAsset* ItemDefinition = PartyInventoryComponent->FindItemDefinition(Item.ItemDefinitionId))
		{
			return ItemDefinition;
		}
	}

	return LevelRuntimeActor ? LevelRuntimeActor->ResolveRuntimeItemDefinition(Item.ItemDefinitionId) : nullptr;
}

bool AGrimrockPartyPawn::DoesEquippedItemEmitLight(const FGridItemInstance& Item) const
{
	const UGridItemDefinitionAsset* ItemDefinition = ResolveEquippedItemDefinition(Item);
	return Item.IsValid() && ((ItemDefinition && ItemDefinition->bCanEmitLight) || Item.bLightsEnabled);
}

bool AGrimrockPartyPawn::RecomputeEquippedLightState(
	const FGridItemInstance& MainHandItem, bool bHasMainHandItem, const FGridItemInstance& OffHandItem, bool bHasOffHandItem) const
{
	const bool bMainLight = bHasMainHandItem && DoesEquippedItemEmitLight(MainHandItem);
	const bool bOffLight = bHasOffHandItem && DoesEquippedItemEmitLight(OffHandItem);
	const bool bResult = bMainLight || bOffLight;

	UE_LOG(LogTemp, Log, TEXT("GridEquipmentLight Recompute MainHand=%s MainLight=%s OffHand=%s OffLight=%s Result=%s"),
		bHasMainHandItem ? *MainHandItem.ItemDefinitionId.ToString() : TEXT("None"), bMainLight ? TEXT("true") : TEXT("false"),
		bHasOffHandItem ? *OffHandItem.ItemDefinitionId.ToString() : TEXT("None"), bOffLight ? TEXT("true") : TEXT("false"),
		bResult ? TEXT("true") : TEXT("false"));

	return bResult;
}

void AGrimrockPartyPawn::SyncHeldVisualFromSelectedCharacterEquipment()
{
	// TODO 5C: call this after any direct PartyInventoryComponent::SetSelectedCharacterIndex usage outside the pawn.
	if (!PartyInventoryComponent)
	{
		ClearHeldItem();
		return;
	}

	const int32 CharacterIndex = PartyInventoryComponent->GetSelectedCharacterIndex();
	FGridItemInstance MainHandItem;
	FGridItemInstance OffHandItem;
	const bool bHasMainHandItem = PartyInventoryComponent->GetEquippedItem(CharacterIndex, EGridEquipmentSlot::MainHand, MainHandItem);
	const bool bHasOffHandItem = PartyInventoryComponent->GetEquippedItem(CharacterIndex, EGridEquipmentSlot::OffHand, OffHandItem);

	const bool bMainLight = bHasMainHandItem && DoesEquippedItemEmitLight(MainHandItem);
	const bool bOffLight = bHasOffHandItem && DoesEquippedItemEmitLight(OffHandItem);
	const bool bAnyEquippedLight = RecomputeEquippedLightState(MainHandItem, bHasMainHandItem, OffHandItem, bHasOffHandItem);

	const FGridItemInstance* VisualItem = nullptr;
	EGridEquipmentSlot VisualSlot = EGridEquipmentSlot::None;
	if (bMainLight)
	{
		VisualItem = &MainHandItem;
		VisualSlot = EGridEquipmentSlot::MainHand;
	}
	else if (bOffLight)
	{
		VisualItem = &OffHandItem;
		VisualSlot = EGridEquipmentSlot::OffHand;
	}

	if (!VisualItem || VisualSlot == EGridEquipmentSlot::None)
	{
		ClearHeldItem();
		UE_LOG(LogTemp, Verbose, TEXT("GridInventory HeldVisual Sync None Character=%d MainHand=%s MainLight=%s OffHand=%s OffLight=%s Reason=NoEquippedLight"),
			CharacterIndex, bHasMainHandItem ? *MainHandItem.ItemDefinitionId.ToString() : TEXT("None"), bMainLight ? TEXT("true") : TEXT("false"),
			bHasOffHandItem ? *OffHandItem.ItemDefinitionId.ToString() : TEXT("None"), bOffLight ? TEXT("true") : TEXT("false"));
		return;
	}

	if (GetHeldItemDefinitionId() != VisualItem->ItemDefinitionId && !EquipHeldItem(VisualItem->ItemDefinitionId))
	{
		return;
	}

	if (HeldItemActor)
	{
		HeldItemActor->SetItemLightsEnabled(bAnyEquippedLight);
	}
	bHasTorchInHand = bAnyEquippedLight;

	UE_LOG(LogTemp, Log, TEXT("GridInventory HeldVisual Sync Equipped Character=%d Slot=%s Item=%s"), CharacterIndex,
		GridPartyPawnHeldItemGetEquipmentSlotName(VisualSlot), *VisualItem->ItemDefinitionId.ToString());
}
