#include "Runtime/Combat/GridCombatDiagnostics.h"

#include "Misc/Crc.h"

DEFINE_LOG_CATEGORY(LogGridCombatPerformance);

namespace
{
	constexpr uint32 EncounterSeedSalt = 0x4D4F4E15u;

	uint32 MixSeedValue(uint32 CurrentHash, uint32 Value)
	{
		return HashCombineFast(CurrentHash, Value);
	}

	uint32 MixSeedString(uint32 CurrentHash, const FString& Value)
	{
		return MixSeedValue(CurrentHash, FCrc::StrCrc32(*Value));
	}
}

int32 FGridEncounterSeedBuilder::BuildEncounterSeed(int32 BaseSeed, FName DungeonLevelId, const TArray<FGuid>& ParticipantIds)
{
	uint32 Hash = EncounterSeedSalt;
	Hash = MixSeedValue(Hash, static_cast<uint32>(BaseSeed));
	Hash = MixSeedString(Hash, DungeonLevelId.ToString());

	TSet<FGuid> UniqueParticipantIds;
	for (const FGuid& ParticipantId : ParticipantIds)
	{
		if (ParticipantId.IsValid())
		{
			UniqueParticipantIds.Add(ParticipantId);
		}
	}

	TArray<FString> SortedParticipantKeys;
	SortedParticipantKeys.Reserve(UniqueParticipantIds.Num());
	for (const FGuid& ParticipantId : UniqueParticipantIds)
	{
		SortedParticipantKeys.Add(ParticipantId.ToString(EGuidFormats::Digits));
	}
	SortedParticipantKeys.Sort();

	for (const FString& ParticipantKey : SortedParticipantKeys)
	{
		Hash = MixSeedString(Hash, ParticipantKey);
	}

	return static_cast<int32>(Hash);
}

void FGridCombatRuntimeMetrics::Reset()
{
	*this = FGridCombatRuntimeMetrics();
}
