#pragma once

#include "CoreMinimal.h"
#include "Runtime/Combat/GridWeaponVisualCatalogueAsset.h"

class UGridItemDefinitionAsset;
class UGridItemSubsystem;
struct FGridItemInstance;

/** Transient presentation for an item's icon (not an equipped combat-action icon). */
struct GRIMROCKPROTOTYPE_API FGridResolvedInventoryItemIcon
{
	bool bCanonicalIdentity = false;
	FName VisualId = NAME_None;
	TSoftObjectPtr<UTexture2D> Icon;
};

/**
 * ARMEMENT01.6.4: UI-only, event-driven bridge to the read-only visual resolver.
 * Never changes item stats, hotbar bindings, saves or combat actions.
 */
struct GRIMROCKPROTOTYPE_API FGridWeaponVisualUiResolver
{
	/** Canonical item icons never read DA_Weapon_* even on invalid levels. */
	static FGridResolvedInventoryItemIcon ResolveInventoryItemIcon(
		const UGridItemSubsystem* Registry, const UGridItemDefinitionAsset* LegacyDefinition,
		const FGridItemInstance& Item);

#if WITH_DEV_AUTOMATION_TESTS
	/** Historical preview for worldless Automation fixtures only; absent in Shipping. */
	static FGridResolvedWeaponVisual Resolve(const UGridItemDefinitionAsset& Definition, const FGridItemInstance* Item);

	/** Historical action-icon preview for worldless Automation fixtures only. */
	static void ApplyCatalogueIcon(const UGridItemDefinitionAsset* Definition, const FGridItemInstance* Item,
		TSoftObjectPtr<UTexture2D>& InOutActionIcon);
#endif
};
