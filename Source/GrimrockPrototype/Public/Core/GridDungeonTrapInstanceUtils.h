#pragma once

#include "Core/GridLevelPlacementTypes.h"
#include "Core/GridWorldObjectDefinitionAsset.h"

/** Effective trap configuration. Both editor and runtime use this single authority. */
namespace GridDungeonTrapInstance
{
    inline FGridDungeonTrapDefinitionParams Resolve(
        const FGridWorldObjectInstance& Instance, const UGridWorldObjectDefinitionAsset& Definition)
    {
        FGridDungeonTrapDefinitionParams Result = Definition.DungeonTrap;
        const FGridDungeonTrapInstanceOverrides& O = Instance.InstanceConfig.DungeonTrap;
        if (O.bOverrideDetectionDifficulty) Result.DetectionDifficulty = O.DetectionDifficulty;
        if (O.bOverrideMaxDetectionRangeCells) Result.MaxDetectionRangeCells = O.MaxDetectionRangeCells;
        if (O.bOverrideDamageType) Result.DamageType = O.DamageType;
        if (O.bOverridePhysicalSubtype) Result.PhysicalSubtype = O.PhysicalSubtype;
        if (O.bOverrideMinimumDamage) Result.MinimumDamage = O.MinimumDamage;
        if (O.bOverrideMaximumDamage) Result.MaximumDamage = O.MaximumDamage;
        return Result;
    }
    inline int32 ResolveDisarmDifficulty(
        const FGridWorldObjectInstance& Instance, const UGridWorldObjectDefinitionAsset& Definition)
    {
        const FGridDungeonTrapInstanceOverrides& O = Instance.InstanceConfig.DungeonTrap;
        return O.bOverrideDisarmDifficulty ? O.DisarmDifficulty : Definition.DisarmDifficulty;
    }
}
