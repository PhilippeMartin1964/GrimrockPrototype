#pragma once

#include "CoreMinimal.h"

class UGridItemDefinitionAsset;
class UGridPartyInventoryComponent;

/**
 * LOT02.4: one data-authored ammo unit per accepted ranged attack.
 * No global ammo pool; each hero's bag is authoritative.
 * NAME_None is deliberately backward compatible with existing weapons.
 */
class UGridItemSubsystem;
struct FGridResolvedItem;

/** Immutable decision shared by action availability and final execution.
 * A registered weapon NEVER consults the historical RequiredAmmunitionItemDefinitionId.
 */
struct FGridRangedAmmunitionRequirement
{
	bool bCanonicalSource = false;
	bool bValid = false;
	FName AmmunitionItemId = NAME_None;
	float AmmunitionWeightKg = 0.0f;
	bool bCanonicalAmmunition = false;
	int32 AmmunitionMaxStack = 0;
	float AmmunitionLaunchSpeed = 0.0f;
};

class GRIMROCKPROTOTYPE_API FGridRangedAmmunitionPolicy
{
public:
	/** Canonical flight policy only. An enabled specialized launcher wins;
	 * otherwise use the unlevelled ammo's migrated generic speed, with the
	 * existing ranged-ammo clamp. Invalid data never falls back. */
	static bool TryResolveCanonicalLaunchSpeed(const FGridResolvedItem* Weapon,
		const FGridResolvedItem& Ammo, float& OutSpeed);

	static FGridRangedAmmunitionRequirement ResolveForAttack(
		const UGridItemSubsystem* Registry, FName WeaponItemId, int32 WeaponLevel,
		const UGridItemDefinitionAsset* LegacyWeapon, bool bRangedAttack);

	static bool HasRequiredAmmunitionForAttack(
		const UGridPartyInventoryComponent& Inventory, int32 CharacterIndex,
		const FGridRangedAmmunitionRequirement& Requirement);

	static bool ConsumeRequiredAmmunitionForAttack(
		UGridPartyInventoryComponent& Inventory, int32 CharacterIndex,
		const FGridRangedAmmunitionRequirement& Requirement);

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
