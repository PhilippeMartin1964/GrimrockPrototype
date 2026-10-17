#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#include "UI/RPGTalentNodeStyleSettings.h"

namespace UIRPGTalentCodeAudit026
{
	bool LoadProjectSource(const TCHAR* RelativePath, FString& OutSource)
	{
		return FFileHelper::LoadFileToString(
			OutSource,
			*FPaths::Combine(FPaths::ProjectDir(), RelativePath));
	}

	bool ReadStyle(
		FAutomationTestBase& Test,
		const URPGTalentNodeStyleSettings* Settings,
		EGridTalentNodeState State,
		FRPGTalentNodeStateVisualStyle& OutStyle)
	{
		const bool bResolved = Settings && Settings->TryGetStyle(State, OutStyle);
		Test.TestTrue(
			*FString::Printf(TEXT("Configured style resolves for semantic state %d"), static_cast<int32>(State)),
			bResolved);
		if (bResolved)
		{
			Test.TestTrue(TEXT("State text color is visible"), OutStyle.TextColor.A > 0.0f);
			Test.TestTrue(TEXT("State node tint is visible"), OutStyle.NodeTint.A > 0.0f);
		}
		return bResolved;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGTalentCodeAudit026ConfiguredStylesTest,
	"Grimrock.UI.RPG.CODEAUDIT02.TalentNodeStateStyle.ConfiguredStyles",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGTalentCodeAudit026ConfiguredStylesTest::RunTest(const FString&)
{
	using namespace UIRPGTalentCodeAudit026;

	const URPGTalentNodeStyleSettings* Settings = GetDefault<URPGTalentNodeStyleSettings>();
	if (!TestNotNull(TEXT("Talent node style settings CDO exists"), Settings))
	{
		return false;
	}
	TestTrue(TEXT("All six semantic Talent state styles are configured"), Settings->IsValidDefinition());

	FRPGTalentNodeStateVisualStyle Acquired;
	FRPGTalentNodeStateVisualStyle Available;
	FRPGTalentNodeStateVisualStyle LockedLevel;
	FRPGTalentNodeStateVisualStyle LockedPrerequisite;
	FRPGTalentNodeStateVisualStyle LockedPoints;
	FRPGTalentNodeStateVisualStyle LockedExclusive;

	ReadStyle(*this, Settings, EGridTalentNodeState::Acquired, Acquired);
	ReadStyle(*this, Settings, EGridTalentNodeState::Available, Available);
	ReadStyle(*this, Settings, EGridTalentNodeState::LockedLevel, LockedLevel);
	ReadStyle(*this, Settings, EGridTalentNodeState::LockedPrerequisite, LockedPrerequisite);
	ReadStyle(*this, Settings, EGridTalentNodeState::LockedPoints, LockedPoints);
	ReadStyle(*this, Settings, EGridTalentNodeState::LockedExclusive, LockedExclusive);

	TestFalse(TEXT("Acquired and Available keep distinct presentation"),
		Acquired.NodeTint.Equals(Available.NodeTint));
	TestFalse(TEXT("Available and LockedLevel keep distinct presentation"),
		Available.NodeTint.Equals(LockedLevel.NodeTint));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGTalentCodeAudit026CppStyleBoundaryTest,
	"Grimrock.UI.RPG.CODEAUDIT02.TalentNodeStateStyle.CppStyleBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGTalentCodeAudit026CppStyleBoundaryTest::RunTest(const FString&)
{
	using namespace UIRPGTalentCodeAudit026;

	FString NodeSource;
	if (!TestTrue(
		TEXT("GridTalentNodeWidget.cpp loads"),
		LoadProjectSource(
			TEXT("Source/GrimrockPrototype/Private/UI/GridTalentNodeWidget.cpp"),
			NodeSource)))
	{
		return false;
	}

	TestFalse(TEXT("Legacy NodeStateTextColor helper is removed"),
		NodeSource.Contains(TEXT("NodeStateTextColor")));
	TestFalse(TEXT("Legacy NodeStateTint helper is removed"),
		NodeSource.Contains(TEXT("NodeStateTint")));
	TestFalse(TEXT("Node widget no longer hard-codes acquired green"),
		NodeSource.Contains(TEXT("0.30f, 0.90f, 0.30f")));
	TestFalse(TEXT("Node widget no longer hard-codes available green"),
		NodeSource.Contains(TEXT("0.45f, 1.00f, 0.45f")));
	TestFalse(TEXT("Node widget no longer hard-codes locked luminance"),
		NodeSource.Contains(TEXT("Luminance")));
	TestTrue(TEXT("Node widget consumes data-owned state style settings"),
		NodeSource.Contains(TEXT("URPGTalentNodeStyleSettings")));
	TestTrue(TEXT("Node widget keeps semantic state as the lookup key"),
		NodeSource.Contains(TEXT("TryGetStyle(NodeView.State")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGTalentCodeAudit026ConfigOwnershipTest,
	"Grimrock.UI.RPG.CODEAUDIT02.TalentNodeStateStyle.ConfigOwnership",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGTalentCodeAudit026ConfigOwnershipTest::RunTest(const FString&)
{
	using namespace UIRPGTalentCodeAudit026;

	FString Config;
	if (!TestTrue(
		TEXT("DefaultGame.ini loads"),
		LoadProjectSource(TEXT("Config/DefaultGame.ini"), Config)))
	{
		return false;
	}

	TestTrue(TEXT("DefaultGame owns the Talent node style settings section"),
		Config.Contains(TEXT("[/Script/GrimrockPrototype.RPGTalentNodeStyleSettings]")));

	for (const TCHAR* PropertyName : {
		TEXT("AcquiredStyle="),
		TEXT("AvailableStyle="),
		TEXT("LockedLevelStyle="),
		TEXT("LockedPrerequisiteStyle="),
		TEXT("LockedPointsStyle="),
		TEXT("LockedExclusiveStyle=")
	})
	{
		TestTrue(
			*FString::Printf(TEXT("DefaultGame config owns %s"), PropertyName),
			Config.Contains(PropertyName));
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
