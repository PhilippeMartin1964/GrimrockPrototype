#include "Runtime/Combat/GridWeaponProgressionTypes.h"
#include "Runtime/Combat/GridWeaponProgressionMath.h"

bool FGridWeaponLevelProfile::IsValidProfile() const
{
	if (ArchetypeId.IsNone() || ItemLevel < 1 || ItemLevel > 20 ||
		DisplayName.IsEmpty() || Family.IsEmpty() || HandUsage.IsEmpty() ||
		PhysicalArmor < 0 || MagicalArmor < 0 || ActionPointCost < 0 || ActionPointCost > 32 ||
		RangeCells < 0 || RangeCells > 32 || !FMath::IsFinite(WeightKg) || WeightKg < 0.0f)
	{
		return false;
	}

	if (DamageDice.bEnabled)
	{
		if (!DamageDice.IsValid() || ActionPointCost < 1 || RangeCells < 1)
		{
			return false;
		}
		FGridAttackDefinition Attack;
		return TryBuildAttackDefinition(Attack);
	}

	// Passive shields and passive magical focuses never roll an artificial 0d0.
	return DamageDice.DiceCount == 0 && DamageDice.DiceSides == 0 &&
		DamageDice.FlatBonus == 0 && ActionPointCost == 0 && RangeCells == 0;
}

bool FGridWeaponLevelProfile::TryBuildAttackDefinition(FGridAttackDefinition& OutAttack) const
{
	OutAttack = FGridAttackDefinition();
	if (!DamageDice.bEnabled || !DamageDice.IsValid() || ActionPointCost < 1 ||
		RangeCells < 1 || RangeCells > 32 ||
		(DamageType != EGridDamageType::Physical && PhysicalSubtype != EGridPhysicalDamageSubtype::None))
	{
		return false;
	}
	OutAttack.DamageType = DamageType;
	OutAttack.PhysicalSubtype = PhysicalSubtype;
	OutAttack.AccuracyBonus = AccuracyBonus;
	OutAttack.DamageDice = DamageDice;
	OutAttack.MinDamage = DamageDice.GetMinimumDamage();
	OutAttack.MaxDamage = DamageDice.GetMaximumDamage();
	return OutAttack.IsValid();
}

bool FGridWeaponIntegerGrowthRule::IsValid() const
{
	if (StepEveryLevels < 1 || StepEveryLevels > 20) return false;
	TSet<int32> SeenLevels;
	for (const FGridWeaponGrowthException& Exception : Exceptions)
	{
		if (Exception.ItemLevel < 2 || Exception.ItemLevel > 20 ||
			SeenLevels.Contains(Exception.ItemLevel))
		{
			return false;
		}
		SeenLevels.Add(Exception.ItemLevel);
	}
	return true;
}

int32 FGridWeaponIntegerGrowthRule::Resolve(int32 ItemLevel, int32 N1Value) const
{
	int32 Value = GridWeaponProgressionMath::CalculateGrowthValue(N1Value, ItemLevel, FMath::Max(1, StepEveryLevels), StepAmount);
	for (const FGridWeaponGrowthException& Exception : Exceptions)
	{
		if (Exception.ItemLevel == ItemLevel)
		{
			Value = Exception.Value;
			break;
		}
	}
	return Value;
}
