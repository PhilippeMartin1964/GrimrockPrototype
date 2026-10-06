#include "RPG/RPGClassProgressionAuthoring.h"

#include "RPG/RPGClassAsset.h"

namespace RPGClassProgressionAuthoring
{
	constexpr int32 TalentGrantLevels[] = { 2, 4, 6, 8, 10, 12, 14, 16, 18, 20 };
}

void FRPGClassProgressionAuthoring::ConfigureCanonicalTalentGrants(URPGClassAsset& ClassAsset)
{
	ClassAsset.ProgressionLevelGrants.Reset();
	for (const int32 Level : RPGClassProgressionAuthoring::TalentGrantLevels)
	{
		FRPGClassProgressionLevelGrant Grant;
		Grant.Level = Level;
		Grant.ChoicePointsGranted = 1;
		ClassAsset.ProgressionLevelGrants.Add(Grant);
	}
}

bool FRPGClassProgressionAuthoring::HasCanonicalTalentGrants(const URPGClassAsset& ClassAsset)
{
	if (ClassAsset.ProgressionLevelGrants.Num() != UE_ARRAY_COUNT(RPGClassProgressionAuthoring::TalentGrantLevels))
	{
		return false;
	}

	for (int32 Index = 0; Index < ClassAsset.ProgressionLevelGrants.Num(); ++Index)
	{
		const FRPGClassProgressionLevelGrant& Grant = ClassAsset.ProgressionLevelGrants[Index];
		if (Grant.Level != RPGClassProgressionAuthoring::TalentGrantLevels[Index] ||
			Grant.ChoicePointsGranted != 1 || !Grant.GrantedRequirementIds.IsEmpty())
		{
			return false;
		}
	}
	return true;
}
