#include "RPG/RPGClassAsset.h"

bool URPGClassAsset::IsValidDefinition () const
{
    if (ClassId.IsNone () || HealthAtLevelOne <= 0)
    {
        return false;
    }
    TSet<FName> ActionIds;
    for (const FGridCombatActionDefinition& Action : CombatActions)
    {
        if (!Action.IsValid () ||
            (Action.SourcePolicy !=
                    EGridCombatActionSourcePolicy::Ability &&
                Action.SourcePolicy !=
                    EGridCombatActionSourcePolicy::Spell &&
                Action.SourcePolicy !=
                    EGridCombatActionSourcePolicy::Universal) ||
            ActionIds.Contains (Action.ActionId))
        {
            return false;
        }
        ActionIds.Add (Action.ActionId);
    }
    return true;
}
