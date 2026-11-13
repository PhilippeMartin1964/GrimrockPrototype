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
		if (!Archetype || !Archetype->bHandAndKindProven ||
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
		OutItem.ArchetypeId = Armament->ArchetypeId;
		OutItem.bIsArmament = true;
		OutItem.WorldMesh = Visual->WorldMesh;
		// The legacy visual resolver uses WorldMesh for a missing equipped mesh.
		// Keep this transient resolution: do not serialize a second mesh pointer.
		OutItem.EquippedMesh = Visual->EquippedMesh.IsNull() ?
			Visual->WorldMesh : Visual->EquippedMesh;
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
