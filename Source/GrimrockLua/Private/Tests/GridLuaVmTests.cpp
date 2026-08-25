#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "GridLuaVm.h"

namespace
{
	FGridLuaScriptSource MakeLuaScript(const TCHAR* ScriptId, const TCHAR* Source, bool bEnabled = true)
	{
		FGridLuaScriptSource Script;
		Script.ScriptId = FName(ScriptId);
		Script.bEnabled = bEnabled;
		Script.Source = Source;
		return Script;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON1931LuaVersionLifecycleTest, "Grimrock.MON19.3.Lua.Foundation.VersionAndLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON1931LuaVersionLifecycleTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridLuaVm Vm;
	FString Error;
	TestTrue(TEXT("Empty script set creates a ready VM"), Vm.Reload({}, FGridLuaVmConfig(), Error));
	TestTrue(TEXT("VM is ready"), Vm.IsReady());
	TestEqual(TEXT("Runtime is pinned to Lua 5.4.8"), Vm.GetVersionString(), FString(TEXT("Lua 5.4.8")));
	TestEqual(TEXT("No script is loaded"), Vm.GetLoadedScriptCount(), 0);
	TestTrue(TEXT("VM allocates within quota"), Vm.GetAllocatedBytes() > 0);
	TestTrue(TEXT("VM allocation stays below default quota"), Vm.GetAllocatedBytes() <= FGridLuaVmConfig().MemoryLimitBytes);

	Vm.Reset();
	TestFalse(TEXT("Reset destroys the VM"), Vm.IsReady());
	TestEqual(TEXT("Reset releases tracked memory"), Vm.GetAllocatedBytes(), SIZE_T(0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON1931LuaMultipleScriptsTest, "Grimrock.MON19.3.Lua.Foundation.MultipleScriptsIsolated",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON1931LuaMultipleScriptsTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const TArray<FGridLuaScriptSource> Scripts = { MakeLuaScript(TEXT("PuzzleA"),
													   TEXT("counter = 1\n") TEXT("math.pi = 1\n") TEXT("pi_scaled = math.floor(math.pi * 1000)\n")
														   TEXT("function bump() counter = counter + 1 end\n")),
		MakeLuaScript(
			TEXT("PuzzleB"), TEXT("counter = 40\n") TEXT("pi_scaled = math.floor(math.pi * 1000)\n") TEXT("function bump() counter = counter + 2 end\n")) };

	FGridLuaVm Vm;
	FString Error;
	if (!Vm.Reload(Scripts, FGridLuaVmConfig(), Error))
	{
		AddError(FString::Printf(TEXT("Lua reload failed: %s"), *Error));
		return false;
	}

	TestEqual(TEXT("Two scripts share one VM"), Vm.GetLoadedScriptCount(), 2);
	TestTrue(TEXT("PuzzleA is registered"), Vm.HasScript(TEXT("PuzzleA")));
	TestTrue(TEXT("PuzzleB is registered"), Vm.HasScript(TEXT("PuzzleB")));
	TestTrue(TEXT("PuzzleA callback succeeds"), Vm.CallFunction(TEXT("PuzzleA"), TEXT("bump"), Error));
	TestTrue(TEXT("PuzzleB callback succeeds"), Vm.CallFunction(TEXT("PuzzleB"), TEXT("bump"), Error));

	int32 CounterA = 0;
	int32 CounterB = 0;
	int32 PiA = 0;
	int32 PiB = 0;
	TestTrue(TEXT("PuzzleA counter is readable"), Vm.TryGetScriptInt32(TEXT("PuzzleA"), TEXT("counter"), CounterA, Error));
	TestTrue(TEXT("PuzzleB counter is readable"), Vm.TryGetScriptInt32(TEXT("PuzzleB"), TEXT("counter"), CounterB, Error));
	TestTrue(TEXT("PuzzleA math table is readable"), Vm.TryGetScriptInt32(TEXT("PuzzleA"), TEXT("pi_scaled"), PiA, Error));
	TestTrue(TEXT("PuzzleB math table is readable"), Vm.TryGetScriptInt32(TEXT("PuzzleB"), TEXT("pi_scaled"), PiB, Error));
	TestEqual(TEXT("PuzzleA keeps its own global state"), CounterA, 2);
	TestEqual(TEXT("PuzzleB keeps its own global state"), CounterB, 42);
	TestEqual(TEXT("PuzzleA may mutate its own math table"), PiA, 1000);
	TestEqual(TEXT("PuzzleB does not see PuzzleA math mutation"), PiB, 3141);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON1931LuaSandboxSurfaceTest, "Grimrock.MON19.3.Lua.Foundation.SandboxSurface",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON1931LuaSandboxSurfaceTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const TArray<FGridLuaScriptSource> Scripts = { MakeLuaScript(TEXT("Sandbox"),
		TEXT("unsafe_absent = io == nil and os == nil and package == nil and debug == nil ")
			TEXT("and require == nil and dofile == nil and loadfile == nil and load == nil ") TEXT("and collectgarbage == nil and coroutine == nil\n")
				TEXT("safe_present = type(math) == 'table' and type(string) == 'table' ") TEXT("and type(table) == 'table' and type(utf8) == 'table' ")
					TEXT("and type(pairs) == 'function' and _G ~= nil\n")) };

	FGridLuaVm Vm;
	FString Error;
	if (!Vm.Reload(Scripts, FGridLuaVmConfig(), Error))
	{
		AddError(FString::Printf(TEXT("Lua sandbox reload failed: %s"), *Error));
		return false;
	}

	bool bUnsafeAbsent = false;
	bool bSafePresent = false;
	TestTrue(TEXT("Sandbox result is readable"), Vm.TryGetScriptBool(TEXT("Sandbox"), TEXT("unsafe_absent"), bUnsafeAbsent, Error));
	TestTrue(TEXT("Safe surface result is readable"), Vm.TryGetScriptBool(TEXT("Sandbox"), TEXT("safe_present"), bSafePresent, Error));
	TestTrue(TEXT("Unsafe standard libraries/functions are absent"), bUnsafeAbsent);
	TestTrue(TEXT("Whitelisted standard surface is present"), bSafePresent);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON1931LuaAtomicReloadTest, "Grimrock.MON19.3.Lua.Foundation.InvalidDefinitionsAtomicReload",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON1931LuaAtomicReloadTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridLuaVm Vm;
	FString Error;
	const TArray<FGridLuaScriptSource> ValidScripts = { MakeLuaScript(TEXT("Stable"), TEXT("value = 7\nfunction increment() value = value + 1 end\n")) };
	if (!Vm.Reload(ValidScripts, FGridLuaVmConfig(), Error))
	{
		AddError(FString::Printf(TEXT("Initial Lua reload failed: %s"), *Error));
		return false;
	}

	const TArray<FGridLuaScriptSource> DuplicateScripts = { MakeLuaScript(TEXT("Duplicate"), TEXT("x = 1")),
		MakeLuaScript(TEXT("Duplicate"), TEXT("x = 2"), false) };
	TestFalse(TEXT("Duplicate ScriptId is rejected even when one entry is disabled"), Vm.Reload(DuplicateScripts, FGridLuaVmConfig(), Error));
	TestTrue(TEXT("Old VM survives definition failure"), Vm.IsReady());
	TestTrue(TEXT("Old script survives definition failure"), Vm.HasScript(TEXT("Stable")));

	const TArray<FGridLuaScriptSource> SyntaxErrorScripts = { MakeLuaScript(TEXT("Broken"), TEXT("function broken( this is not lua")) };
	TestFalse(TEXT("Syntax error rejects candidate VM"), Vm.Reload(SyntaxErrorScripts, FGridLuaVmConfig(), Error));
	TestTrue(TEXT("Old VM survives compile failure"), Vm.HasScript(TEXT("Stable")));

	TestTrue(TEXT("Old callback still executes"), Vm.CallFunction(TEXT("Stable"), TEXT("increment"), Error));
	int32 StableValue = 0;
	TestTrue(TEXT("Old script state remains readable"), Vm.TryGetScriptInt32(TEXT("Stable"), TEXT("value"), StableValue, Error));
	TestEqual(TEXT("Old state was not replaced"), StableValue, 8);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON1931LuaInstructionBudgetTest, "Grimrock.MON19.3.Lua.Foundation.InstructionBudget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON1931LuaInstructionBudgetTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const TArray<FGridLuaScriptSource> Scripts = { MakeLuaScript(
		TEXT("Budget"), TEXT("value = 1\n") TEXT("function spin() while true do value = value + 1 end end\n")) };

	FGridLuaVmConfig Config;
	Config.InstructionBudgetPerCall = 10000;

	FGridLuaVm Vm;
	FString Error;
	if (!Vm.Reload(Scripts, Config, Error))
	{
		AddError(FString::Printf(TEXT("Lua budget reload failed: %s"), *Error));
		return false;
	}

	TestFalse(TEXT("Infinite loop is interrupted"), Vm.CallFunction(TEXT("Budget"), TEXT("spin"), Error));
	TestTrue(TEXT("Budget failure is diagnosed"), Error.Contains(TEXT("instruction budget exceeded")));
	TestTrue(TEXT("VM remains ready after protected budget error"), Vm.IsReady());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMON1931LuaMemoryQuotaTest, "Grimrock.MON19.3.Lua.Foundation.MemoryQuota", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON1931LuaMemoryQuotaTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const TArray<FGridLuaScriptSource> Scripts = { MakeLuaScript(TEXT("Memory"),
		TEXT("function allocate()\n") TEXT("  local values = {}\n") TEXT("  for i = 1, 200000 do values[i] = i end\n") TEXT("  retained = values\n")
			TEXT("end\n")) };

	FGridLuaVmConfig Config;
	Config.MemoryLimitBytes = 1024 * 1024;
	Config.InstructionBudgetPerCall = 5000000;

	FGridLuaVm Vm;
	FString Error;
	if (!Vm.Reload(Scripts, Config, Error))
	{
		AddError(FString::Printf(TEXT("Lua quota reload failed: %s"), *Error));
		return false;
	}

	TestFalse(TEXT("Allocator quota rejects unbounded growth"), Vm.CallFunction(TEXT("Memory"), TEXT("allocate"), Error));
	TestTrue(TEXT("VM remains ready after protected allocation error"), Vm.IsReady());
	TestTrue(TEXT("Tracked allocation never exceeds quota"), Vm.GetAllocatedBytes() <= Config.MemoryLimitBytes);
	return true;
}

#endif
