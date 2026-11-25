#include "Runtime/Items/GridItemSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogGridCanonicalItems, Log, All);

void UGridItemSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	if (ConfiguredItemSystem.IsNull())
	{
		// An unconfigured GameInstance leaves the registry empty.
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
	for (UGridItemDefinitionAsset* Item : System->ItemDefinitions)
	{
		ItemIndex.Add(Item->ItemId, Item);
	}
	return true;
}

bool UGridItemSubsystem::ResolvePhysicalItem(
	const FGridItemInstance& Instance, FGridResolvedItem& OutItem) const
{
	if (!Instance.IsValid())
	{
		OutItem = FGridResolvedItem();
		return false;
	}
	return ResolveItem(Instance.ItemDefinitionId, Instance.ItemLevel, OutItem) &&
		OutItem.ItemId == Instance.ItemDefinitionId && OutItem.ItemLevel == Instance.ItemLevel;
}

bool UGridItemSubsystem::ResolveItem(FName ItemId, int32 ItemLevel, FGridResolvedItem& OutItem) const
{
	OutItem = FGridResolvedItem();
	if (!RootRegistry)
	{
		OutItem.Error = EGridItemResolveError::RegistryNotConfigured;
		return false;
	}
	// Armaments have authored N1-N20 progression. Ammo, food and other
	// unlevelled identities legitimately persist as ItemLevel=0.
	if (ItemLevel < 0 || ItemLevel > 20)
	{
		OutItem.Error = EGridItemResolveError::InvalidItemLevel;
		return false;
	}
	const TObjectPtr<UGridItemDefinitionAsset>* Found = ItemIndex.Find(ItemId);
	if (!Found || !IsValid(Found->Get()))
	{
		OutItem.Error = EGridItemResolveError::UnknownItemId;
		return false;
	}

	const UGridItemDefinitionAsset* Definition = Found->Get();
	const bool bArmament = Definition->IsA<UGridArmamentDefinitionAsset>();
	if ((bArmament && ItemLevel == 0) || (!bArmament && ItemLevel != 0))
	{
		// One physical level contract for every reader. No non-ARM N1 family
		// alias and no ARM N0 default promotion.
		OutItem.Error = EGridItemResolveError::InvalidItemLevel;
		return false;
	}
	OutItem.ItemId = Definition->ItemId;
	OutItem.ItemLevel = ItemLevel;
	OutItem.DisplayName = Definition->DisplayName;
	OutItem.Description = Definition->Description;
	OutItem.WeightKg = Definition->WeightKg;
	OutItem.Tags = Definition->Tags;

	// Project non-ARM physical presentation BEFORE the ammunition fast path:
	// arrows and other ammo must retain their canonical Niagara/physics data.
	if (const UGridNonArmamentDefinitionAsset* Other =
		Cast<UGridNonArmamentDefinitionAsset>(Definition))
	{
		OutItem.GenericThrow = Other->GenericThrow;
		OutItem.LightEmitter = Other->LightEmitter;
		OutItem.VisualEffects = Other->VisualEffects;
		OutItem.bEnableWorldSparkle = Other->bEnableWorldSparkle;
		OutItem.WorldSparkleMaterial = Other->WorldSparkleMaterial;
		OutItem.WorldSparkleColor = Other->WorldSparkleColor;
		OutItem.WorldSparkleIntensity = Other->WorldSparkleIntensity;
		OutItem.WorldSparkleSpeed = Other->WorldSparkleSpeed;
		OutItem.WorldSparkleVariation = Other->WorldSparkleVariation;
		OutItem.ReadText = Other->ReadText;
		OutItem.bReadable = Other->bReadable;
		OutItem.bUseItemWeightAsWorldPhysicsMass =
			Other->bUseItemWeightAsWorldPhysicsMass;
		OutItem.WorldPhysicsInitialTiltDegrees =
			Other->WorldPhysicsInitialTiltDegrees;
	}

	// Ammunition is a distinct canonical family. Finalize its identity before
	// any other equipment/consumable projection can be considered.
	if (const UGridAmmoDefinitionAsset* Ammo = Cast<UGridAmmoDefinitionAsset>(Definition))
	{
		OutItem.ItemIcon = Ammo->Icon;
		OutItem.WorldMesh = Ammo->WorldMesh;
		OutItem.AmmoKindId = Ammo->AmmoKindId;
		OutItem.MaxStack = Ammo->MaxStack;
		OutItem.Error = EGridItemResolveError::None;
		return true;
	}

	if (const UGridArmamentDefinitionAsset* Armament =
		Cast<UGridArmamentDefinitionAsset>(Definition))
	{
		const FGridWeaponArchetype* Archetype =
			RootRegistry->WeaponCatalogue->Find(Armament->ArchetypeId);
		// Numeric parity never grants an active weapon an implicit attack.
		// A real offensive ARM requires a bound attack and a source hand.
		const bool bMissingOffensiveAction = Archetype &&
			Archetype->Kind == EGridArmamentKind::Weapon &&
			(Archetype->AttackSourceSlots.IsEmpty() ||
				!Archetype->Actions.ContainsByPredicate(
					[this](const FGridItemActionBinding& Binding)
					{
						const FGridAction* Action =
							RootRegistry->ActionCatalogue->Find(Binding.ActionId);
						return Action && Action->Kind == EGridActionKind::Attack;
					}));
		if (!Archetype || !Archetype->bHandAndKindProven || bMissingOffensiveAction ||
			!Archetype->TryResolve(ItemLevel, OutItem.Weapon))
		{
			OutItem = FGridResolvedItem();
			OutItem.Error = EGridItemResolveError::InvalidArchetype;
			return false;
		}
		const FGridWeaponVisual* Visual =
			RootRegistry->WeaponVisualCatalogue->FindForArchetype(Armament->ArchetypeId);
		if (!Visual)
		{
			OutItem = FGridResolvedItem();
			OutItem.Error = EGridItemResolveError::MissingVisual;
			return false;
		}
		if (OutItem.Weapon.Projectile.bEnabled !=
				Visual->ProjectileAppearance.bEnabled ||
			(OutItem.Weapon.Projectile.bEnabled && Visual->ProjectileMesh.IsNull()))
		{
			OutItem = FGridResolvedItem();
			OutItem.Error = EGridItemResolveError::IncompleteProjectileVisual;
			return false;
		}
		OutItem.ArchetypeId = Armament->ArchetypeId;
		OutItem.VisualId = Visual->VisualId;
		OutItem.bIsArmament = true;
		OutItem.WorldMesh = Visual->WorldMesh;
		OutItem.WorldVisualTransform = Visual->WorldTransform;
		if (const FGridWeaponFinish* Finish = RootRegistry->WeaponVisualCatalogue->ResolveFinish(*Visual, ItemLevel))
		{
			OutItem.WeaponFinish = *Finish;
		}
		// The visual catalogue owns world pickup presentation.
		OutItem.LightEmitter = Visual->LightEmitter;
		OutItem.VisualEffects = Visual->VisualEffects;
		OutItem.bEnableWorldSparkle = Visual->bEnableWorldSparkle;
		OutItem.WorldSparkleMaterial = Visual->WorldSparkleMaterial;
		OutItem.WorldSparkleColor = Visual->WorldSparkleColor;
		OutItem.WorldSparkleIntensity = Visual->WorldSparkleIntensity;
		OutItem.WorldSparkleSpeed = Visual->WorldSparkleSpeed;
		OutItem.WorldSparkleVariation = Visual->WorldSparkleVariation;
		// Physical world-pickup policy belongs to the ARM archetype, because
		// several weapons may share a visual without sharing their physics.
		OutItem.bUseItemWeightAsWorldPhysicsMass = Archetype->bUseItemWeightAsWorldPhysicsMass;
		OutItem.WorldPhysicsInitialTiltDegrees = Archetype->WorldPhysicsInitialTiltDegrees;
		// Reuse the world mesh when no equipped mesh is authored.
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
			OutItem.AvailableActions = Other->ActionBindings;
		}
		// ARM-CLEAN06.4Q: generic non-ARM stack capacity is an item-family
		// authority, distinct from consumable and ammunition MaxStack.
		if (const UGridOtherEquipmentDefinitionAsset* Equipment =
			Cast<UGridOtherEquipmentDefinitionAsset>(Definition))
		{
			OutItem.MaxStack = Equipment->MaxStack;
			OutItem.EquippedMesh = Equipment->GetEffectiveEquippedMesh();
			// Hand and armor-slot permissions are authored on the ONE
			// registered non-ARM equipment definition. A torch must not
			// become non-equippable merely because it is not an ARM weapon.
			OutItem.NonArmHandPolicy = Equipment->HandPolicy;
			OutItem.NonArmEquipmentSlots = Equipment->GetAvailableSlots();
			OutItem.NonArmEquipmentBonuses = Equipment->Bonuses;
			OutItem.NonArmEquipmentResistances = Equipment->Resistances;
			OutItem.bNonArmTwoHanded =
				Equipment->HandPolicy == EGridNonArmamentHandPolicy::TwoHanded &&
				OutItem.NonArmEquipmentSlots.Contains(EGridEquipmentSlot::MainHand);
		}
		if (const UGridGenericStackableDefinitionAsset* Generic =
			Cast<UGridGenericStackableDefinitionAsset>(Definition))
		{
			OutItem.MaxStack = Generic->MaxStack;
		}
		if (const UGridConsumableDefinitionAsset* Consumable =
			Cast<UGridConsumableDefinitionAsset>(Definition))
		{
			OutItem.MaxStack = Consumable->MaxStack;
			OutItem.AvailableActions = Consumable->UseActions;
		}
	}
	OutItem.Error = EGridItemResolveError::None;
	return true;
}

bool UGridItemSubsystem::ResolveAmmoKind(FName AmmoKindId, FGridResolvedItem& OutAmmo) const
{
	OutAmmo = FGridResolvedItem();
	if (!RootRegistry || AmmoKindId.IsNone())
	{
		return false;
	}

	// RegisterItemSystem validates AmmoKindId uniqueness and requires every
	// weapon's RequiredAmmoKindId to resolve. No secondary runtime index.
	for (const TPair<FName, TObjectPtr<UGridItemDefinitionAsset>>& Pair : ItemIndex)
	{
		const UGridAmmoDefinitionAsset* Ammo = Cast<UGridAmmoDefinitionAsset>(Pair.Value.Get());
		if (IsValid(Ammo) && Ammo->AmmoKindId == AmmoKindId)
		{
			return ResolveItem(Pair.Key, 0, OutAmmo) &&
				!OutAmmo.bIsArmament && OutAmmo.AmmoKindId == AmmoKindId;
		}
	}
	return false;
}

TSoftObjectPtr<UTexture2D> UGridItemSubsystem::FindEquippedActionIcon(
	EGridEquippedWeaponActionIconKind Kind) const
{
	if (Kind == EGridEquippedWeaponActionIconKind::None || !RootRegistry ||
		!RootRegistry->WeaponVisualCatalogue)
	{
		return TSoftObjectPtr<UTexture2D>();
	}
	const TSoftObjectPtr<UTexture2D>* Found =
		RootRegistry->WeaponVisualCatalogue->ActionIcons.Find(Kind);
	return Found ? *Found : TSoftObjectPtr<UTexture2D>();
}

bool UGridItemSubsystem::ResolveArmorReserveForInstance(
	const FGridItemInstance& Instance,
	FGridArmorReserveView& OutReserve) const
{
	// A failed read must never expose a cached amount from an earlier item.
	OutReserve = FGridArmorReserveView();
	if (!RootRegistry || !Instance.IsValid() ||
		Instance.AbsorbedEquipmentPhysicalArmor < 0 ||
		Instance.AbsorbedEquipmentMagicalArmor < 0)
	{
		return false;
	}

	// Resolve the real physical item. An unlevelled equipment definition
	// must never be synthesized as a weapon at N1, and vice versa.
	FGridResolvedItem Resolved;
	if (!ResolveItem(Instance.ItemDefinitionId, Instance.ItemLevel, Resolved))
	{
		return false;
	}
	return ProjectArmorReserve(Instance, Resolved, OutReserve);
}

bool UGridItemSubsystem::ProjectArmorReserve(const FGridItemInstance& Instance,
	const FGridResolvedItem& Resolved, FGridArmorReserveView& OutReserve)
{
	OutReserve = FGridArmorReserveView();
	if (!Instance.IsValid() || !Resolved.IsValid() ||
		Resolved.ItemId != Instance.ItemDefinitionId || Resolved.ItemLevel != Instance.ItemLevel ||
		Instance.AbsorbedEquipmentPhysicalArmor < 0 || Instance.AbsorbedEquipmentMagicalArmor < 0)
	{
		return false;
	}
	if (!Resolved.bIsArmament)
	{
		if (Instance.ItemLevel != 0 || Resolved.NonArmEquipmentSlots.IsEmpty())
		{
			return false;
		}
		OutReserve.NominalPhysicalArmor = Resolved.NonArmEquipmentBonuses.ArmorBonus;
		OutReserve.NominalMagicalArmor = Resolved.NonArmEquipmentBonuses.MagicalArmorBonus;
		OutReserve.SpentPhysicalArmor = Instance.AbsorbedEquipmentPhysicalArmor;
		OutReserve.SpentMagicalArmor = Instance.AbsorbedEquipmentMagicalArmor;
		// Preserve the prior non-ARM rule for negative authored modifiers.
		// Positive reserves deplete, a penalty is not a recoverable reserve.
		OutReserve.RemainingPhysicalArmor = OutReserve.NominalPhysicalArmor > 0
			? FMath::Max(0, OutReserve.NominalPhysicalArmor - OutReserve.SpentPhysicalArmor)
			: OutReserve.NominalPhysicalArmor;
		OutReserve.RemainingMagicalArmor = OutReserve.NominalMagicalArmor > 0
			? FMath::Max(0, OutReserve.NominalMagicalArmor - OutReserve.SpentMagicalArmor)
			: OutReserve.NominalMagicalArmor;
		return true;
	}
	if (Instance.ItemLevel < 1 || Instance.ItemLevel > 20)
	{
		return false;
	}
	return FGridArmorReserveResolver::TryCompute(
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
		OutAction.Error = EGridBoundActionError::RegistryNotConfigured;
		return false;
	}

	FGridResolvedItem Item;
	if (!ResolveItem(ItemId, ItemLevel, Item))
	{
		OutAction.Error = EGridBoundActionError::ItemResolutionFailed;
		OutAction.ItemError = Item.Error;
		return false;
	}

	return ResolveBoundAction(Item, ActionId, OutAction);
}

bool UGridItemSubsystem::ResolveBoundAction(
	const FGridResolvedItem& Item, FName ActionId, FGridResolvedBoundAction& OutAction) const
{
	OutAction = FGridResolvedBoundAction();
	if (!RootRegistry)
	{
		OutAction.Error = EGridBoundActionError::RegistryNotConfigured;
		return false;
	}
	if (!Item.IsValid())
	{
		OutAction.Error = EGridBoundActionError::ItemResolutionFailed;
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
		OutAction.Error = EGridBoundActionError::ActionNotBound;
		return false;
	}

	const FGridAction* Action = RootRegistry->ActionCatalogue ?
		RootRegistry->ActionCatalogue->Find(Binding->ActionId) : nullptr;
	if (!Action)
	{
		OutAction.Error = EGridBoundActionError::ActionDefinitionMissing;
		return false;
	}
	const FGridActionPresentation* Presentation =
		RootRegistry->ActionPresentationCatalogue ?
		RootRegistry->ActionPresentationCatalogue->Find(Binding->PresentationId) : nullptr;
	if (!Presentation)
	{
		OutAction.Error = EGridBoundActionError::PresentationDefinitionMissing;
		return false;
	}
	OutAction.ItemId = Item.ItemId;
	OutAction.ItemLevel = Item.ItemLevel;
	OutAction.ActionId = Binding->ActionId;
	OutAction.PresentationId = Binding->PresentationId;
	OutAction.Action = *Action;
	OutAction.Presentation = *Presentation;
	OutAction.ItemError = EGridItemResolveError::None;
	OutAction.Error = EGridBoundActionError::None;
	return true;
}

bool UGridItemSubsystem::ResolveDetailedNonArmAction(
	FName ItemId, int32 ItemLevel, FName ActionId,
	FGridCombatActionDefinition& OutAction) const
{
	OutAction = FGridCombatActionDefinition();
	if (!RootRegistry || !RootRegistry->ActionCatalogue ||
		!RootRegistry->ActionPresentationCatalogue || ActionId.IsNone())
	{
		return false;
	}
	FGridResolvedItem Resolved;
	if (!ResolveItem(ItemId, ItemLevel, Resolved))
	{
		return false;
	}
	return ResolveDetailedNonArmAction(Resolved, ActionId, OutAction);
}

bool UGridItemSubsystem::ResolveDetailedNonArmAction(
	const FGridResolvedItem& Resolved, FName ActionId,
	FGridCombatActionDefinition& OutAction) const
{
	OutAction = FGridCombatActionDefinition();
	if (!RootRegistry || !RootRegistry->ActionCatalogue ||
		!RootRegistry->ActionPresentationCatalogue || ActionId.IsNone() ||
		!Resolved.IsValid() || Resolved.bIsArmament || !Resolved.AmmoKindId.IsNone())
	{
		return false;
	}
	const FGridItemActionBinding* Binding = Resolved.AvailableActions.FindByPredicate(
		[ActionId](const FGridItemActionBinding& Candidate)
		{
			return Candidate.ActionId == ActionId && !Candidate.PresentationId.IsNone();
		});
	if (!Binding || RootRegistry->ActionCatalogue->Find(Binding->ActionId) ||
		RootRegistry->ActionPresentationCatalogue->Find(Binding->PresentationId) == nullptr &&
		RootRegistry->ActionPresentationCatalogue->FindDetailed(Binding->PresentationId) == nullptr)
	{
		return false;
	}
	const FGridCombatActionDefinition* Detailed =
		RootRegistry->ActionCatalogue->FindDetailed(Binding->ActionId);
	if (!Detailed || !Detailed->IsValid() || Detailed->ActionId != ActionId)
	{
		return false;
	}
	// The binding owns the presentation identity; original execution fields
	// (including AP, cooldown, target and effects) remain untouched.
	if (!Detailed->PresentationProfileId.IsNone() &&
		Detailed->PresentationProfileId != Binding->PresentationId)
	{
		return false;
	}
	OutAction = *Detailed;
	return true;
}

bool UGridItemSubsystem::ResolveDetailedNonArmEquippedAction(
	const FGridItemInstance& Instance, EGridEquipmentSlot SourceHand,
	FName ActionId, FGridCombatActionDefinition& OutAction) const
{
	OutAction = FGridCombatActionDefinition();
	if (!RootRegistry || !Instance.IsValid() || Instance.ItemLevel < 0 ||
		Instance.ItemLevel > 20 || Instance.Quantity < 1 ||
		(SourceHand != EGridEquipmentSlot::MainHand &&
		 SourceHand != EGridEquipmentSlot::OffHand) ||
		Instance.EquipmentSlot != SourceHand || ActionId.IsNone())
	{
		return false;
	}
	const TObjectPtr<UGridItemDefinitionAsset>* Found =
		ItemIndex.Find(Instance.ItemDefinitionId);
	const UGridOtherEquipmentDefinitionAsset* Equipment =
		Found ? Cast<UGridOtherEquipmentDefinitionAsset>(Found->Get()) : nullptr;
	if (!IsValid(Equipment) ||
		!Equipment->EquipmentSlots.Contains(SourceHand) ||
		!Equipment->CanEquipToSlot(SourceHand) ||
		(Equipment->HandPolicy == EGridNonArmamentHandPolicy::TwoHanded &&
		 SourceHand == EGridEquipmentSlot::OffHand))
	{
		return false;
	}
	FGridCombatActionDefinition Exact;
	if (!ResolveDetailedNonArmAction(Instance.ItemDefinitionId,
		Instance.ItemLevel, ActionId, Exact) ||
		Exact.SourcePolicy != EGridCombatActionSourcePolicy::Equipment ||
		Exact.ActionId != ActionId ||
		(Exact.ResolutionProfile == EGridCombatActionResolutionProfile::Attack &&
		 !Exact.OffensiveProfile.IsValid()))
	{
		return false;
	}
	OutAction = MoveTemp(Exact);
	return true;
}

bool UGridItemSubsystem::ResolveDetailedNonArmEquippedPresentation(
	FName ItemId, EGridEquipmentSlot SourceHand,
	FGridPlayerAttackPresentationProfile& OutPresentation) const
{
	OutPresentation = FGridPlayerAttackPresentationProfile();
	if (!RootRegistry || !RootRegistry->ActionCatalogue ||
		!RootRegistry->ActionPresentationCatalogue || ItemId.IsNone() ||
		(SourceHand != EGridEquipmentSlot::MainHand &&
		 SourceHand != EGridEquipmentSlot::OffHand))
	{
		return false;
	}
	const TObjectPtr<UGridItemDefinitionAsset>* Found = ItemIndex.Find(ItemId);
	const UGridOtherEquipmentDefinitionAsset* Equipment =
		Found ? Cast<UGridOtherEquipmentDefinitionAsset>(Found->Get()) : nullptr;
	if (!IsValid(Equipment) || !Equipment->EquipmentSlots.Contains(SourceHand) ||
		!Equipment->CanEquipToSlot(SourceHand) ||
		(Equipment->HandPolicy == EGridNonArmamentHandPolicy::TwoHanded &&
		 SourceHand == EGridEquipmentSlot::OffHand))
	{
		return false;
	}
	for (const FGridItemActionBinding& Binding : Equipment->ActionBindings)
	{
		const FGridCombatActionDefinition* Action =
			RootRegistry->ActionCatalogue->FindDetailed(Binding.ActionId);
		const FGridPlayerAttackPresentationProfile* Profile =
			RootRegistry->ActionPresentationCatalogue->FindDetailed(Binding.PresentationId);
		if (Action && Action->IsValid() &&
			Action->SourcePolicy == EGridCombatActionSourcePolicy::Equipment &&
			Profile && Profile->IsValid())
		{
			OutPresentation = *Profile;
			return true;
		}
	}
	return false;
}

bool UGridItemSubsystem::ResolveBoundAttackForHand(
	FName ItemId, int32 ItemLevel, FName ActionId, EGridEquipmentSlot SourceHand,
	FGridResolvedBoundAction& OutAttack) const
{
	OutAttack = FGridResolvedBoundAction();
	if (!RootRegistry)
	{
		OutAttack.Error = EGridBoundActionError::RegistryNotConfigured;
		return false;
	}
	FGridResolvedItem Item;
	if (!ResolveItem(ItemId, ItemLevel, Item))
	{
		OutAttack.Error = EGridBoundActionError::ItemResolutionFailed;
		OutAttack.ItemError = Item.Error;
		return false;
	}
	if (!Item.bIsArmament || !Item.Weapon.HasAttackSourceInHand(SourceHand))
	{
		OutAttack.Error = EGridBoundActionError::AttackSourceNotAuthorized;
		return false;
	}
	if (!ResolveBoundAction(ItemId, ItemLevel, ActionId, OutAttack))
	{
		return false; // Preserve exact action/presentation resolution failure.
	}
	if (OutAttack.Action.Kind != EGridActionKind::Attack)
	{
		OutAttack = FGridResolvedBoundAction();
		OutAttack.Error = EGridBoundActionError::ActionIsNotAttack;
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
		OutAttack.Error = EGridBoundActionError::RegistryNotConfigured;
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
		OutAttack.Error = EGridBoundActionError::InvalidSourceInstance;
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
		OutAttack.Error = EGridBoundActionError::EquipmentSourceMismatch;
		return false;
	}

	return ResolveBoundAttackForHand(
		Instance.ItemDefinitionId, Instance.ItemLevel,
		ActionId, SourceHand, OutAttack);
}
