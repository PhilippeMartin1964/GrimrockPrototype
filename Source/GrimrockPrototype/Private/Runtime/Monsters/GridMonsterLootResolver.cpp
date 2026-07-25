#include "Runtime/Monsters/GridMonsterLootResolver.h"

namespace
{
    int32 ResolveMON8Quantity (
        const FGridMonsterLootEntry& Entry,
        int32 QuantityRoll)
    {
        const int32 Range = Entry.MaxQuantity - Entry.MinQuantity + 1;
        return Entry.MinQuantity +
            static_cast<int32> (static_cast<uint32> (QuantityRoll) % static_cast<uint32> (Range));
    }
}

FGridMonsterLootRollResult FGridMonsterLootResolver::ResolveLoot (
    const TArray<FGridMonsterLootEntry>& LootTable,
    FRandomStream& RandomStream)
{
    const float SelectionRoll = RandomStream.FRand ();
    FGridMonsterLootRollResult Result;
    Result.SelectionRoll = SelectionRoll;

    float CumulativeChance = 0.0f;
    for (int32 EntryIndex = 0; EntryIndex < LootTable.Num (); ++EntryIndex)
    {
        const FGridMonsterLootEntry& Entry = LootTable[EntryIndex];
        if (!Entry.IsValidDefinition ())
        {
            continue;
        }

        CumulativeChance += Entry.DropChance;
        if (SelectionRoll < CumulativeChance)
        {
            Result.bHasLoot = true;
            Result.SelectedEntryIndex = EntryIndex;
            Result.ItemDefinitionId = Entry.GetResolvedItemDefinitionId ();
            Result.Quantity = RandomStream.RandRange (
                Entry.MinQuantity,
                Entry.MaxQuantity);
            return Result;
        }
    }

    return Result;
}

FGridMonsterLootRollResult FGridMonsterLootResolver::ResolveLootFromRolls (
    const TArray<FGridMonsterLootEntry>& LootTable,
    float SelectionRoll,
    int32 QuantityRoll)
{
    FGridMonsterLootRollResult Result;
    Result.SelectionRoll = FMath::Clamp (SelectionRoll, 0.0f, 1.0f);

    float CumulativeChance = 0.0f;
    for (int32 EntryIndex = 0; EntryIndex < LootTable.Num (); ++EntryIndex)
    {
        const FGridMonsterLootEntry& Entry = LootTable[EntryIndex];
        if (!Entry.IsValidDefinition ())
        {
            continue;
        }

        CumulativeChance += Entry.DropChance;
        if (Result.SelectionRoll < CumulativeChance)
        {
            Result.bHasLoot = true;
            Result.SelectedEntryIndex = EntryIndex;
            Result.ItemDefinitionId = Entry.GetResolvedItemDefinitionId ();
            Result.Quantity = ResolveMON8Quantity (Entry, QuantityRoll);
            return Result;
        }
    }

    return Result;
}
