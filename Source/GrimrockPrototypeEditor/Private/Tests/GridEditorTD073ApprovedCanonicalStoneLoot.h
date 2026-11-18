#pragma once
#include "Runtime/Monsters/GridMonsterTypes.h"

/** X.60/X.61 exceptions to TD07's historical pointer-only policy.
 * Only the five exact rows of the two audited native packages qualify.
 * The X.61 transaction and independent X.48 gate prove the entire root
 * and both tables are in one exact state. No other ID-only row is exempt.
 */
namespace GridTD073ApprovedCanonicalStoneLoot
{
    inline bool IsExact(const FString& Path, int32 Index, const FGridMonsterLootEntry& Loot)
    {
        if (Loot.ItemDefinitionAsset || Loot.ItemLevel != 0 || !Loot.IsValidDefinition()) return false;
        if (Path == TEXT("/Game/GrimrockPrototype/Monsters/GoblinThrower/Data/DA_MON_GoblinThrower.DA_MON_GoblinThrower"))
        {
            if (Index < 0 || Index >= 3) return false;
            const FName Ids[] = {TEXT("GoblinKnife"), TEXT("Stone"), TEXT("EmptyVial")};
            return Loot.ItemDefinitionId == Ids[Index] &&
                Loot.DropChance == (Index == 1 ? 0.5f : 0.25f) &&
                Loot.MinQuantity == 1 && Loot.MaxQuantity == 1;
        }
        if (Path == TEXT("/Game/GrimrockPrototype/Monsters/RatGiant/Data/DA_MON_RatGiant.DA_MON_RatGiant"))
        {
            if (Index < 0 || Index >= 2) return false;
            const FName Ids[] = {TEXT("Item_RatMeat"), TEXT("Item_RatTooth")};
            return Loot.ItemDefinitionId == Ids[Index];
        }
        return false;
    }
}
