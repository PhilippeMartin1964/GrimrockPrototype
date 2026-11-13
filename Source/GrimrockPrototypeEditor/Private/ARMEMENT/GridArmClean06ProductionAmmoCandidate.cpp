#include "ARMEMENT/GridArmClean06ProductionAmmoCandidate.h"

#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridCanonicalItemDefinitionAsset.h"

bool GridArmClean06ProductionAmmoCandidate::AddExistingArrows(
	const TArray<FGridWeaponPipelineRow>& Manifest,
	UGridItemSystemAsset& Root,
	GridArmClean06FullProductionCandidate::FSummary& RootSummary,
	FSummary& OutSummary)
{
	OutSummary = FSummary();
	if (Manifest.Num() != 22 || Root.ItemDefinitions.Num() != 22 ||
		!Root.WeaponCatalogue || Root.WeaponCatalogue->Archetypes.Num() != 82 ||
		RootSummary.UnmappedAmmunition <= 0)
	{
		return false;
	}
	const UGridItemDefinitionAsset* SourceAmmo =
		LoadObject<UGridItemDefinitionAsset>(nullptr,
			TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Arrows_01.DA_Item_Arrows_01"));
	const FName ExistingArrowId(TEXT("Item_Arrows_01"));
	if (!IsValid(SourceAmmo) || SourceAmmo->ItemDefinitionId != ExistingArrowId ||
		!SourceAmmo->bStackable || SourceAmmo->MaxStackSize < 1 ||
		!FMath::IsFinite(SourceAmmo->Weight) || SourceAmmo->Weight < 0.0f)
	{
		return false;
	}
	const bool bAmmoWasDirty = SourceAmmo->GetOutermost()->IsDirty();
	if (Root.ItemDefinitions.ContainsByPredicate([ExistingArrowId](
			const TObjectPtr<UGridCanonicalItemDefinitionAsset>& Definition)
			{
				return Definition.Get() && Definition->ItemId == ExistingArrowId;
			}))
	{
		return false;
	}

	TSet<FName> SeenArmIds;
	TArray<FGridCanonicalWeaponArchetype*> RequiringWeapons;
	for (const FGridWeaponPipelineRow& Row : Manifest)
	{
		const FName ArmId(*Row.ArchetypeId);
		if (Row.ProductionState != TEXT("Active") || SeenArmIds.Contains(ArmId))
		{
			return false;
		}
		SeenArmIds.Add(ArmId);
		const UGridItemDefinitionAsset* Weapon =
			LoadObject<UGridItemDefinitionAsset>(nullptr, *Row.ItemPath);
		FGridCanonicalWeaponArchetype* Canonical = Root.WeaponCatalogue->Archetypes.FindByPredicate(
			[ArmId](const FGridCanonicalWeaponArchetype& Entry)
			{
				return Entry.ArchetypeId == ArmId;
			});
		if (!IsValid(Weapon) || !Canonical || !Canonical->bHandAndKindProven ||
			Weapon->WeaponArchetypeId != ArmId)
		{
			return false;
		}
		if (!Weapon->RequiredAmmunitionItemDefinitionId.IsNone())
		{
			// Only the REAL arrows item is currently present in production.
			// Additional ammunition types must receive their own explicit
			// projection, not be silently remapped to these arrows.
			if (Weapon->RequiredAmmunitionItemDefinitionId != ExistingArrowId ||
				Canonical->Kind != EGridCanonicalArmamentKind::Weapon ||
				!Canonical->RequiredAmmoKindId.IsNone())
			{
				return false;
			}
			RequiringWeapons.Add(Canonical);
		}
	}

	if (SeenArmIds.Num() != 22 || RequiringWeapons.IsEmpty() ||
		RequiringWeapons.Num() != RootSummary.UnmappedAmmunition)
	{
		return false;
	}
	UGridAmmoDefinitionAsset* Ammo = NewObject<UGridAmmoDefinitionAsset>(&Root);
	Ammo->ItemId = SourceAmmo->ItemDefinitionId;
	Ammo->DisplayName = SourceAmmo->DisplayName;
	Ammo->Description = SourceAmmo->Description;
	Ammo->WeightKg = SourceAmmo->Weight;
	Ammo->Tags = SourceAmmo->ItemTags;
	Ammo->Icon = SourceAmmo->Icon;
	Ammo->WorldMesh = SourceAmmo->WorldMesh;
	Ammo->MaxStack = SourceAmmo->MaxStackSize;
	Ammo->AmmoKindId = SourceAmmo->ItemDefinitionId; // Reuse one proven source ID.
	if (!Ammo->HasValidIdentity() || Ammo->AmmoKindId.IsNone() ||
		Ammo->ItemId != SourceAmmo->ItemDefinitionId ||
		!Ammo->DisplayName.EqualTo(SourceAmmo->DisplayName) ||
		!Ammo->Description.EqualTo(SourceAmmo->Description) ||
		Ammo->MaxStack != SourceAmmo->MaxStackSize ||
		Ammo->WeightKg != SourceAmmo->Weight ||
		Ammo->Tags != SourceAmmo->ItemTags ||
		Ammo->Icon.ToSoftObjectPath() != SourceAmmo->Icon.ToSoftObjectPath() ||
		Ammo->WorldMesh.ToSoftObjectPath() != SourceAmmo->WorldMesh.ToSoftObjectPath() ||
		SourceAmmo->GetOutermost()->IsDirty() != bAmmoWasDirty)
	{
		return false;
	}

	Root.ItemDefinitions.Add(Ammo);
	for (FGridCanonicalWeaponArchetype* Weapon : RequiringWeapons)
	{
		Weapon->RequiredAmmoKindId = Ammo->AmmoKindId;
	}
	OutSummary.CanonicalAmmoDefinitions = 1;
	OutSummary.WeaponAmmoReferences = RequiringWeapons.Num();
	OutSummary.SourceAmmoItemId = SourceAmmo->ItemDefinitionId;
	OutSummary.ResultAmmoKindId = Ammo->AmmoKindId;
	RootSummary.UnmappedAmmunition = 0;
	// Ammo usage policy remains in the canonical weapon catalogue. This
	// does NOT grant an attack or circumvent the unbound-action cutover gate.
	return true;
}
