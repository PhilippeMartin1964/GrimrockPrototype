#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "GridLuaVm.h"

namespace
{
	FGridLuaScriptSource MakeMON1971Script(const TCHAR* ScriptId, const TCHAR* Source)
	{
		FGridLuaScriptSource Script;
		Script.ScriptId = FName(ScriptId);
		Script.bEnabled = true;
		Script.Source = Source;
		return Script;
	}

	FGridLuaHostApi MakeMON1971Host(TMap<FName, bool>& BoolValues, TMap<FName, int32>& IntValues)
	{
		FGridLuaHostApi Host;
		Host.GetBool = [&BoolValues](FName VariableId, bool& OutValue, FString& OutError)
		{
			if (const bool* Value = BoolValues.Find(VariableId))
			{
				OutValue = *Value;
				OutError.Reset();
				return true;
			}
			OutError = FString::Printf(TEXT("Missing Bool '%s'."), *VariableId.ToString());
			return false;
		};
		Host.SetBool = [&BoolValues](FName VariableId, bool Value, FString& OutError)
		{
			if (!BoolValues.Contains(VariableId))
			{
				OutError = FString::Printf(TEXT("Missing Bool '%s'."), *VariableId.ToString());
				return false;
			}
			BoolValues.Add(VariableId, Value);
			OutError.Reset();
			return true;
		};
		Host.GetInt32 = [&IntValues](FName VariableId, int32& OutValue, FString& OutError)
		{
			if (const int32* Value = IntValues.Find(VariableId))
			{
				OutValue = *Value;
				OutError.Reset();
				return true;
			}
			OutError = FString::Printf(TEXT("Missing Int32 '%s'."), *VariableId.ToString());
			return false;
		};
		Host.SetInt32 = [&IntValues](FName VariableId, int32 Value, FString& OutError)
		{
			if (!IntValues.Contains(VariableId))
			{
				OutError = FString::Printf(TEXT("Missing Int32 '%s'."), *VariableId.ToString());
				return false;
			}
			IntValues.Add(VariableId, Value);
			OutError.Reset();
			return true;
		};
		Host.Command = [](const FString&, const FString&, FString& OutError)
		{
			OutError.Reset();
			return true;
		};
		Host.Log = [](const FString&)
		{
		};
		return Host;
	}

	FGridLuaEventContext MakeMON1971Event()
	{
		FGridLuaEventContext Event;
		Event.SourceObjectId = TEXT("MON1971Source");
		Event.EventName = TEXT("Activated");
		return Event;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON1971PersistentTableTest, "Grimrock.MON19.7.1.LuaAuthoring.PersistentTable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON1971PersistentTableTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const FGridLuaScriptSource Script = MakeMON1971Script(TEXT("Puzzle"),
		TEXT("persistent = { GateOpen = false, RuneCount = 2 }\n") TEXT("function on_trigger(event)\n") TEXT("  persistent.GateOpen = true\n")
			TEXT("  persistent.RuneCount = persistent.RuneCount + 1\n") TEXT("end\n"));

	FGridLuaVm Vm;
	FString Error;
	if (!Vm.Reload({ Script }, FGridLuaVmConfig(), Error))
	{
		AddError(FString::Printf(TEXT("Persistent authoring VM failed to load: %s"), *Error));
		return false;
	}

	const TArray<FGridLuaPersistentVariableDefinition> Definitions = Vm.GetPersistentVariableDefinitions();
	TestEqual(TEXT("Two persistent variables are detected"), Definitions.Num(), 2);
	TestEqual(TEXT("First persistent id is stable"), Definitions[0].VariableId, FName(TEXT("GateOpen")));
	TestEqual(TEXT("GateOpen is Bool"), Definitions[0].Type, EGridLuaPersistentValueType::Bool);
	TestFalse(TEXT("GateOpen default is false"), Definitions[0].bDefaultBoolValue);
	TestEqual(TEXT("Second persistent id is stable"), Definitions[1].VariableId, FName(TEXT("RuneCount")));
	TestEqual(TEXT("RuneCount is Int32"), Definitions[1].Type, EGridLuaPersistentValueType::Int32);
	TestEqual(TEXT("RuneCount default is 2"), Definitions[1].DefaultInt32Value, 2);

	TMap<FName, bool> BoolValues;
	BoolValues.Add(TEXT("GateOpen"), false);
	TMap<FName, int32> IntValues;
	IntValues.Add(TEXT("RuneCount"), 7);
	const FGridLuaHostApi Host = MakeMON1971Host(BoolValues, IntValues);

	TestTrue(TEXT("First persistent callback succeeds"), Vm.CallEventFunction(TEXT("Puzzle"), TEXT("on_trigger"), MakeMON1971Event(), Host, Error));
	TestTrue(TEXT("Persistent Bool is committed"), BoolValues.FindRef(TEXT("GateOpen")));
	TestEqual(TEXT("Persistent Int starts from current store value"), IntValues.FindRef(TEXT("RuneCount")), 8);

	TestTrue(TEXT("Second persistent callback succeeds"), Vm.CallEventFunction(TEXT("Puzzle"), TEXT("on_trigger"), MakeMON1971Event(), Host, Error));
	TestEqual(TEXT("Persistent Int is synchronized before each callback"), IntValues.FindRef(TEXT("RuneCount")), 9);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON1971PersistentValidationTest, "Grimrock.MON19.7.1.LuaAuthoring.PersistentValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON1971PersistentValidationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FString Error;
	FGridLuaVm Vm;
	TestFalse(TEXT("Unsupported persistent default type is rejected"),
		Vm.Reload({ MakeMON1971Script(TEXT("BadType"), TEXT("persistent = { Bad = 'text' }\n")) }, FGridLuaVmConfig(), Error));
	TestTrue(TEXT("Unsupported type is diagnosed"), Error.Contains(TEXT("Bool or Int32")));

	TestFalse(TEXT("Conflicting persistent declarations are rejected"),
		Vm.Reload(
			{ MakeMON1971Script(TEXT("A"), TEXT("persistent = { Shared = false }\n")), MakeMON1971Script(TEXT("B"), TEXT("persistent = { Shared = 1 }\n")) },
			FGridLuaVmConfig(), Error));
	TestTrue(TEXT("Conflicting declaration is diagnosed"), Error.Contains(TEXT("conflicting")));

	const FGridLuaScriptSource TypoScript =
		MakeMON1971Script(TEXT("Typo"), TEXT("persistent = { GateOpen = false }\n") TEXT("function on_trigger(event) persistent.GateOepn = true end\n"));
	TestTrue(TEXT("Typo script loads before callback"), Vm.Reload({ TypoScript }, FGridLuaVmConfig(), Error));

	TMap<FName, bool> BoolValues;
	BoolValues.Add(TEXT("GateOpen"), false);
	TMap<FName, int32> IntValues;
	const FGridLuaHostApi Host = MakeMON1971Host(BoolValues, IntValues);
	TestFalse(
		TEXT("Undeclared persistent key is rejected after callback"), Vm.CallEventFunction(TEXT("Typo"), TEXT("on_trigger"), MakeMON1971Event(), Host, Error));
	TestTrue(TEXT("Undeclared persistent key is diagnosed"), Error.Contains(TEXT("was not declared")));
	TestFalse(TEXT("Rejected typo does not mutate authoritative Bool"), BoolValues.FindRef(TEXT("GateOpen")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON1971LegacyGridVarsCompatibilityTest, "Grimrock.MON19.7.1.LuaAuthoring.LegacyGridVarsCompatibility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON1971LegacyGridVarsCompatibilityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const FGridLuaScriptSource Script = MakeMON1971Script(TEXT("Legacy"),
		TEXT("persistent = { GateOpen = false }\n") TEXT("function on_trigger(event)\n") TEXT("  local ok, err = grid.vars.set_bool('GateOpen', true)\n")
			TEXT("  assert(ok, err)\n") TEXT("end\n"));

	FGridLuaVm Vm;
	FString Error;
	TestTrue(TEXT("Legacy compatibility VM loads"), Vm.Reload({ Script }, FGridLuaVmConfig(), Error));

	TMap<FName, bool> BoolValues;
	BoolValues.Add(TEXT("GateOpen"), false);
	TMap<FName, int32> IntValues;
	const FGridLuaHostApi Host = MakeMON1971Host(BoolValues, IntValues);
	TestTrue(TEXT("Legacy grid.vars callback succeeds"), Vm.CallEventFunction(TEXT("Legacy"), TEXT("on_trigger"), MakeMON1971Event(), Host, Error));
	TestTrue(TEXT("Unchanged persistent table does not overwrite direct grid.vars mutation"), BoolValues.FindRef(TEXT("GateOpen")));
	return true;
}

#endif
