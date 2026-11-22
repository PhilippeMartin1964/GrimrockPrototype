#include "Runtime/GrimrockPartyPawn.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "GridItemMeshResolver.h"
#include "GridLegacyItemLookup.h"
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
	// Compatibility of the reflected ID-only API, not of the old item data:
	// it can display an unlevelled canonical non-ARM, never invent ARM N1.
	const UWorld* World = GetWorld();
	UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
	const UGridItemSubsystem* Registry = GI ? GI->GetSubsystem<UGridItemSubsystem>() : nullptr;
	FGridResolvedItem Canonical;
	if (!Registry || !Registry->ResolveItem(ItemDefinitionId, 0, Canonical) ||
		!Canonical.IsValid() || Canonical.bIsArmament)
	{
		return false;
	}
	FGridItemInstance Visual;
	Visual.RuntimeObjectId = FGuid::NewGuid();
	Visual.ItemDefinitionId = ItemDefinitionId;
	Visual.ItemLevel = 0;
	Visual.Quantity = 1;
	Visual.bLightsEnabled = Canonical.LightEmitter.HasEmitter() && Canonical.LightEmitter.bDefaultEnabled;
	return EquipHeldItemFromInstance(Visual);
}

// X.42: the selected equipped instance is the only place where ARM
// ItemLevel N1-N20 is known. Resolve its complete visual without ever
// loading or inheriting a historical DA_Weapon.
bool AGrimrockPartyPawn::EquipHeldItemFromInstance(const FGridItemInstance& Item)
{
	if (!Item.IsValid())
	{
		return false;
	}
	const UWorld* OwningWorld = GetWorld();
	UGameInstance* GI = OwningWorld ? OwningWorld->GetGameInstance() : nullptr;
	const UGridItemSubsystem* Registry = GI ? GI->GetSubsystem<UGridItemSubsystem>() : nullptr;
	if (!Registry || !Registry->ContainsItem(Item.ItemDefinitionId))
	{
		return false;
	}

	FGridResolvedItem Canonical;
	if (!Registry->ResolveItem(Item.ItemDefinitionId, Item.ItemLevel, Canonical) ||
		!Canonical.IsValid() || (!Canonical.bIsArmament && Item.ItemLevel != 0))
	{
		return false; // No canonical ARM N0, non-ARM N1 or implicit promotion.
	}
	if (!LevelRuntimeActor)
	{
		LevelRuntimeActor = Cast<AGridLevelRuntimeActor>(
			UGameplayStatics::GetActorOfClass(GetWorld(), AGridLevelRuntimeActor::StaticClass()));
	}
	if (!GetWorld())
	{
		return false;
	}

	ClearHeldItem();
	USceneComponent* AttachParent = HeldItemRoot ? HeldItemRoot.Get() : GetRootComponent();
	if (LevelRuntimeActor)
	{
		HeldItemActor = LevelRuntimeActor->SpawnItemActorForDefinition(
			nullptr, Item.ItemDefinitionId, this, AttachParent);
	}
	else
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = GetInstigator();
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		HeldItemActor = GetWorld()->SpawnActor<AGridItemActor>(
			AGridItemActor::StaticClass(), FTransform::Identity, SpawnParams);
	}
	if (!HeldItemActor)
	{
		return false;
	}

	// The held mesh has its own ephemeral presentation GUID; the source
	// equipped instance retains its actual GUID for inventory/save logic.
	FGridItemInstance Visual = Item;
	Visual.RuntimeObjectId = FGuid::NewGuid();
	if (!HeldItemActor->InitializeFromItemInstance(Visual, Registry))
	{
		HeldItemActor->Destroy();
		HeldItemActor = nullptr;
		return false;
	}
	if (HeldItemActor->MeshComponent)
	{
		HeldItemActor->MeshComponent->SetStaticMesh(Canonical.EquippedMesh.LoadSynchronous());
	}
	if (HeldItemActor->LightEmitterComponent)
	{
		// The party proxy emits the actual PointLight; attached Niagara stays.
		HeldItemActor->LightEmitterComponent->SetPointLightPresentationEnabled(false);
	}
	HeldItemActor->ConfigureAsAttachedItem();
	HeldItemActor->AttachToComponent(AttachParent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	HeldItemActor->SetActorRelativeLocation(HeldItemRelativeLocation);
	HeldItemActor->SetActorRelativeRotation(HeldItemRelativeRotation);
	HeldItemActor->SetActorRelativeScale3D(HeldItemRelativeScale);
	HeldItemActor->OnPlacedInWorld();
	HeldItemActor->SetItemLightsEnabled(Item.bLightsEnabled && Canonical.LightEmitter.HasEmitter());
	HeldItemDefinitionId = Item.ItemDefinitionId;
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

	return FGridLegacyItemLookup::Resolve(this, Item.ItemDefinitionId);
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
	if (Registry && Registry->ContainsItem(Item.ItemDefinitionId))
	{
		FGridResolvedItem Canonical;
		if (!Registry->ResolveItem(Item.ItemDefinitionId, Item.ItemLevel, Canonical))
		{
			return false; // No invalid ARM N0 -> N1 default.
		}
		// X.41: the 22 ARM emitter profiles are already native-verified
		// equal to DA_Weapon_* (X.38). A registered item must never get its
		// held-light status from a stale historical definition. Only N0 is
		// physically valid for registered non-ARM items.
		return (Canonical.bIsArmament || Item.ItemLevel == 0) &&
			Canonical.LightEmitter.HasEmitter();
	}
	return false; // Unknown identities have no held-light authority.
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

	const bool bNeedsNewHeldVisual =
		GetHeldItemDefinitionId() != VisualItem->ItemDefinitionId ||
		!HeldItemActor || HeldItemActor->WorldItemLevel != VisualItem->ItemLevel;
	if (bNeedsNewHeldVisual && !EquipHeldItemFromInstance(*VisualItem))
	{
		return; // Never build an ARM held visual from an invented physical N-level.
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
					const FGridItemMeshView Visual =
						FGridItemMeshResolver::Resolve(Registry, *VisualItem);
					if (Visual.bRegisteredIdentity && HeldItemActor->MeshComponent)
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
