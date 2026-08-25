#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "GridLuaVm.h"

namespace
{
	FGridLuaScriptSource MakeMON197Script(const TCHAR* ScriptId, FString Source, bool bEnabled = true)
	{
		FGridLuaScriptSource Script;
		Script.ScriptId = FName(ScriptId);
		Script.bEnabled = bEnabled;
		Script.Source = MoveTemp(Source);
		return Script;
	}

	FString MakeRepeatedAscii(int32 Count)
	{
		FString Result;
		Result.Reserve(Count);
		for (int32 Index = 0; Index < Count; ++Index)
		{
			Result.AppendChar(static_cast<TCHAR>('a'));
		}
		return Result;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON197LuaSourceHardLimitsTest, "Grimrock.MON19.7.LuaSandboxPackaging.SourceHardLimits",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON197LuaSourceHardLimitsTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	TestEqual(TEXT("Hard script-count limit is stable"), FGridLuaVm::HardMaxScriptCount, 64);
	TestEqual(TEXT("Hard per-script source limit is stable"), static_cast<uint64>(FGridLuaVm::HardMaxSourceBytesPerScript), static_cast<uint64>(256 * 1024));
	TestEqual(TEXT("Hard total source limit is stable"), static_cast<uint64>(FGridLuaVm::HardMaxTotalSourceBytes), static_cast<uint64>(1024 * 1024));

	FString Error;

	TArray<FGridLuaScriptSource> TooManyScripts;
	for (int32 Index = 0; Index <= FGridLuaVm::HardMaxScriptCount; ++Index)
	{
		const FString ScriptId = FString::Printf(TEXT("Script_%d"), Index);
		TooManyScripts.Add(MakeMON197Script(*ScriptId, TEXT("value = 1\n")));
	}
	TestFalse(TEXT("A level cannot exceed the hard script-count limit"), FGridLuaVm::ValidateScriptDefinitions(TooManyScripts, Error));
	TestTrue(TEXT("Script-count rejection is diagnosed"), Error.Contains(TEXT("hard limit")));

	TArray<FGridLuaScriptSource> OversizedScript;
	OversizedScript.Add(MakeMON197Script(TEXT("Oversized"), MakeRepeatedAscii(static_cast<int32>(FGridLuaVm::HardMaxSourceBytesPerScript + 1))));
	TestFalse(TEXT("A script cannot exceed the hard UTF-8 source limit"), FGridLuaVm::ValidateScriptDefinitions(OversizedScript, Error));
	TestTrue(TEXT("Per-script rejection is diagnosed"), Error.Contains(TEXT("per-script limit")));

	TArray<FGridLuaScriptSource> OversizedDisabledScript;
	OversizedDisabledScript.Add(
		MakeMON197Script(TEXT("DisabledPayload"), MakeRepeatedAscii(static_cast<int32>(FGridLuaVm::HardMaxSourceBytesPerScript + 1)), false));
	TestFalse(TEXT("Disabled scripts still count toward package source limits"), FGridLuaVm::ValidateScriptDefinitions(OversizedDisabledScript, Error));

	const int32 ChunkBytes = static_cast<int32>(FGridLuaVm::HardMaxTotalSourceBytes / 5 + 1);
	const FString Chunk = MakeRepeatedAscii(ChunkBytes);
	TArray<FGridLuaScriptSource> OversizedLevel;
	for (int32 Index = 0; Index < 5; ++Index)
	{
		const FString ScriptId = FString::Printf(TEXT("Chunk_%d"), Index);
		OversizedLevel.Add(MakeMON197Script(*ScriptId, Chunk));
	}
	TestFalse(TEXT("Combined sources cannot exceed the hard level limit"), FGridLuaVm::ValidateScriptDefinitions(OversizedLevel, Error));
	TestTrue(TEXT("Total-source rejection is diagnosed"), Error.Contains(TEXT("hard total limit")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON197LuaTextOnlyTest, "Grimrock.MON19.7.LuaSandboxPackaging.TextOnlyBytecodeRejection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON197LuaTextOnlyTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FString BinaryChunk;
	BinaryChunk.AppendChar(static_cast<TCHAR>(0x1B));
	BinaryChunk += TEXT("Lua54-binary-like-payload");

	const TArray<FGridLuaScriptSource> Scripts = { MakeMON197Script(TEXT("BinaryChunk"), MoveTemp(BinaryChunk)) };

	FGridLuaVm Vm;
	FString Error;
	TestFalse(TEXT("Lua bytecode-like input is rejected by text-only loading"), Vm.Reload(Scripts, FGridLuaVmConfig(), Error));
	TestFalse(TEXT("Rejected binary input never produces a ready VM"), Vm.IsReady());
	TestTrue(
		TEXT("Binary/text-only rejection is diagnosed"), Error.Contains(TEXT("binary")) || Error.Contains(TEXT("mode")) || Error.Contains(TEXT("compile")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON197LuaForbiddenSurfaceTest, "Grimrock.MON19.7.LuaSandboxPackaging.ForbiddenSurfaceHardened",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON197LuaForbiddenSurfaceTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const TArray<FGridLuaScriptSource> Scripts = { MakeMON197Script(TEXT("Sandbox197"),
		TEXT("filesystem_absent = io == nil and os == nil and package == nil ") TEXT("and require == nil and dofile == nil and loadfile == nil\n") TEXT(
			"dynamic_code_absent = load == nil and string.dump == nil\n") TEXT("debug_absent = debug == nil and collectgarbage == nil and coroutine == nil\n")
			TEXT("safe_surface_present = type(math) == 'table' and type(string) == 'table' ") TEXT("and type(table) == 'table' and type(utf8) == 'table'\n")) };

	FGridLuaVm Vm;
	FString Error;
	if (!Vm.Reload(Scripts, FGridLuaVmConfig(), Error))
	{
		AddError(FString::Printf(TEXT("MON19.7 sandbox reload failed: %s"), *Error));
		return false;
	}

	bool bFilesystemAbsent = false;
	bool bDynamicCodeAbsent = false;
	bool bDebugAbsent = false;
	bool bSafeSurfacePresent = false;
	TestTrue(TEXT("Filesystem/module surface is readable"), Vm.TryGetScriptBool(TEXT("Sandbox197"), TEXT("filesystem_absent"), bFilesystemAbsent, Error));
	TestTrue(TEXT("Dynamic-code surface is readable"), Vm.TryGetScriptBool(TEXT("Sandbox197"), TEXT("dynamic_code_absent"), bDynamicCodeAbsent, Error));
	TestTrue(TEXT("Debug surface is readable"), Vm.TryGetScriptBool(TEXT("Sandbox197"), TEXT("debug_absent"), bDebugAbsent, Error));
	TestTrue(TEXT("Safe surface is readable"), Vm.TryGetScriptBool(TEXT("Sandbox197"), TEXT("safe_surface_present"), bSafeSurfacePresent, Error));

	TestTrue(TEXT("Filesystem and require are absent"), bFilesystemAbsent);
	TestTrue(TEXT("load and string.dump are absent"), bDynamicCodeAbsent);
	TestTrue(TEXT("Debug/process-adjacent helpers are absent"), bDebugAbsent);
	TestTrue(TEXT("Whitelisted pure libraries remain available"), bSafeSurfacePresent);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON197LuaEmbeddedSourceTest, "Grimrock.MON19.7.LuaSandboxPackaging.EmbeddedSourceRuntime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON197LuaEmbeddedSourceTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	// The runtime receives only in-memory FGridLuaScriptSource data. No path,
	// loader, package table or filesystem callback participates in execution.
	const TArray<FGridLuaScriptSource> Scripts = { MakeMON197Script(TEXT("Embedded"), TEXT("value = 41\n") TEXT("function bump() value = value + 1 end\n")) };

	FGridLuaVm Vm;
	FString Error;
	if (!Vm.Reload(Scripts, FGridLuaVmConfig(), Error))
	{
		AddError(FString::Printf(TEXT("Embedded-source reload failed: %s"), *Error));
		return false;
	}

	TestTrue(TEXT("Embedded callback executes without external source files"), Vm.CallFunction(TEXT("Embedded"), TEXT("bump"), Error));

	int32 Value = 0;
	TestTrue(TEXT("Embedded script state is readable"), Vm.TryGetScriptInt32(TEXT("Embedded"), TEXT("value"), Value, Error));
	TestEqual(TEXT("Embedded script executed"), Value, 42);
	return true;
}

#endif
