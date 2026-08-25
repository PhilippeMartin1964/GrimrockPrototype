#include "RPG/RPGSkillAsset.h"

FPrimaryAssetId URPGSkillAsset::GetPrimaryAssetId() const
{
	if (SkillId.IsNone())
	{
		return Super::GetPrimaryAssetId();
	}
	return FPrimaryAssetId(FPrimaryAssetType(TEXT("RPGSkill")), SkillId);
}

bool URPGSkillAsset::IsValidDefinition() const
{
	if (SkillId.IsNone() || MaxRank <= 0)
	{
		return false;
	}

	for (const FRPGSkillRequirementGrant& Grant : RequirementGrants)
	{
		if (!Grant.IsValid(MaxRank))
		{
			return false;
		}
	}
	return true;
}
