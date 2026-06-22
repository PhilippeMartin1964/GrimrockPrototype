#include "RPG/RPGClassAsset.h"

bool URPGClassAsset::IsValidDefinition () const
{
    return !ClassId.IsNone () && HealthAtLevelOne > 0;
}
