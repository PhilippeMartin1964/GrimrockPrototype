#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "RPG/RPGSkillAsset.h"
#include "RPG/RPGSkillService.h"
#include "Runtime/GridInventoryTypes.h"

namespace
{
	URPGSkillAsset* MakeMON2062Skill(UObject* Outer, FName SkillId = TEXT("Skill_Lockpicking"), int32 MaxRank = 5)
	{
		URPGSkillAsset* SkillDefinition = NewObject<URPGSkillAsset>(Outer);
		SkillDefinition->SkillId = SkillId;
		SkillDefinition->DisplayName = FText::FromString(TEXT("Crochetage"));
		SkillDefinition->Description = FText::FromString(TEXT("Ouvrir les serrures mécaniques."));
		SkillDefinition->GoverningAttribute = ERPGSkillGoverningAttribute::Dexterity;
		SkillDefinition->MaxRank = MaxRank;
		SkillDefinition->bAllowUntrainedChecks = true;
		return SkillDefinition;
	}

	FGridCharacterInventoryState MakeMON2062Character()
	{
		FGridCharacterInventoryState Character;
		Character.CharacterId = FGuid::NewGuid();
		Character.DisplayName = FText::FromString(TEXT("MON20.6 Tester"));
		return Character;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2062DefinitionValidationTest, "Grimrock.MON20.6.Skills.DefinitionValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2062DefinitionValidationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	URPGSkillAsset* Valid = MakeMON2062Skill(GetTransientPackage());
	TestTrue(TEXT("Valid skill definition is accepted"), Valid->IsValidDefinition());

	URPGSkillAsset* MissingId = MakeMON2062Skill(GetTransientPackage(), NAME_None, 5);
	TestFalse(TEXT("SkillId None is rejected"), MissingId->IsValidDefinition());

	URPGSkillAsset* InvalidMaxRank = MakeMON2062Skill(GetTransientPackage(), TEXT("Skill_Invalid"), 0);
	TestFalse(TEXT("MaxRank zero is rejected"), InvalidMaxRank->IsValidDefinition());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGMON2062DefaultRuntimeStateTest, "Grimrock.MON20.6.Skills.DefaultRuntimeState", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2062DefaultRuntimeStateTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const FGridCharacterInventoryState Character = MakeMON2062Character();

	TestTrue(TEXT("Default skill state is structurally valid"), FRPGSkillService::ValidateSkillState(Character));
	TestEqual(TEXT("Default sparse rank array is empty"), Character.SkillRanks.Num(), 0);
	TestEqual(TEXT("Absent skill resolves to rank zero"), FRPGSkillService::GetSkillRank(Character, TEXT("Skill_Lockpicking")), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGMON2062SetRankTest, "Grimrock.MON20.6.Skills.SetRank", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2062SetRankTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridCharacterInventoryState Character = MakeMON2062Character();
	URPGSkillAsset* Skill = MakeMON2062Skill(GetTransientPackage());
	FRPGSkillMutationResult Result;

	TestTrue(TEXT("Setting rank two succeeds"), FRPGSkillService::TrySetSkillRank(Character, Skill, 2, Result));
	TestTrue(TEXT("Setting a new rank changes state"), Result.bChanged);
	TestEqual(TEXT("Previous rank was zero"), Result.PreviousRank, 0);
	TestEqual(TEXT("New rank is two"), Result.NewRank, 2);
	TestEqual(TEXT("Exactly one sparse entry exists"), Character.SkillRanks.Num(), 1);
	TestEqual(TEXT("Service reads committed rank"), FRPGSkillService::GetSkillRank(Character, Skill->SkillId), 2);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGMON2062IncreaseRankTest, "Grimrock.MON20.6.Skills.IncreaseRank", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2062IncreaseRankTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridCharacterInventoryState Character = MakeMON2062Character();
	URPGSkillAsset* Skill = MakeMON2062Skill(GetTransientPackage());
	FRPGSkillMutationResult Result;

	TestTrue(TEXT("Initial rank succeeds"), FRPGSkillService::TrySetSkillRank(Character, Skill, 1, Result));
	TestTrue(TEXT("Positive increase succeeds"), FRPGSkillService::TryIncreaseSkillRank(Character, Skill, 2, Result));
	TestEqual(TEXT("Increase reports previous rank"), Result.PreviousRank, 1);
	TestEqual(TEXT("Increase reports new rank"), Result.NewRank, 3);
	TestEqual(TEXT("Rank is now three"), FRPGSkillService::GetSkillRank(Character, Skill->SkillId), 3);
	TestEqual(TEXT("Increase does not duplicate sparse entry"), Character.SkillRanks.Num(), 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGMON2062MaxRankAtomicRejectTest, "Grimrock.MON20.6.Skills.MaxRankAtomicReject", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2062MaxRankAtomicRejectTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridCharacterInventoryState Character = MakeMON2062Character();
	URPGSkillAsset* Skill = MakeMON2062Skill(GetTransientPackage());
	FRPGSkillMutationResult Result;

	TestTrue(TEXT("Seed rank succeeds"), FRPGSkillService::TrySetSkillRank(Character, Skill, 2, Result));

	const TArray<FRPGSkillRank> Before = Character.SkillRanks;
	TestFalse(TEXT("Rank above MaxRank is rejected"), FRPGSkillService::TrySetSkillRank(Character, Skill, 6, Result));
	TestEqual(TEXT("Reject reason is RankOutOfRange"), Result.RejectReason, ERPGSkillMutationRejectReason::RankOutOfRange);
	TestEqual(TEXT("Rank remains unchanged after rejection"), FRPGSkillService::GetSkillRank(Character, Skill->SkillId), 2);
	TestEqual(TEXT("Sparse entry count remains unchanged"), Character.SkillRanks.Num(), Before.Num());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2062NegativeRankAtomicRejectTest, "Grimrock.MON20.6.Skills.NegativeRankAtomicReject",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2062NegativeRankAtomicRejectTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridCharacterInventoryState Character = MakeMON2062Character();
	URPGSkillAsset* Skill = MakeMON2062Skill(GetTransientPackage());
	FRPGSkillMutationResult Result;

	TestTrue(TEXT("Seed rank succeeds"), FRPGSkillService::TrySetSkillRank(Character, Skill, 2, Result));
	TestFalse(TEXT("Negative rank is rejected"), FRPGSkillService::TrySetSkillRank(Character, Skill, -1, Result));
	TestEqual(TEXT("Negative rank reject reason"), Result.RejectReason, ERPGSkillMutationRejectReason::RankOutOfRange);
	TestEqual(TEXT("Rank remains two"), FRPGSkillService::GetSkillRank(Character, Skill->SkillId), 2);
	TestEqual(TEXT("No duplicate was introduced"), Character.SkillRanks.Num(), 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2062ZeroRankRemovesSparseEntryTest, "Grimrock.MON20.6.Skills.ZeroRankRemovesSparseEntry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2062ZeroRankRemovesSparseEntryTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridCharacterInventoryState Character = MakeMON2062Character();
	URPGSkillAsset* Skill = MakeMON2062Skill(GetTransientPackage());
	FRPGSkillMutationResult Result;

	TestTrue(TEXT("Seed rank succeeds"), FRPGSkillService::TrySetSkillRank(Character, Skill, 2, Result));
	TestTrue(TEXT("Setting zero succeeds"), FRPGSkillService::TrySetSkillRank(Character, Skill, 0, Result));
	TestTrue(TEXT("Removing sparse entry changes state"), Result.bChanged);
	TestEqual(TEXT("Sparse entry is removed"), Character.SkillRanks.Num(), 0);
	TestEqual(TEXT("Removed skill resolves to zero"), FRPGSkillService::GetSkillRank(Character, Skill->SkillId), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2062DuplicateSkillStateRejectedTest, "Grimrock.MON20.6.Skills.DuplicateSkillStateRejected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2062DuplicateSkillStateRejectedTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridCharacterInventoryState Character = MakeMON2062Character();
	URPGSkillAsset* Skill = MakeMON2062Skill(GetTransientPackage());

	FRPGSkillRank RankOne;
	RankOne.SkillId = Skill->SkillId;
	RankOne.Rank = 1;
	Character.SkillRanks.Add(RankOne);

	FRPGSkillRank RankTwo = RankOne;
	RankTwo.Rank = 2;
	Character.SkillRanks.Add(RankTwo);

	TestFalse(TEXT("Duplicate SkillId makes state invalid"), FRPGSkillService::ValidateSkillState(Character));

	const int32 BeforeCount = Character.SkillRanks.Num();
	FRPGSkillMutationResult Result;
	TestFalse(TEXT("Mutation rejects invalid current state"), FRPGSkillService::TrySetSkillRank(Character, Skill, 3, Result));
	TestEqual(TEXT("Reject reason is InvalidCurrentState"), Result.RejectReason, ERPGSkillMutationRejectReason::InvalidCurrentState);
	TestEqual(TEXT("Rejected mutation preserves duplicate state atomically"), Character.SkillRanks.Num(), BeforeCount);

	return true;
}

#endif
