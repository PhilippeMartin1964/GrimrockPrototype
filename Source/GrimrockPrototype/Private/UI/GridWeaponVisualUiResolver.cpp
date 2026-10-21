#include "UI/GridWeaponVisualUiResolver.h"

#include "Runtime/Combat/GridWeaponVisualResolver.h"
#include "Runtime/GridItemDefinitionAsset.h"

FGridResolvedWeaponVisual FGridWeaponVisualUiResolver::Resolve(
	const UGridItemDefinitionAsset& Definition, const FGridItemInstance* Item)
{
	// The core resolver deliberately never synchronously loads assets.
	// UI loads only at an inventory / hotbar refresh event, not on Tick.
	if (Definition.bUseWeaponVisualCatalogue && !Definition.WeaponVisualCatalogue.IsNull() &&
		!Definition.WeaponVisualCatalogue.IsValid())
	{
		Definition.WeaponVisualCatalogue.LoadSynchronous();
	}

	FGridResolvedWeaponVisual Visual;
	EGridWeaponVisualResolutionError Error = EGridWeaponVisualResolutionError::None;
	FGridWeaponVisualResolver::TryResolve(Definition, Item, Visual, Error);
	return Visual;
}

void FGridWeaponVisualUiResolver::ApplyCatalogueIcon(
	const UGridItemDefinitionAsset* Definition, const FGridItemInstance* Item,
	TSoftObjectPtr<UTexture2D>& InOutActionIcon)
{
	if (!IsValid(Definition) || !Definition->bUseWeaponVisualCatalogue)
	{
		return;
	}

	const FGridResolvedWeaponVisual Visual = Resolve(*Definition, Item);
	// Preserve ability/spell/Legacy action-specific icons on every failure.
	if (Visual.bResolved && !Visual.VisualId.IsNone() && !Visual.bLegacyFallback &&
		Visual.Error == EGridWeaponVisualResolutionError::None && !Visual.Icon.IsNull())
	{
		InOutActionIcon = Visual.Icon;
	}
}
