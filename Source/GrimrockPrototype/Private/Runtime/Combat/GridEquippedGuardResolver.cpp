#include "Runtime/Combat/GridEquippedGuardResolver.h"

#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/Monsters/GridMonsterTypes.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "GridEquippedGuardItemReadModel.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

namespace
{
	bool ResolveGenuineOffHand(const UGridPartyInventoryComponent* Inventory,
		int32 CharacterIndex, FGridItemInstance& OutItem)
	{
		return IsValid(Inventory) &&
			Inventory->GetEquippedItem(CharacterIndex, EGridEquipmentSlot::OffHand, OutItem) &&
			OutItem.IsValid() && OutItem.EquipmentSlot == EGridEquipmentSlot::OffHand;
	}

	const UGridItemSubsystem* ResolveGuardRegistry(const UGridPartyInventoryComponent* Inventory)
	{
		const UWorld* World = IsValid(Inventory) ? Inventory->GetWorld() : nullptr;
		UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		return GameInstance ? GameInstance->GetSubsystem<UGridItemSubsystem>() : nullptr;
	}

	FGridEquippedGuardItemReadView ResolveGuardItem(const UGridPartyInventoryComponent* Inventory,
		const FGridItemInstance& Item)
	{
		return FGridEquippedGuardItemReadModel::Resolve(
			ResolveGuardRegistry(Inventory),
			Inventory->FindItemDefinition(Item.ItemDefinitionId), Item);
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
	if (!ResolveGenuineOffHand(Inventory, CharacterIndex, OffHand))
	{
		return false;
	}
	// The canonical catalogue owns ARM013 identity and offhand eligibility.
	// The provisional parry chance and attack restrictions are unchanged.
	return ResolveGuardItem(Inventory, OffHand).bCanParry;
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
	if (!ResolveGenuineOffHand(Inventory, CharacterIndex, OffHand) ||
		!ResolveGuardItem(Inventory, OffHand).bIsShield)
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
