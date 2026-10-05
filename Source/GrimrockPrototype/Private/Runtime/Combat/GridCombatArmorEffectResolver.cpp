#include "Runtime/Combat/GridCombatArmorEffectResolver.h"

#include "RPG/RPGCharacterRulesLibrary.h"

namespace
{
	int32 ScalePositive(int32 BaseAmount, int32 Percent)
	{
		if (BaseAmount <= 0 || Percent <= 0)
		{
			return 0;
		}
		const int64 Scaled = static_cast<int64>(BaseAmount) * static_cast<int64>(Percent) / 100;
		return static_cast<int32>(FMath::Clamp<int64>(FMath::Max<int64>(1, Scaled), 0, MAX_int32));
	}

	int32 ApplyPercentModifier(int32 BaseAmount, int32 PercentModifier)
	{
		if (BaseAmount <= 0)
		{
			return 0;
		}
		const int32 SafePercent = FMath::Max(0, 100 + PercentModifier);
		if (SafePercent <= 0)
		{
			return 0;
		}
		const int64 Scaled = static_cast<int64>(BaseAmount) * static_cast<int64>(SafePercent) / 100;
		return static_cast<int32>(FMath::Clamp<int64>(Scaled, 0, MAX_int32));
	}

	int32 ResolveReference(const FGridCombatArmorPoolSnapshot& Snapshot, EGridCombatArmorPool Pool)
	{
		return Pool == EGridCombatArmorPool::Physical ? Snapshot.ReferencePhysicalArmor : Snapshot.ReferenceMagicalArmor;
	}

	int32 ResolveCurrent(const FGridCombatArmorPoolSnapshot& Snapshot, EGridCombatArmorPool Pool)
	{
		return Pool == EGridCombatArmorPool::Physical ? Snapshot.CurrentPhysicalArmor : Snapshot.CurrentMagicalArmor;
	}

	void WriteCurrent(FGridCombatArmorPoolSnapshot& Snapshot, EGridCombatArmorPool Pool, int32 Value)
	{
		if (Pool == EGridCombatArmorPool::Physical)
		{
			Snapshot.CurrentPhysicalArmor = FMath::Max(0, Value);
		}
		else
		{
			Snapshot.CurrentMagicalArmor = FMath::Max(0, Value);
		}
	}

	int32 ResolveScalingAttributeValue(const FRPGAttributes& Attributes, EGridAttackScalingAttribute Attribute)
	{
		switch (Attribute)
		{
			case EGridAttackScalingAttribute::Strength:
				return Attributes.Strength;
			case EGridAttackScalingAttribute::Dexterity:
				return Attributes.Dexterity;
			case EGridAttackScalingAttribute::Constitution:
				return Attributes.Constitution;
			case EGridAttackScalingAttribute::Intelligence:
				return Attributes.Intelligence;
			case EGridAttackScalingAttribute::Wisdom:
				return Attributes.Wisdom;
			case EGridAttackScalingAttribute::Charisma:
				return Attributes.Charisma;
			default:
				return 10;
		}
	}

	int32 ResolveSkillRank(const FGridCombatArmorEffectSourceContext& SourceContext, FName SkillId)
	{
		if (SkillId.IsNone())
		{
			return 0;
		}
		const FRPGSkillRank* Rank = SourceContext.SkillRanks.FindByPredicate(
			[SkillId](const FRPGSkillRank& Candidate)
			{
				return Candidate.SkillId == SkillId && Candidate.Rank > 0;
			});
		return Rank ? Rank->Rank : 0;
	}

	int32 ResolveRequestedAmount(const FGridCombatArmorEffectProfile& Profile, const FGridCombatArmorPoolSnapshot& Snapshot,
		const FGridCombatArmorEffectSourceContext* SourceContext, const FGridAttackResult* AttackResult)
	{
		switch (Profile.Magnitude)
		{
			case EGridCombatArmorEffectMagnitude::Flat:
			{
				int64 Amount = Profile.Amount;
				if (Profile.AttributeModifierScale > 0)
				{
					if (!SourceContext)
					{
						return 0;
					}
					const int32 AttributeValue = ResolveScalingAttributeValue(SourceContext->Attributes, Profile.ScalingAttribute);
					Amount += static_cast<int64>(Profile.AttributeModifierScale) *
						static_cast<int64>(URPGCharacterRulesLibrary::GetAttributeModifier(AttributeValue));
				}
				if (Profile.SkillRankScale > 0)
				{
					if (!SourceContext)
					{
						return 0;
					}
					Amount += static_cast<int64>(Profile.SkillRankScale) *
						static_cast<int64>(ResolveSkillRank(*SourceContext, Profile.ScalingSkillId));
				}
				return static_cast<int32>(FMath::Clamp<int64>(FMath::Max<int64>(1, Amount), 1, MAX_int32));
			}
			case EGridCombatArmorEffectMagnitude::ReferencePercent:
				return ScalePositive(ResolveReference(Snapshot, Profile.Pool), Profile.Amount);
			case EGridCombatArmorEffectMagnitude::RawDamagePercent:
				return AttackResult ? ScalePositive(FMath::Max(0, AttackResult->RawDamage), Profile.Amount) : 0;
			default:
				return 0;
		}
	}
}

FGridCombatArmorEffectSourceContext FGridCombatArmorEffectResolver::MakeSourceContext(const FGridCharacterInventoryState& Character)
{
	return MakeSourceContext(Character, Character.Attributes);
}

FGridCombatArmorEffectSourceContext FGridCombatArmorEffectResolver::MakeSourceContext(
	const FGridCharacterInventoryState& Character, const FRPGAttributes& EffectiveAttributes)
{
	FGridCombatArmorEffectSourceContext Context;
	Context.Attributes = EffectiveAttributes;
	Context.SkillRanks = Character.SkillRanks;
	return Context;
}

void FGridCombatArmorEffectResolver::ApplyReferenceModifiers(
	FGridCombatArmorPoolSnapshot& InOutSnapshot, const FGridResolvedCombatModifiers& Modifiers)
{
	InOutSnapshot.ReferencePhysicalArmor =
		ApplyPercentModifier(FMath::Max(0, InOutSnapshot.ReferencePhysicalArmor), Modifiers.PhysicalArmorReferencePercentModifier);
	InOutSnapshot.ReferenceMagicalArmor =
		ApplyPercentModifier(FMath::Max(0, InOutSnapshot.ReferenceMagicalArmor), Modifiers.MagicalArmorReferencePercentModifier);
}

int32 FGridCombatArmorEffectResolver::GetRestorationPercentModifier(
	EGridCombatArmorPool Pool, const FGridResolvedCombatModifiers& Modifiers)
{
	return Pool == EGridCombatArmorPool::Physical ? Modifiers.PhysicalArmorRestorationPercentModifier
												 : Modifiers.MagicalArmorRestorationPercentModifier;
}

bool FGridCombatArmorEffectResolver::ResolveOne(const FGridCombatArmorEffectProfile& Profile, const FGridCombatArmorPoolSnapshot& Snapshot,
	const FGridResolvedCombatModifiers& Modifiers, const FGridAttackResult* AttackResult, FGridCombatArmorEffectResult& OutResult)
{
	OutResult = FGridCombatArmorEffectResult();
	if (!Profile.IsValid() || !Snapshot.IsValid())
	{
		return false;
	}
	if (Profile.Trigger == EGridCombatArmorEffectTrigger::AfterSuccessfulHit && (!AttackResult || !AttackResult->bHit))
	{
		return false;
	}

	OutResult.Pool = Profile.Pool;
	OutResult.Operation = Profile.Operation;
	OutResult.ArmorBefore = ResolveCurrent(Snapshot, Profile.Pool);
	OutResult.ReferenceArmor = ResolveReference(Snapshot, Profile.Pool);
	OutResult.RequestedAmount = ResolveRequestedAmount(Profile, Snapshot, SourceContext, AttackResult);
	if (OutResult.RequestedAmount <= 0)
	{
		OutResult.ArmorAfter = OutResult.ArmorBefore;
		return true;
	}

	if (Profile.Operation == EGridCombatArmorEffectOperation::Restore)
	{
		OutResult.RequestedAmount =
			ApplyPercentModifier(OutResult.RequestedAmount, GetRestorationPercentModifier(Profile.Pool, Modifiers));
		OutResult.ArmorAfter = FMath::Clamp(OutResult.ArmorBefore + OutResult.RequestedAmount, 0, FMath::Max(0, OutResult.ReferenceArmor));
		OutResult.AppliedAmount = FMath::Max(0, OutResult.ArmorAfter - OutResult.ArmorBefore);
	}
	else
	{
		OutResult.ArmorAfter = FMath::Max(0, OutResult.ArmorBefore - OutResult.RequestedAmount);
		OutResult.AppliedAmount = FMath::Max(0, OutResult.ArmorBefore - OutResult.ArmorAfter);
	}
	return true;
}

bool FGridCombatArmorEffectResolver::WouldAnyRestore(const TArray<FGridCombatArmorEffectProfile>& Profiles,
	const FGridCombatArmorPoolSnapshot& Snapshot, const FGridResolvedCombatModifiers& Modifiers,
	const FGridCombatArmorEffectSourceContext* SourceContext)
{
	FGridCombatArmorPoolSnapshot Candidate = Snapshot;
	for (const FGridCombatArmorEffectProfile& Profile : Profiles)
	{
		if (Profile.Operation != EGridCombatArmorEffectOperation::Restore || Profile.Trigger != EGridCombatArmorEffectTrigger::AfterResolution)
		{
			continue;
		}
		FGridCombatArmorEffectResult Result;
		if (ResolveOne(Profile, Candidate, Modifiers, SourceContext, nullptr, Result) && Result.DidMutate())
		{
			return true;
		}
	}
	return false;
}

int32 FGridCombatArmorEffectResolver::ApplyRestoreEffects(const TArray<FGridCombatArmorEffectProfile>& Profiles,
	FGridCombatArmorPoolSnapshot& InOutSnapshot, const FGridResolvedCombatModifiers& Modifiers,
	const FGridCombatArmorEffectSourceContext* SourceContext, TArray<FGridCombatArmorEffectResult>* OutResults)
{
	int32 MutationCount = 0;
	if (OutResults)
	{
		OutResults->Reset();
	}
	for (const FGridCombatArmorEffectProfile& Profile : Profiles)
	{
		if (Profile.Operation != EGridCombatArmorEffectOperation::Restore || Profile.Trigger != EGridCombatArmorEffectTrigger::AfterResolution)
		{
			continue;
		}
		FGridCombatArmorEffectResult Result;
		if (!ResolveOne(Profile, InOutSnapshot, Modifiers, SourceContext, nullptr, Result))
		{
			continue;
		}
		if (OutResults)
		{
			OutResults->Add(Result);
		}
		if (!Result.DidMutate())
		{
			continue;
		}
		WriteCurrent(InOutSnapshot, Profile.Pool, Result.ArmorAfter);
		++MutationCount;
	}
	return MutationCount;
}

int32 FGridCombatArmorEffectResolver::ApplyAttackDamageEffects(const TArray<FGridCombatArmorEffectProfile>& Profiles,
	const FGridCombatArmorPoolSnapshot& ReferenceSnapshot, const FGridResolvedCombatModifiers& Modifiers, FGridAttackResult& InOutAttackResult,
	const FGridCombatArmorEffectSourceContext* SourceContext, TArray<FGridCombatArmorEffectResult>* OutResults)
{
	if (OutResults)
	{
		OutResults->Reset();
	}
	if (!InOutAttackResult.bHit || InOutAttackResult.TargetHealthAfter <= 0)
	{
		return 0;
	}

	FGridCombatArmorPoolSnapshot PostPrimary = ReferenceSnapshot;
	PostPrimary.CurrentPhysicalArmor =
		FMath::Max(0, ReferenceSnapshot.CurrentPhysicalArmor - FMath::Max(0, InOutAttackResult.PhysicalArmorDamage));
	PostPrimary.CurrentMagicalArmor =
		FMath::Max(0, ReferenceSnapshot.CurrentMagicalArmor - FMath::Max(0, InOutAttackResult.MagicalArmorDamage));

	int32 MutationCount = 0;
	for (const FGridCombatArmorEffectProfile& Profile : Profiles)
	{
		if (Profile.Operation != EGridCombatArmorEffectOperation::Damage ||
			Profile.Trigger != EGridCombatArmorEffectTrigger::AfterSuccessfulHit)
		{
			continue;
		}

		FGridCombatArmorEffectResult Result;
		if (!ResolveOne(Profile, PostPrimary, Modifiers, SourceContext, &InOutAttackResult, Result))
		{
			continue;
		}
		if (OutResults)
		{
			OutResults->Add(Result);
		}
		if (!Result.DidMutate())
		{
			continue;
		}

		WriteCurrent(PostPrimary, Profile.Pool, Result.ArmorAfter);
		if (Profile.Pool == EGridCombatArmorPool::Physical)
		{
			InOutAttackResult.PhysicalArmorDamage += Result.AppliedAmount;
		}
		else
		{
			InOutAttackResult.MagicalArmorDamage += Result.AppliedAmount;
		}
		++MutationCount;
	}
	return MutationCount;
}
