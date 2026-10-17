#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace UIRPGTalentCodeAudit027
{
	bool LoadProjectSource(const TCHAR* RelativePath, FString& OutSource)
	{
		return FFileHelper::LoadFileToString(
			OutSource,
			*FPaths::Combine(FPaths::ProjectDir(), RelativePath));
	}

	int32 CountLines(const FString& Source)
	{
		TArray<FString> Lines;
		Source.ParseIntoArrayLines(Lines, false);
		return Lines.Num();
	}

	struct FSourceBoundary
	{
		const TCHAR* RelativePath;
		int32 MaximumLines;
		const TCHAR* RequiredToken;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGTalentCodeAudit027FileSizeTest,
	"Grimrock.UI.RPG.CODEAUDIT02.SourceSplit.FileSizes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGTalentCodeAudit027FileSizeTest::RunTest(const FString&)
{
	using namespace UIRPGTalentCodeAudit027;

	const FSourceBoundary Boundaries[] = {
		{ TEXT("Source/GrimrockPrototype/Private/UI/GridSkillsWidget.cpp"), 900, TEXT("UGridSkillsWidget::RefreshSkills") },
		{ TEXT("Source/GrimrockPrototype/Private/UI/GridSkillsWidgetSkills.cpp"), 900, TEXT("UGridSkillsWidget::CommitSkillRankIncrease") },
		{ TEXT("Source/GrimrockPrototype/Private/UI/GridSkillsWidgetTalents.cpp"), 900, TEXT("UGridSkillsWidget::CommitConfirmedTalentChoice") },
		{ TEXT("Source/GrimrockPrototype/Private/UI/GridSkillsWidgetPresentation.cpp"), 900, TEXT("UGridSkillsWidget::ApplyDesignerPresentation") },
		{ TEXT("Source/GrimrockPrototype/Private/UI/GridSkillsPageService.cpp"), 900, TEXT("FGridSkillsPageService::TryBuildCharacterView") },
		{ TEXT("Source/GrimrockPrototype/Private/UI/GridSkillsPageServiceTalentTree.cpp"), 900, TEXT("BuildTalentTree") },
		{ TEXT("Source/GrimrockPrototype/Private/UI/GridSkillsPageServiceTalentDetail.cpp"), 900, TEXT("BuildStructuredEffects") }
	};

	for (const FSourceBoundary& Boundary : Boundaries)
	{
		FString Source;
		if (!TestTrue(
			*FString::Printf(TEXT("%s loads"), Boundary.RelativePath),
			LoadProjectSource(Boundary.RelativePath, Source)))
		{
			continue;
		}

		TestTrue(
			*FString::Printf(TEXT("%s stays below %d lines"), Boundary.RelativePath, Boundary.MaximumLines),
			CountLines(Source) <= Boundary.MaximumLines);
		TestTrue(
			*FString::Printf(TEXT("%s keeps its intended responsibility"), Boundary.RelativePath),
			Source.Contains(Boundary.RequiredToken));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGTalentCodeAudit027ResponsibilityTest,
	"Grimrock.UI.RPG.CODEAUDIT02.SourceSplit.Responsibilities",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGTalentCodeAudit027ResponsibilityTest::RunTest(const FString&)
{
	using namespace UIRPGTalentCodeAudit027;

	FString WidgetMain;
	FString WidgetSkills;
	FString WidgetTalents;
	FString WidgetPresentation;
	FString PageMain;
	FString PageTree;
	FString PageDetail;

	TestTrue(TEXT("GridSkillsWidget.cpp loads"),
		LoadProjectSource(TEXT("Source/GrimrockPrototype/Private/UI/GridSkillsWidget.cpp"), WidgetMain));
	TestTrue(TEXT("GridSkillsWidgetSkills.cpp loads"),
		LoadProjectSource(TEXT("Source/GrimrockPrototype/Private/UI/GridSkillsWidgetSkills.cpp"), WidgetSkills));
	TestTrue(TEXT("GridSkillsWidgetTalents.cpp loads"),
		LoadProjectSource(TEXT("Source/GrimrockPrototype/Private/UI/GridSkillsWidgetTalents.cpp"), WidgetTalents));
	TestTrue(TEXT("GridSkillsWidgetPresentation.cpp loads"),
		LoadProjectSource(TEXT("Source/GrimrockPrototype/Private/UI/GridSkillsWidgetPresentation.cpp"), WidgetPresentation));
	TestTrue(TEXT("GridSkillsPageService.cpp loads"),
		LoadProjectSource(TEXT("Source/GrimrockPrototype/Private/UI/GridSkillsPageService.cpp"), PageMain));
	TestTrue(TEXT("GridSkillsPageServiceTalentTree.cpp loads"),
		LoadProjectSource(TEXT("Source/GrimrockPrototype/Private/UI/GridSkillsPageServiceTalentTree.cpp"), PageTree));
	TestTrue(TEXT("GridSkillsPageServiceTalentDetail.cpp loads"),
		LoadProjectSource(TEXT("Source/GrimrockPrototype/Private/UI/GridSkillsPageServiceTalentDetail.cpp"), PageDetail));

	TestFalse(TEXT("Widget shell no longer owns Skill mutations"),
		WidgetMain.Contains(TEXT("UGridSkillsWidget::CommitSkillRankIncrease")));
	TestFalse(TEXT("Widget shell no longer owns Talent mutations"),
		WidgetMain.Contains(TEXT("UGridSkillsWidget::CommitConfirmedTalentChoice")));
	TestFalse(TEXT("Widget shell no longer owns visual rendering"),
		WidgetMain.Contains(TEXT("UGridSkillsWidget::ApplyDesignerPresentation")));

	TestTrue(TEXT("Skill split owns Skill mutation"),
		WidgetSkills.Contains(TEXT("UGridSkillsWidget::CommitSkillRankIncrease")));
	TestTrue(TEXT("Skill split owns Skill sorting"),
		WidgetSkills.Contains(TEXT("UGridSkillsWidget::SortSkillEntries")));
	TestTrue(TEXT("Talent split owns Talent mutation"),
		WidgetTalents.Contains(TEXT("UGridSkillsWidget::CommitConfirmedTalentChoice")));
	TestTrue(TEXT("Talent split owns variant flyout"),
		WidgetTalents.Contains(TEXT("UGridSkillsWidget::OpenTalentVariantFlyout")));
	TestTrue(TEXT("Presentation split owns branch rendering"),
		WidgetPresentation.Contains(TEXT("UGridSkillsWidget::ApplyDesignerPresentation")));
	TestTrue(TEXT("Presentation split owns class visual identity"),
		WidgetPresentation.Contains(TEXT("UGridSkillsWidget::ApplyClassVisualIdentity")));

	TestFalse(TEXT("Page service public API file no longer implements Talent nodes"),
		PageMain.Contains(TEXT("bool BuildTalentNode(")));
	TestFalse(TEXT("Page service public API file no longer formats Talent effects"),
		PageMain.Contains(TEXT("void BuildStructuredEffects(")));
	TestTrue(TEXT("Talent tree split owns node topology"),
		PageTree.Contains(TEXT("bool BuildTalentNode(")));
	TestTrue(TEXT("Talent tree split owns branch construction"),
		PageTree.Contains(TEXT("bool BuildTalentTree(")));
	TestTrue(TEXT("Talent detail split owns structured effects"),
		PageDetail.Contains(TEXT("void BuildStructuredEffects(")));
	TestTrue(TEXT("Talent detail split owns acquisition projection"),
		PageDetail.Contains(TEXT("void BuildAcquisitionView(")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
