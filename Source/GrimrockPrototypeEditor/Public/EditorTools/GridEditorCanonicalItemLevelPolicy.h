#pragma once

#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"

/** Editor-only validation of NEW authored levels; never touches SaveGame instances. */
struct FGridEditorCanonicalItemLevelPolicy
{
	/** Persisted ID/N-level authoring: no legacy definition or N1 default
	 * is ever inferred after the physical identity has been authored. */
	static bool CanAuthorCanonicalLevel(
		const UGridItemSubsystem* Registry, FName ItemId, int32 PhysicalLevel)
	{
		if (!IsValid(Registry) || !Registry->IsConfigured() ||
			!Registry->HasCanonicalItem(ItemId) ||
			PhysicalLevel < 0 || PhysicalLevel > 20)
		{
			return false;
		}
		FGridResolvedItem Resolved;
		return Registry->ResolveItem(ItemId, PhysicalLevel, Resolved) &&
			Resolved.IsValid() && Resolved.ItemId == ItemId &&
			Resolved.ItemLevel == PhysicalLevel &&
			(Resolved.bIsArmament || PhysicalLevel == 0);
	}

	static bool IsLevelEditable(const UGridItemSubsystem* Registry, const UGridItemDefinitionAsset* Definition)
	{
		if (!IsValid(Registry) || !Registry->IsConfigured() ||
			!IsValid(Definition) || Definition->ItemDefinitionId.IsNone())
		{
			return false; // Missing ItemSystem cannot silently validate legacy fields.
		}
		if (Registry->HasCanonicalItem(Definition->ItemDefinitionId))
		{
			FGridResolvedItem Family;
			return Registry->ResolveItem(Definition->ItemDefinitionId, 1, Family) && Family.bIsArmament;
		}
		return Definition->bUseWeaponCatalogue; // Unmigrated IDs only, through 06.4.
	}

	static bool CanAuthorLevel(const UGridItemSubsystem* Registry,
		const UGridItemDefinitionAsset* Definition, int32 AuthoredLevel)
	{
		if (!IsValid(Registry) || !Registry->IsConfigured() ||
			!IsValid(Definition) || Definition->ItemDefinitionId.IsNone() ||
			AuthoredLevel < 0 || AuthoredLevel > 20)
		{
			return false;
		}
		const FName ItemId = Definition->ItemDefinitionId;
		if (Registry->HasCanonicalItem(ItemId))
		{
			// Exactly the authored vs persisted rule used by the world reader:
			// only fresh ARM N0 can inherit DefaultWeaponItemLevel once.
			FGridResolvedItem Family;
			if (!Registry->ResolveItem(ItemId, 1, Family)) return false;
			if (!Family.bIsArmament && AuthoredLevel != 0) return false;
			const int32 FixedLevel = Family.bIsArmament && AuthoredLevel == 0
				? Definition->DefaultWeaponItemLevel : AuthoredLevel;
			if (Family.bIsArmament && (FixedLevel < 1 || FixedLevel > 20)) return false;
			FGridResolvedItem Canonical;
			return Registry->ResolveItem(ItemId, FixedLevel, Canonical) &&
				Canonical.bIsArmament == Family.bIsArmament;
		}

		// Until 06.4 migrates the binary assets, honor only unknown historical IDs.
		if (!Definition->bUseWeaponCatalogue) return AuthoredLevel == 0;
		if (AuthoredLevel == 0) return true; // Historical inherited-default request.
		const FGridWeaponLevelProfile* Profile = nullptr;
		return Definition->TryGetWeaponCatalogueProfile(AuthoredLevel, Profile) && Profile;
	}
};
