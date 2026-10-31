#pragma once

#include "Core/GridLevelAsset.h"

/** Resolve only a unique, typed PIT object on a trigger's cell.
 * Fail closed on missing, invalid or ambiguous placements.
 */
namespace GridDungeonTrapPit
{
    inline bool FindUniqueSameCellPit(
        const UGridLevelAsset& Level, const FGridWorldObjectInstance& Trap, FGuid& OutPitId)
    {
        OutPitId.Invalidate();
        if (Trap.Type != EGridLevelObjectType::Trigger || !Trap.InstanceId.IsValid())
            return false;
        for (const FGridWorldObjectInstance& Candidate : Level.WorldObjectInstances)
        {
            if (Candidate.Type != EGridLevelObjectType::Pit ||
                Candidate.CellX != Trap.CellX || Candidate.CellY != Trap.CellY)
                continue;
            if (!Candidate.InstanceId.IsValid() || OutPitId.IsValid())
            {
                OutPitId.Invalidate();
                return false;
            }
            OutPitId = Candidate.InstanceId;
        }
        return OutPitId.IsValid();
    }
}
