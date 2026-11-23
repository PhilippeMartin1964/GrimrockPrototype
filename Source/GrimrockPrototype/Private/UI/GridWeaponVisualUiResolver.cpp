#include "UI/GridWeaponVisualUiResolver.h"

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
		LegacyDefinition->ItemDefinitionId == Item.ItemDefinitionId && !LegacyDefinition->bUseWeaponVisualCatalogue)
	{
		Result.Icon = LegacyDefinition->Icon;
	}
#else
	(void)LegacyDefinition;
#endif
	return Result;
}
