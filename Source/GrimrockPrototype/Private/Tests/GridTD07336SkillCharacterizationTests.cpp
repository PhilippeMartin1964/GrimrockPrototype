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
	TestEqual(TEXT("Skill service reads character-owned rank"), FRPGSkillService::GetSkillRank(Character, Skill->SkillId), 3);
	Character.SkillRanks[0].Rank = 4;
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

	FGridCharacterInventoryState Character = MakeCharacter(MakeId(2));
	URPGSkillAsset* SkillB = MakeSkill(TEXT("Skill_B"));
	URPGSkillAsset* SkillA = MakeSkill(TEXT("Skill_A"));
	FRPGSkillMutationResult Mutation;
	TestTrue(TEXT("Skill B mutation succeeds"), FRPGSkillService::TrySetSkillRank(Character, SkillB, 2, Mutation));
	TestTrue(TEXT("Skill A mutation succeeds"), FRPGSkillService::TrySetSkillRank(Character, SkillA, 1, Mutation));
	TestEqual(TEXT("Durable sparse state contains two positive ranks"), Character.SkillRanks.Num(), 2);
	if (Character.SkillRanks.Num() == 2)
	{
		TestEqual(TEXT("Durable state preserves deterministic SkillId ordering"), Character.SkillRanks[0].SkillId, FName(TEXT("Skill_A")));
		TestEqual(TEXT("Second deterministic SkillId"), Character.SkillRanks[1].SkillId, FName(TEXT("Skill_B")));
	}
	TestTrue(TEXT("Setting zero removes sparse entry"), FRPGSkillService::TrySetSkillRank(Character, SkillA, 0, Mutation));
	TestEqual(TEXT("Zero rank remains represented by absence"), Character.SkillRanks.Num(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07336RestoreReplacementBoundaryTest,
	"Grimrock.TechnicalDebt.TD07_3_3_6.Characterization.RestoreReplacementBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07336RestoreReplacementBoundaryTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD07336Characterization;

	FGridPartyInventoryState Source;
	FGridCharacterInventoryState Trained = MakeCharacter(MakeId(5));
	AddRank(Trained, TEXT("Skill_A"), 2);
	Source.ActiveCharacters.Add(Trained);
	Source.ActiveCharacters.Add(MakeCharacter(MakeId(6)));

	FGridPartyInventoryState Restored = Source;
	AddRank(Restored.ActiveCharacters[0], TEXT("Skill_B"), 1);
	AddRank(Restored.ActiveCharacters[1], TEXT("Skill_B"), 4);
	Restored = Source;
	TestEqual(TEXT("Whole-party restore replaces stale rank state"), FRPGSkillService::GetSkillRank(Restored.ActiveCharacters[0], TEXT("Skill_B")), 0);
	TestEqual(TEXT("Untrained character remains empty"), Restored.ActiveCharacters[1].SkillRanks.Num(), 0);
	TestEqual(TEXT("Persisted durable rank remains"), FRPGSkillService::GetSkillRank(Restored.ActiveCharacters[0], TEXT("Skill_A")), 2);
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
	FGridPartyInventoryState State;
	FGridCharacterInventoryState Character = MakeCharacter(MakeId(7));
	AddRank(Character, TEXT("Skill_A"), 3);
	State.ActiveCharacters.Add(Character);

	FString Error;
	TestTrue(TEXT("Character-owned Skill state validates directly"),
		FRPGSkillPersistence::ValidatePartySkills(
			State, [&Definitions](FName SkillId) { return Resolve(Definitions, SkillId); }, Error));
	const FGridPartyInventoryState OrdinaryCopy = State;
	TestEqual(TEXT("Ordinary party-state copy retains the Skill without a separate mirror"),
		FRPGSkillService::GetSkillRank(OrdinaryCopy.ActiveCharacters[0], TEXT("Skill_A")), 3);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
