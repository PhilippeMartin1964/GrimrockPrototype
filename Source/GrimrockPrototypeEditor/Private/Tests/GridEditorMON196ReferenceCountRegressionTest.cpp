#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "EditorTools/GridEditorLuaService.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON196LuaReferenceCountRegressionTest, "Grimrock.MON19.6.Editor.LuaReferenceCountRegression",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON196LuaReferenceCountRegressionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridLevelAsset* Level = NewObject<UGridLevelAsset>(GetTransientPackage());
	TestNotNull(TEXT("Level asset exists"), Level);
	if (!Level)
	{
		return false;
	}

	FGridObjectLink First;
	First.Command = EGridObjectCommand::LuaCallback;
	First.LuaScriptId = TEXT("Puzzle");
	First.LuaCallbackName = TEXT("on_trigger");
	Level->Links.Add(First);

	FGridObjectLink Second = First;
	Second.LuaCallbackName = TEXT("on_reset");
	Level->Links.Add(Second);

	FGridObjectLink Other = First;
	Other.LuaScriptId = TEXT("OtherPuzzle");
	Level->Links.Add(Other);

	TestEqual(TEXT("Two bindings referencing Puzzle are counted"), GridEditorLuaService::CountScriptReferences(*Level, TEXT("Puzzle")), 2);
	TestEqual(TEXT("One binding referencing OtherPuzzle is counted"), GridEditorLuaService::CountScriptReferences(*Level, TEXT("OtherPuzzle")), 1);
	TestEqual(TEXT("Missing script has zero references"), GridEditorLuaService::CountScriptReferences(*Level, TEXT("Missing")), 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
