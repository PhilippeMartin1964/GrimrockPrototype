#pragma once

#include "CoreMinimal.h"
#include "Runtime/Monsters/GridMonsterTypes.h"

struct GRIMROCKPROTOTYPE_API FGridMonsterLootRollResult
{
    bool bHasLoot = false;
    int32 SelectedEntryIndex = INDEX_NONE;
    FName ItemDefinitionId = NAME_None;
    int32 Quantity = 0;
    float SelectionRoll = 0.0f;
};

/** Pure MON8 cumulative loot-table resolver. */
class GRIMROCKPROTOTYPE_API FGridMonsterLootResolver
{
public:
    static FGridMonsterLootRollResult ResolveLoot (
        const TArray<FGridMonsterLootEntry>& LootTable,
        FRandomStream& RandomStream);

    static FGridMonsterLootRollResult ResolveLootFromRolls (
        const TArray<FGridMonsterLootEntry>& LootTable,
        float SelectionRoll,
        int32 QuantityRoll);
};
