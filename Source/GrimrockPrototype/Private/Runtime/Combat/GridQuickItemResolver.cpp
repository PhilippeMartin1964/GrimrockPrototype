#include "Runtime/Combat/GridQuickItemResolver.h"

namespace
{
	int32 ScalePositiveByModifier(int32 Value, int32 PercentModifier)
	{
		if (Value <= 0)
		{
			return 0;
		}
		const int32 SafePercent = FMath::Max(0, 100 + PercentModifier);
		const int64 Scaled = static_cast<int64>(Value) * static_cast<int64>(SafePercent) / 100;
		return static_cast<int32>(FMath::Clamp<int64>(Scaled, 0, MAX_int32));
	}

	int32 AddScaledRank(int32 Base, int32 Rank, int32 Scale)
	{
		const int64 Value = static_cast<int64>(FMath::Max(0, Base)) +
			static_cast<int64>(FMath::Max(0, Rank)) * static_cast<int64>(FMath::Max(0, Scale));
		return static_cast<int32>(FMath::Clamp<int64>(Value, 0, MAX_int32));
	}
}

int32 FGridQuickItemResolver::ResolveSkillRank(const TArray<FRPGSkillRank>& SkillRanks, FName SkillId)
{
	if (SkillId.IsNone())
	{
		return 0;
	}
	const FRPGSkillRank* Rank = SkillRanks.FindByPredicate(
		[SkillId](const FRPGSkillRank& Candidate)
		{
			return Candidate.SkillId == SkillId && Candidate.Rank > 0;
		});
	return Rank ? Rank->Rank : 0;
}

bool FGridQuickItemResolver::ResolveEffectProfile(const FGridCombatActionDefinition& Definition, const TArray<FRPGSkillRank>& SkillRanks,
	const FGridResolvedCombatModifiers& Modifiers, FGridCombatActionEffectProfile& OutProfile)
{
	OutProfile = Definition.EffectProfile;
	if (Definition.SourcePolicy != EGridCombatActionSourcePolicy::QuickItem || !Definition.QuickItemScaling.IsValid())
	{
		return Definition.SourcePolicy != EGridCombatActionSourcePolicy::QuickItem ? OutProfile.IsValid() : false;
	}

	const int32 SkillRank = ResolveSkillRank(SkillRanks, Definition.QuickItemScaling.ScalingSkillId);
	OutProfile.RestoreHealth = AddScaledRank(
		Definition.EffectProfile.RestoreHealth, SkillRank, Definition.QuickItemScaling.RestoreHealthSkillRankScale);
	OutProfile.RestoreMana =
		AddScaledRank(Definition.EffectProfile.RestoreMana, SkillRank, Definition.QuickItemScaling.RestoreManaSkillRankScale);
	OutProfile.RestoreHealth = ScalePositiveByModifier(OutProfile.RestoreHealth, Modifiers.PositiveEffectPercentModifier);
	OutProfile.RestoreMana = ScalePositiveByModifier(OutProfile.RestoreMana, Modifiers.PositiveEffectPercentModifier);
	return OutProfile.IsValid();
}

void FGridQuickItemResolver::ApplyDirectDamageSkillScaling(
	const FGridCombatActionDefinition& Definition, const TArray<FRPGSkillRank>& SkillRanks, FGridAttackSourceStats& InOutSource)
{
	if (Definition.SourcePolicy != EGridCombatActionSourcePolicy::QuickItem || !Definition.QuickItemScaling.IsValid() ||
		Definition.QuickItemScaling.DirectDamageSkillRankScale <= 0)
	{
		return;
	}
	const int32 Rank = ResolveSkillRank(SkillRanks, Definition.QuickItemScaling.ScalingSkillId);
	const int64 Bonus = static_cast<int64>(FMath::Max(0, Rank)) *
		static_cast<int64>(Definition.QuickItemScaling.DirectDamageSkillRankScale);
	InOutSource.DamageBonus = static_cast<int32>(FMath::Clamp<int64>(
		static_cast<int64>(InOutSource.DamageBonus) + Bonus, static_cast<int64>(MIN_int32), static_cast<int64>(MAX_int32)));
}

FGridQuickItemSecondaryEffectProjection FGridQuickItemResolver::ResolveSecondaryEffect(const FGridResolvedCombatModifiers& Modifiers)
{
	FGridQuickItemSecondaryEffectProjection Projection;
	Projection.TargetCount = FMath::Clamp(Modifiers.QuickItemSecondaryTargetCount, 0, 6);
	Projection.MagnitudePercent = FMath::Clamp(Modifiers.QuickItemSecondaryMagnitudePercent, 0, 100);
	Projection.DurationPercent = FMath::Clamp(Modifiers.QuickItemSecondaryDurationPercent, 0, 100);
	return Projection;
}
