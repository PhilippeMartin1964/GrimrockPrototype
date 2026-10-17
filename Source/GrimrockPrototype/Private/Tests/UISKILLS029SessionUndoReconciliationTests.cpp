#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "UI/GridSkillsWidgetSessionUndoInternal.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUISKILLS029SessionUndoReconciliationTest,
	"Grimrock.UI.Skills02.SessionUndo.Reconciliation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUISKILLS029SessionUndoReconciliationTest::RunTest(const FString&)
{
	const FString SkillA(TEXT("CharacterA|Skill_A"));
	const FString SkillB(TEXT("CharacterA|Skill_B"));

	TMap<FString, int32> PurchaseCounts;
	TMap<FString, int32> RankFloors;
	PurchaseCounts.Add(SkillA, 2);
	RankFloors.Add(SkillA, 1);
	PurchaseCounts.Add(SkillB, 1);
	RankFloors.Add(SkillB, 0);

	TestFalse(
		TEXT("Coherent session history is preserved"),
		GridSkillsWidgetSessionUndo::ReconcileHistory(
			SkillA, 3, false, PurchaseCounts, RankFloors));
	TestEqual(TEXT("Coherent purchase count remains two"), PurchaseCounts.FindRef(SkillA), 2);
	TestEqual(TEXT("Coherent floor remains one"), RankFloors.FindRef(SkillA), 1);

	TestFalse(
		TEXT("Own synchronous transaction broadcast does not reconcile prematurely"),
		GridSkillsWidgetSessionUndo::ReconcileHistory(
			SkillA, 4, true, PurchaseCounts, RankFloors));
	TestEqual(TEXT("Suppressed reconciliation preserves purchase count"), PurchaseCounts.FindRef(SkillA), 2);
	TestEqual(TEXT("Suppressed reconciliation preserves floor"), RankFloors.FindRef(SkillA), 1);

	TestTrue(
		TEXT("External authoritative rank increase invalidates only that Skill history"),
		GridSkillsWidgetSessionUndo::ReconcileHistory(
			SkillA, 4, false, PurchaseCounts, RankFloors));
	TestFalse(TEXT("Invalidated Skill purchase count is removed"), PurchaseCounts.Contains(SkillA));
	TestFalse(TEXT("Invalidated Skill floor is removed"), RankFloors.Contains(SkillA));
	TestEqual(TEXT("Other Skill purchase history is preserved"), PurchaseCounts.FindRef(SkillB), 1);
	TestEqual(TEXT("Other Skill floor is preserved"), RankFloors.FindRef(SkillB), 0);

	PurchaseCounts.Add(SkillA, 2);
	RankFloors.Add(SkillA, 1);
	TestTrue(
		TEXT("External authoritative rank decrease invalidates tracked history"),
		GridSkillsWidgetSessionUndo::ReconcileHistory(
			SkillA, 2, false, PurchaseCounts, RankFloors));
	TestFalse(TEXT("Externally decreased Skill purchase count is removed"), PurchaseCounts.Contains(SkillA));
	TestFalse(TEXT("Externally decreased Skill floor is removed"), RankFloors.Contains(SkillA));

	PurchaseCounts.Add(SkillA, 1);
	TestTrue(
		TEXT("Partial session history is invalidated defensively"),
		GridSkillsWidgetSessionUndo::ReconcileHistory(
			SkillA, 1, false, PurchaseCounts, RankFloors));
	TestFalse(TEXT("Partial purchase history is removed"), PurchaseCounts.Contains(SkillA));
	TestFalse(TEXT("Partial floor history remains absent"), RankFloors.Contains(SkillA));

	return true;
}

#endif
