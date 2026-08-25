#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "RPG/RPGSkillAsset.h"
#include "RPG/RPGSkillCheckService.h"
#include "RPG/RPGSkillRuntimeService.h"
#include "Runtime/GridPartyInventoryComponent.h"

namespace RPGMON2064SkillRuntimeAccessTests
{
	const FName LockpickingId = TEXT("Skill_Lockpicking");

	URPGSkillAsset* MakeSkill()
	{
		URPGSkillAsset* Skill = NewObject<URPGSkillAsset>();
		Skill->SkillId = LockpickingId;
		Skill->DisplayName = FText::FromString(TEXT("Crochetage"));
		Skill->GoverningAttribute = ERPGSkillGoverningAttribute::Dexterity;
		Skill->MaxRank = 5;
		Skill->bAllowUntrainedChecks = true;
		return Skill;
	}

	FGridCharacterInventoryState MakeCharacter(const TCHAR* Name, int32 Dexterity, int32 Rank)
	{
		FGridCharacterInventoryState Character;
		Character.CharacterId = FGuid::NewGuid();
		Character.DisplayName = FText::FromString(Name);
		Character.Attributes = FRPGAttributes(10, Dexterity, 10, 10, 10, 10);
		if (Rank > 0)
		{
			FRPGSkillRank SkillRank;
			SkillRank.SkillId = LockpickingId;
			SkillRank.Rank = Rank;
			Character.SkillRanks.Add(SkillRank);
		}
		return Character;
	}

	UGridPartyInventoryComponent* MakeParty()
	{
		UGridPartyInventoryComponent* Party = NewObject<UGridPartyInventoryComponent>();
		Party->PartyInventoryState = FGridPartyInventoryState();
		Party->PartyInventoryState.ActiveCharacters.Add(MakeCharacter(TEXT("A"), 10, 1));
		Party->PartyInventoryState.ActiveCharacters.Add(MakeCharacter(TEXT("B"), 16, 4));
		Party->PartyInventoryState.ActiveEquipment.SetNum(2);
		Party->PartyInventoryState.SelectedCharacterIndex = 0;
		return Party;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGMON2064RuntimeExplicitRankTest, "Grimrock.MON20.6.Skills.RuntimeExplicitRank", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2064RuntimeExplicitRankTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPGMON2064SkillRuntimeAccessTests;

	UGridPartyInventoryComponent* Party = MakeParty();
	int32 Rank = INDEX_NONE;
	TestTrue(TEXT("Explicit character rank query succeeds"), FRPGSkillRuntimeService::TryGetCharacterSkillRank(Party, 1, LockpickingId, Rank));
	TestEqual(TEXT("Character 1 rank is returned"), Rank, 4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGMON2064RuntimeSelectedRankTest, "Grimrock.MON20.6.Skills.RuntimeSelectedRank", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2064RuntimeSelectedRankTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPGMON2064SkillRuntimeAccessTests;

	UGridPartyInventoryComponent* Party = MakeParty();
	int32 Rank = INDEX_NONE;

	TestTrue(TEXT("Select character 1"), Party->SetSelectedCharacterIndex(1));
	TestTrue(TEXT("Selected rank query succeeds for character 1"), FRPGSkillRuntimeService::TryGetSelectedCharacterSkillRank(Party, LockpickingId, Rank));
	TestEqual(TEXT("Selected character 1 rank"), Rank, 4);

	TestTrue(TEXT("Select character 0"), Party->SetSelectedCharacterIndex(0));
	TestTrue(TEXT("Selected rank query follows selection"), FRPGSkillRuntimeService::TryGetSelectedCharacterSkillRank(Party, LockpickingId, Rank));
	TestEqual(TEXT("Selected character 0 rank"), Rank, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2064RuntimeSelectedMutationTest, "Grimrock.MON20.6.Skills.RuntimeSelectedMutation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2064RuntimeSelectedMutationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPGMON2064SkillRuntimeAccessTests;

	UGridPartyInventoryComponent* Party = MakeParty();
	URPGSkillAsset* Skill = MakeSkill();
	TestTrue(TEXT("Select character 1"), Party->SetSelectedCharacterIndex(1));

	FRPGSkillMutationResult Result;
	TestTrue(TEXT("Selected mutation succeeds"), FRPGSkillRuntimeService::TrySetSelectedCharacterSkillRank(Party, Skill, 2, Result));
	TestTrue(TEXT("Mutation reports a change"), Result.bChanged);
	TestEqual(TEXT("Previous selected rank"), Result.PreviousRank, 4);
	TestEqual(TEXT("New selected rank"), Result.NewRank, 2);
	TestEqual(TEXT("Selected character changed"), Party->PartyInventoryState.ActiveCharacters[1].SkillRanks[0].Rank, 2);
	TestEqual(TEXT("Unselected character unchanged"), Party->PartyInventoryState.ActiveCharacters[0].SkillRanks[0].Rank, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2064RuntimeIncreaseMutationTest, "Grimrock.MON20.6.Skills.RuntimeIncreaseMutation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2064RuntimeIncreaseMutationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPGMON2064SkillRuntimeAccessTests;

	UGridPartyInventoryComponent* Party = MakeParty();
	URPGSkillAsset* Skill = MakeSkill();

	FRPGSkillMutationResult Result;
	TestTrue(TEXT("Explicit increase succeeds"), FRPGSkillRuntimeService::TryIncreaseCharacterSkillRank(Party, 0, Skill, 2, Result));
	TestEqual(TEXT("Previous rank"), Result.PreviousRank, 1);
	TestEqual(TEXT("Increased rank"), Result.NewRank, 3);

	int32 Rank = 0;
	TestTrue(TEXT("Updated rank remains queryable"), FRPGSkillRuntimeService::TryGetCharacterSkillRank(Party, 0, LockpickingId, Rank));
	TestEqual(TEXT("Runtime rank is three"), Rank, 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2064RuntimeInvalidMutationAtomicTest, "Grimrock.MON20.6.Skills.RuntimeInvalidMutationAtomic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2064RuntimeInvalidMutationAtomicTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPGMON2064SkillRuntimeAccessTests;

	UGridPartyInventoryComponent* Party = MakeParty();
	URPGSkillAsset* Skill = MakeSkill();
	const int32 RankBefore = Party->PartyInventoryState.ActiveCharacters[0].SkillRanks[0].Rank;

	FRPGSkillMutationResult Result;
	TestFalse(TEXT("Invalid character mutation is rejected"), FRPGSkillRuntimeService::TrySetCharacterSkillRank(Party, 99, Skill, 5, Result));
	TestTrue(TEXT("Reject reason uses invalid current state"), Result.RejectReason == ERPGSkillMutationRejectReason::InvalidCurrentState);
	TestEqual(TEXT("Existing character rank is unchanged"), Party->PartyInventoryState.ActiveCharacters[0].SkillRanks[0].Rank, RankBefore);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2064RuntimeExplicitCheckTest, "Grimrock.MON20.6.Skills.RuntimeExplicitCheck",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2064RuntimeExplicitCheckTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPGMON2064SkillRuntimeAccessTests;

	UGridPartyInventoryComponent* Party = MakeParty();
	URPGSkillAsset* Skill = MakeSkill();
	FRandomStream DirectStream(2064);
	FRandomStream RuntimeStream(2064);
	FRPGSkillCheckResult DirectResult;
	FRPGSkillCheckResult RuntimeResult;

	TestTrue(TEXT("Direct service resolves"),
		FRPGSkillCheckService::TryResolveSkillCheck(Party->PartyInventoryState.ActiveCharacters[0], Skill, 12, DirectStream, DirectResult));
	TestTrue(
		TEXT("Runtime explicit service resolves"), FRPGSkillRuntimeService::TryResolveCharacterSkillCheck(Party, 0, Skill, 12, RuntimeStream, RuntimeResult));
	TestEqual(TEXT("Runtime roll matches direct roll"), RuntimeResult.Roll, DirectResult.Roll);
	TestEqual(TEXT("Runtime total matches direct total"), RuntimeResult.Total, DirectResult.Total);
	TestEqual(TEXT("Runtime rank matches direct rank"), RuntimeResult.Rank, DirectResult.Rank);
	TestTrue(TEXT("Runtime success matches direct success"), RuntimeResult.bSuccess == DirectResult.bSuccess);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2064RuntimeSelectedCheckTest, "Grimrock.MON20.6.Skills.RuntimeSelectedCheck",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2064RuntimeSelectedCheckTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPGMON2064SkillRuntimeAccessTests;

	UGridPartyInventoryComponent* Party = MakeParty();
	URPGSkillAsset* Skill = MakeSkill();
	TestTrue(TEXT("Select character 1"), Party->SetSelectedCharacterIndex(1));

	FRandomStream DirectStream(6402);
	FRandomStream SelectedStream(6402);
	FRPGSkillCheckResult DirectResult;
	FRPGSkillCheckResult SelectedResult;

	TestTrue(TEXT("Direct selected character check resolves"),
		FRPGSkillCheckService::TryResolveSkillCheck(Party->PartyInventoryState.ActiveCharacters[1], Skill, 18, DirectStream, DirectResult));
	TestTrue(TEXT("Selected runtime check resolves"),
		FRPGSkillRuntimeService::TryResolveSelectedCharacterSkillCheck(Party, Skill, 18, SelectedStream, SelectedResult));
	TestEqual(TEXT("Selected rank is character 1 rank"), SelectedResult.Rank, 4);
	TestEqual(TEXT("Selected attribute value"), SelectedResult.AttributeValue, 16);
	TestEqual(TEXT("Selected roll matches direct"), SelectedResult.Roll, DirectResult.Roll);
	TestEqual(TEXT("Selected total matches direct"), SelectedResult.Total, DirectResult.Total);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2064RuntimeInvalidCharacterCheckNoRandomTest, "Grimrock.MON20.6.Skills.RuntimeInvalidCharacterCheckNoRandom",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2064RuntimeInvalidCharacterCheckNoRandomTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPGMON2064SkillRuntimeAccessTests;

	UGridPartyInventoryComponent* Party = MakeParty();
	URPGSkillAsset* Skill = MakeSkill();
	FRandomStream BaselineStream(99);
	FRandomStream RuntimeStream(99);
	const int32 ExpectedNextRoll = BaselineStream.RandRange(1, 20);

	FRPGSkillCheckResult Result;
	TestFalse(TEXT("Invalid character check is rejected"), FRPGSkillRuntimeService::TryResolveCharacterSkillCheck(Party, 99, Skill, 10, RuntimeStream, Result));
	TestTrue(TEXT("Invalid character reject reason"), Result.RejectReason == ERPGSkillCheckRejectReason::InvalidCharacterState);
	TestEqual(TEXT("Invalid character does not consume RNG"), RuntimeStream.RandRange(1, 20), ExpectedNextRoll);
	return true;
}

#endif
