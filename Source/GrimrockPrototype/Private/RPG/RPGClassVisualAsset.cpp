#include "RPG/RPGClassVisualAsset.h"

bool URPGClassVisualAsset::IsValidDefinition () const
{
    return !ClassId.IsNone () && !ClassIcon.IsNull ();
}

bool URPGClassVisualAsset::IsValidForClass (FName InClassId) const
{
    return IsValidDefinition () && !InClassId.IsNone () && ClassId == InClassId;
}
