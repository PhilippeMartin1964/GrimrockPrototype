#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "RPG/RPGSkillAsset.h"
#include "RPG/RPGSkillPersistence.h"
#include "RPG/RPGSkillService.h"
#include "Runtime/GridInventoryTypes.h"
#include "Save/GrimrockPartySaveGame.h"

namespace RPGMON2092Tests
{
	FGuid MakeId(uint32 Suffix)
	{
		return FGuid(0, 0, 0, Suffix);
	}

	URPGSkillAsset* MakeSkill(FName SkillId, int32 MaxRank)
	{
		URPGSkillAsset* Skill = NewObject<URPGSkillAsset>();
		Skill->SkillId = SkillId;
		Skill->MaxRank = MaxRank;
		return Skill;
	}

	FGridCharacterInventoryState MakeCharacter(const FGuid& CharacterId)
	{
		FGridCharacterInventoryState Character;
		Character.CharacterId = CharacterId;
		Character.Level = 1;
		Character.Experience = 0;
		return Character;
	}

	void AddRank(FGridCharacterInventoryState& Character, FName SkillId, int32 Rank)
	{
		FRPGSkillRank Entry;
		Entry.SkillId = SkillId;
		Entry.Rank = Rank;
		Character.SkillRanks.Add(Entry);
	}

	const URPGSkillAsset* Resolve(const TMap<FName, URPGSkillAsset*>& Definitions, FName SkillId)
	{
		URPGSkillAsset* const* Found = Definitions.Find(SkillId);
		return Found ? *Found : nullptr;
	}

	FRPGCharacterSkillSaveState MakeSentinelSnapshot()
	{
		FRPGCharacterSkillSaveState Sentinel;
		Sentinel.CharacterId = MakeId(999);
		FRPGSkillRankSaveState Rank;
		Rank.SkillId = TEXT("Sentinel");
		Rank.Rank = 1;
		Sentinel.SkillRanks.Add(Rank);
		return Sentinel;
	}
}

using namespace RPGMON2092Tests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2092CaptureActivePoolSparseTest, "Grimrock.MON20.9.SkillPersistence.CaptureActivePoolSparse",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2092CaptureActivePoolSparseTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	TMap<FName, URPGSkillAsset*> Definitions;
	Definitions.Add(TEXT("Skill_A"), MakeSkill(TEXT("Skill_A"), 5));
	Definitions.Add(TEXT("Skill_B"), MakeSkill(TEXT("Skill_B"), 5));
	Definitions.Add(TEXT("Skill_C"), MakeSkill(TEXT("Skill_C"), 5));

	FGridPartyInventoryState Party;
	FGridCharacterInventoryState Active = MakeCharacter(MakeId(1));
	AddRank(Active, TEXT("Skill_B"), 2);
	AddRank(Active, TEXT("Skill_A"), 1);
	Party.ActiveCharacters.Add(Active);
	Party.ActiveCharacters.Add(MakeCharacter(MakeId(3)));

	FGridCharacterInventoryState Pooled = MakeCharacter(MakeId(2));
	AddRank(Pooled, TEXT("Skill_C"), 3);
	Party.CharacterPool.Add(Pooled);

	TArray<FRPGCharacterSkillSaveState> Saved;
	FString Error;
	TestTrue(TEXT("Active and pooled Skills capture"),
		FRPGSkillPersistence::CapturePartySkills(
			Party,
			[&Definitions](FName SkillId)
			{
				return Resolve(Definitions, SkillId);
			},
			Saved, Error));
	TestEqual(TEXT("Only trained characters get sparse snapshots"), Saved.Num(), 2);
	if (Saved.Num() != 2)
	{
		return false;
	}

	TestTrue(TEXT("Snapshots sort by CharacterId"), Saved[0].CharacterId == MakeId(1));
	TestTrue(TEXT("Pooled character is captured"), Saved[1].CharacterId == MakeId(2));
	TestEqual(TEXT("Active snapshot contains two ranks"), Saved[0].SkillRanks.Num(), 2);
	if (Saved[0].SkillRanks.Num() == 2)
	{
		TestEqual(TEXT("Skill ranks sort by SkillId"), Saved[0].SkillRanks[0].SkillId, FName(TEXT("Skill_A")));
		TestEqual(TEXT("Second sorted SkillId"), Saved[0].SkillRanks[1].SkillId, FName(TEXT("Skill_B")));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2092CaptureDeterministicOrderTest, "Grimrock.MON20.9.SkillPersistence.CaptureDeterministicOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2092CaptureDeterministicOrderTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	TMap<FName, URPGSkillAsset*> Definitions;
	Definitions.Add(TEXT("Skill_A"), MakeSkill(TEXT("Skill_A"), 5));
	Definitions.Add(TEXT("Skill_B"), MakeSkill(TEXT("Skill_B"), 5));

	FGridCharacterInventoryState Character1A = MakeCharacter(MakeId(1));
	AddRank(Character1A, TEXT("Skill_B"), 2);
	AddRank(Character1A, TEXT("Skill_A"), 1);
	FGridCharacterInventoryState Character2A = MakeCharacter(MakeId(2));
	AddRank(Character2A, TEXT("Skill_A"), 4);

	FGridPartyInventoryState PartyA;
	PartyA.ActiveCharacters = { Character2A, Character1A };

	FGridCharacterInventoryState Character1B = MakeCharacter(MakeId(1));
	AddRank(Character1B, TEXT("Skill_A"), 1);
	AddRank(Character1B, TEXT("Skill_B"), 2);
	FGridCharacterInventoryState Character2B = MakeCharacter(MakeId(2));
	AddRank(Character2B, TEXT("Skill_A"), 4);

	FGridPartyInventoryState PartyB;
	PartyB.ActiveCharacters = { Character1B, Character2B };

	const auto Resolver = [&Definitions](FName SkillId)
	{
		return Resolve(Definitions, SkillId);
	};

	TArray<FRPGCharacterSkillSaveState> SavedA;
	TArray<FRPGCharacterSkillSaveState> SavedB;
	FString Error;
	TestTrue(TEXT("First ordering captures"), FRPGSkillPersistence::CapturePartySkills(PartyA, Resolver, SavedA, Error));
	TestTrue(TEXT("Second ordering captures"), FRPGSkillPersistence::CapturePartySkills(PartyB, Resolver, SavedB, Error));
	TestEqual(TEXT("Snapshot counts match"), SavedA.Num(), SavedB.Num());
	if (SavedA.Num() != SavedB.Num())
	{
		return false;
	}

	for (int32 Index = 0; Index < SavedA.Num(); ++Index)
	{
		TestTrue(TEXT("Character order is deterministic"), SavedA[Index].CharacterId == SavedB[Index].CharacterId);
		TestEqual(TEXT("Rank counts are deterministic"), SavedA[Index].SkillRanks.Num(), SavedB[Index].SkillRanks.Num());
		for (int32 RankIndex = 0; RankIndex < SavedA[Index].SkillRanks.Num() && RankIndex < SavedB[Index].SkillRanks.Num(); ++RankIndex)
		{
			TestEqual(TEXT("SkillId order is deterministic"), SavedA[Index].SkillRanks[RankIndex].SkillId, SavedB[Index].SkillRanks[RankIndex].SkillId);
			TestEqual(TEXT("Rank values are deterministic"), SavedA[Index].SkillRanks[RankIndex].Rank, SavedB[Index].SkillRanks[RankIndex].Rank);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2092CaptureInvalidCharacterAtomicTest, "Grimrock.MON20.9.SkillPersistence.CaptureInvalidCharacterAtomic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2092CaptureInvalidCharacterAtomicTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const FGuid DuplicateId = MakeId(1);
	FGridPartyInventoryState Party;
	Party.ActiveCharacters.Add(MakeCharacter(DuplicateId));
	Party.CharacterPool.Add(MakeCharacter(DuplicateId));

	TArray<FRPGCharacterSkillSaveState> Saved;
	Saved.Add(MakeSentinelSnapshot());
	FString Error;
	TestFalse(TEXT("Duplicate CharacterId is rejected"),
		FRPGSkillPersistence::CapturePartySkills(
			Party,
			[](FName) -> const URPGSkillAsset*
			{
				return nullptr;
			},
			Saved, Error));
	TestEqual(TEXT("Failed capture leaves output untouched"), Saved.Num(), 1);
	TestTrue(TEXT("Sentinel snapshot survives failed capture"), Saved[0].CharacterId == MakeId(999));

	Party = FGridPartyInventoryState();
	Party.ActiveCharacters.Add(MakeCharacter(FGuid()));
	TestFalse(TEXT("Invalid CharacterId is rejected"),
		FRPGSkillPersistence::CapturePartySkills(
			Party,
			[](FName) -> const URPGSkillAsset*
			{
				return nullptr;
			},
			Saved, Error));
	TestEqual(TEXT("Second failed capture is also atomic"), Saved.Num(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2092CaptureInvalidSkillAtomicTest, "Grimrock.MON20.9.SkillPersistence.CaptureInvalidSkillAtomic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2092CaptureInvalidSkillAtomicTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	TMap<FName, URPGSkillAsset*> Definitions;
	Definitions.Add(TEXT("Skill_A"), MakeSkill(TEXT("Skill_A"), 2));
	const auto Resolver = [&Definitions](FName SkillId)
	{
		return Resolve(Definitions, SkillId);
	};

	TArray<FRPGCharacterSkillSaveState> Saved;
	Saved.Add(MakeSentinelSnapshot());
	FString Error;

	FGridPartyInventoryState DuplicateParty;
	FGridCharacterInventoryState DuplicateCharacter = MakeCharacter(MakeId(1));
	AddRank(DuplicateCharacter, TEXT("Skill_A"), 1);
	AddRank(DuplicateCharacter, TEXT("Skill_A"), 2);
	DuplicateParty.ActiveCharacters.Add(DuplicateCharacter);
	TestFalse(TEXT("Duplicate runtime SkillId is rejected"), FRPGSkillPersistence::CapturePartySkills(DuplicateParty, Resolver, Saved, Error));
	TestEqual(TEXT("Duplicate rejection is atomic"), Saved.Num(), 1);

	FGridPartyInventoryState OverRankParty;
	FGridCharacterInventoryState OverRankCharacter = MakeCharacter(MakeId(1));
	AddRank(OverRankCharacter, TEXT("Skill_A"), 3);
	OverRankParty.ActiveCharacters.Add(OverRankCharacter);
	TestFalse(TEXT("Rank above MaxRank is rejected"), FRPGSkillPersistence::CapturePartySkills(OverRankParty, Resolver, Saved, Error));
	TestEqual(TEXT("MaxRank rejection is atomic"), Saved.Num(), 1);

	FGridPartyInventoryState MissingParty;
	FGridCharacterInventoryState MissingCharacter = MakeCharacter(MakeId(1));
	AddRank(MissingCharacter, TEXT("Skill_Missing"), 1);
	MissingParty.ActiveCharacters.Add(MissingCharacter);
	TestFalse(TEXT("Missing canonical definition is rejected"), FRPGSkillPersistence::CapturePartySkills(MissingParty, Resolver, Saved, Error));
	TestEqual(TEXT("Missing-definition rejection is atomic"), Saved.Num(), 1);
	TestTrue(TEXT("Sentinel output remains intact"), Saved[0].CharacterId == MakeId(999));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2092RestoreRoundTripTest, "Grimrock.MON20.9.SkillPersistence.RestoreRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2092RestoreRoundTripTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	TMap<FName, URPGSkillAsset*> Definitions;
	Definitions.Add(TEXT("Skill_A"), MakeSkill(TEXT("Skill_A"), 5));
	Definitions.Add(TEXT("Skill_B"), MakeSkill(TEXT("Skill_B"), 5));
	const auto Resolver = [&Definitions](FName SkillId)
	{
		return Resolve(Definitions, SkillId);
	};

	FGridPartyInventoryState Party;
	FGridCharacterInventoryState Active = MakeCharacter(MakeId(1));
	AddRank(Active, TEXT("Skill_A"), 2);
	Party.ActiveCharacters.Add(Active);
	Party.ActiveCharacters.Add(MakeCharacter(MakeId(3)));
	FGridCharacterInventoryState Pooled = MakeCharacter(MakeId(2));
	AddRank(Pooled, TEXT("Skill_B"), 4);
	Party.CharacterPool.Add(Pooled);

	TArray<FRPGCharacterSkillSaveState> Saved;
	FString Error;
	TestTrue(TEXT("Source state captures"), FRPGSkillPersistence::CapturePartySkills(Party, Resolver, Saved, Error));

	FGridPartyInventoryState Restored = Party;
	Restored.ActiveCharacters[0].SkillRanks.Reset();
	AddRank(Restored.ActiveCharacters[0], TEXT("Skill_B"), 1);
	AddRank(Restored.ActiveCharacters[1], TEXT("Skill_A"), 5);
	Restored.CharacterPool[0].SkillRanks.Reset();

	TestTrue(TEXT("Captured state restores"), FRPGSkillPersistence::RestorePartySkills(Restored, Saved, Resolver, Error));
	TestEqual(TEXT("Active Skill rank is restored"), FRPGSkillService::GetSkillRank(Restored.ActiveCharacters[0], TEXT("Skill_A")), 2);
	TestEqual(TEXT("Unpersisted active character is cleared"), Restored.ActiveCharacters[1].SkillRanks.Num(), 0);
	TestEqual(TEXT("Pooled Skill rank is restored"), FRPGSkillService::GetSkillRank(Restored.CharacterPool[0], TEXT("Skill_B")), 4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2092RestoreByCharacterIdTest, "Grimrock.MON20.9.SkillPersistence.RestoreByCharacterId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2092RestoreByCharacterIdTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	TMap<FName, URPGSkillAsset*> Definitions;
	Definitions.Add(TEXT("Skill_A"), MakeSkill(TEXT("Skill_A"), 5));
	Definitions.Add(TEXT("Skill_B"), MakeSkill(TEXT("Skill_B"), 5));
	const auto Resolver = [&Definitions](FName SkillId)
	{
		return Resolve(Definitions, SkillId);
	};

	FGridPartyInventoryState Party;
	Party.ActiveCharacters.Add(MakeCharacter(MakeId(2)));
	Party.CharacterPool.Add(MakeCharacter(MakeId(1)));

	FRPGCharacterSkillSaveState ForPooled;
	ForPooled.CharacterId = MakeId(1);
	FRPGSkillRankSaveState RankA;
	RankA.SkillId = TEXT("Skill_A");
	RankA.Rank = 1;
	ForPooled.SkillRanks.Add(RankA);

	FRPGCharacterSkillSaveState ForActive;
	ForActive.CharacterId = MakeId(2);
	FRPGSkillRankSaveState RankB;
	RankB.SkillId = TEXT("Skill_B");
	RankB.Rank = 3;
	ForActive.SkillRanks.Add(RankB);

	TArray<FRPGCharacterSkillSaveState> ReversedSnapshots = { ForActive, ForPooled };
	FString Error;
	TestTrue(TEXT("Reversed snapshots restore"), FRPGSkillPersistence::RestorePartySkills(Party, ReversedSnapshots, Resolver, Error));
	TestEqual(TEXT("Active character receives its CharacterId-matched Skill"), FRPGSkillService::GetSkillRank(Party.ActiveCharacters[0], TEXT("Skill_B")), 3);
	TestEqual(TEXT("Pooled character receives its CharacterId-matched Skill"), FRPGSkillService::GetSkillRank(Party.CharacterPool[0], TEXT("Skill_A")), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2092RestoreInvalidSnapshotAtomicTest, "Grimrock.MON20.9.SkillPersistence.RestoreInvalidSnapshotAtomic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2092RestoreInvalidSnapshotAtomicTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	TMap<FName, URPGSkillAsset*> Definitions;
	Definitions.Add(TEXT("Skill_A"), MakeSkill(TEXT("Skill_A"), 2));
	const auto Resolver = [&Definitions](FName SkillId)
	{
		return Resolve(Definitions, SkillId);
	};

	FGridPartyInventoryState Party;
	FGridCharacterInventoryState Character = MakeCharacter(MakeId(1));
	AddRank(Character, TEXT("Skill_A"), 1);
	Party.ActiveCharacters.Add(Character);

	FRPGCharacterSkillSaveState MissingDefinition;
	MissingDefinition.CharacterId = MakeId(1);
	FRPGSkillRankSaveState MissingRank;
	MissingRank.SkillId = TEXT("Skill_Missing");
	MissingRank.Rank = 1;
	MissingDefinition.SkillRanks.Add(MissingRank);

	FString Error;
	TestFalse(TEXT("Missing definition rejects restore"), FRPGSkillPersistence::RestorePartySkills(Party, { MissingDefinition }, Resolver, Error));
	TestEqual(TEXT("Failed restore preserves original runtime rank"), FRPGSkillService::GetSkillRank(Party.ActiveCharacters[0], TEXT("Skill_A")), 1);

	FRPGCharacterSkillSaveState OverRank;
	OverRank.CharacterId = MakeId(1);
	FRPGSkillRankSaveState InvalidRank;
	InvalidRank.SkillId = TEXT("Skill_A");
	InvalidRank.Rank = 3;
	OverRank.SkillRanks.Add(InvalidRank);
	TestFalse(TEXT("Rank above MaxRank rejects restore"), FRPGSkillPersistence::RestorePartySkills(Party, { OverRank }, Resolver, Error));
	TestEqual(TEXT("Second failed restore remains atomic"), FRPGSkillService::GetSkillRank(Party.ActiveCharacters[0], TEXT("Skill_A")), 1);
	return true;
}

#endif
