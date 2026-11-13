#include "Runtime/Items/GridItemSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogGridCanonicalItems, Log, All);

void UGridItemSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	if (ConfiguredItemSystem.IsNull())
	{
		// ARM-CLEAN06.1 has no DA_ItemSystem binary yet. Never use the legacy model.
		return;
	}
	UGridItemSystemAsset* Registry = ConfiguredItemSystem.LoadSynchronous();
	TArray<FString> Errors;
	if (!RegisterItemSystem(Registry, Errors))
	{
		for (const FString& Error : Errors)
		{
			UE_LOG(LogGridCanonicalItems, Error, TEXT("[ARM-CLEAN06.1] %s"), *Error);
		}
	}
}

void UGridItemSubsystem::Deinitialize()
{
	ItemIndex.Reset();
	RootRegistry = nullptr;
	bConfigurationAttempted = false;
	Super::Deinitialize();
}

bool UGridItemSubsystem::RegisterItemSystem(UGridItemSystemAsset* System, TArray<FString>& Errors)
{
	if (bConfigurationAttempted)
	{
		Errors.Add(TEXT("ItemSystem was already configured or rejected this session"));
		return false;
	}
	bConfigurationAttempted = true;
	if (!IsValid(System))
	{
		Errors.Add(TEXT("DA_ItemSystem missing; canonical resolver cannot start"));
		return false;
	}
	if (!System->Validate(Errors))
	{
		return false;
	}
	// Strong UPROPERTY ownership prevents collection of definitions after boot.
	RootRegistry = System;
	for (UGridCanonicalItemDefinitionAsset* Item : System->ItemDefinitions)
	{
		ItemIndex.Add(Item->ItemId, Item);
	}
	return true;
}

bool UGridItemSubsystem::ResolveItem(FName ItemId, int32 ItemLevel, FGridResolvedItem& OutItem) const
{
	OutItem = FGridResolvedItem();
	if (!RootRegistry)
	{
		OutItem.Error = EGridCanonicalItemResolveError::RegistryNotConfigured;
		return false;
	}
	if (ItemLevel < 1 || ItemLevel > 20)
	{
		OutItem.Error = EGridCanonicalItemResolveError::InvalidItemLevel;
		return false;
	}
	const TObjectPtr<UGridCanonicalItemDefinitionAsset>* Found = ItemIndex.Find(ItemId);
	if (!Found || !IsValid(Found->Get()))
	{
		OutItem.Error = EGridCanonicalItemResolveError::UnknownItemId;
		return false;
	}

	const UGridCanonicalItemDefinitionAsset* Definition = Found->Get();
	OutItem.ItemId = Definition->ItemId;
	OutItem.ItemLevel = ItemLevel;
	OutItem.DisplayName = Definition->DisplayName;
	OutItem.Description = Definition->Description;
	OutItem.WeightKg = Definition->WeightKg;
	OutItem.Tags = Definition->Tags;

	// Ammunition is a distinct canonical family. Finalize its identity before
	// any other equipment/consumable projection can be considered.
	if (const UGridAmmoDefinitionAsset* Ammo = Cast<UGridAmmoDefinitionAsset>(Definition))
	{
		OutItem.ItemIcon = Ammo->Icon;
		OutItem.WorldMesh = Ammo->WorldMesh;
		OutItem.AmmoKindId = Ammo->AmmoKindId;
		OutItem.MaxStack = Ammo->MaxStack;
		OutItem.Error = EGridCanonicalItemResolveError::None;
		return true;
	}

	if (const UGridArmamentDefinitionAsset* Armament =
		Cast<UGridArmamentDefinitionAsset>(Definition))
	{
		const FGridCanonicalWeaponArchetype* Archetype =
			RootRegistry->WeaponCatalogue->Find(Armament->ArchetypeId);
		// Numeric parity never grants an active weapon an implicit attack.
		// A real offensive ARM requires a bound attack and a source hand.
		const bool bMissingOffensiveAction = Archetype &&
			Archetype->Kind == EGridCanonicalArmamentKind::Weapon &&
			(Archetype->AttackSourceSlots.IsEmpty() ||
				!Archetype->Actions.ContainsByPredicate(
					[this](const FGridItemActionBinding& Binding)
					{
						const FGridCanonicalAction* Action =
							RootRegistry->ActionCatalogue->Find(Binding.ActionId);
						return Action && Action->Kind == EGridCanonicalActionKind::Attack;
					}));
		if (!Archetype || !Archetype->bHandAndKindProven || bMissingOffensiveAction ||
			!Archetype->TryResolve(ItemLevel, OutItem.Weapon))
		{
			OutItem = FGridResolvedItem();
			OutItem.Error = EGridCanonicalItemResolveError::InvalidArchetype;
			return false;
		}
		const FGridCanonicalWeaponVisual* Visual =
			RootRegistry->WeaponVisualCatalogue->FindForArchetype(Armament->ArchetypeId);
		if (!Visual)
		{
			OutItem = FGridResolvedItem();
			OutItem.Error = EGridCanonicalItemResolveError::MissingVisual;
			return false;
		}
		if (OutItem.Weapon.Projectile.bEnabled !=
				Visual->ProjectileAppearance.bEnabled ||
			(OutItem.Weapon.Projectile.bEnabled && Visual->ProjectileMesh.IsNull()))
		{
			OutItem = FGridResolvedItem();
			OutItem.Error = EGridCanonicalItemResolveError::IncompleteProjectileVisual;
			return false;
		}
		OutItem.ArchetypeId = Armament->ArchetypeId;
		OutItem.bIsArmament = true;
		OutItem.WorldMesh = Visual->WorldMesh;
		// The legacy visual resolver uses WorldMesh for a missing equipped mesh.
		// Keep this transient resolution: do not serialize a second mesh pointer.
		OutItem.EquippedMesh = Visual->EquippedMesh.IsNull() ?
			Visual->WorldMesh : Visual->EquippedMesh;
		OutItem.ProjectileMesh = Visual->ProjectileMesh;
		OutItem.ProjectileAppearance = Visual->ProjectileAppearance;
		OutItem.ItemIcon = Visual->ItemIcon;
		OutItem.AvailableActions = Archetype->Actions;
	}
	else
	{
		if (const UGridNonArmamentDefinitionAsset* Other =
			Cast<UGridNonArmamentDefinitionAsset>(Definition))
		{
			OutItem.ItemIcon = Other->Icon;
			OutItem.WorldMesh = Other->WorldMesh;
		}
		if (const UGridConsumableDefinitionAsset* Consumable =
			Cast<UGridConsumableDefinitionAsset>(Definition))
		{
			OutItem.MaxStack = Consumable->MaxStack;
			OutItem.AvailableActions = Consumable->UseActions;
		}
	}
	OutItem.Error = EGridCanonicalItemResolveError::None;
	return true;
}

bool UGridItemSubsystem::ResolveArmorReserveForInstance(
	const FGridItemInstance& Instance,
	FGridCanonicalArmorReserveView& OutReserve) const
{
	// A failed read must never expose a cached amount from an earlier item.
	OutReserve = FGridCanonicalArmorReserveView();
	if (!RootRegistry || !Instance.IsValid() ||
		Instance.ItemLevel < 1 || Instance.ItemLevel > 20 ||
		Instance.AbsorbedEquipmentPhysicalArmor < 0 ||
		Instance.AbsorbedEquipmentMagicalArmor < 0)
	{
		return false;
	}

	// Only canonical N1-N20 numbers are allowed; the name/weight snapshots
	// retained in old saves are deliberately ignored by this read model.
	FGridResolvedItem Resolved;
	if (!ResolveItem(Instance.ItemDefinitionId, Instance.ItemLevel, Resolved) ||
		!Resolved.bIsArmament)
	{
		return false;
	}

	return FGridCanonicalArmorReserveResolver::TryCompute(
		Resolved.Weapon,
		Instance.AbsorbedEquipmentPhysicalArmor,
		Instance.AbsorbedEquipmentMagicalArmor,
		OutReserve);
}

bool UGridItemSubsystem::ResolveBoundAction(
	FName ItemId, int32 ItemLevel, FName ActionId, FGridResolvedBoundAction& OutAction) const
{
	OutAction = FGridResolvedBoundAction();
	if (!RootRegistry)
	{
		OutAction.Error = EGridCanonicalBoundActionError::RegistryNotConfigured;
		return false;
	}

	FGridResolvedItem Item;
	if (!ResolveItem(ItemId, ItemLevel, Item))
	{
		OutAction.Error = EGridCanonicalBoundActionError::ItemResolutionFailed;
		OutAction.ItemError = Item.Error;
		return false;
	}

	const FGridItemActionBinding* Binding = Item.AvailableActions.FindByPredicate(
		[ActionId](const FGridItemActionBinding& Entry)
		{
			return !ActionId.IsNone() && Entry.ActionId == ActionId &&
				!Entry.PresentationId.IsNone();
		});
	if (!Binding)
	{
		OutAction.Error = EGridCanonicalBoundActionError::ActionNotBound;
		return false;
	}

	const FGridCanonicalAction* Action = RootRegistry->ActionCatalogue ?
		RootRegistry->ActionCatalogue->Find(Binding->ActionId) : nullptr;
	if (!Action)
	{
		OutAction.Error = EGridCanonicalBoundActionError::ActionDefinitionMissing;
		return false;
	}
	const FGridCanonicalActionPresentation* Presentation =
		RootRegistry->ActionPresentationCatalogue ?
		RootRegistry->ActionPresentationCatalogue->Find(Binding->PresentationId) : nullptr;
	if (!Presentation)
	{
		OutAction.Error = EGridCanonicalBoundActionError::PresentationDefinitionMissing;
		return false;
	}
	OutAction.ItemId = Item.ItemId;
	OutAction.ItemLevel = Item.ItemLevel;
	OutAction.ActionId = Binding->ActionId;
	OutAction.PresentationId = Binding->PresentationId;
	OutAction.Action = *Action;
	OutAction.Presentation = *Presentation;
	OutAction.ItemError = EGridCanonicalItemResolveError::None;
	OutAction.Error = EGridCanonicalBoundActionError::None;
	return true;
}

bool UGridItemSubsystem::ResolveBoundAttackForHand(
	FName ItemId, int32 ItemLevel, FName ActionId, EGridEquipmentSlot SourceHand,
	FGridResolvedBoundAction& OutAttack) const
{
	OutAttack = FGridResolvedBoundAction();
	if (!RootRegistry)
	{
		OutAttack.Error = EGridCanonicalBoundActionError::RegistryNotConfigured;
		return false;
	}
	FGridResolvedItem Item;
	if (!ResolveItem(ItemId, ItemLevel, Item))
	{
		OutAttack.Error = EGridCanonicalBoundActionError::ItemResolutionFailed;
		OutAttack.ItemError = Item.Error;
		return false;
	}
	if (!Item.bIsArmament || !Item.Weapon.HasAttackSourceInHand(SourceHand))
	{
		OutAttack.Error = EGridCanonicalBoundActionError::AttackSourceNotAuthorized;
		return false;
	}
	if (!ResolveBoundAction(ItemId, ItemLevel, ActionId, OutAttack))
	{
		return false; // Preserve exact action/presentation resolution failure.
	}
	if (OutAttack.Action.Kind != EGridCanonicalActionKind::Attack)
	{
		OutAttack = FGridResolvedBoundAction();
		OutAttack.Error = EGridCanonicalBoundActionError::ActionIsNotAttack;
		return false;
	}
	return true;
}

bool UGridItemSubsystem::ResolveAttackFromEquippedInstance(
	const FGridItemInstance& Instance, FName ActionId,
	EGridEquipmentSlot SourceHand, FGuid ObservedSlotRuntimeObjectId,
	FGuid ObservedCharacterId, int32 ObservedCharacterIndex,
	FGridResolvedBoundAction& OutAttack) const
{
	OutAttack = FGridResolvedBoundAction();
	if (!RootRegistry)
	{
		OutAttack.Error = EGridCanonicalBoundActionError::RegistryNotConfigured;
		return false;
	}

	// Instance.ItemLevel=0 was a legacy format. Never substitute N1 or
	// silently borrow the character's RPG level. No ammo cost is paid here.
	if (!Instance.IsValid() || Instance.ItemLevel < 1 ||
		Instance.ItemLevel > 20 ||
		Instance.AbsorbedEquipmentPhysicalArmor < 0 ||
		Instance.AbsorbedEquipmentMagicalArmor < 0 ||
		!ObservedSlotRuntimeObjectId.IsValid() ||
		!ObservedCharacterId.IsValid() || ObservedCharacterIndex < 0)
	{
		OutAttack.Error = EGridCanonicalBoundActionError::InvalidSourceInstance;
		return false;
	}

	// Ownership AND actual slot identity must agree. CanEquipInHand alone
	// is insufficient; the existing hand action resolver is still authoritative.
	if (Instance.OwnerType != EGridItemOwnerType::EquipmentSlot ||
		Instance.EquipmentSlot != SourceHand ||
		(SourceHand != EGridEquipmentSlot::MainHand &&
		 SourceHand != EGridEquipmentSlot::OffHand) ||
		Instance.RuntimeObjectId != ObservedSlotRuntimeObjectId ||
		Instance.OwnerGuid != ObservedCharacterId ||
		Instance.OwnerCharacterIndex != ObservedCharacterIndex)
	{
		OutAttack.Error = EGridCanonicalBoundActionError::EquipmentSourceMismatch;
		return false;
	}

	return ResolveBoundAttackForHand(
		Instance.ItemDefinitionId, Instance.ItemLevel,
		ActionId, SourceHand, OutAttack);
}
