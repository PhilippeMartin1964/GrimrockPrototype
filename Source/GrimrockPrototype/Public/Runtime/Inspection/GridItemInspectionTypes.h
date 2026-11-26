#pragma once

#include "Runtime/Items/GridItemSubsystem.h"

/** Transient read-only projection. No owner, physical GUID or serialized preview state.
 * Definition fields remain in the canonical resolved item, not duplicated as UI values.
 */
struct GRIMROCKPROTOTYPE_API FGridItemInspectionView
{
	FGridResolvedItem Item;
	bool bPhysicalInstance = false;
	bool bInvalidPhysicalInstance = false;
	int32 Quantity = 0;
	bool bItemLightsEnabled = false;
	bool bHasPhysicalReserve = false;
	FGridArmorReserveView PhysicalReserve;

	bool IsValid() const { return !bInvalidPhysicalInstance && Item.IsValid(); }
	bool HasLevel() const { return IsValid() && Item.bIsArmament; }
	bool HasMeshReference() const { return IsValid() && !Item.WorldMesh.IsNull(); }
	bool HasEquipment() const { return IsValid() && (Item.bIsArmament || !Item.NonArmEquipmentSlots.IsEmpty()); }
	bool HasAttackStats() const { return HasLevel() && !Item.Weapon.AttackSourceSlots.IsEmpty(); }
	/** Authored bindings, not permissions to execute them. */
	bool HasActionBindings() const { return IsValid() && !Item.AvailableActions.IsEmpty(); }
	bool HasProjectileAppearance() const { return IsValid() && Item.ProjectileAppearance.bEnabled && !Item.ProjectileMesh.IsNull(); }
	bool bCanRead = false;

	int32 GetNominalPhysicalArmor() const
	{
		return !IsValid() ? 0 : Item.bIsArmament ? Item.Weapon.PhysicalArmor : Item.NonArmEquipmentBonuses.ArmorBonus;
	}
	int32 GetNominalMagicalArmor() const
	{
		return !IsValid() ? 0 : Item.bIsArmament ? Item.Weapon.MagicalArmor : Item.NonArmEquipmentBonuses.MagicalArmorBonus;
	}
	bool HasProtection() const
	{
		return HasEquipment() && ((Item.bIsArmament && !HasAttackStats()) || GetNominalPhysicalArmor() != 0 || GetNominalMagicalArmor() != 0);
	}
};

/** A resolved definition may legitimately have no mesh. Do not show a previous item. */
enum class EGridItemPreviewResult : uint8
{
	Ready,
	MissingMesh,
	InvalidSelection,
	InvalidHost
};
