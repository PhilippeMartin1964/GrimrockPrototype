#include "Runtime/Combat/GridEquippedGuardResolver.h"

#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/Monsters/GridMonsterTypes.h"

namespace
{
	const UGridItemDefinitionAsset* ResolveGenuineOffHand(const UGridPartyInventoryComponent* Inventory,
		int32 CharacterIndex, FGridItemInstance& OutItem)
	{
		if (!IsValid(Inventory) ||
			!Inventory->GetEquippedItem(CharacterIndex, EGridEquipmentSlot::OffHand, OutItem) ||
			!OutItem.IsValid() || OutItem.EquipmentSlot != EGridEquipmentSlot::OffHand)
		{
			return nullptr;
		}
		return Inventory->FindItemDefinition(OutItem.ItemDefinitionId);
	}
}

bool FGridEquippedGuardResolver::CanParry(const UGridPartyInventoryComponent* Inventory, int32 CharacterIndex,
	const FGridMonsterAttackDefinition& Attack, const FGridAttackResult& Result)
{
	if (!Result.bHit || Result.bCriticalHit || Result.DamageType != EGridDamageType::Physical ||
		Result.DamageAfterModifiers <= 0 || Attack.IsRangedAttack())
	{
		return false;
	}
	FGridItemInstance OffHand;
	const UGridItemDefinitionAsset* Definition = ResolveGenuineOffHand(Inventory, CharacterIndex, OffHand);
	// Prototype policy: only an actual ARM013 Main-gauche in the secondary
	// hand grants the provisional parry aptitude. The catalog, not the text
	// of the item's display name, defines the eligible equipment.
	return IsValid(Definition) && Definition->ItemType == EGridItemType::Weapon &&
		Definition->bUseWeaponCatalogue && Definition->WeaponArchetypeId == FName(TEXT("ARM013"));
}

bool FGridEquippedGuardResolver::IsSuccessfulParryRoll(int32 Roll1To100, int32 ParryChancePercent)
{
	return Roll1To100 >= 1 && Roll1To100 <= 100 &&
		Roll1To100 <= FMath::Clamp(ParryChancePercent, 0, 100);
}

void FGridEquippedGuardResolver::ApplyParry(FGridAttackResult& Result)
{
	if (!Result.bHit)
	{
		return;
	}
	Result.bHit = false;
	Result.bCriticalHit = false;
	Result.bEquipmentParried = true;
	Result.RawDamage = 0;
	Result.DamageAfterModifiers = 0;
	Result.PhysicalArmorDamage = 0;
	Result.MagicalArmorDamage = 0;
	Result.HealthDamage = 0;
	Result.TargetHealthAfter = Result.TargetHealthBefore;
	// Preserve the actual d20 and raw damage ROLL as evidence, but never
	// publish phantom damage/armor consumption in the HUD tooltip trace.
	Result.Trace.Damage.RawDamage = 0;
	Result.Trace.Damage.DamageAfterMultiplier = 0;
	Result.Trace.Damage.DamageAfterResistance = 0;
	Result.Trace.Damage.PhysicalArmorDamage = 0;
	Result.Trace.Damage.MagicalArmorDamage = 0;
	Result.Trace.Damage.HealthDamage = 0;
	Result.Trace.Damage.HealthAfter = Result.TargetHealthBefore;
	Result.Trace.Damage.PhysicalArmorSources.Reset();
}

int32 FGridEquippedGuardResolver::ResolveAbsorbedShieldDamage(const UGridPartyInventoryComponent* Inventory,
	int32 CharacterIndex, const FGridAttackResult& Result)
{
	if (!Result.bHit || Result.DamageType != EGridDamageType::Physical || Result.PhysicalArmorDamage <= 0)
	{
		return 0;
	}
	FGridItemInstance OffHand;
	const UGridItemDefinitionAsset* Definition = ResolveGenuineOffHand(Inventory, CharacterIndex, OffHand);
	if (!IsValid(Definition) || Definition->ItemType != EGridItemType::Shield)
	{
		return 0;
	}
	int32 Absorbed = 0;
	for (const FGridArmorAbsorptionSourceTrace& Source : Result.Trace.Damage.PhysicalArmorSources)
	{
		if (Source.bEquipment && Source.ItemDefinitionId == OffHand.ItemDefinitionId && Source.Absorbed > 0)
		{
			Absorbed += Source.Absorbed;
		}
	}
	// Shield absorption cannot exceed what the attack really consumed.
	return FMath::Min(Result.PhysicalArmorDamage, Absorbed);
}
