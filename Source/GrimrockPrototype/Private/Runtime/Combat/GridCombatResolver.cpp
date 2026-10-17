#include "Runtime/Combat/GridCombatResolver.h"
#include "Runtime/Combat/GridCombatTargetingResolver.h"

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
		Result.Trace.Damage.PhysicalArmorBefore = FMath::Max(0, Target.PhysicalArmor);
		Result.Trace.Damage.MagicalArmorBefore = FMath::Max(0, Target.MagicalArmor);

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

		Result.Trace.Damage.PhysicalArmorDamage = Result.PhysicalArmorDamage;
		Result.Trace.Damage.MagicalArmorDamage = Result.MagicalArmorDamage;
		Result.Trace.Damage.HealthBefore = Result.TargetHealthBefore;
		Result.Trace.Damage.HealthDamage = Result.HealthDamage;
		Result.Trace.Damage.HealthAfter = Result.TargetHealthAfter;
	}

	void ResolveDamageModifiers(const FGridAttackTargetStats& Target, FGridAttackResult& Result)
	{
		const int32 DamageAfterMultiplier = FMath::Max(
			0, FMath::RoundToInt(static_cast<float>(Result.RawDamage) * Result.DamageMultiplier));
		Result.Trace.Damage.DamageAfterMultiplier = DamageAfterMultiplier;
		Result.DamageAfterModifiers = ApplyResistancePercent(DamageAfterMultiplier, Result.ResistancePercent);
		Result.Trace.Damage.DamageAfterResistance = Result.DamageAfterModifiers;
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
	Result.DamageMultiplier = FMath::Max(0.0f, Source.DamageMultiplier) * FMath::Max(0.0f, Target.DamageMultiplier);
	Result.TargetHealthBefore = FMath::Max(0, Target.CurrentHealth);
	Result.TargetHealthAfter = Result.TargetHealthBefore;

	Result.Trace.bValid = Attack.IsValid() && Target.CurrentHealth > 0;
	Result.Trace.Attack.Roll.bUsed = true;
	Result.Trace.Attack.Roll.Kind = EGridCombatRollTraceKind::Die;
	Result.Trace.Attack.Roll.DiceCount = 1;
	Result.Trace.Attack.Roll.DiceSides = 20;
	Result.Trace.Attack.Roll.Minimum = 1;
	Result.Trace.Attack.Roll.Maximum = 20;
	Result.Trace.Attack.Roll.IndividualRolls = { Result.NaturalAttackRoll };
	Result.Trace.Attack.Roll.FinalValue = Result.NaturalAttackRoll;
	Result.Trace.Attack.SourceAccuracy = Source.Accuracy;
	Result.Trace.Attack.AttackAccuracyBonus = Attack.AccuracyBonus;
	Result.Trace.Attack.FinalValue = Result.AttackRoll;
	Result.Trace.Attack.bAlwaysHits = Attack.bAlwaysHits;
	Result.Trace.Defense.BaseValue = 10;
	Result.Trace.Defense.EvasionModifier = Target.Evasion;
	Result.Trace.Defense.FinalValue = Result.DefenseValue;
	Result.Trace.Damage.SourceDamageBonus = Source.DamageBonus;
	Result.Trace.Damage.RawDamagePercent = FMath::Clamp(Source.RawDamagePercent, 0, 1000);
	Result.Trace.Damage.SourceDamageMultiplier = FMath::Max(0.0f, Source.DamageMultiplier);
	Result.Trace.Damage.TargetDamageMultiplier = FMath::Max(0.0f, Target.DamageMultiplier);
	Result.Trace.Damage.CombinedDamageMultiplier = Result.DamageMultiplier;
	Result.Trace.Damage.ResistancePercent = Result.ResistancePercent;
	Result.Trace.Damage.Roll.Kind = EGridCombatRollTraceKind::Range;
	Result.Trace.Damage.Roll.Minimum = Attack.MinDamage;
	Result.Trace.Damage.Roll.Maximum = Attack.MaxDamage;

	if (!Attack.IsValid() || Target.CurrentHealth <= 0)
	{
		return Result;
	}

	const bool bNaturalMiss = Result.NaturalAttackRoll == 1;
	const bool bNaturalHit = Result.NaturalAttackRoll == 20;
	Result.Trace.Attack.bNaturalAutoMiss = bNaturalMiss;
	Result.Trace.Attack.bNaturalAutoHit = bNaturalHit;
	Result.bHit = Attack.bAlwaysHits || (!bNaturalMiss && (bNaturalHit || Result.AttackRoll >= Result.DefenseValue));

	const int32 SafeCriticalChance = FMath::Clamp(Source.CriticalChancePercent, 0, 100);
	const int32 CriticalRollCount = FMath::Clamp((SafeCriticalChance + 4) / 5, 0, 20);
	const int32 CriticalMinimumRoll = CriticalRollCount > 0 ? 21 - CriticalRollCount : 21;
	Result.Trace.Attack.bCanCriticalHit = Attack.bCanCriticalHit;
	Result.Trace.Attack.CriticalChancePercent = SafeCriticalChance;
	Result.Trace.Attack.CriticalMinimumRoll = CriticalMinimumRoll;
	Result.bCriticalHit = Attack.bCanCriticalHit && Result.bHit && CriticalRollCount > 0 && Result.NaturalAttackRoll >= CriticalMinimumRoll;

	if (!Result.bHit)
	{
		return Result;
	}

	const int32 SafeDamageRoll = FMath::Clamp(DamageRoll, Attack.MinDamage, Attack.MaxDamage);
	const int32 BaseDamage = FMath::Max(0, SafeDamageRoll + Source.DamageBonus);
	const int32 SafeRawDamagePercent = FMath::Clamp(Source.RawDamagePercent, 0, 1000);
	Result.Trace.Damage.bResolved = true;
	Result.Trace.Damage.Roll.bUsed = true;
	Result.Trace.Damage.Roll.IndividualRolls = { SafeDamageRoll };
	Result.Trace.Damage.Roll.FinalValue = SafeDamageRoll;
	Result.Trace.Damage.BaseDamage = BaseDamage;
	const int64 ScaledBaseDamage64 = static_cast<int64>(BaseDamage) * static_cast<int64>(SafeRawDamagePercent) / 100;
	const int32 ScaledBaseDamage = BaseDamage > 0 && SafeRawDamagePercent > 0
		? FMath::Max(1, static_cast<int32>(FMath::Clamp<int64>(ScaledBaseDamage64, 0, MAX_int32)))
		: 0;
	Result.Trace.Damage.ScaledBaseDamage = ScaledBaseDamage;
	Result.Trace.Damage.CriticalDamagePercent = Result.bCriticalHit
		? FMath::Clamp(Source.CriticalDamagePercent, 100, 1000)
		: 100;
	if (Result.bCriticalHit)
	{
		const int32 SafeCriticalDamagePercent = FMath::Clamp(Source.CriticalDamagePercent, 100, 1000);
		const int64 CriticalDamage = static_cast<int64>(ScaledBaseDamage) * static_cast<int64>(SafeCriticalDamagePercent) / 100;
		Result.RawDamage = static_cast<int32>(
			FMath::Clamp<int64>(CriticalDamage, static_cast<int64>(0), static_cast<int64>(MAX_int32)));
	}
	else
	{
		Result.RawDamage = ScaledBaseDamage;
	}
	Result.Trace.Damage.RawDamage = Result.RawDamage;
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
	Result.Trace.bValid = Target.CurrentHealth > 0 && Result.RawDamage > 0;
	Result.Trace.Damage.bDirectDamage = true;
	Result.Trace.Damage.bResolved = Result.Trace.bValid;
	Result.Trace.Damage.RawDamagePercent = 100;
	Result.Trace.Damage.ScaledBaseDamage = Result.RawDamage;
	Result.Trace.Damage.CriticalDamagePercent = 100;
	Result.Trace.Damage.RawDamage = Result.RawDamage;
	Result.Trace.Damage.BaseDamage = Result.RawDamage;
	Result.Trace.Damage.SourceDamageMultiplier = 1.0f;
	Result.Trace.Damage.TargetDamageMultiplier = FMath::Max(0.0f, Target.DamageMultiplier);
	Result.Trace.Damage.CombinedDamageMultiplier = Result.DamageMultiplier;
	Result.Trace.Damage.ResistancePercent = Result.ResistancePercent;

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
		if (Character.Resources.CurrentHealth <= 0 ||
			!FGridCombatTargetingResolver::IsDirectHostileTargetable(Character.StatusEffects))
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
