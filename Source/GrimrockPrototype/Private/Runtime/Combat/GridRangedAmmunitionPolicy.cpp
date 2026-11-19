#include "Runtime/Combat/GridRangedAmmunitionPolicy.h"

#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/Items/GridItemSubsystem.h"

bool FGridRangedAmmunitionPolicy::TryResolveCanonicalLaunchSpeed(
	const FGridResolvedItem* Weapon, const FGridResolvedItem& Ammo, float& OutSpeed)
{
	OutSpeed = 0.0f;
	if (!Ammo.IsValid() || Ammo.bIsArmament || Ammo.ItemLevel != 0 ||
		Ammo.ItemId.IsNone() || Ammo.AmmoKindId.IsNone() || Ammo.MaxStack <= 1 ||
		Ammo.WorldMesh.IsNull() || !FMath::IsFinite(Ammo.WeightKg) || Ammo.WeightKg < 0.0f ||
		(Weapon && (!Weapon->IsValid() || !Weapon->bIsArmament ||
			Weapon->Weapon.RequiredAmmoKindId != Ammo.AmmoKindId)))
	{
		return false;
	}
	const bool bSpecialized = Weapon && Weapon->Weapon.Projectile.bEnabled;
	const float AuthoredSpeed = bSpecialized ? Weapon->Weapon.Projectile.InitialSpeed :
		Ammo.GenericThrow.InitialSpeed;
	if (!FMath::IsFinite(AuthoredSpeed) || AuthoredSpeed <= KINDA_SMALL_NUMBER)
	{
		return false;
	}
	// Preserve the existing ordinary arrow clamp, not the hand-toss speed
	// scale or an arbitrary substitute for invalid authored data.
	OutSpeed = bSpecialized ? AuthoredSpeed : FMath::Clamp(AuthoredSpeed, 400.0f, 4800.0f);
	return true;
}

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
		Result.bCanonicalAmmunition = true;
		Result.AmmunitionItemId = Ammo.ItemId;
		Result.AmmunitionWeightKg = Ammo.WeightKg;
		Result.AmmunitionMaxStack = Ammo.MaxStack;
		Result.bValid = TryResolveCanonicalLaunchSpeed(&Weapon, Ammo, Result.AmmunitionLaunchSpeed);
		return Result;
	}

	// Unregistered items remain on their original path until binary migration.
	Result.bValid = true;
	if (IsValid(LegacyWeapon))
	{
		Result.AmmunitionItemId = LegacyWeapon->RequiredAmmunitionItemDefinitionId;
	}
	// An unregistered launcher can still require a registered arrow. Its
	// identity join is historical, but the arrow's own data never is.
	if (IsValid(Registry) && Registry->HasCanonicalItem(Result.AmmunitionItemId))
	{
		Result.bCanonicalAmmunition = true;
		FGridResolvedItem Ammo;
		Result.bValid = Registry->ResolveItem(Result.AmmunitionItemId, 0, Ammo) &&
			TryResolveCanonicalLaunchSpeed(nullptr, Ammo, Result.AmmunitionLaunchSpeed);
		if (Result.bValid)
		{
			Result.AmmunitionWeightKg = Ammo.WeightKg;
			Result.AmmunitionMaxStack = Ammo.MaxStack;
		}
	}
	return Result;
}

bool FGridRangedAmmunitionPolicy::HasRequiredAmmunitionForAttack(
	const UGridPartyInventoryComponent& Inventory, int32 CharacterIndex,
	const FGridRangedAmmunitionRequirement& Requirement)
{
	if (!Requirement.bValid) return false;
	if (Requirement.AmmunitionItemId.IsNone()) return true;
	if (Requirement.bCanonicalAmmunition)
	{
		if (!Inventory.IsValidCharacterIndex(CharacterIndex)) return false;
		// The ID-based debit below must never be able to select a malformed
		// physical stack before a valid one, even when the same ID is present.
		for (const FGridInventorySlot& Slot : Inventory.PartyInventoryState.ActiveCharacters[CharacterIndex].InventorySlots)
		{
			if (Slot.bOccupied && Slot.Item.ItemDefinitionId == Requirement.AmmunitionItemId &&
				(Slot.IsEmpty() || !Slot.Item.IsValid() || Slot.Item.ItemLevel != 0 ||
				 Slot.Item.Quantity < 1 || Slot.Item.Quantity > Requirement.AmmunitionMaxStack))
				return false;
		}
	}
	return Inventory.CountItemDefinitionInCharacterInventory(CharacterIndex, Requirement.AmmunitionItemId) >= 1;
}

bool FGridRangedAmmunitionPolicy::ConsumeRequiredAmmunitionForAttack(
	UGridPartyInventoryComponent& Inventory, int32 CharacterIndex,
	const FGridRangedAmmunitionRequirement& Requirement)
{
	return HasRequiredAmmunitionForAttack(Inventory, CharacterIndex, Requirement) &&
		(Requirement.AmmunitionItemId.IsNone() ||
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
