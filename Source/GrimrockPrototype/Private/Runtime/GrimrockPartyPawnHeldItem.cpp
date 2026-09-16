#include "Runtime/GrimrockPartyPawn.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GridEquipmentSlotUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridLightEmitterComponent.h"
#include "Runtime/GridPartyIlluminationComponent.h"
#include "Runtime/GridPartyInventoryComponent.h"

bool AGrimrockPartyPawn::EquipHeldItem(FName ItemDefinitionId)
{
	if (ItemDefinitionId.IsNone())
	{
		return false;
	}

	if (!LevelRuntimeActor)
	{
		LevelRuntimeActor = Cast<AGridLevelRuntimeActor>(UGameplayStatics::GetActorOfClass(GetWorld(), AGridLevelRuntimeActor::StaticClass()));
	}

	UGridItemDefinitionAsset* ItemDefinition = PartyInventoryComponent ? PartyInventoryComponent->FindItemDefinition(ItemDefinitionId) : nullptr;
	if (!ItemDefinition && LevelRuntimeActor)
	{
		ItemDefinition = LevelRuntimeActor->ResolveRuntimeItemDefinition(ItemDefinitionId);
	}
	if (!ItemDefinition)
	{
		UE_LOG(LogTemp, Warning, TEXT("Held item equip failed: item definition %s could not be resolved."), *ItemDefinitionId.ToString());
		return false;
	}

	ClearHeldItem();

	USceneComponent* AttachParent = HeldItemRoot ? HeldItemRoot.Get() : GetRootComponent();
	if (LevelRuntimeActor)
	{
		HeldItemActor = LevelRuntimeActor->SpawnItemActorForDefinition(ItemDefinition, ItemDefinitionId, this, AttachParent);
	}
	else if (UWorld* World = GetWorld())
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = GetInstigator();
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		HeldItemActor = World->SpawnActor<AGridItemActor>(AGridItemActor::StaticClass(), FTransform::Identity, SpawnParams);
		if (HeldItemActor)
		{
			HeldItemActor->InitializeFromItemDefinition(ItemDefinition, FGuid());
		}
	}

	if (!HeldItemActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("Held item equip failed: could not spawn item definition %s."), *ItemDefinitionId.ToString());
		return false;
	}

	if (HeldItemActor->MeshComponent)
	{
		HeldItemActor->MeshComponent->SetStaticMesh(ItemDefinition->LoadHeldMesh());
		// The first-person held visual must never cast a world shadow. External
		// lights (for example a wall torch behind the party) would otherwise
		// project the held mesh onto dungeon geometry in front of the camera.
		HeldItemActor->MeshComponent->SetCastShadow(false);
	}
	if (HeldItemActor->LightEmitterComponent)
	{
		// A held item keeps its flame/Niagara presentation, but its physical
		// PointLight is delegated to the party illumination proxy.
		HeldItemActor->LightEmitterComponent->SetEmitterChannelsEnabled(true, false);
	}
	HeldItemActor->ConfigureAsAttachedItem();
	HeldItemActor->AttachToComponent(AttachParent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	HeldItemActor->SetActorRelativeLocation(HeldItemRelativeLocation);
	HeldItemActor->SetActorRelativeRotation(HeldItemRelativeRotation);
	HeldItemActor->SetActorRelativeScale3D(HeldItemRelativeScale);
	HeldItemActor->OnPlacedInWorld();
	HeldItemDefinitionId = ItemDefinitionId;

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
	return Item.IsValid() && Item.bLightsEnabled && ItemDefinition && ItemDefinition->HasLightEmitter();
}

void AGrimrockPartyPawn::SyncHeldVisualFromSelectedCharacterEquipment()
{
	if (UGridPartyIlluminationComponent* PartyIllumination = FindComponentByClass<UGridPartyIlluminationComponent>())
	{
		PartyIllumination->RefreshFromEquipment(PartyInventoryComponent, LevelRuntimeActor);
	}

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
	const bool bAnyEquippedLight = bMainLight || bOffLight;

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

	UE_LOG(LogTemp, Log, TEXT("GridInventory HeldVisual Sync Equipped Character=%d Slot=%s Item=%s"), CharacterIndex,
		GridEquipmentSlotUtils::GetLogName(VisualSlot), *VisualItem->ItemDefinitionId.ToString());
}
