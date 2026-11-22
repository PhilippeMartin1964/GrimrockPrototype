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
	if (Registry && Registry->ContainsItem(Item.ItemDefinitionId))
	{
		Result.bRegisteredIdentity = true;
		FGridResolvedItem Canonical;
		if (Registry->ResolveItem(Item.ItemDefinitionId, Item.ItemLevel, Canonical))
		{
			Result.VisualId = Canonical.VisualId;
			Result.Icon = Canonical.ItemIcon;
		}
		return Result; // Never use a copied legacy icon for an invalid canonical item.
	}
	// H.68: a configured game never borrows an icon from an unknown
	// historical item. Worldless Automation alone may characterize that old UI.
#if WITH_DEV_AUTOMATION_TESTS
	if (!Registry && GIsAutomationTesting && IsValid(LegacyDefinition) &&
		LegacyDefinition->ItemDefinitionId == Item.ItemDefinitionId)
	{
		const FGridResolvedWeaponVisual Legacy = Resolve(*LegacyDefinition, &Item);
		Result.VisualId = Legacy.VisualId;
		Result.Icon = Legacy.Icon;
	}
#else
	(void)LegacyDefinition;
#endif
	return Result;
}

#if WITH_DEV_AUTOMATION_TESTS
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

#endif // WITH_DEV_AUTOMATION_TESTS: historical UI fixtures only
