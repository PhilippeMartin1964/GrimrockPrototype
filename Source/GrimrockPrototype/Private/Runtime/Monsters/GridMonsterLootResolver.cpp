#include "Runtime/Monsters/GridMonsterLootResolver.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"

namespace
{
	constexpr uint32 MON8IndependentLootSeedSalt = 0x494C4F54u;

	int32 ResolveMON8Quantity(const FGridMonsterLootEntry& Entry, int32 QuantityRoll)
	{
		const int32 Range = Entry.MaxQuantity - Entry.MinQuantity + 1;
		return Entry.MinQuantity + static_cast<int32>(static_cast<uint32>(QuantityRoll) % static_cast<uint32>(Range));
	}
}

TArray<FGridMonsterLootRollResult> FGridMonsterLootResolver::ResolveLoot(const TArray<FGridMonsterLootEntry>& LootTable, int32 BaseSeed)
{
	TArray<FGridMonsterLootRollResult> Results;
	Results.Reserve(LootTable.Num());

	for (int32 EntryIndex = 0; EntryIndex < LootTable.Num(); ++EntryIndex)
	{
		const FGridMonsterLootEntry& Entry = LootTable[EntryIndex];
		if (!Entry.IsValidDefinition())
		{
			continue;
		}

		uint32 EntrySeed = HashCombine(static_cast<uint32>(BaseSeed), GetTypeHash(Entry.GetResolvedItemDefinitionId()));
		EntrySeed = HashCombine(EntrySeed, MON8IndependentLootSeedSalt);
		FRandomStream EntryStream(static_cast<int32>(EntrySeed));

		const float DropRoll = EntryStream.FRand();
		FGridMonsterLootRollResult Result = ResolveEntryFromRolls(Entry, EntryIndex, DropRoll, 0);
		if (Result.bDropped)
		{
			Result = ResolveEntryFromRolls(Entry, EntryIndex, DropRoll, static_cast<int32>(EntryStream.GetUnsignedInt()));
		}
		Results.Add(Result);
	}

	return Results;
}

FGridMonsterLootRollResult FGridMonsterLootResolver::ResolveEntryFromRolls(
	const FGridMonsterLootEntry& Entry, int32 EntryIndex, float DropRoll, int32 QuantityRoll)
{
	FGridMonsterLootRollResult Result;
	Result.EntryIndex = EntryIndex;
	Result.ItemDefinitionId = Entry.GetResolvedItemDefinitionId();
	Result.DropRoll = FMath::IsFinite(DropRoll) ? FMath::Clamp(DropRoll, 0.0f, 1.0f) : 1.0f;

	if (!Entry.IsValidDefinition() || Entry.DropChance <= 0.0f)
	{
		return Result;
	}

	Result.bDropped = Entry.DropChance >= 1.0f || Result.DropRoll < Entry.DropChance;
	if (Result.bDropped)
	{
		Result.Quantity = ResolveMON8Quantity(Entry, QuantityRoll);
	}
	return Result;
}

bool FGridMonsterLootResolver::TryResolveItemLevel(
	const FGridMonsterLootEntry& Entry, const UGridItemDefinitionAsset* ItemDefinition,
	int32& OutItemLevel, const UGridItemSubsystem* CanonicalRegistry)
{
	OutItemLevel = 0;
	if (!Entry.IsValidDefinition())
	{
		return false;
	}
	const FName ItemId = Entry.GetResolvedItemDefinitionId();
	if (IsValid(ItemDefinition) && ItemDefinition->ItemDefinitionId != ItemId)
	{
		return false; // Never inherit a default from another item identity.
	}
	if (CanonicalRegistry && CanonicalRegistry->HasCanonicalItem(ItemId))
	{
		// Probe N1 solely to classify the item family. The eventual N0 for
		// arrows is real, not a weapon default to be promoted to N1.
		FGridResolvedItem Family;
		if (!CanonicalRegistry->ResolveItem(ItemId, 1, Family))
		{
			return false;
		}
		if (!Family.bIsArmament)
		{
			FGridResolvedItem Unlevelled;
			return Entry.ItemLevel == 0 &&
				CanonicalRegistry->ResolveItem(ItemId, 0, Unlevelled) &&
				!Unlevelled.bIsArmament;
		}
		// Authored zero uses the old default only for a NEW armament.
		// Without the authoring definition there is no default to invent.
		if (Entry.ItemLevel == 0 && !IsValid(ItemDefinition))
		{
			return false;
		}
		const int32 FixedLevel = Entry.ItemLevel == 0
			? ItemDefinition->DefaultWeaponItemLevel : Entry.ItemLevel;
		FGridResolvedItem Fixed;
		if (!CanonicalRegistry->ResolveItem(ItemId, FixedLevel, Fixed) || !Fixed.bIsArmament)
		{
			return false; // Known invalid ARM: no old catalogue fallback.
		}
		OutItemLevel = FixedLevel;
		return true;
	}
	// Existing nonregistered/id-only MON8 loot semantics remain intact.
	if (!IsValid(ItemDefinition))
	{
		return Entry.ItemLevel == 0;
	}
	if (!ItemDefinition->bUseWeaponCatalogue)
	{
		return Entry.ItemLevel == 0;
	}
	const int32 FixedLevel = Entry.ItemLevel == 0 ? ItemDefinition->DefaultWeaponItemLevel : Entry.ItemLevel;
	const FGridWeaponLevelProfile* Profile = nullptr;
	if (FixedLevel < 1 || FixedLevel > 20 || !ItemDefinition->TryGetWeaponCatalogueProfile(FixedLevel, Profile))
	{
		return false;
	}
	OutItemLevel = FixedLevel;
	return true;
}
