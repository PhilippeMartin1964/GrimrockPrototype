#include "Runtime/Monsters/GridMonsterLootResolver.h"

namespace
{
    constexpr uint32 MON8IndependentLootSeedSalt = 0x494C4F54u;

    int32 ResolveMON8Quantity (
        const FGridMonsterLootEntry& Entry,
        int32 QuantityRoll)
    {
        const int32 Range = Entry.MaxQuantity - Entry.MinQuantity + 1;
        return Entry.MinQuantity +
            static_cast<int32> (static_cast<uint32> (QuantityRoll) % static_cast<uint32> (Range));
    }
}

TArray<FGridMonsterLootRollResult> FGridMonsterLootResolver::ResolveLoot (
    const TArray<FGridMonsterLootEntry>& LootTable,
    int32 BaseSeed)
{
    TArray<FGridMonsterLootRollResult> Results;
    Results.Reserve (LootTable.Num ());

    for (int32 EntryIndex = 0; EntryIndex < LootTable.Num (); ++EntryIndex)
    {
        const FGridMonsterLootEntry& Entry = LootTable[EntryIndex];
        if (!Entry.IsValidDefinition ())
        {
            continue;
        }

        uint32 EntrySeed = HashCombine (
            static_cast<uint32> (BaseSeed),
            GetTypeHash (Entry.GetResolvedItemDefinitionId ()));
        EntrySeed = HashCombine (
            EntrySeed,
            MON8IndependentLootSeedSalt);
        FRandomStream EntryStream (static_cast<int32> (EntrySeed));

        const float DropRoll = EntryStream.FRand ();
        FGridMonsterLootRollResult Result = ResolveEntryFromRolls (
            Entry,
            EntryIndex,
            DropRoll,
            0);
        if (Result.bDropped)
        {
            Result = ResolveEntryFromRolls (
                Entry,
                EntryIndex,
                DropRoll,
                static_cast<int32> (EntryStream.GetUnsignedInt ()));
        }
        Results.Add (Result);
    }

    return Results;
}

FGridMonsterLootRollResult FGridMonsterLootResolver::ResolveEntryFromRolls (
    const FGridMonsterLootEntry& Entry,
    int32 EntryIndex,
    float DropRoll,
    int32 QuantityRoll)
{
    FGridMonsterLootRollResult Result;
    Result.EntryIndex = EntryIndex;
    Result.ItemDefinitionId = Entry.GetResolvedItemDefinitionId ();
    Result.DropRoll = FMath::IsFinite (DropRoll)
        ? FMath::Clamp (DropRoll, 0.0f, 1.0f)
        : 1.0f;

    if (!Entry.IsValidDefinition () || Entry.DropChance <= 0.0f)
    {
        return Result;
    }

    Result.bDropped =
        Entry.DropChance >= 1.0f ||
        Result.DropRoll < Entry.DropChance;
    if (Result.bDropped)
    {
        Result.Quantity = ResolveMON8Quantity (Entry, QuantityRoll);
    }
    return Result;
}
