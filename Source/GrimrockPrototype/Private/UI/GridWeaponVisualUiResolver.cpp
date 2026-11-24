#include "GridWeaponVisualUiResolver.h"

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
	FGridResolvedItem Resolved;
	if (Registry && Registry->ResolvePhysicalItem(Item, Resolved))
	{
		Result.bRegisteredIdentity = true;
		Result.VisualId = Resolved.VisualId;
		Result.Icon = Resolved.ItemIcon;
	}
	else
	{
		Result.bRegisteredIdentity = Registry && Registry->ContainsItem(Item.ItemDefinitionId);
	}
	return Result;
}
