#include "Runtime/Combat/GridRangedAmmunitionPolicy.h"

#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"

bool FGridRangedAmmunitionPolicy::HasRequiredAmmunition(
	const UGridPartyInventoryComponent& Inventory, int32 CharacterIndex,
	const UGridItemDefinitionAsset& Weapon)
{
	const FName AmmunitionId = Weapon.RequiredAmmunitionItemDefinitionId;
	return AmmunitionId.IsNone() ||
		Inventory.CountItemDefinitionInCharacterInventory(CharacterIndex, AmmunitionId) >= 1;
}

bool FGridRangedAmmunitionPolicy::ConsumeRequiredAmmunition(
	UGridPartyInventoryComponent& Inventory, int32 CharacterIndex,
	const UGridItemDefinitionAsset& Weapon)
{
	const FName AmmunitionId = Weapon.RequiredAmmunitionItemDefinitionId;
	return AmmunitionId.IsNone() ||
		Inventory.RemoveItemDefinitionFromCharacterInventory(CharacterIndex, AmmunitionId, 1);
}
