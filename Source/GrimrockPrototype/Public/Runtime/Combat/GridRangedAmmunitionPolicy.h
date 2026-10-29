#pragma once

#include "CoreMinimal.h"

class UGridItemDefinitionAsset;
class UGridPartyInventoryComponent;

/**
 * LOT02.4: one data-authored ammo unit per accepted ranged attack.
 * No global ammo pool; each hero's bag is authoritative.
 * NAME_None is deliberately backward compatible with existing weapons.
 */
class GRIMROCKPROTOTYPE_API FGridRangedAmmunitionPolicy
{
public:
	static bool HasRequiredAmmunition(
		const UGridPartyInventoryComponent& Inventory, int32 CharacterIndex,
		const UGridItemDefinitionAsset& Weapon);

	static bool ConsumeRequiredAmmunition(
		UGridPartyInventoryComponent& Inventory, int32 CharacterIndex,
		const UGridItemDefinitionAsset& Weapon);

	/** Only an actual ranged attack requires ammunition. Non-ranged actions
	 * never check or consume arrows, even when sourced from an equipped bow.
	 */
	static bool HasRequiredAmmunitionForAttack(
		const UGridPartyInventoryComponent& Inventory, int32 CharacterIndex,
		const UGridItemDefinitionAsset& Weapon, bool bRangedAttack);

	static bool ConsumeRequiredAmmunitionForAttack(
		UGridPartyInventoryComponent& Inventory, int32 CharacterIndex,
		const UGridItemDefinitionAsset& Weapon, bool bRangedAttack);
};
