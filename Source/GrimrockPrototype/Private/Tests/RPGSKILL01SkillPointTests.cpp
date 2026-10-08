#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "RPG/RPGSkillAsset.h"
#include "RPG/RPGSkillPointService.h"
#include "RPG/RPGSkillService.h"
#include "RPGMON155TestHelpers.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "UI/GridSkillEntryWidget.h"
#include "UI/GridSkillsPageService.h"
#include "UI/GridSkillsWidget.h"
#include "UObject/UnrealType.h"

namespace RPGSKILL01Tests
{
	URPGSkillAsset* MakeSkill(UObject* Outer, FName SkillId, const TCHAR* DisplayName = TEXT("Compétence"))
	{
		URPGSkillAsset* Skill = NewObject<URPGSkillAsset>(Outer);
		Skill->SkillId = SkillId;
		Skill->DisplayName = FText::FromString(DisplayName);
		Skill->Description = FText::FromString(TEXT("Compétence RPG-SKILL01."));
		Skill->GoverningAttribute = ERPGSkillGoverningAttribute::Dexterity;
		Skill->MaxRank = 5;
		Skill->bAllowUntrainedChecks = true;
		return Skill;
	}

	void SetRank(FGridCharacterInventoryState& Character, URPGSkillAsset* Skill, int32 Rank)
	{
		FRPGSkillMutationResult Mutation;
		check(FRPGSkillService::TrySetSkillRank(Character, Skill, Rank, Mutation));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGSKILL01GrantScheduleTest,
	"Grimrock.RPG.SKILL01.Economy.GrantScheduleAndRankCaps",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGSKILL01GrantScheduleTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	TestEqual(TEXT("Level 1 grants four Skill Points"), FRPGSkillPointService::GetTotalPointsGranted(1), 4);
	TestEqual(TEXT("Level 20 grants twenty-three Skill Points"), FRPGSkillPointService::GetTotalPointsGranted(20), 23);
	TestEqual(TEXT("Level 4 rank cap"), FRPGSkillPointService::GetRankCapForLevel(4), 2);
	TestEqual(TEXT("Level 5 opens rank three"), FRPGSkillPointService::GetRankCapForLevel(5), 3);
	TestEqual(TEXT("Level 10 opens rank four"), FRPGSkillPointService::GetRankCapForLevel(10), 4);
	TestEqual(TEXT("Level 15 opens rank five"), FRPGSkillPointService::GetRankCapForLevel(15), 5);
	TestEqual(TEXT("Invalid level grants no points"), FRPGSkillPointService::GetTotalPointsGranted(21), 0);
	TestEqual(TEXT("Invalid level has no cap"), FRPGSkillPointService::GetRankCapForLevel(0), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGSKILL01DerivedBalanceTest,
	"Grimrock.RPG.SKILL01.Economy.DerivedBalance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGSKILL01DerivedBalanceTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPGSKILL01Tests;

	FGridCharacterInventoryState Character;
	Character.Level = 5;
	URPGSkillAsset* A = MakeSkill(GetTransientPackage(), TEXT("Skill_A"));
	URPGSkillAsset* B = MakeSkill(GetTransientPackage(), TEXT("Skill_B"));
	SetRank(Character, A, 2);
	SetRank(Character, B, 1);

	FRPGSkillPointBalance Balance;
	TestTrue(TEXT("Balance derives from Level plus sparse ranks"), FRPGSkillPointService::TryGetBalance(Character, Balance));
	TestEqual(TEXT("Level five granted points"), Balance.GrantedPoints, 8);
	TestEqual(TEXT("Spent points equal sum of ranks"), Balance.SpentPoints, 3);
	TestEqual(TEXT("Unspent points are derived"), Balance.RemainingPoints, 5);
	TestEqual(TEXT("Level five current rank cap"), Balance.RankCap, 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGSKILL01PurchaseTest,
	"Grimrock.RPG.SKILL01.Allocation.PurchaseAndLevelCap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGSKILL01PurchaseTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPGSKILL01Tests;

	UGridPartyInventoryComponent* Party = NewObject<UGridPartyInventoryComponent>();
	FGridCharacterInventoryState Character;
	Character.CharacterId = FGuid::NewGuid();
	Character.Level = 1;
	Party->PartyInventoryState.ActiveCharacters.Add(Character);
	Party->PartyInventoryState.ActiveEquipment.SetNum(1);
	Party->PartyInventoryState.SelectedCharacterIndex = 0;

	URPGSkillAsset* Skill = MakeSkill(Party, TEXT("Skill_Test"), TEXT("Test"));
	FRPGSkillPointMutationResult Result;

	TestTrue(TEXT("Rank zero to one spends one point"), FRPGSkillPointService::TryPurchaseNextRank(Party, 0, Skill, Result));
	TestTrue(TEXT("Transaction reports committed"), Result.bCommitted);
	TestEqual(TEXT("First purchase reaches rank one"), Result.NewRank, 1);
	TestEqual(TEXT("Three points remain"), Result.RemainingPoints, 3);

	TestTrue(TEXT("Rank one to two spends another point"), FRPGSkillPointService::TryPurchaseNextRank(Party, 0, Skill, Result));
	TestEqual(TEXT("Second purchase reaches level-one cap"), Result.NewRank, 2);
	TestEqual(TEXT("Two points remain"), Result.RemainingPoints, 2);

	TestFalse(TEXT("Third purchase is blocked by level rank cap"), FRPGSkillPointService::TryPurchaseNextRank(Party, 0, Skill, Result));
	TestTrue(TEXT("Reject reason is level cap"), Result.RejectReason == ERPGSkillPointMutationRejectReason::LevelRankCapReached);
	TestEqual(TEXT("Rejected purchase preserves rank two"),
		FRPGSkillService::GetSkillRank(Party->PartyInventoryState.ActiveCharacters[0], Skill->SkillId), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGSKILL01NoPointsTest,
	"Grimrock.RPG.SKILL01.Allocation.RejectNoPoints",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGSKILL01NoPointsTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPGSKILL01Tests;

	UGridPartyInventoryComponent* Party = NewObject<UGridPartyInventoryComponent>();
	FGridCharacterInventoryState Character;
	Character.CharacterId = FGuid::NewGuid();
	Character.Level = 1;
	Party->PartyInventoryState.ActiveCharacters.Add(Character);
	Party->PartyInventoryState.ActiveEquipment.SetNum(1);

	URPGSkillAsset* A = MakeSkill(Party, TEXT("Skill_A"));
	URPGSkillAsset* B = MakeSkill(Party, TEXT("Skill_B"));
	URPGSkillAsset* C = MakeSkill(Party, TEXT("Skill_C"));
	SetRank(Party->PartyInventoryState.ActiveCharacters[0], A, 2);
	SetRank(Party->PartyInventoryState.ActiveCharacters[0], B, 2);

	FRPGSkillPointMutationResult Result;
	TestFalse(TEXT("All four level-one points being spent blocks another purchase"),
		FRPGSkillPointService::TryPurchaseNextRank(Party, 0, C, Result));
	TestTrue(TEXT("Reject reason is no Skill Points"), Result.RejectReason == ERPGSkillPointMutationRejectReason::NoSkillPoints);
	TestEqual(TEXT("No rank was invented"), FRPGSkillService::GetSkillRank(Party->PartyInventoryState.ActiveCharacters[0], C->SkillId), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGSKILL01ReadModelTest,
	"Grimrock.RPG.SKILL01.UI.ReadModelAllocation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGSKILL01ReadModelTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPGSKILL01Tests;
	FMON155RuntimeStateGuard Guard;

	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* Party = MakeMON155Inventory(5, 10000, ClassDefinition);
	FGridCharacterInventoryState& Character = Party->PartyInventoryState.ActiveCharacters[0];

	URPGSkillAsset* Ranked = MakeSkill(Party, TEXT("Skill_Ranked"), TEXT("Ranked"));
	URPGSkillAsset* Open = MakeSkill(Party, TEXT("Skill_Open"), TEXT("Open"));
	SetRank(Character, Ranked, 3);

	FGridSkillsPageView View;
	TestTrue(TEXT("Skills page builds with Skill economy"),
		FGridSkillsPageService::TryBuildCharacterView(Party, 0, { Ranked, Open }, View));
	TestEqual(TEXT("Level five page grants eight points"), View.GrantedSkillPoints, 8);
	TestEqual(TEXT("Three points are spent"), View.SpentSkillPoints, 3);
	TestEqual(TEXT("Five points remain"), View.RemainingSkillPoints, 5);
	TestEqual(TEXT("Current page rank cap is three"), View.SkillRankCap, 3);

	const FGridSkillEntryView* RankedView = View.Skills.FindByPredicate(
		[](const FGridSkillEntryView& Entry) { return Entry.SkillId == TEXT("Skill_Ranked"); });
	const FGridSkillEntryView* OpenView = View.Skills.FindByPredicate(
		[](const FGridSkillEntryView& Entry) { return Entry.SkillId == TEXT("Skill_Open"); });
	TestNotNull(TEXT("Ranked entry exists"), RankedView);
	TestNotNull(TEXT("Open entry exists"), OpenView);
	if (RankedView)
	{
		TestEqual(TEXT("Entry exposes current cap"), RankedView->CurrentRankCap, 3);
		TestFalse(TEXT("Entry at cap cannot increase"), RankedView->bCanIncreaseRank);
	}
	if (OpenView)
	{
		TestEqual(TEXT("Open entry exposes current cap"), OpenView->CurrentRankCap, 3);
		TestTrue(TEXT("Open entry can spend a point"), OpenView->bCanIncreaseRank);
	}
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGSKILL01SafeUndoRefundTest,
	"Grimrock.RPG.SKILL01.Undo.RefundToSessionFloor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGSKILL01SafeUndoRefundTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPGSKILL01Tests;

	UGridPartyInventoryComponent* Party = NewObject<UGridPartyInventoryComponent>();
	FGridCharacterInventoryState Character;
	Character.CharacterId = FGuid::NewGuid();
	Character.Level = 1;
	Party->PartyInventoryState.ActiveCharacters.Add(Character);
	Party->PartyInventoryState.ActiveEquipment.SetNum(1);

	URPGSkillAsset* Skill = MakeSkill(Party, TEXT("Skill_Undo"), TEXT("Undo"));
	SetRank(Party->PartyInventoryState.ActiveCharacters[0], Skill, 1);

	FRPGSkillPointMutationResult Result;
	TestTrue(TEXT("Session purchase from baseline rank one succeeds"),
		FRPGSkillPointService::TryPurchaseNextRank(Party, 0, Skill, Result));
	TestEqual(TEXT("Purchase reaches rank two"), Result.NewRank, 2);

	TestTrue(TEXT("Purchased rank can be refunded to session floor"),
		FRPGSkillPointService::TryRefundPurchasedRank(Party, 0, Skill, 1, Result));
	TestEqual(TEXT("Refund restores baseline rank one"), Result.NewRank, 1);
	TestEqual(TEXT("Refund restores three available points"), Result.RemainingPoints, 3);
	TestEqual(TEXT("Runtime rank is baseline rank one"),
		FRPGSkillService::GetSkillRank(Party->PartyInventoryState.ActiveCharacters[0], Skill->SkillId), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGSKILL01SafeUndoFloorRejectTest,
	"Grimrock.RPG.SKILL01.Undo.RejectBelowSessionFloor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGSKILL01SafeUndoFloorRejectTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPGSKILL01Tests;

	UGridPartyInventoryComponent* Party = NewObject<UGridPartyInventoryComponent>();
	FGridCharacterInventoryState Character;
	Character.CharacterId = FGuid::NewGuid();
	Character.Level = 1;
	Party->PartyInventoryState.ActiveCharacters.Add(Character);
	Party->PartyInventoryState.ActiveEquipment.SetNum(1);

	URPGSkillAsset* Skill = MakeSkill(Party, TEXT("Skill_UndoFloor"), TEXT("Undo Floor"));
	SetRank(Party->PartyInventoryState.ActiveCharacters[0], Skill, 1);

	FRPGSkillPointMutationResult Result;
	TestFalse(TEXT("Baseline rank cannot be refunded"),
		FRPGSkillPointService::TryRefundPurchasedRank(Party, 0, Skill, 1, Result));
	TestTrue(TEXT("Reject reason is session undo boundary"),
		Result.RejectReason == ERPGSkillPointMutationRejectReason::NoSessionPurchaseToUndo);
	TestEqual(TEXT("Rejected refund preserves baseline rank"),
		FRPGSkillService::GetSkillRank(Party->PartyInventoryState.ActiveCharacters[0], Skill->SkillId), 1);

	TestFalse(TEXT("Floor above current rank is rejected"),
		FRPGSkillPointService::TryRefundPurchasedRank(Party, 0, Skill, 2, Result));
	TestTrue(TEXT("Invalid floor is explicit"),
		Result.RejectReason == ERPGSkillPointMutationRejectReason::InvalidSessionFloor);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGSKILL01SafeUndoUIContractTest,
	"Grimrock.RPG.SKILL01.Undo.UIContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGSKILL01SafeUndoUIContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UClass* EntryClass = UGridSkillEntryWidget::StaticClass();
	TestNotNull(TEXT("Decrease button binding point exists"),
		FindFProperty<FProperty>(EntryClass, FName(TEXT("Button_DecreaseSkill"))));

	UClass* SkillsClass = UGridSkillsWidget::StaticClass();
	TestNotNull(TEXT("Skills widget exposes session reset"),
		SkillsClass->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UGridSkillsWidget, BeginSkillAllocationSession)));
	TestNotNull(TEXT("Skills widget exposes safe decrement transaction"),
		SkillsClass->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UGridSkillsWidget, CommitSkillRankDecrease)));
	return true;
}

#endif
