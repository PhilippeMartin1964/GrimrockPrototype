#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Runtime/Combat/GridCombatEventFeedFormatter.h"
#include "Runtime/Combat/GridCombatResolver.h"
#include "UI/GridEventFeedWidget.h"
#include "UI/GridNavigationTooltipWidget.h"

namespace
{
	FGridCombatLogEntry MakeCompactAttackLog(
		const TCHAR* SourceName,
		const TCHAR* TargetName,
		const FGridAttackResult& Result)
	{
		FGridCombatLogEntry Entry;
		Entry.Type = Result.bHit ? EGridCombatLogEntryType::AttackHit : EGridCombatLogEntryType::AttackMiss;
		Entry.SourceId = TEXT("Compact.Source");
		Entry.SourceDisplayName = FText::FromString(SourceName);
		Entry.TargetId = TEXT("Compact.Target");
		Entry.TargetDisplayName = FText::FromString(TargetName);
		Entry.AttackResult = Result;
		return Entry;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridUIEventHUD018PVirtualDiceNotationTest,
	"Grimrock.UI.EventHUD01.CompactCombat.VirtualDiceNotation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIEventHUD018PVirtualDiceNotationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridCombatRollTrace Roll;
	Roll.bUsed = true;
	Roll.Kind = EGridCombatRollTraceKind::Range;

	Roll.Minimum = 1;
	Roll.Maximum = 3;
	Roll.FinalValue = 2;
	TestEqual(
		TEXT("1..3 is displayed as 1d3"),
		FGridCombatEventFeedFormatter::FormatRollAsDice(Roll).ToString(),
		FString(TEXT("1d3 (2)")));

	Roll.Minimum = 2;
	Roll.Maximum = 8;
	Roll.FinalValue = 5;
	TestEqual(
		TEXT("2..8 is displayed as virtual 1d7 plus one"),
		FGridCombatEventFeedFormatter::FormatRollAsDice(Roll).ToString(),
		FString(TEXT("1d7 (4) + 1 = 5")));

	Roll.Minimum = 3;
	Roll.Maximum = 10;
	Roll.FinalValue = 8;
	TestEqual(
		TEXT("3..10 is displayed as 1d8 plus two"),
		FGridCombatEventFeedFormatter::FormatRollAsDice(Roll).ToString(),
		FString(TEXT("1d8 (6) + 2 = 8")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridUIEventHUD018PCriticalSummaryTest,
	"Grimrock.UI.EventHUD01.CompactCombat.CriticalSummary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIEventHUD018PCriticalSummaryTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridAttackSourceStats Source;
	Source.CriticalChancePercent = 5;
	Source.CriticalDamagePercent = 200;

	FGridAttackTargetStats Target;
	Target.Evasion = 1;
	Target.CurrentHealth = 8;

	FGridAttackDefinition Attack;
	Attack.DamageType = EGridDamageType::Physical;
	Attack.PhysicalSubtype = EGridPhysicalDamageSubtype::Slashing;
	Attack.MinDamage = 1;
	Attack.MaxDamage = 3;

	const FGridAttackResult Result =
		FGridCombatResolver::ResolveAttackFromRolls(Source, Target, Attack, 20, 2);

	if (!TestTrue(TEXT("Fixture resolves as critical hit"), Result.bHit && Result.bCriticalHit))
	{
		return false;
	}

	FGridHudEventEntry FeedEntry;
	TestTrue(
		TEXT("Critical hit projects to Event Feed"),
		FGridCombatEventFeedFormatter::BuildEntry(
			MakeCompactAttackLog(TEXT("Daniel"), TEXT("Rat géant"), Result),
			FeedEntry));

	TestEqual(
		TEXT("Critical header is compact"),
		FeedEntry.Header.ToString(),
		FString(TEXT("Daniel attaque Rat géant — COUP CRITIQUE")));

	const FString Expected =
		TEXT("Attaque : 1d20 (20) = 20; Défense : 10 + 1 = 11\n")
		TEXT("Dégâts : 1d3 (2) × 200 % = 4; PV Rat géant : 8 → 4");
	TestEqual(TEXT("Critical result uses the agreed compact two-line resolution"), FeedEntry.Message.ToString(), Expected);
	TestTrue(TEXT("Detailed calculation remains available"), FeedEntry.Details.Num() > 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridUIEventHUD018POffsetDiceSummaryTest,
	"Grimrock.UI.EventHUD01.CompactCombat.OffsetDiceSummary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIEventHUD018POffsetDiceSummaryTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridAttackSourceStats Source;

	FGridAttackTargetStats Target;
	Target.CurrentHealth = 20;

	FGridAttackDefinition Attack;
	Attack.DamageType = EGridDamageType::Physical;
	Attack.PhysicalSubtype = EGridPhysicalDamageSubtype::Slashing;
	Attack.MinDamage = 3;
	Attack.MaxDamage = 10;

	const FGridAttackResult Result =
		FGridCombatResolver::ResolveAttackFromRolls(Source, Target, Attack, 15, 8);

	if (!TestTrue(TEXT("Fixture attack hits"), Result.bHit))
	{
		return false;
	}

	FGridHudEventEntry FeedEntry;
	TestTrue(
		TEXT("Offset-range attack projects"),
		FGridCombatEventFeedFormatter::BuildEntry(
			MakeCompactAttackLog(TEXT("Daniel"), TEXT("Rat géant"), Result),
			FeedEntry));

	TestTrue(
		TEXT("3..10 is visibly represented as 1d8 plus two"),
		FeedEntry.Message.ToString().Contains(TEXT("1d8 (6) + 2 = 8")));
	TestFalse(
		TEXT("Compact damage never exposes Min..Max notation"),
		FeedEntry.Message.ToString().Contains(TEXT("3..10")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridUIEventHUD018PTooltipProjectionTest,
	"Grimrock.UI.EventHUD01.CompactCombat.TooltipProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIEventHUD018PTooltipProjectionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridHudEventEntry Combat;
	Combat.Category = EGridHudEventCategory::Combat;
	Combat.Header = FText::FromString(TEXT("Daniel attaque Rat géant — TOUCHÉ"));
	Combat.Message = FText::FromString(
		TEXT("Attaque : 1d20 (14) + 3 = 17; Défense : 10 + 1 = 11\n")
		TEXT("Dégâts : 1d8 (6) + 2 = 8; PV Rat géant : 12 → 4"));
	Combat.Details = {
		FText::FromString(TEXT("Jet d'attaque")),
		FText::FromString(TEXT("Jet : 1d20 (14)")),
		FText::FromString(TEXT("Total attaque : 17")),
		FText::FromString(TEXT("Dégâts")),
		FText::FromString(TEXT("Jet de dégâts : 1d8 (6)"))
	};

	const FString Visible = UGridEventFeedWidget::BuildEntryText(Combat).ToString();
	const FString Tooltip = UGridEventFeedWidget::BuildEntryTooltipText(Combat).ToString();

	TestTrue(TEXT("Compact visible text keeps the header"), Visible.Contains(TEXT("Daniel attaque Rat géant")));
	TestTrue(TEXT("Compact visible text keeps attack summary"), Visible.Contains(TEXT("Attaque : 1d20")));
	TestTrue(TEXT("Compact visible text keeps damage summary"), Visible.Contains(TEXT("Dégâts : 1d8")));
	TestFalse(TEXT("Detailed section is hidden from permanent combat text"), Visible.Contains(TEXT("Total attaque : 17")));
	TestTrue(TEXT("Tooltip contains the detailed attack total"), Tooltip.Contains(TEXT("Total attaque : 17")));
	TestTrue(TEXT("Tooltip contains the detailed damage roll"), Tooltip.Contains(TEXT("Jet de dégâts : 1d8 (6)")));

	FGridHudEventEntry Dungeon;
	Dungeon.Category = EGridHudEventCategory::Dungeon;
	Dungeon.Header = FText::FromString(TEXT("Inscription"));
	Dungeon.Details = { FText::FromString(TEXT("Le texte reste visible.")) };
	TestTrue(
		TEXT("Non-combat detail behavior is unchanged"),
		UGridEventFeedWidget::BuildEntryText(Dungeon).ToString().Contains(TEXT("Le texte reste visible.")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridUIEventHUD018PSharedTooltipClassTest,
	"Grimrock.UI.EventHUD01.CompactCombat.SharedTooltipClass",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIEventHUD018PSharedTooltipClassTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridEventFeedWidget* FeedWidget = NewObject<UGridEventFeedWidget>();
	if (!TestNotNull(TEXT("Event Feed widget exists"), FeedWidget))
	{
		return false;
	}

	FeedWidget->SetSharedTooltipWidgetClass(UGridNavigationTooltipWidget::StaticClass());
	TestEqual(
		TEXT("Combat feed stores the shared persistent-HUD tooltip presenter"),
		FeedWidget->SharedTooltipWidgetClass.Get(),
		UGridNavigationTooltipWidget::StaticClass());
	return true;
}

#endif
