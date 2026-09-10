#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "GridLuaScriptTypes.h"
#include "GridLuaVm.h"

namespace
{
	FGridLuaScriptSource MakePUZZLE01LuaScript(const FString& Source)
	{
		FGridLuaScriptSource Script;
		Script.ScriptId = TEXT("GuardianPuzzle");
		Script.bEnabled = true;
		Script.Source = Source;
		return Script;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridPUZZLE01Lua01VisualApiTest,
	"Grimrock.PUZZLE01.LUA01.LuaVisualApi",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridPUZZLE01Lua01VisualApiTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const FString ScriptSource =
		TEXT("function on_inserted(event)\n")
		TEXT("  local ok, err = grid.visual.set_material('Guardian', 'EyesLeft', 'BlueGem')\n")
		TEXT("  assert(ok, err)\n")
		TEXT("end\n");

	FGridLuaVm Vm;
	FString Error;
	TestTrue(TEXT("PUZZLE01-LUA01 script loads"), Vm.Reload({ MakePUZZLE01LuaScript(ScriptSource) }, FGridLuaVmConfig(), Error));
	if (!Vm.IsReady())
	{
		AddError(FString::Printf(TEXT("Lua VM failed to initialize: %s"), *Error));
		return false;
	}

	FString CapturedTarget;
	FString CapturedSlot;
	FString CapturedAlias;
	FGridLuaHostApi Host;
	Host.SetMaterial = [&CapturedTarget, &CapturedSlot, &CapturedAlias](
		const FString& Target, const FString& Slot, const FString& Alias, FString& OutHostError)
	{
		CapturedTarget = Target;
		CapturedSlot = Slot;
		CapturedAlias = Alias;
		OutHostError.Reset();
		return true;
	};

	FGridLuaEventContext Event;
	Event.SourceObjectId = TEXT("00000001-00000001-00000001-00000001");
	Event.EventName = TEXT("ItemInserted");
	TestTrue(TEXT("grid.visual.set_material invokes the host primitive"),
		Vm.CallEventFunction(TEXT("GuardianPuzzle"), TEXT("on_inserted"), Event, Host, Error));
	TestEqual(TEXT("Target LogicId crosses the sandbox boundary as data"), CapturedTarget, FString(TEXT("Guardian")));
	TestEqual(TEXT("Material slot crosses the sandbox boundary as data"), CapturedSlot, FString(TEXT("EyesLeft")));
	TestEqual(TEXT("Material alias crosses the sandbox boundary as data"), CapturedAlias, FString(TEXT("BlueGem")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridPUZZLE01Lua01VisualApiFailureTest,
	"Grimrock.PUZZLE01.LUA01.LuaVisualApiFailureIsData",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridPUZZLE01Lua01VisualApiFailureTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const FString ScriptSource =
		TEXT("handled = false\n")
		TEXT("function on_inserted(event)\n")
		TEXT("  local ok, err = grid.visual.set_material('Guardian', 'EyesLeft', 'BlueGem')\n")
		TEXT("  handled = ok == false and type(err) == 'string'\n")
		TEXT("end\n");

	FGridLuaVm Vm;
	FString Error;
	if (!Vm.Reload({ MakePUZZLE01LuaScript(ScriptSource) }, FGridLuaVmConfig(), Error))
	{
		AddError(FString::Printf(TEXT("Lua VM failed to initialize: %s"), *Error));
		return false;
	}

	FGridLuaHostApi Host;
	FGridLuaEventContext Event;
	Event.SourceObjectId = TEXT("00000001-00000001-00000001-00000001");
	Event.EventName = TEXT("ItemInserted");
	TestTrue(TEXT("Missing visual host is returned to Lua without aborting the callback"),
		Vm.CallEventFunction(TEXT("GuardianPuzzle"), TEXT("on_inserted"), Event, Host, Error));

	bool bHandled = false;
	TestTrue(TEXT("Failure observation is readable"), Vm.TryGetScriptBool(TEXT("GuardianPuzzle"), TEXT("handled"), bHandled, Error));
	TestTrue(TEXT("Lua receives visual host failure as false + error string"), bHandled);
	return true;
}

#endif