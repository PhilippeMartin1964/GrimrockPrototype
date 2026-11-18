#pragma once
#include "Runtime/Monsters/GridMonsterTypes.h"

/** Only the exact approved X.60 native Stone row is exempt from old
 * TD07 pointer-based authoring audits. Every other id-only row remains a finding.
 * X.60 native plan independently verifies registry membership and N0.
 */
namespace GridTD073ApprovedCanonicalStoneLoot
{
    inline bool IsExact(const FString& Path, int32 Index, const FGridMonsterLootEntry& Loot)
    {
        return Path == TEXT("/Game/GrimrockPrototype/Monsters/GoblinThrower/Data/DA_MON_GoblinThrower.DA_MON_GoblinThrower") &&
            Index == 1 && !Loot.ItemDefinitionAsset && Loot.ItemDefinitionId == FName(TEXT("Stone")) &&
            Loot.ItemLevel == 0 && Loot.DropChance == 0.5f && Loot.MinQuantity == 1 && Loot.MaxQuantity == 1;
    }
}
