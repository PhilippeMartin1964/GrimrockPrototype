#include "RPG/RPGRaceAsset.h"

URPGRaceAsset::URPGRaceAsset()
	: AttributeBonuses(0, 0, 0, 0, 0, 0)
{
}

bool URPGRaceAsset::IsValidDefinition() const
{
	return !RaceId.IsNone();
}
