#include "Runtime/GrimrockPartyPawn.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "GridCanonicalItemMeshReadModel.h"
#include "Engine/World.h"
#include "GridEquipmentSlotUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridLightEmitterComponent.h"
#include "Runtime/GridPartyIlluminationComponent.h"
#include "Runtime/GridPartyInventoryComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogGridItemTransfer, Log, All);

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
	const UWorld* OwningWorld = GetWorld();
	UGameInstance* GameInstance = OwningWorld ? OwningWorld->GetGameInstance() : nullptr;
	const UGridItemSubsystem* Registry = GameInstance
		? GameInstance->GetSubsystem<UGridItemSubsystem>() : nullptr;
	FGridResolvedItem Canonical;
	// EquipHeldItem receives an ID, not an ItemLevel. Only genuinely
	// unlevelled registered NON-ARM items (such as the torch) can use N0.
	// A pointerless ARM may never be guessed into a N1 equipped visual.
	const bool bCanonicalNonArm = Registry &&
		Registry->HasCanonicalItem(ItemDefinitionId) &&
		Registry->ResolveItem(ItemDefinitionId, 0, Canonical) &&
		!Canonical.bIsArmament;
	if (!ItemDefinition && !bCanonicalNonArm)
	{
		UE_LOG(LogGridItemTransfer, Warning,
			TEXT("Held item equip failed: item definition %s could not be resolved."), *ItemDefinitionId.ToString());
		return false;
	}

	ClearHeldItem();

	USceneComponent* AttachParent = HeldItemRoot ? HeldItemRoot.Get() : GetRootComponent();
	if (LevelRuntimeActor)
	{
		HeldItemActor = LevelRuntimeActor->SpawnItemActorForDefinition(
			bCanonicalNonArm ? nullptr : ItemDefinition, ItemDefinitionId, this, AttachParent);
	}
	else if (UWorld* World = GetWorld())
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = GetInstigator();
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		HeldItemActor = World->SpawnActor<AGridItemActor>(AGridItemActor::StaticClass(), FTransform::Identity, SpawnParams);
		if (HeldItemActor && !bCanonicalNonArm)
		{
			HeldItemActor->InitializeFromItemDefinition(ItemDefinition, FGuid());
		}
	}

	if (!HeldItemActor)
	{
		UE_LOG(LogGridItemTransfer, Warning, TEXT("Held item equip failed: could not spawn item definition %s."), *ItemDefinitionId.ToString());
		return false;
	}

	if (bCanonicalNonArm)
	{
		FGridItemInstance CanonicalInstance;
		CanonicalInstance.RuntimeObjectId = FGuid::NewGuid();
		CanonicalInstance.ItemDefinitionId = ItemDefinitionId;
		CanonicalInstance.ItemLevel = 0;
		CanonicalInstance.Quantity = 1;
		if (!HeldItemActor->InitializeFromCanonicalItem(CanonicalInstance, Registry))
		{
			HeldItemActor->Destroy();
			HeldItemActor = nullptr;
			return false; // Registered identity must never degrade to old art.
		}
	}
	if (HeldItemActor->MeshComponent)
	{
		HeldItemActor->MeshComponent->SetStaticMesh(bCanonicalNonArm
			? Canonical.EquippedMesh.LoadSynchronous() : ItemDefinition->LoadHeldMesh());
	}
	if (HeldItemActor->LightEmitterComponent)
	{
		// Independent VisualEffects stay on the held mesh; its local
		// PointLight is delegated to the party illumination proxy.
		HeldItemActor->LightEmitterComponent->SetPointLightPresentationEnabled(false);
	}
	HeldItemActor->ConfigureAsAttachedItem();
	HeldItemActor->AttachToComponent(AttachParent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	HeldItemActor->SetActorRelativeLocation(HeldItemRelativeLocation);
	HeldItemActor->SetActorRelativeRotation(HeldItemRelativeRotation);
	HeldItemActor->SetActorRelativeScale3D(HeldItemRelativeScale);
	HeldItemActor->OnPlacedInWorld();
	HeldItemDefinitionId = ItemDefinitionId;

	UE_LOG(LogGridItemTransfer, Verbose, TEXT("Held item equipped: %s Mesh=%s"), *ItemDefinitionId.ToString(),
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
	if (!Item.IsValid() || !Item.bLightsEnabled)
	{
		return false;
	}
	const UWorld* World = GetWorld();
	UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
	const UGridItemSubsystem* Registry = GI
		? GI->GetSubsystem<UGridItemSubsystem>() : nullptr;
	if (Registry && Registry->HasCanonicalItem(Item.ItemDefinitionId))
	{
		FGridResolvedItem Canonical;
		if (!Registry->ResolveItem(Item.ItemDefinitionId, Item.ItemLevel, Canonical))
		{
			return false; // No invalid ARM N0 -> N1 default.
		}
		// X.41: the 22 ARM emitter profiles are already native-verified
		// equal to DA_Weapon_* (X.38). A registered item must never get its
		// held-light status from a stale historical definition. Only N0 is
		// physically valid for registered non-ARM items; the generic registry
		// can intentionally resolve some non-ARM N1 family queries.
		return (Canonical.bIsArmament || Item.ItemLevel == 0) &&
			Canonical.LightEmitter.HasEmitter();
	}
	const UGridItemDefinitionAsset* ItemDefinition = ResolveEquippedItemDefinition(Item);
	return ItemDefinition && ItemDefinition->HasLightEmitter();
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
		UE_LOG(LogGridItemTransfer, Verbose, TEXT("GridInventory HeldVisual Sync None Character=%d MainHand=%s MainLight=%s OffHand=%s OffLight=%s Reason=NoEquippedLight"),
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
		// Only this path has the actual equipped instance and its N1-N20 level.
		// A registered ARM may not take its held mesh from DA_Weapon_* copies.
		if (UWorld* World = GetWorld())
		{
			if (UGameInstance* GameInstance = World->GetGameInstance())
			{
				if (const UGridItemSubsystem* Registry = GameInstance->GetSubsystem<UGridItemSubsystem>())
				{
					const FGridCanonicalItemMeshReadView Visual =
						FGridCanonicalItemMeshReadModel::Resolve(Registry, *VisualItem);
					if (Visual.bCanonicalIdentity && HeldItemActor->MeshComponent)
					{
						UStaticMesh* CanonicalHeldMesh = Visual.bResolved
							? Visual.EquippedMesh.LoadSynchronous() : nullptr;
						if (HeldItemActor->MeshComponent->GetStaticMesh() != CanonicalHeldMesh)
						{
							HeldItemActor->MeshComponent->SetStaticMesh(CanonicalHeldMesh);
						}
					}
				}
			}
		}
		HeldItemActor->SetItemLightsEnabled(bAnyEquippedLight);
	}

	UE_LOG(LogGridItemTransfer, Verbose, TEXT("GridInventory HeldVisual Sync Equipped Character=%d Slot=%s Item=%s"), CharacterIndex,
		GridEquipmentSlotUtils::GetLogName(VisualSlot), *VisualItem->ItemDefinitionId.ToString());
}
