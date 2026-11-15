#include "Runtime/Combat/GridRangedAmmunitionPolicy.h"

#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/Items/GridItemSubsystem.h"

FGridRangedAmmunitionRequirement FGridRangedAmmunitionPolicy::ResolveForAttack(
	const UGridItemSubsystem* Registry, FName WeaponItemId, int32 WeaponLevel,
	const UGridItemDefinitionAsset* LegacyWeapon, bool bRangedAttack)
{
	FGridRangedAmmunitionRequirement Result;
	if (!bRangedAttack)
	{
		Result.bValid = true; // Melee actions do not require or consume arrows.
		return Result;
	}
	if (IsValid(Registry) && Registry->HasCanonicalItem(WeaponItemId))
	{
		Result.bCanonicalSource = true;
		FGridResolvedItem Weapon;
		if (!Registry->ResolveItem(WeaponItemId, WeaponLevel, Weapon) ||
			!Weapon.bIsArmament)
		{
			return Result; // Known invalid level/identity: fail closed.
		}
		if (Weapon.Weapon.RequiredAmmoKindId.IsNone())
		{
			Result.bValid = true; // An actual no-ammo weapon, not a legacy fallback.
			return Result;
		}
		FGridResolvedItem Ammo;
		if (!Registry->ResolveAmmoKind(Weapon.Weapon.RequiredAmmoKindId, Ammo))
		{
			return Result;
		}
		Result.AmmunitionItemId = Ammo.ItemId;
		Result.AmmunitionWeightKg = Ammo.WeightKg;
		Result.bValid = true;
		return Result;
	}

	// Unregistered items remain on their original path until binary migration.
	Result.bValid = true;
	if (IsValid(LegacyWeapon))
	{
		Result.AmmunitionItemId = LegacyWeapon->RequiredAmmunitionItemDefinitionId;
	}
	return Result;
}

bool FGridRangedAmmunitionPolicy::HasRequiredAmmunitionForAttack(
	const UGridPartyInventoryComponent& Inventory, int32 CharacterIndex,
	const FGridRangedAmmunitionRequirement& Requirement)
{
	return Requirement.bValid && (Requirement.AmmunitionItemId.IsNone() ||
		Inventory.CountItemDefinitionInCharacterInventory(CharacterIndex, Requirement.AmmunitionItemId) >= 1);
}

bool FGridRangedAmmunitionPolicy::ConsumeRequiredAmmunitionForAttack(
	UGridPartyInventoryComponent& Inventory, int32 CharacterIndex,
	const FGridRangedAmmunitionRequirement& Requirement)
{
	return Requirement.bValid && (Requirement.AmmunitionItemId.IsNone() ||
		Inventory.RemoveItemDefinitionFromCharacterInventory(CharacterIndex, Requirement.AmmunitionItemId, 1));
}

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
