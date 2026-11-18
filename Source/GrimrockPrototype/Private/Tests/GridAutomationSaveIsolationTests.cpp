#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/GridAutomationSaveIsolation.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridAutomationSaveIsolationTest,
	"Grimrock.TechnicalDebt.ARM.CLEAN06_4X66.AutomationSaveIsolation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridAutomationSaveIsolationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridAutomationSaveIsolation;
	const FString Project = FullDirectory(FPaths::ProjectDir());
	const FString Root = Project / TEXT("Saved/Automation/GrimrockAutomation-fixture");
	TestTrue(TEXT("Exact independent Saved directory accepted"), IsIsolated(Root, Root / TEXT("Saved"), Project));
	TestFalse(TEXT("Live Saved directory rejected"), IsIsolated(Root, Project / TEXT("Saved"), Project));
	TestFalse(TEXT("Live SaveGames descendants rejected"), IsIsolated(Project / TEXT("Saved/SaveGames/GrimrockAutomation-fixture"),
		Project / TEXT("Saved/SaveGames/GrimrockAutomation-fixture/Saved"), Project));
	TestFalse(TEXT("Relative root rejected"), IsIsolated(TEXT("GrimrockAutomation-fixture"), Root / TEXT("Saved"), Project));
	TestFalse(TEXT("Wrong basename rejected"), IsIsolated(Project / TEXT("Other"), Project / TEXT("Other/Saved"), Project));
	return Require(*this);
}
#endif
