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

bool FGridRangedAmmunitionPolicy::HasRequiredAmmunitionForAttack(
	const UGridPartyInventoryComponent& Inventory, int32 CharacterIndex,
	const UGridItemDefinitionAsset& Weapon, bool bRangedAttack)
{
	return !bRangedAttack || HasRequiredAmmunition(Inventory, CharacterIndex, Weapon);
}

bool FGridRangedAmmunitionPolicy::ConsumeRequiredAmmunitionForAttack(
	UGridPartyInventoryComponent& Inventory, int32 CharacterIndex,
	const UGridItemDefinitionAsset& Weapon, bool bRangedAttack)
{
	return !bRangedAttack || ConsumeRequiredAmmunition(Inventory, CharacterIndex, Weapon);
}
