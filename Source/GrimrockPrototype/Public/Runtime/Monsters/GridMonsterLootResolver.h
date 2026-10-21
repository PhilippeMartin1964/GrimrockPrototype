#pragma once

#include "CoreMinimal.h"
#include "Runtime/Monsters/GridMonsterTypes.h"

struct GRIMROCKPROTOTYPE_API FGridMonsterLootRollResult
{
	bool bDropped = false;
	int32 EntryIndex = INDEX_NONE;
	FName ItemDefinitionId = NAME_None;
	int32 Quantity = 0;
	float DropRoll = 0.0f;
};

/** Pure MON8 independent loot-table resolver. */
class GRIMROCKPROTOTYPE_API FGridMonsterLootResolver
{
public:
	static TArray<FGridMonsterLootRollResult> ResolveLoot(const TArray<FGridMonsterLootEntry>& LootTable, int32 BaseSeed);

	static FGridMonsterLootRollResult ResolveEntryFromRolls(const FGridMonsterLootEntry& Entry, int32 EntryIndex, float DropRoll, int32 QuantityRoll);

	/** Resolve a loot item's fixed catalogue level once, without randomization. */
	static bool TryResolveItemLevel(const FGridMonsterLootEntry& Entry, const UGridItemDefinitionAsset* ItemDefinition, int32& OutItemLevel);
};
