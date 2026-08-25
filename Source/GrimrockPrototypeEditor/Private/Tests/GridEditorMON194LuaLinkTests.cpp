#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridTypes.h"
#include "EditorTools/GridEditorLinkPolicy.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMON194LuaLinkIdentityTest, "Grimrock.MON19.4.Editor.LuaLinkIdentity", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON194LuaLinkIdentityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridObjectLink Base;
	Base.SourceObjectId = FGuid(19, 4, 90, 1);
	Base.SourceEvent = EGridObjectEvent::Activated;
	Base.Command = EGridObjectCommand::LuaCallback;
	Base.LuaScriptId = TEXT("Puzzle");
	Base.LuaCallbackName = TEXT("on_trigger");

	FGridObjectLink Same = Base;
	TestTrue(TEXT("Same Lua callback link is exactly equivalent"), GridEditorLinkPolicy::AreLinksExactlyEquivalent(Base, Same));

	FGridObjectLink OtherScript = Base;
	OtherScript.LuaScriptId = TEXT("OtherPuzzle");
	TestFalse(TEXT("ScriptId participates in exact link identity"), GridEditorLinkPolicy::AreLinksExactlyEquivalent(Base, OtherScript));

	FGridObjectLink OtherCallback = Base;
	OtherCallback.LuaCallbackName = TEXT("on_reset");
	TestFalse(TEXT("CallbackName participates in exact link identity"), GridEditorLinkPolicy::AreLinksExactlyEquivalent(Base, OtherCallback));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
