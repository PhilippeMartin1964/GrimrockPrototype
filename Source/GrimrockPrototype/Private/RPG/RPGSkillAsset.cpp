#include "RPG/RPGSkillAsset.h"

bool URPGSkillAsset::IsValidDefinition () const
{
    return !SkillId.IsNone () && MaxRank > 0;
}
