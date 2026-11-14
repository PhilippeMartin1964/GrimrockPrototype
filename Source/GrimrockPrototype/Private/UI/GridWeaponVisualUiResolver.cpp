#include "UI/GridWeaponVisualUiResolver.h"

#include "Runtime/Combat/GridWeaponVisualResolver.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"

FGridResolvedInventoryItemIcon FGridWeaponVisualUiResolver::ResolveInventoryItemIcon(
	const UGridItemSubsystem* Registry, const UGridItemDefinitionAsset* LegacyDefinition,
	const FGridItemInstance& Item)
{
	FGridResolvedInventoryItemIcon Result;
	if (!Item.IsValid())
	{
		return Result;
	}
	if (Registry && Registry->HasCanonicalItem(Item.ItemDefinitionId))
	{
		Result.bCanonicalIdentity = true;
		FGridResolvedItem Canonical;
		if (Registry->ResolveItem(Item.ItemDefinitionId, Item.ItemLevel, Canonical))
		{
			Result.VisualId = Canonical.VisualId;
			Result.Icon = Canonical.ItemIcon;
		}
		return Result; // Never use a copied legacy icon for an invalid canonical item.
	}
	if (IsValid(LegacyDefinition) && LegacyDefinition->ItemDefinitionId == Item.ItemDefinitionId)
	{
		const FGridResolvedWeaponVisual Legacy = Resolve(*LegacyDefinition, &Item);
		Result.VisualId = Legacy.VisualId;
		Result.Icon = Legacy.Icon;
	}
	return Result;
}

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
