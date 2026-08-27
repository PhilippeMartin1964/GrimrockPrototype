#include "RPG/RPGRaceAsset.h"

FPrimaryAssetId URPGRaceAsset::GetPrimaryAssetId() const
{
	if (RaceId.IsNone())
	{
		return Super::GetPrimaryAssetId();
	}
	return FPrimaryAssetId(FPrimaryAssetType(TEXT("RPGRace")), RaceId);
}

URPGRaceAsset::URPGRaceAsset()
	: AttributeBonuses(0, 0, 0, 0, 0, 0)
{
}

bool URPGRaceAsset::IsValidDefinition() const
{
	return !RaceId.IsNone();
}
