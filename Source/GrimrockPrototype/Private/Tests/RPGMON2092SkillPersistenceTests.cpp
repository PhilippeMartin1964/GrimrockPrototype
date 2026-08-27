#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "RPG/RPGSkillAsset.h"
#include "RPG/RPGSkillPersistence.h"
#include "RPG/RPGSkillService.h"
#include "Runtime/GridInventoryTypes.h"

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
		Skill->DisplayName = FText::FromName(SkillId);
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

	const FGridCharacterInventoryState* FindCharacter(const FGridPartyInventoryState& State, const FGuid& CharacterId)
	{
		if (const FGridCharacterInventoryState* Active = State.ActiveCharacters.FindByPredicate(
				[&CharacterId](const FGridCharacterInventoryState& Character) { return Character.CharacterId == CharacterId; }))
		{
			return Active;
		}
		return State.CharacterPool.FindByPredicate(
			[&CharacterId](const FGridCharacterInventoryState& Character) { return Character.CharacterId == CharacterId; });
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
	AddRank(Active, TEXT("Skill_A"), 1);
	AddRank(Active, TEXT("Skill_B"), 2);
	Party.ActiveCharacters.Add(Active);
	Party.ActiveCharacters.Add(MakeCharacter(MakeId(3)));
	FGridCharacterInventoryState Pooled = MakeCharacter(MakeId(2));
	AddRank(Pooled, TEXT("Skill_C"), 3);
	Party.CharacterPool.Add(Pooled);

	FString Error;
	TestTrue(TEXT("Durable active and pooled Skill state validates"),
		FRPGSkillPersistence::ValidatePartySkills(
			Party, [&Definitions](FName SkillId) { return Resolve(Definitions, SkillId); }, Error));
	TestEqual(TEXT("Active trained character owns two sparse ranks"), Party.ActiveCharacters[0].SkillRanks.Num(), 2);
	TestEqual(TEXT("Untrained active character owns no sparse ranks"), Party.ActiveCharacters[1].SkillRanks.Num(), 0);
	TestEqual(TEXT("Pooled character owns its durable rank"), Party.CharacterPool[0].SkillRanks.Num(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2092CaptureDeterministicOrderTest, "Grimrock.MON20.9.SkillPersistence.CaptureDeterministicOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2092CaptureDeterministicOrderTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridCharacterInventoryState Character = MakeCharacter(MakeId(1));
	URPGSkillAsset* SkillB = MakeSkill(TEXT("Skill_B"), 5);
	URPGSkillAsset* SkillA = MakeSkill(TEXT("Skill_A"), 5);
	FRPGSkillMutationResult Mutation;
	TestTrue(TEXT("Skill B mutation succeeds"), FRPGSkillService::TrySetSkillRank(Character, SkillB, 2, Mutation));
	TestTrue(TEXT("Skill A mutation succeeds"), FRPGSkillService::TrySetSkillRank(Character, SkillA, 1, Mutation));
	TestEqual(TEXT("Durable rank count"), Character.SkillRanks.Num(), 2);
	if (Character.SkillRanks.Num() == 2)
	{
		TestEqual(TEXT("Durable ranks sort by SkillId"), Character.SkillRanks[0].SkillId, FName(TEXT("Skill_A")));
		TestEqual(TEXT("Second durable SkillId"), Character.SkillRanks[1].SkillId, FName(TEXT("Skill_B")));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2092CaptureInvalidCharacterAtomicTest, "Grimrock.MON20.9.SkillPersistence.CaptureInvalidCharacterAtomic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2092CaptureInvalidCharacterAtomicTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridPartyInventoryState Party;
	Party.ActiveCharacters.Add(MakeCharacter(MakeId(1)));
	Party.CharacterPool.Add(MakeCharacter(MakeId(1)));
	const FGridPartyInventoryState Before = Party;
	FString Error;
	TestFalse(TEXT("Duplicate CharacterId is rejected"),
		FRPGSkillPersistence::ValidatePartySkills(Party, [](FName) -> const URPGSkillAsset* { return nullptr; }, Error));
	TestEqual(TEXT("Validation never mutates active character count"), Party.ActiveCharacters.Num(), Before.ActiveCharacters.Num());
	TestEqual(TEXT("Validation never mutates pool character count"), Party.CharacterPool.Num(), Before.CharacterPool.Num());

	Party = FGridPartyInventoryState();
	Party.ActiveCharacters.Add(MakeCharacter(FGuid()));
	TestFalse(TEXT("Invalid CharacterId is rejected"),
		FRPGSkillPersistence::ValidatePartySkills(Party, [](FName) -> const URPGSkillAsset* { return nullptr; }, Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2092CaptureInvalidSkillAtomicTest, "Grimrock.MON20.9.SkillPersistence.CaptureInvalidSkillAtomic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2092CaptureInvalidSkillAtomicTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	TMap<FName, URPGSkillAsset*> Definitions;
	Definitions.Add(TEXT("Skill_A"), MakeSkill(TEXT("Skill_A"), 2));
	const auto Resolver = [&Definitions](FName SkillId) { return Resolve(Definitions, SkillId); };
	FString Error;

	FGridPartyInventoryState DuplicateParty;
	FGridCharacterInventoryState Duplicate = MakeCharacter(MakeId(1));
	AddRank(Duplicate, TEXT("Skill_A"), 1);
	AddRank(Duplicate, TEXT("Skill_A"), 2);
	DuplicateParty.ActiveCharacters.Add(Duplicate);
	TestFalse(TEXT("Duplicate durable SkillId is rejected"), FRPGSkillPersistence::ValidatePartySkills(DuplicateParty, Resolver, Error));

	FGridPartyInventoryState OverRankParty;
	FGridCharacterInventoryState OverRank = MakeCharacter(MakeId(2));
	AddRank(OverRank, TEXT("Skill_A"), 3);
	OverRankParty.ActiveCharacters.Add(OverRank);
	TestFalse(TEXT("Rank above MaxRank is rejected"), FRPGSkillPersistence::ValidatePartySkills(OverRankParty, Resolver, Error));

	FGridPartyInventoryState MissingParty;
	FGridCharacterInventoryState Missing = MakeCharacter(MakeId(3));
	AddRank(Missing, TEXT("Skill_Missing"), 1);
	MissingParty.ActiveCharacters.Add(Missing);
	TestFalse(TEXT("Missing canonical definition is rejected"), FRPGSkillPersistence::ValidatePartySkills(MissingParty, Resolver, Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2092RestoreRoundTripTest, "Grimrock.MON20.9.SkillPersistence.RestoreRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2092RestoreRoundTripTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridPartyInventoryState Source;
	FGridCharacterInventoryState Active = MakeCharacter(MakeId(1));
	AddRank(Active, TEXT("Skill_A"), 2);
	Source.ActiveCharacters.Add(Active);
	FGridCharacterInventoryState Pool = MakeCharacter(MakeId(2));
	AddRank(Pool, TEXT("Skill_B"), 4);
	Source.CharacterPool.Add(Pool);

	const FGridPartyInventoryState Restored = Source;
	TestEqual(TEXT("Active durable rank survives ordinary party-state copy"), FRPGSkillService::GetSkillRank(Restored.ActiveCharacters[0], TEXT("Skill_A")), 2);
	TestEqual(TEXT("Pooled durable rank survives ordinary party-state copy"), FRPGSkillService::GetSkillRank(Restored.CharacterPool[0], TEXT("Skill_B")), 4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2092RestoreByCharacterIdTest, "Grimrock.MON20.9.SkillPersistence.RestoreByCharacterId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2092RestoreByCharacterIdTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const FGuid CharacterId = MakeId(7);
	FGridPartyInventoryState State;
	FGridCharacterInventoryState Character = MakeCharacter(CharacterId);
	AddRank(Character, TEXT("Skill_A"), 3);
	State.ActiveCharacters.Add(Character);

	FGridCharacterInventoryState Moved = State.ActiveCharacters[0];
	State.ActiveCharacters.Reset();
	State.CharacterPool.Add(Moved);
	const FGridCharacterInventoryState* Found = FindCharacter(State, CharacterId);
	TestNotNull(TEXT("Moved durable character remains resolvable by CharacterId"), Found);
	if (Found)
	{
		TestEqual(TEXT("Skill rank moves with character authority"), FRPGSkillService::GetSkillRank(*Found, TEXT("Skill_A")), 3);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2092RestoreInvalidSnapshotAtomicTest, "Grimrock.MON20.9.SkillPersistence.RestoreInvalidSnapshotAtomic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2092RestoreInvalidSnapshotAtomicTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	TMap<FName, URPGSkillAsset*> Definitions;
	Definitions.Add(TEXT("Skill_A"), MakeSkill(TEXT("Skill_A"), 2));
	const auto Resolver = [&Definitions](FName SkillId) { return Resolve(Definitions, SkillId); };

	FGridPartyInventoryState Valid;
	FGridCharacterInventoryState Character = MakeCharacter(MakeId(1));
	AddRank(Character, TEXT("Skill_A"), 1);
	Valid.ActiveCharacters.Add(Character);

	FGridPartyInventoryState Candidate = Valid;
	Candidate.ActiveCharacters[0].SkillRanks[0].Rank = 3;
	FString Error;
	TestFalse(TEXT("Invalid durable candidate is rejected"), FRPGSkillPersistence::ValidatePartySkills(Candidate, Resolver, Error));
	TestEqual(TEXT("Rejected candidate does not mutate original durable state"), FRPGSkillService::GetSkillRank(Valid.ActiveCharacters[0], TEXT("Skill_A")), 1);
	return true;
}

#endif
