#include "Runtime/Combat/GridWeaponCatalogueAsset.h"

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

bool UGridWeaponCatalogueAsset::IsValidCatalogue() const
{
	if (Profiles.Num() != 82 * 20)
	{
		return false;
	}
	TSet<FString> UniqueKeys;
	TMap<FName, int32> ArchetypeCounts;
	TMap<FName, FGridWeaponLevelProfile> Baselines;
	int32 PassiveCount = 0;

	for (const FGridWeaponLevelProfile& Profile : Profiles)
	{
		if (!Profile.IsValidProfile())
		{
			return false;
		}
		// Canonical ARM070..ARM082 are the only passive archetypes in the workbook.
		const FString ProfileId = Profile.ArchetypeId.ToString();
		const int32 Ordinal = ProfileId.StartsWith(TEXT("ARM"))
			? FCString::Atoi(*ProfileId.RightChop(3)) : 0;
		if (Ordinal < 1 || Ordinal > 82 ||
			(Profile.DamageDice.bEnabled != (Ordinal < 70)))
		{
			return false;
		}
		if (!Profile.DamageDice.bEnabled)
		{
			++PassiveCount;
		}
		const FString Key = FString::Printf(TEXT("%s:%02d"),
			*Profile.ArchetypeId.ToString(), Profile.ItemLevel);
		if (UniqueKeys.Contains(Key))
		{
			return false;
		}
		UniqueKeys.Add(Key);
		ArchetypeCounts.FindOrAdd(Profile.ArchetypeId)++;
		if (const FGridWeaponLevelProfile* First = Baselines.Find(Profile.ArchetypeId))
		{
			if (!First->DisplayName.EqualTo(Profile.DisplayName) ||
				First->Family != Profile.Family || First->HandUsage != Profile.HandUsage ||
				First->DamageType != Profile.DamageType ||
				First->PhysicalSubtype != Profile.PhysicalSubtype ||
				First->ScalingAttribute != Profile.ScalingAttribute ||
				First->AccuracyBonus != Profile.AccuracyBonus ||
				First->ActionPointCost != Profile.ActionPointCost ||
				First->RangeCells != Profile.RangeCells ||
				!FMath::IsNearlyEqual(First->WeightKg, Profile.WeightKg, 0.0001f) ||
				First->DamageDice.bEnabled != Profile.DamageDice.bEnabled)
			{
				return false;
			}
		}
		else
		{
			Baselines.Add(Profile.ArchetypeId, Profile);
		}
	}
	if (ArchetypeCounts.Num() != 82 || PassiveCount != 260)
	{
		return false;
	}
	for (int32 Index = 1; Index <= 82; ++Index)
	{
		const FName Expected(*FString::Printf(TEXT("ARM%03d"), Index));
		const int32* Count = ArchetypeCounts.Find(Expected);
		if (!Count || *Count != 20)
		{
			return false;
		}
	}
	return true;
}

const FGridWeaponLevelProfile* UGridWeaponCatalogueAsset::FindProfile(FName ArchetypeId, int32 ItemLevel) const
{
	if (ArchetypeId.IsNone() || ItemLevel < 1 || ItemLevel > 20)
	{
		return nullptr;
	}
	return Profiles.FindByPredicate([ArchetypeId, ItemLevel](const FGridWeaponLevelProfile& Profile)
	{
		return Profile.ArchetypeId == ArchetypeId && Profile.ItemLevel == ItemLevel;
	});
}

bool UGridWeaponCatalogueAsset::TryBuildAttack(FName ArchetypeId, int32 ItemLevel, FGridAttackDefinition& OutAttack) const
{
	OutAttack = FGridAttackDefinition();
	const FGridWeaponLevelProfile* Profile = FindProfile(ArchetypeId, ItemLevel);
	return Profile && Profile->IsValidProfile() && Profile->TryBuildAttackDefinition(OutAttack);
}
