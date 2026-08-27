#include "Runtime/Combat/GridCombatResolver.h"

namespace
{
	int32 ClampResistancePercent(int32 ResistancePercent)
	{
		return FMath::Clamp(ResistancePercent, -100, 100);
	}

	int32 ApplyResistancePercent(int32 Damage, int32 ResistancePercent)
	{
		const int32 SafeDamage = FMath::Max(0, Damage);
		const int32 SafeResistance = ClampResistancePercent(ResistancePercent);
		const float Scale = static_cast<float>(100 - SafeResistance) / 100.0f;
		return FMath::Max(0, FMath::RoundToInt(static_cast<float>(SafeDamage) * Scale));
	}

	void SplitDamageAcrossPools(const FGridAttackTargetStats& Target, FGridAttackResult& Result)
	{
		int32 RemainingDamage = FMath::Max(0, Result.DamageAfterModifiers);

		if (Result.DamageType == EGridDamageType::Physical)
		{
			Result.PhysicalArmorDamage = FMath::Min(FMath::Max(0, Target.PhysicalArmor), RemainingDamage);
			RemainingDamage -= Result.PhysicalArmorDamage;
		}
		else
		{
			Result.MagicalArmorDamage = FMath::Min(FMath::Max(0, Target.MagicalArmor), RemainingDamage);
			RemainingDamage -= Result.MagicalArmorDamage;
		}

		Result.HealthDamage = FMath::Min(FMath::Max(0, Target.CurrentHealth), RemainingDamage);
		Result.TargetHealthBefore = FMath::Max(0, Target.CurrentHealth);
		Result.TargetHealthAfter = FMath::Max(0, Result.TargetHealthBefore - Result.HealthDamage);
	}

	void ResolveDamageModifiers(const FGridAttackTargetStats& Target, FGridAttackResult& Result)
	{
		const int32 DamageAfterMultiplier = FMath::Max(0, FMath::RoundToInt(static_cast<float>(Result.RawDamage) * Result.DamageMultiplier));
		Result.DamageAfterModifiers = ApplyResistancePercent(DamageAfterMultiplier, Result.ResistancePercent);
		SplitDamageAcrossPools(Target, Result);
	}
}

FGridAttackResult FGridCombatResolver::ResolveAttack(
	const FGridAttackSourceStats& Source, const FGridAttackTargetStats& Target, const FGridAttackDefinition& Attack, FRandomStream& RandomStream)
{
	const int32 NaturalAttackRoll = RandomStream.RandRange(1, 20);
	const int32 DamageRoll = Attack.IsValid() ? RandomStream.RandRange(Attack.MinDamage, Attack.MaxDamage) : 0;

	return ResolveAttackFromRolls(Source, Target, Attack, NaturalAttackRoll, DamageRoll);
}

FGridAttackResult FGridCombatResolver::ResolveAttackFromRolls(
	const FGridAttackSourceStats& Source, const FGridAttackTargetStats& Target, const FGridAttackDefinition& Attack, int32 NaturalAttackRoll, int32 DamageRoll)
{
	FGridAttackResult Result;
	Result.DamageType = Attack.DamageType;
	Result.PhysicalSubtype = Attack.PhysicalSubtype;
	Result.NaturalAttackRoll = FMath::Clamp(NaturalAttackRoll, 1, 20);
	Result.AttackRoll = Result.NaturalAttackRoll + Source.Accuracy + Attack.AccuracyBonus;
	Result.DefenseValue = 10 + Target.Evasion;
	Result.ResistancePercent = ClampResistancePercent(Target.ResistancePercent);
	Result.DamageMultiplier = FMath::Max(0.0f, Target.DamageMultiplier);
	Result.TargetHealthBefore = FMath::Max(0, Target.CurrentHealth);
	Result.TargetHealthAfter = Result.TargetHealthBefore;

	if (!Attack.IsValid() || Target.CurrentHealth <= 0)
	{
		return Result;
	}

	const bool bNaturalMiss = Result.NaturalAttackRoll == 1;
	const bool bNaturalHit = Result.NaturalAttackRoll == 20;
	Result.bHit = !bNaturalMiss && (bNaturalHit || Result.AttackRoll >= Result.DefenseValue);
	Result.bCriticalHit = Result.bHit && bNaturalHit;

	if (!Result.bHit)
	{
		return Result;
	}

	const int32 SafeDamageRoll = FMath::Clamp(DamageRoll, Attack.MinDamage, Attack.MaxDamage);
	const int32 BaseDamage = FMath::Max(0, SafeDamageRoll + Source.DamageBonus);
	Result.RawDamage = Result.bCriticalHit ? BaseDamage * 2 : BaseDamage;
	ResolveDamageModifiers(Target, Result);
	return Result;
}

FGridAttackResult FGridCombatResolver::ResolveDirectDamage(
	const FGridAttackTargetStats& Target, EGridDamageType DamageType, int32 RawDamage, EGridPhysicalDamageSubtype PhysicalSubtype)
{
	FGridAttackResult Result;
	Result.DamageType = DamageType;
	Result.PhysicalSubtype = DamageType == EGridDamageType::Physical ? PhysicalSubtype : EGridPhysicalDamageSubtype::None;
	Result.ResistancePercent = ClampResistancePercent(Target.ResistancePercent);
	Result.DamageMultiplier = FMath::Max(0.0f, Target.DamageMultiplier);
	Result.TargetHealthBefore = FMath::Max(0, Target.CurrentHealth);
	Result.TargetHealthAfter = Result.TargetHealthBefore;
	Result.RawDamage = FMath::Max(0, RawDamage);

	if (Target.CurrentHealth <= 0 || Result.RawDamage <= 0)
	{
		return Result;
	}

	Result.bHit = true;
	ResolveDamageModifiers(Target, Result);
	return Result;
}

int32 FGridCombatResolver::GetResistancePercent(const FGridDamageResistanceSet& Resistances, EGridDamageType DamageType)
{
	switch (DamageType)
	{
		case EGridDamageType::Physical:
			return Resistances.PhysicalResistance;
		case EGridDamageType::Fire:
			return Resistances.FireResistance;
		case EGridDamageType::Ice:
			return Resistances.IceResistance;
		case EGridDamageType::Lightning:
			return Resistances.LightningResistance;
		case EGridDamageType::Poison:
			return Resistances.PoisonResistance;
		case EGridDamageType::Holy:
			return Resistances.HolyResistance;
		case EGridDamageType::Necrotic:
			return Resistances.NecroticResistance;
		case EGridDamageType::Arcane:
			return Resistances.ArcaneResistance;
		default:
			return 0;
	}
}

int32 FGridPartyTargetSelector::SelectTarget(const FGridPartyInventoryState& PartyState, FRandomStream& RandomStream, int32 FrontLineSlotCount)
{
	const int32 SafeFrontLineCount = FMath::Max(0, FrontLineSlotCount);
	TArray<int32> FrontLineCandidates;
	TArray<int32> SecondLineCandidates;

	for (int32 CharacterIndex = 0; CharacterIndex < PartyState.ActiveCharacters.Num(); ++CharacterIndex)
	{
		const FGridCharacterInventoryState& Character = PartyState.ActiveCharacters[CharacterIndex];
		if (Character.Resources.CurrentHealth <= 0)
		{
			continue;
		}

		if (CharacterIndex < SafeFrontLineCount)
		{
			FrontLineCandidates.Add(CharacterIndex);
		}
		else
		{
			SecondLineCandidates.Add(CharacterIndex);
		}
	}

	const TArray<int32>& Candidates = FrontLineCandidates.IsEmpty() ? SecondLineCandidates : FrontLineCandidates;
	if (Candidates.IsEmpty())
	{
		return INDEX_NONE;
	}

	return Candidates[RandomStream.RandRange(0, Candidates.Num() - 1)];
}
