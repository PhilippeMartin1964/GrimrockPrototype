#include "UI/GridWeaponVisualUiResolver.h"

#include "Runtime/Items/GridItemSubsystem.h"

FGridResolvedInventoryItemIcon FGridWeaponVisualUiResolver::ResolveInventoryItemIcon(
	const UGridItemSubsystem* Registry,
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
	return Result;
}
