#pragma once

#if WITH_DEV_AUTOMATION_TESTS
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "UObject/StrongObjectPtr.h"

/** Attach the real ItemSystem (or a private transient extension for synthetic
 * fixture IDs) to a world which previously had no GameInstance. No source
 * package or global configuration is modified. Detach before destroying World. */
struct FGridCanonicalLootTestScope
{
	UWorld* World = nullptr;
	TStrongObjectPtr<UGameInstance> GI;
	UGridItemSubsystem* Registry = nullptr;

	bool Attach(UWorld* InWorld, const TArray<FName>& SyntheticIds = {})
	{
		TMap<FName, float> SyntheticWeights;
		for (const FName Id : SyntheticIds)
		{
			if (SyntheticWeights.Contains(Id)) return false;
			SyntheticWeights.Add(Id, 1.0f);
		}
		return AttachWithWeights(InWorld, SyntheticWeights);
	}

	// Synthetic definitions belong to one transient registry. No global
	// metadata, instance copies or authored package edits are needed.
	bool AttachWithWeights(UWorld* InWorld, const TMap<FName, float>& SyntheticWeights,
		const TFunction<void(UGridItemSystemAsset&)>& Extend = {})
	{
		if (World || !IsValid(InWorld) || InWorld->GetGameInstance()) return false;
		World = InWorld;
		GI.Reset(NewObject<UGameInstance>(GetTransientPackage()));
		if (!GI.IsValid()) return false;
		GI->Init();
		World->SetGameInstance(GI.Get());
		Registry = GI->GetSubsystem<UGridItemSubsystem>();
		if (!Registry || !Registry->IsConfigured()) return false;
		if (SyntheticWeights.IsEmpty() && !Extend) return true;
		UGridItemSystemAsset* Production = Registry->LoadConfiguredItemSystemForPreview();
		if (!IsValid(Production)) return false;
		UGridItemSystemAsset* Root = DuplicateObject<UGridItemSystemAsset>(Production, GetTransientPackage());
		if (!IsValid(Root)) return false;
		for (const auto& Pair : SyntheticWeights)
		{
			const FName Id = Pair.Key;
			if (Id.IsNone() || Registry->ContainsItem(Id) ||
				!FMath::IsFinite(Pair.Value) || Pair.Value < 0.0f) return false;
			auto* Item = NewObject<UGridNonArmamentDefinitionAsset>(Root);
			Item->ItemId = Id;
			Item->DisplayName = FText::FromName(Id);
			Item->WeightKg = Pair.Value;
			Root->ItemDefinitions.Add(Item);
		}
		if (Extend) Extend(*Root);
		Registry->Deinitialize();
		TArray<FString> Errors;
		return Registry->RegisterItemSystem(Root, Errors);
	}

	// Compact ARM fixture using the real validators, action binding and presentation.
	static void AddWeapon(UGridItemSystemAsset& Root, FName ItemId, FName ActionId, const FText& Name,
		EGridEquipmentSlot Hand, EGridPhysicalDamageSubtype Subtype, EGridAttackScalingAttribute Scaling,
		int32 DiceSides, int32 Range, int32 AP)
	{
		if (Root.WeaponCatalogue->GetOuter() != &Root)
		{
			Root.WeaponCatalogue = DuplicateObject<UGridCanonicalWeaponCatalogueAsset>(Root.WeaponCatalogue, &Root);
			Root.WeaponVisualCatalogue = DuplicateObject<UGridCanonicalWeaponVisualCatalogueAsset>(Root.WeaponVisualCatalogue, &Root);
			Root.ActionCatalogue = DuplicateObject<UGridActionCatalogueAsset>(Root.ActionCatalogue, &Root);
		}
		// The production schema reserves ARM001–ARM082. Replace a slot in
		// this private catalogue and remove its original concrete definitions.
		const FGridWeaponArchetype* Replaced = Root.WeaponCatalogue->Archetypes.FindByPredicate([&Root](const FGridWeaponArchetype& Row)
		{
			return Row.Kind == EGridArmamentKind::Weapon &&
				!Root.ItemDefinitions.ContainsByPredicate([&Root, &Row](const auto& Entry)
				{
					const auto* Arm = Cast<UGridArmamentDefinitionAsset>(Entry.Get());
					return Arm && Arm->GetOuter() == &Root && Arm->ArchetypeId == Row.ArchetypeId;
				});
		});
		check(Replaced);
		const FName ArchetypeId = Replaced->ArchetypeId;
		Root.ItemDefinitions.RemoveAll([ArchetypeId](const auto& Entry)
		{
			const auto* Arm = Cast<UGridArmamentDefinitionAsset>(Entry.Get());
			return Arm && Arm->ArchetypeId == ArchetypeId;
		});
		Root.WeaponCatalogue->Archetypes.RemoveAll([ArchetypeId](const auto& Row) { return Row.ArchetypeId == ArchetypeId; });
		Root.WeaponVisualCatalogue->ArchetypeBindings.RemoveAll([ArchetypeId](const auto& Row) { return Row.ArchetypeId == ArchetypeId; });
		auto* Item = NewObject<UGridArmamentDefinitionAsset>(&Root);
		Item->ItemId = ItemId;
		Item->ArchetypeId = ArchetypeId;
		Item->DisplayName = Name;
		Root.ItemDefinitions.Add(Item);

		FGridWeaponArchetype& Archetype = Root.WeaponCatalogue->Archetypes.AddDefaulted_GetRef();
		Archetype.ArchetypeId = ArchetypeId;
		Archetype.bHandAndKindProven = true;
		Archetype.HandPolicy = Hand == EGridEquipmentSlot::OffHand ? EGridHandPolicy::OffHand : EGridHandPolicy::MainHand;
		Archetype.PhysicalSubtype = Subtype;
		Archetype.ScalingAttribute = Scaling;
		Archetype.ActionPointCost = AP;
		Archetype.RangeCells = Range;
		Archetype.DamageDiceN1.bEnabled = true;
		Archetype.DamageDiceN1.DiceSides = DiceSides;
		Archetype.PrimaryClassId = Root.WeaponCatalogue->ClassPreferences[0].ClassId;
		Archetype.AttackSourceSlots.Add(Hand);
		FGridItemActionBinding& Binding = Archetype.Actions.AddDefaulted_GetRef();
		Binding.ActionId = ActionId;
		Binding.PresentationId = Root.ActionPresentationCatalogue->Presentations[0].PresentationId;

		FGridAction& Action = Root.ActionCatalogue->Actions.AddDefaulted_GetRef();
		Action.ActionId = ActionId;
		Action.Targeting = EGridCombatTargetingPolicy::FirstAxialTarget;
		Action.bRangedAttack = Range > 1;

		const FGridWeaponVisual* Visual = Root.WeaponVisualCatalogue->VisualProfiles.FindByPredicate(
			[](const FGridWeaponVisual& Row) { return !Row.ProjectileAppearance.bEnabled; });
		FGridWeaponVisualBinding& VisualBinding = Root.WeaponVisualCatalogue->ArchetypeBindings.AddDefaulted_GetRef();
		VisualBinding.ArchetypeId = ArchetypeId;
		VisualBinding.VisualId = Visual->VisualId;
	}

	// Keep authored action rows in an isolated copy of the global catalogue.
	static void BindDetailedAction(UGridItemSystemAsset& Root, TArray<FGridItemActionBinding>& Bindings,
		const FGridCombatActionDefinition& Definition)
	{
		Root.ActionCatalogue = DuplicateObject<UGridActionCatalogueAsset>(Root.ActionCatalogue, &Root);
		Root.ActionCatalogue->DetailedActions.AddDefaulted_GetRef().Definition = Definition;
		FGridItemActionBinding& Binding = Bindings.AddDefaulted_GetRef();
		Binding.ActionId = Definition.ActionId;
		Binding.PresentationId = Root.ActionPresentationCatalogue->Presentations[0].PresentationId;
	}

	void Detach()
	{
		if (World) World->SetGameInstance(nullptr);
		if (GI.IsValid()) GI->Shutdown();
		Registry = nullptr;
		World = nullptr;
		GI.Reset();
	}

	~FGridCanonicalLootTestScope() { Detach(); }
};
#endif
