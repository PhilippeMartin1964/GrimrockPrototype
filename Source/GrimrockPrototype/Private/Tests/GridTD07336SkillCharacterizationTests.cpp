#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "RPG/RPGSkillAsset.h"
#include "RPG/RPGSkillPersistence.h"
#include "RPG/RPGSkillService.h"
#include "Runtime/GridInventoryTypes.h"

namespace GridTD07336Characterization
{
	FGuid MakeId(uint32 Suffix)
	{
		return FGuid(7, 3, 36, Suffix);
	}

	URPGSkillAsset* MakeSkill(FName SkillId, int32 MaxRank = 5)
	{
		URPGSkillAsset* Skill = NewObject<URPGSkillAsset>();
		Skill->SkillId = SkillId;
		Skill->DisplayName = FText::FromName(SkillId);
		Skill->MaxRank = MaxRank;
		return Skill;
	}

	FGridCharacterInventoryState MakeCharacter(const FGuid& CharacterId)
	{
		FGridCharacterInventoryState Character;
		Character.CharacterId = CharacterId;
		Character.Experience = 0;
		Character.Level = 1;
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
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07336RuntimeAuthorityBoundaryTest,
	"Grimrock.TechnicalDebt.TD07_3_3_6.Characterization.RuntimeAuthorityBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07336RuntimeAuthorityBoundaryTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD07336Characterization;

	FGridCharacterInventoryState Character = MakeCharacter(MakeId(1));
	URPGSkillAsset* Skill = MakeSkill(TEXT("Skill_Lockpicking"));

	FRPGSkillMutationResult Mutation;
	TestTrue(TEXT("Skill service writes rank directly into character state"),
		FRPGSkillService::TrySetSkillRank(Character, Skill, 3, Mutation));
	TestTrue(TEXT("Mutation reports changed state"), Mutation.bChanged);
	TestEqual(TEXT("Character owns one sparse rank"), Character.SkillRanks.Num(), 1);
	TestEqual(TEXT("Skill service reads character-owned rank"), FRPGSkillService::GetSkillRank(Character, Skill->SkillId), 3);

	FRPGSkillRank& DirectRank = Character.SkillRanks[0];
	DirectRank.Rank = 4;
	TestEqual(TEXT("Consumers immediately observe direct character-state mutation"), FRPGSkillService::GetSkillRank(Character, Skill->SkillId), 4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07336SparseSnapshotContractTest,
	"Grimrock.TechnicalDebt.TD07_3_3_6.Characterization.SparseSnapshotContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07336SparseSnapshotContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD07336Characterization;

	TMap<FName, URPGSkillAsset*> Definitions;
	Definitions.Add(TEXT("Skill_A"), MakeSkill(TEXT("Skill_A")));
	Definitions.Add(TEXT("Skill_B"), MakeSkill(TEXT("Skill_B")));
	Definitions.Add(TEXT("Skill_C"), MakeSkill(TEXT("Skill_C")));

	FGridPartyInventoryState Party;
	FGridCharacterInventoryState Active = MakeCharacter(MakeId(2));
	AddRank(Active, TEXT("Skill_B"), 2);
	AddRank(Active, TEXT("Skill_A"), 1);
	Party.ActiveCharacters.Add(Active);
	Party.ActiveCharacters.Add(MakeCharacter(MakeId(4)));

	FGridCharacterInventoryState Pooled = MakeCharacter(MakeId(3));
	AddRank(Pooled, TEXT("Skill_C"), 3);
	Party.CharacterPool.Add(Pooled);

	TArray<FRPGCharacterSkillSaveState> Saved;
	FString Error;
	TestTrue(TEXT("Separate Skill snapshot captures active and pooled characters"),
		FRPGSkillPersistence::CapturePartySkills(
			Party,
			[&Definitions](FName SkillId)
			{
				return Resolve(Definitions, SkillId);
			},
			Saved, Error));

	TestEqual(TEXT("Only trained characters receive sparse save records"), Saved.Num(), 2);
	if (Saved.Num() != 2)
	{
		return false;
	}

	TestEqual(TEXT("Snapshot sorts stable CharacterIds"), Saved[0].CharacterId, MakeId(2));
	TestEqual(TEXT("Pooled character participates in persistence"), Saved[1].CharacterId, MakeId(3));
	TestEqual(TEXT("First trained character keeps two ranks"), Saved[0].SkillRanks.Num(), 2);
	if (Saved[0].SkillRanks.Num() == 2)
	{
		TestEqual(TEXT("Snapshot sorts SkillIds deterministically"), Saved[0].SkillRanks[0].SkillId, FName(TEXT("Skill_A")));
		TestEqual(TEXT("Second sorted SkillId follows"), Saved[0].SkillRanks[1].SkillId, FName(TEXT("Skill_B")));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07336RestoreReplacementBoundaryTest,
	"Grimrock.TechnicalDebt.TD07_3_3_6.Characterization.RestoreReplacementBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07336RestoreReplacementBoundaryTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD07336Characterization;

	TMap<FName, URPGSkillAsset*> Definitions;
	Definitions.Add(TEXT("Skill_A"), MakeSkill(TEXT("Skill_A")));
	Definitions.Add(TEXT("Skill_B"), MakeSkill(TEXT("Skill_B")));
	const auto Resolver = [&Definitions](FName SkillId)
	{
		return Resolve(Definitions, SkillId);
	};

	FGridPartyInventoryState Party;
	FGridCharacterInventoryState Trained = MakeCharacter(MakeId(5));
	AddRank(Trained, TEXT("Skill_A"), 2);
	Party.ActiveCharacters.Add(Trained);
	Party.ActiveCharacters.Add(MakeCharacter(MakeId(6)));

	TArray<FRPGCharacterSkillSaveState> Saved;
	FString Error;
	TestTrue(TEXT("Source Skill snapshot captures"), FRPGSkillPersistence::CapturePartySkills(Party, Resolver, Saved, Error));

	AddRank(Party.ActiveCharacters[0], TEXT("Skill_B"), 1);
	AddRank(Party.ActiveCharacters[1], TEXT("Skill_B"), 4);

	TestTrue(TEXT("Snapshot restore atomically replaces runtime SkillRanks"),
		FRPGSkillPersistence::RestorePartySkills(Party, Saved, Resolver, Error));
	TestEqual(TEXT("Persisted rank returns"), FRPGSkillService::GetSkillRank(Party.ActiveCharacters[0], TEXT("Skill_A")), 2);
	TestEqual(TEXT("Non-snapshot stale rank is removed from trained character"), FRPGSkillService::GetSkillRank(Party.ActiveCharacters[0], TEXT("Skill_B")), 0);
	TestEqual(TEXT("Character absent from sparse snapshot is cleared"), Party.ActiveCharacters[1].SkillRanks.Num(), 0);

	FRPGCharacterSkillSaveState Invalid;
	Invalid.CharacterId = MakeId(5);
	FRPGSkillRankSaveState InvalidRank;
	InvalidRank.SkillId = TEXT("Skill_A");
	InvalidRank.Rank = 99;
	Invalid.SkillRanks.Add(InvalidRank);

	TestFalse(TEXT("Invalid restore is rejected"), FRPGSkillPersistence::RestorePartySkills(Party, { Invalid }, Resolver, Error));
	TestEqual(TEXT("Rejected restore preserves prior valid rank"), FRPGSkillService::GetSkillRank(Party.ActiveCharacters[0], TEXT("Skill_A")), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07336SeparatePersistenceMirrorTest,
	"Grimrock.TechnicalDebt.TD07_3_3_6.Characterization.SeparatePersistenceMirror",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07336SeparatePersistenceMirrorTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD07336Characterization;

	TMap<FName, URPGSkillAsset*> Definitions;
	Definitions.Add(TEXT("Skill_A"), MakeSkill(TEXT("Skill_A")));
	const auto Resolver = [&Definitions](FName SkillId)
	{
		return Resolve(Definitions, SkillId);
	};

	FGridPartyInventoryState RuntimeState;
	FGridCharacterInventoryState Character = MakeCharacter(MakeId(7));
	AddRank(Character, TEXT("Skill_A"), 3);
	RuntimeState.ActiveCharacters.Add(Character);

	TArray<FRPGCharacterSkillSaveState> Saved;
	FString Error;
	TestTrue(TEXT("Separate Skill mirror captures character-owned state"),
		FRPGSkillPersistence::CapturePartySkills(RuntimeState, Resolver, Saved, Error));

	FGridPartyInventoryState OrdinaryCopy = RuntimeState;
	OrdinaryCopy.ActiveCharacters[0].SkillRanks.Reset();
	TestEqual(TEXT("Without separate restore the cleared ordinary copy has no rank"),
		FRPGSkillService::GetSkillRank(OrdinaryCopy.ActiveCharacters[0], TEXT("Skill_A")), 0);

	TestTrue(TEXT("Separate Skill mirror restores the rank"),
		FRPGSkillPersistence::RestorePartySkills(OrdinaryCopy, Saved, Resolver, Error));
	TestEqual(TEXT("Consumer reads restored rank directly from character state"),
		FRPGSkillService::GetSkillRank(OrdinaryCopy.ActiveCharacters[0], TEXT("Skill_A")), 3);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
