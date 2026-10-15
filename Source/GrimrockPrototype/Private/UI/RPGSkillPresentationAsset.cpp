#include "UI/RPGSkillPresentationAsset.h"

namespace
{
	bool BuildUniqueNameSet(const TArray<FName>& Values, TSet<FName>& OutValues)
	{
		OutValues.Reset();
		for (const FName Value : Values)
		{
			if (Value.IsNone() || OutValues.Contains(Value))
			{
				OutValues.Reset();
				return false;
			}
			OutValues.Add(Value);
		}
		return true;
	}
}

bool FRPGSkillPresentationDefinition::IsValidDefinition() const
{
	if (SkillId.IsNone())
	{
		return false;
	}

	TSet<FName> RecommendedClasses;
	TSet<FName> UsefulClasses;
	TSet<FName> Synergies;
	if (!BuildUniqueNameSet(RecommendedClassIds, RecommendedClasses) ||
		!BuildUniqueNameSet(UsefulClassIds, UsefulClasses) ||
		!BuildUniqueNameSet(SynergySkillIds, Synergies))
	{
		return false;
	}

	for (const FName ClassId : UsefulClasses)
	{
		if (RecommendedClasses.Contains(ClassId))
		{
			return false;
		}
	}

	return !Synergies.Contains(SkillId);
}

bool URPGSkillPresentationAsset::IsValidDefinition() const
{
	if (Skills.IsEmpty())
	{
		return false;
	}

	TSet<FName> SkillIds;
	for (const FRPGSkillPresentationDefinition& Skill : Skills)
	{
		if (!Skill.IsValidDefinition() || SkillIds.Contains(Skill.SkillId))
		{
			return false;
		}
		SkillIds.Add(Skill.SkillId);
	}

	for (const FRPGSkillPresentationDefinition& Skill : Skills)
	{
		for (const FName SynergySkillId : Skill.SynergySkillIds)
		{
			if (!SkillIds.Contains(SynergySkillId))
			{
				return false;
			}
		}
	}

	return true;
}

const FRPGSkillPresentationDefinition* URPGSkillPresentationAsset::FindSkill(FName SkillId) const
{
	if (SkillId.IsNone())
	{
		return nullptr;
	}

	return Skills.FindByPredicate(
		[SkillId](const FRPGSkillPresentationDefinition& Skill)
		{
			return Skill.SkillId == SkillId;
		});
}

bool URPGSkillPresentationAsset::GetSkillPresentation(
	FName SkillId,
	FRPGSkillPresentationDefinition& OutDefinition) const
{
	OutDefinition = FRPGSkillPresentationDefinition();
	const FRPGSkillPresentationDefinition* Found = FindSkill(SkillId);
	if (!Found)
	{
		return false;
	}

	OutDefinition = *Found;
	return true;
}
