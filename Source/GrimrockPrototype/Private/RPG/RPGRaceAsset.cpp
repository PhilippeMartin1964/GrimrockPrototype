#include "RPG/RPGRaceAsset.h"

bool URPGRaceAsset::IsValidDefinition () const
{
    return !RaceId.IsNone ();
}
