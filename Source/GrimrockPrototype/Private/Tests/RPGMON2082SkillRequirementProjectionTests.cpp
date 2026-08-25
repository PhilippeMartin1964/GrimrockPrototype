#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "RPG/RPGSkillAsset.h"
#include "RPG/RPGSkillRequirementProjectionService.h"
#include "RPG/RPGSkillService.h"
#include "Runtime/GridInventoryTypes.h"

namespace
{
	URPGSkillAsset* MakeMON2082Skill(UObject* Outer, FName SkillId = TEXT("Skill_Lockpicking"), int32 MaxRank = 5)
	{
		URPGSkillAsset* Skill = NewObject<URPGSkillAsset>(Outer);
		Skill->SkillId = SkillId;
		Skill->DisplayName = FText::FromString(TEXT("Crochetage"));
		Skill->Description = FText::FromString(TEXT("Ouvrir les serrures mécaniques."));
		Skill->GoverningAttribute = ERPGSkillGoverningAttribute::Dexterity;
		Skill->MaxRank = MaxRank;
		Skill->bAllowUntrainedChecks = true;
		return Skill;
	}

	FRPGSkillRequirementGrant MakeMON2082Grant(int32 MinimumRank, FName RequirementId)
	{
		FRPGSkillRequirementGrant Grant;
		Grant.MinimumRank = MinimumRank;
		Grant.GrantedRequirementIds.Add(RequirementId);
		return Grant;
	}

	FGridCharacterInventoryState MakeMON2082Character()
	{
		FGridCharacterInventoryState Character;
		Character.CharacterId = FGuid::NewGuid();
		Character.DisplayName = FText::FromString(TEXT("MON20.8.2 Tester"));
		return Character;
	}

	bool SetMON2082Rank(FGridCharacterInventoryState& Character, URPGSkillAsset* Skill, int32 Rank)
	{
		FRPGSkillMutationResult Result;
		return FRPGSkillService::TrySetSkillRank(Character, Skill, Rank, Result);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2082PrimaryAssetIdentityTest, "Grimrock.MON20.8.SkillRequirements.PrimaryAssetIdentity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2082PrimaryAssetIdentityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	URPGSkillAsset* Skill = MakeMON2082Skill(GetTransientPackage());
	const FPrimaryAssetId AssetId = Skill->GetPrimaryAssetId();

	TestTrue(TEXT("Skill PrimaryAssetId is valid"), AssetId.IsValid());
	TestEqual(TEXT("Primary asset type is RPGSkill"), AssetId.PrimaryAssetType, FPrimaryAssetType(TEXT("RPGSkill")));
	TestEqual(TEXT("Primary asset name is stable SkillId"), AssetId.PrimaryAssetName, Skill->SkillId);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2082RequirementGrantValidationTest, "Grimrock.MON20.8.SkillRequirements.RequirementGrantValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2082RequirementGrantValidationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	URPGSkillAsset* Skill = MakeMON2082Skill(GetTransientPackage());
	Skill->RequirementGrants.Add(MakeMON2082Grant(2, TEXT("Req_Lockpicking_Advanced")));
	TestTrue(TEXT("Valid threshold grant is accepted"), Skill->IsValidDefinition());

	Skill->RequirementGrants[0].MinimumRank = 0;
	TestFalse(TEXT("Rank zero threshold is rejected"), Skill->IsValidDefinition());

	Skill->RequirementGrants[0].MinimumRank = Skill->MaxRank + 1;
	TestFalse(TEXT("Threshold above MaxRank is rejected"), Skill->IsValidDefinition());

	Skill->RequirementGrants[0].MinimumRank = 2;
	Skill->RequirementGrants[0].GrantedRequirementIds[0] = NAME_None;
	TestFalse(TEXT("None requirement id is rejected"), Skill->IsValidDefinition());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2082UntrainedNoRequirementsTest, "Grimrock.MON20.8.SkillRequirements.UntrainedNoRequirements",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2082UntrainedNoRequirementsTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const FGridCharacterInventoryState Character = MakeMON2082Character();
	TSet<FName> Requirements;
	Requirements.Add(TEXT("Req_Preexisting"));
	FString Error;

	TestTrue(TEXT("Empty sparse skill state projects successfully"),
		FRPGSkillRequirementProjectionService::AppendSatisfiedRequirements(
			Character,
			[](FName)
			{
				return static_cast<const URPGSkillAsset*>(nullptr);
			},
			Requirements, Error));
	TestEqual(TEXT("No skill requirement was added"), Requirements.Num(), 1);
	TestTrue(TEXT("Preexisting requirement is preserved"), Requirements.Contains(TEXT("Req_Preexisting")));
	TestTrue(TEXT("Successful projection clears error"), Error.IsEmpty());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2082TrainedSkillIdTest, "Grimrock.MON20.8.SkillRequirements.TrainedSkillId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2082TrainedSkillIdTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridCharacterInventoryState Character = MakeMON2082Character();
	URPGSkillAsset* Skill = MakeMON2082Skill(GetTransientPackage());
	TestTrue(TEXT("Rank one setup succeeds"), SetMON2082Rank(Character, Skill, 1));

	TSet<FName> Requirements;
	FString Error;
	TestTrue(TEXT("Projection succeeds"),
		FRPGSkillRequirementProjectionService::AppendSatisfiedRequirements(
			Character,
			[Skill](FName SkillId)
			{
				return SkillId == Skill->SkillId ? Skill : nullptr;
			},
			Requirements, Error));
	TestTrue(TEXT("Positive rank grants SkillId itself"), Requirements.Contains(Skill->SkillId));
	TestEqual(TEXT("Only SkillId is granted without thresholds"), Requirements.Num(), 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2082ThresholdRequirementsTest, "Grimrock.MON20.8.SkillRequirements.ThresholdRequirements",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2082ThresholdRequirementsTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridCharacterInventoryState Character = MakeMON2082Character();
	URPGSkillAsset* Skill = MakeMON2082Skill(GetTransientPackage());
	Skill->RequirementGrants.Add(MakeMON2082Grant(2, TEXT("Req_Lockpicking_Advanced")));
	Skill->RequirementGrants.Add(MakeMON2082Grant(4, TEXT("Req_Lockpicking_Master")));
	TestTrue(TEXT("Rank three setup succeeds"), SetMON2082Rank(Character, Skill, 3));

	TSet<FName> Requirements;
	FString Error;
	TestTrue(TEXT("Threshold projection succeeds"),
		FRPGSkillRequirementProjectionService::AppendSatisfiedRequirements(
			Character,
			[Skill](FName SkillId)
			{
				return SkillId == Skill->SkillId ? Skill : nullptr;
			},
			Requirements, Error));
	TestTrue(TEXT("SkillId is granted"), Requirements.Contains(Skill->SkillId));
	TestTrue(TEXT("Reached threshold is granted"), Requirements.Contains(TEXT("Req_Lockpicking_Advanced")));
	TestFalse(TEXT("Unreached threshold is not granted"), Requirements.Contains(TEXT("Req_Lockpicking_Master")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2082MultipleSkillsProjectionTest, "Grimrock.MON20.8.SkillRequirements.MultipleSkillsProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2082MultipleSkillsProjectionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridCharacterInventoryState Character = MakeMON2082Character();
	URPGSkillAsset* Lockpicking = MakeMON2082Skill(GetTransientPackage(), TEXT("Skill_Lockpicking"));
	Lockpicking->RequirementGrants.Add(MakeMON2082Grant(2, TEXT("Req_Lockpicking_Advanced")));
	URPGSkillAsset* Perception = MakeMON2082Skill(GetTransientPackage(), TEXT("Skill_Perception"));
	Perception->RequirementGrants.Add(MakeMON2082Grant(3, TEXT("Req_Perception_Expert")));

	TestTrue(TEXT("Lockpicking rank setup succeeds"), SetMON2082Rank(Character, Lockpicking, 2));
	TestTrue(TEXT("Perception rank setup succeeds"), SetMON2082Rank(Character, Perception, 3));

	TMap<FName, const URPGSkillAsset*> Definitions;
	Definitions.Add(Lockpicking->SkillId, Lockpicking);
	Definitions.Add(Perception->SkillId, Perception);

	TSet<FName> Requirements;
	FString Error;
	TestTrue(TEXT("Multiple skill projection succeeds"),
		FRPGSkillRequirementProjectionService::AppendSatisfiedRequirements(
			Character,
			[&Definitions](FName SkillId)
			{
				const URPGSkillAsset* const* Found = Definitions.Find(SkillId);
				return Found ? *Found : nullptr;
			},
			Requirements, Error));
	TestTrue(TEXT("Lockpicking SkillId projected"), Requirements.Contains(Lockpicking->SkillId));
	TestTrue(TEXT("Perception SkillId projected"), Requirements.Contains(Perception->SkillId));
	TestTrue(TEXT("Lockpicking threshold projected"), Requirements.Contains(TEXT("Req_Lockpicking_Advanced")));
	TestTrue(TEXT("Perception threshold projected"), Requirements.Contains(TEXT("Req_Perception_Expert")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2082InvalidRankAtomicTest, "Grimrock.MON20.8.SkillRequirements.InvalidRankAtomic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2082InvalidRankAtomicTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridCharacterInventoryState Character = MakeMON2082Character();
	URPGSkillAsset* Skill = MakeMON2082Skill(GetTransientPackage(), TEXT("Skill_Lockpicking"), 3);
	FRPGSkillRank RuntimeRank;
	RuntimeRank.SkillId = Skill->SkillId;
	RuntimeRank.Rank = 4;
	Character.SkillRanks.Add(RuntimeRank);

	TSet<FName> Requirements;
	Requirements.Add(TEXT("Req_Preexisting"));
	FString Error;
	TestFalse(TEXT("Rank above canonical MaxRank is rejected"),
		FRPGSkillRequirementProjectionService::AppendSatisfiedRequirements(
			Character,
			[Skill](FName SkillId)
			{
				return SkillId == Skill->SkillId ? Skill : nullptr;
			},
			Requirements, Error));
	TestEqual(TEXT("Rejected projection preserves requirement count"), Requirements.Num(), 1);
	TestTrue(TEXT("Rejected projection preserves prior requirement"), Requirements.Contains(TEXT("Req_Preexisting")));
	TestFalse(TEXT("Rejected projection does not leak SkillId"), Requirements.Contains(Skill->SkillId));
	TestFalse(TEXT("Reject provides diagnostic"), Error.IsEmpty());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2082MissingDefinitionAtomicTest, "Grimrock.MON20.8.SkillRequirements.MissingDefinitionAtomic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2082MissingDefinitionAtomicTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridCharacterInventoryState Character = MakeMON2082Character();
	URPGSkillAsset* Skill = MakeMON2082Skill(GetTransientPackage());
	TestTrue(TEXT("Rank setup succeeds"), SetMON2082Rank(Character, Skill, 2));

	TSet<FName> Requirements;
	Requirements.Add(TEXT("Req_Preexisting"));
	FString Error;
	TestFalse(TEXT("Missing canonical definition rejects projection"),
		FRPGSkillRequirementProjectionService::AppendSatisfiedRequirements(
			Character,
			[](FName)
			{
				return static_cast<const URPGSkillAsset*>(nullptr);
			},
			Requirements, Error));
	TestEqual(TEXT("Atomic reject preserves prior set"), Requirements.Num(), 1);
	TestTrue(TEXT("Prior requirement remains present"), Requirements.Contains(TEXT("Req_Preexisting")));
	TestFalse(TEXT("Failed projection adds no SkillId"), Requirements.Contains(Skill->SkillId));
	TestFalse(TEXT("Missing definition provides diagnostic"), Error.IsEmpty());

	return true;
}

#endif
