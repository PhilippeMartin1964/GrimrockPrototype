#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EditorTools/GridLuaSyntaxHighlighter.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridLUAUX04SyntaxHighlightingTest,
	"Grimrock.LUAUX04.SyntaxHighlighting",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridLUAUX04SyntaxHighlightingTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const TSharedRef<FGridLuaSyntaxHighlighter> Highlighter = FGridLuaSyntaxHighlighter::Create();
	TestTrue(TEXT("Syntax highlighter requires live Slate updates while editing"), Highlighter->RequiresLiveUpdate());

	TestEqual(TEXT("Lua keyword classification"), FGridLuaSyntaxHighlighter::ClassifyStandaloneToken(TEXT("function")), EGridLuaSyntaxStyle::Keyword);
	TestEqual(TEXT("Lua literal classification"), FGridLuaSyntaxHighlighter::ClassifyStandaloneToken(TEXT("true")), EGridLuaSyntaxStyle::Literal);
	TestEqual(TEXT("Lua number classification"), FGridLuaSyntaxHighlighter::ClassifyStandaloneToken(TEXT("2")), EGridLuaSyntaxStyle::Number);
	TestEqual(TEXT("Persistent state classification"), FGridLuaSyntaxHighlighter::ClassifyStandaloneToken(TEXT("persistent")), EGridLuaSyntaxStyle::Persistent);
	TestEqual(TEXT("Grimrock command API classification"), FGridLuaSyntaxHighlighter::ClassifyStandaloneToken(TEXT("grid.command")), EGridLuaSyntaxStyle::GridApi);
	TestEqual(TEXT("Grimrock visual API classification"), FGridLuaSyntaxHighlighter::ClassifyStandaloneToken(TEXT("grid.visual.set_material")), EGridLuaSyntaxStyle::GridApi);
	TestEqual(TEXT("Ordinary identifiers remain neutral"), FGridLuaSyntaxHighlighter::ClassifyStandaloneToken(TEXT("GuardianGemCount")), EGridLuaSyntaxStyle::Normal);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
