#pragma once

#include "CoreMinimal.h"
#include "Runtime/Combat/GridWeaponVisualCatalogueAsset.h"

class UGridItemDefinitionAsset;
struct FGridItemInstance;

/**
 * ARMEMENT01.6.2 — visual-only read service.
 * Returns soft references, not loaded textures or meshes. Call only at
 * presentation transitions, never every Tick or UI frame.
 */
class GRIMROCKPROTOTYPE_API FGridWeaponVisualResolver final
{
public:
	/**
	 * Resolve a definition's authoring visual with optional real item context.
	 *
	 * True / OutVisual.bResolved: at least one output icon/world/equipped mesh
	 * reference exists, including an explicit Legacy fallback.
	 *
	 * OutError / OutVisual.Error: why a catalogue lookup failed. They remain
	 * non-None even if Legacy assets allow the method to return true.
	 *
	 * nullptr is valid for definition-only editor preview; no level is guessed.
	 * A non-null instance must match the definition and be valid.
	 *
	 * Does not load assets, generate random selections or mutate item state.
	 */
	static bool TryResolve(
		const UGridItemDefinitionAsset& Definition,
		const FGridItemInstance* OptionalInstance,
		FGridResolvedWeaponVisual& OutVisual,
		EGridWeaponVisualResolutionError& OutError);
};
