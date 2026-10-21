#pragma once

#include "CoreMinimal.h"
#include "Runtime/Combat/GridWeaponVisualCatalogueAsset.h"

class UGridItemDefinitionAsset;
struct FGridItemInstance;

/**
 * ARMEMENT01.6.4: UI-only, event-driven bridge to the read-only visual resolver.
 * Never changes item stats, hotbar bindings, saves or combat actions.
 */
struct GRIMROCKPROTOTYPE_API FGridWeaponVisualUiResolver
{
	/** Loads a soft catalogue once on first UI presentation; no per-frame work. */
	static FGridResolvedWeaponVisual Resolve(const UGridItemDefinitionAsset& Definition, const FGridItemInstance* Item);

	/** Change an existing action icon only if a complete catalogue profile resolves. */
	static void ApplyCatalogueIcon(const UGridItemDefinitionAsset* Definition, const FGridItemInstance* Item,
		TSoftObjectPtr<UTexture2D>& InOutActionIcon);
};
