#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Core/GridLevelVariableTypes.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GridLuaScriptTypes.h"
#include "Runtime/GridActivationComponent.h"
#include "Runtime/GridDungeonRuntimeState.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridLevelVariableStore.h"

namespace
{
	FGridLevelVariableDefinition MakeBoolVariable194(FName Id, bool bDefault = false)
	{
		FGridLevelVariableDefinition Definition;
		Definition.VariableId = Id;
		Definition.Type = EGridLevelVariableType::Bool;
		Definition.bDefaultBoolValue = bDefault;
		return Definition;
	}

	FGridLevelVariableDefinition MakeIntVariable194(FName Id, int32 DefaultValue = 0)
	{
		FGridLevelVariableDefinition Definition;
		Definition.VariableId = Id;
		Definition.Type = EGridLevelVariableType::Int32;
		Definition.DefaultInt32Value = DefaultValue;
		return Definition;
	}

	FGridLuaScriptSource MakeLuaScript194(FName ScriptId, const FString& Source)
	{
		FGridLuaScriptSource Script;
		Script.ScriptId = ScriptId;
		Script.bEnabled = true;
		Script.Source = Source;
		return Script;
	}

	FGridLevelObjectData MakeObject194(FGuid ObjectId, EGridLevelObjectType Type)
	{
		FGridLevelObjectData Object;
		Object.ObjectId = ObjectId;
		Object.Type = Type;
		return Object;
	}

	FGridObjectLink MakeLuaLink194(FGuid SourceId, FName ScriptId, FName CallbackName)
	{
		FGridObjectLink Link;
		Link.SourceObjectId = SourceId;
		Link.SourceEvent = EGridObjectEvent::Activated;
		Link.Command = EGridObjectCommand::LuaCallback;
		Link.LuaScriptId = ScriptId;
		Link.LuaCallbackName = CallbackName;
		return Link;
	}

	struct FMON194TestWorld
	{
		UWorld* World = nullptr;

		FMON194TestWorld()
		{
			const UWorld::InitializationValues Values = UWorld::InitializationValues()
															.AllowAudioPlayback(false)
															.RequiresHitProxies(false)
															.CreatePhysicsScene(false)
															.CreateNavigation(false)
															.CreateAISystem(false)
															.ShouldSimulatePhysics(false)
															.SetTransactional(false);

			World = UWorld::CreateWorld(EWorldType::Game, false, FName(*FString::Printf(TEXT("MON194_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
				nullptr, true, ERHIFeatureLevel::Num, &Values);

			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FMON194TestWorld()
		{
			if (!World)
			{
				return;
			}

			World->DestroyWorld(false);
			if (GEngine)
			{
				GEngine->DestroyWorldContext(World);
			}
		}
	};

	struct FMON194RuntimeFixture
	{
		FMON194TestWorld TestWorld;
		AGridLevelRuntimeActor* Runtime = nullptr;
		UGridLevelAsset* Level = nullptr;
		UGridActivationComponent* Activation = nullptr;

		bool Initialize(FAutomationTestBase& Test)
		{
			if (!TestWorld.World)
			{
				Test.AddError(TEXT("Unable to create MON19.4 test world."));
				return false;
			}

			Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
			if (!Runtime)
			{
				Test.AddError(TEXT("Unable to spawn MON19.4 runtime actor."));
				return false;
			}

			Level = NewObject<UGridLevelAsset>(Runtime);
			Level->Width = 1;
			Level->Height = 1;
			Level->EnsureCellCount();
			Level->Cells[0].CellType = EGridCellType::Floor;
			Level->LevelVariables = { MakeBoolVariable194(TEXT("Gate"), false), MakeIntVariable194(TEXT("Count"), 1) };

			Runtime->LevelAsset = Level;
			Runtime->CurrentDungeonLevelId = TEXT("MON194");
			Activation = Runtime->FindComponentByClass<UGridActivationComponent>();
			if (!Activation)
			{
				Test.AddError(TEXT("MON19.4 activation component is missing."));
				return false;
			}
			return true;
		}

		bool FinalizeLevel(FAutomationTestBase& Test)
		{
			FString Error;
			Activation->Initialize(Runtime);
			Activation->RebuildIndexes();
			if (!Activation->ReloadLuaRuntime(&Error))
			{
				Test.AddError(FString::Printf(TEXT("MON19.4 Lua runtime failed to load: %s"), *Error));
				return false;
			}
			return true;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON194EventContextAndVarsTest, "Grimrock.MON19.4.LuaBridge.EventContextAndVariables",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON194EventContextAndVarsTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FMON194RuntimeFixture Fixture;
	if (!Fixture.Initialize(*this))
	{
		return false;
	}

	const FGuid SourceId(19, 4, 1, 1);
	Fixture.Level->Objects.Add(MakeObject194(SourceId, EGridLevelObjectType::Trigger));

	const FString ExpectedSource = SourceId.ToString();
	const FString ScriptSource =
		FString::Printf(TEXT("function on_trigger(event)\n") TEXT("  local count, err = grid.vars.get_int('Count')\n") TEXT("  assert(err == nil, err)\n")
							TEXT("  local ok\n") TEXT("  ok, err = grid.vars.set_int('Count', count + 4)\n") TEXT("  assert(ok, err)\n")
								TEXT("  local context_ok = event.source_object_id == '%s' and event.event == 'Activated'\n")
									TEXT("  ok, err = grid.vars.set_bool('Gate', context_ok)\n") TEXT("  assert(ok, err)\n") TEXT("end\n"),
			*ExpectedSource);

	Fixture.Level->LuaScripts.Add(MakeLuaScript194(TEXT("Puzzle"), ScriptSource));
	Fixture.Level->Links.Add(MakeLuaLink194(SourceId, TEXT("Puzzle"), TEXT("on_trigger")));

	if (!Fixture.FinalizeLevel(*this))
	{
		return false;
	}

	TestTrue(TEXT("Trigger event invokes Lua callback"), Fixture.Activation->ExecuteLinksFromObjectForEvent(SourceId, EGridObjectEvent::Activated));

	FGridLevelRuntimeState* State = Fixture.Runtime->GetOrCreateRuntimeStateForCurrentLevel();
	TestNotNull(TEXT("Runtime state exists after Lua callback"), State);
	if (!State)
	{
		return false;
	}

	FString Error;
	bool bGate = false;
	int32 Count = 0;
	TestTrue(TEXT("Lua wrote Gate through authoritative store"), GridLevelVariableStore::TryGetBool(*Fixture.Level, *State, TEXT("Gate"), bGate, Error));
	TestTrue(TEXT("Lua event context matches source/event"), bGate);
	TestTrue(TEXT("Lua wrote Count through authoritative store"), GridLevelVariableStore::TryGetInt32(*Fixture.Level, *State, TEXT("Count"), Count, Error));
	TestEqual(TEXT("Lua incremented Count"), Count, 5);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON194LuaCommandToLogicTest, "Grimrock.MON19.4.LuaBridge.CommandToExistingRuntime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON194LuaCommandToLogicTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FMON194RuntimeFixture Fixture;
	if (!Fixture.Initialize(*this))
	{
		return false;
	}

	const FGuid SourceId(19, 4, 2, 1);
	const FGuid LogicId(19, 4, 2, 2);
	Fixture.Level->Objects.Add(MakeObject194(SourceId, EGridLevelObjectType::Trigger));

	FGridLevelObjectData AddInt = MakeObject194(LogicId, EGridLevelObjectType::Logic);
	AddInt.Logic.NodeType = EGridLogicNodeType::AddInt;
	AddInt.Logic.VariableId = TEXT("Count");
	AddInt.Logic.IntValue = 3;
	Fixture.Level->Objects.Add(AddInt);

	const FString ScriptSource = FString::Printf(TEXT("function on_trigger(event)\n") TEXT("  local ok, err = grid.command('%s', 'LogicExecute')\n")
													 TEXT("  assert(ok, err)\n") TEXT("end\n"),
		*LogicId.ToString());

	Fixture.Level->LuaScripts.Add(MakeLuaScript194(TEXT("Puzzle"), ScriptSource));
	Fixture.Level->Links.Add(MakeLuaLink194(SourceId, TEXT("Puzzle"), TEXT("on_trigger")));

	if (!Fixture.FinalizeLevel(*this))
	{
		return false;
	}

	TestTrue(
		TEXT("Lua callback can command existing Logic runtime"), Fixture.Activation->ExecuteLinksFromObjectForEvent(SourceId, EGridObjectEvent::Activated));

	FGridLevelRuntimeState* State = Fixture.Runtime->GetOrCreateRuntimeStateForCurrentLevel();
	FString Error;
	int32 Count = 0;
	TestTrue(TEXT("Count is readable after grid.command"), State && GridLevelVariableStore::TryGetInt32(*Fixture.Level, *State, TEXT("Count"), Count, Error));
	TestEqual(TEXT("grid.command reused AddInt implementation"), Count, 4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMON194VariableConditionTest, "Grimrock.MON19.4.LuaBridge.LinkCondition", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON194VariableConditionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FMON194RuntimeFixture Fixture;
	if (!Fixture.Initialize(*this))
	{
		return false;
	}

	const FGuid SourceId(19, 4, 3, 1);
	Fixture.Level->Objects.Add(MakeObject194(SourceId, EGridLevelObjectType::Trigger));
	Fixture.Level->LuaScripts.Add(MakeLuaScript194(TEXT("Conditional"),
		TEXT("function on_trigger(event)\n") TEXT("  local count, err = grid.vars.get_int('Count')\n") TEXT("  assert(err == nil, err)\n") TEXT("  local ok\n")
			TEXT("  ok, err = grid.vars.set_int('Count', count + 1)\n") TEXT("  assert(ok, err)\n") TEXT("end\n")));

	FGridObjectLink Link = MakeLuaLink194(SourceId, TEXT("Conditional"), TEXT("on_trigger"));
	Link.Condition = EGridObjectCondition::LevelVariableBoolEquals;
	Link.ConditionVariableId = TEXT("Gate");
	Link.ConditionBoolValue = true;
	Fixture.Level->Links.Add(Link);

	if (!Fixture.FinalizeLevel(*this))
	{
		return false;
	}

	TestFalse(
		TEXT("False level-variable condition blocks Lua callback"), Fixture.Activation->ExecuteLinksFromObjectForEvent(SourceId, EGridObjectEvent::Activated));

	FGridLevelRuntimeState* State = Fixture.Runtime->GetOrCreateRuntimeStateForCurrentLevel();
	TestNotNull(TEXT("Conditional test state exists"), State);
	if (!State)
	{
		return false;
	}

	FString Error;
	int32 Count = 0;
	GridLevelVariableStore::TryGetInt32(*Fixture.Level, *State, TEXT("Count"), Count, Error);
	TestEqual(TEXT("Blocked callback does not mutate Count"), Count, 1);

	TestTrue(TEXT("Gate can be enabled"), GridLevelVariableStore::SetBool(*Fixture.Level, *State, TEXT("Gate"), true, Error));
	TestTrue(
		TEXT("True level-variable condition permits Lua callback"), Fixture.Activation->ExecuteLinksFromObjectForEvent(SourceId, EGridObjectEvent::Activated));
	GridLevelVariableStore::TryGetInt32(*Fixture.Level, *State, TEXT("Count"), Count, Error);
	TestEqual(TEXT("Permitted callback mutates Count once"), Count, 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMON194HostFailureTest, "Grimrock.MON19.4.LuaBridge.HostFailureIsProtected", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON194HostFailureTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FMON194RuntimeFixture Fixture;
	if (!Fixture.Initialize(*this))
	{
		return false;
	}

	const FGuid SourceId(19, 4, 4, 1);
	Fixture.Level->Objects.Add(MakeObject194(SourceId, EGridLevelObjectType::Trigger));
	Fixture.Level->LuaScripts.Add(MakeLuaScript194(TEXT("Failures"),
		TEXT("function on_trigger(event)\n") TEXT("  local ok, err = grid.command('not-a-guid', 'Open')\n")
			TEXT("  local handled = ok == false and type(err) == 'string'\n") TEXT("  local set_ok, set_err = grid.vars.set_bool('Gate', handled)\n")
				TEXT("  assert(set_ok, set_err)\n") TEXT("end\n")));
	Fixture.Level->Links.Add(MakeLuaLink194(SourceId, TEXT("Failures"), TEXT("on_trigger")));

	if (!Fixture.FinalizeLevel(*this))
	{
		return false;
	}

	TestTrue(TEXT("Host failure is returned to Lua without aborting callback"),
		Fixture.Activation->ExecuteLinksFromObjectForEvent(SourceId, EGridObjectEvent::Activated));

	FGridLevelRuntimeState* State = Fixture.Runtime->GetOrCreateRuntimeStateForCurrentLevel();
	FString Error;
	bool bGate = false;
	TestTrue(
		TEXT("Gate is readable after protected host failure"), State && GridLevelVariableStore::TryGetBool(*Fixture.Level, *State, TEXT("Gate"), bGate, Error));
	TestTrue(TEXT("Lua observed grid.command failure as data"), bGate);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON194SharedActionBudgetTest, "Grimrock.MON19.4.LuaBridge.SharedActionBudget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON194SharedActionBudgetTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FMON194RuntimeFixture Fixture;
	if (!Fixture.Initialize(*this))
	{
		return false;
	}

	const FGuid SourceId(19, 4, 5, 1);
	Fixture.Level->Objects.Add(MakeObject194(SourceId, EGridLevelObjectType::Trigger));

	constexpr int32 RelayCount = 132;
	TArray<FGuid> RelayIds;
	RelayIds.Reserve(RelayCount);
	for (int32 Index = 0; Index < RelayCount; ++Index)
	{
		const FGuid RelayId(19, 4, 50, Index + 1);
		RelayIds.Add(RelayId);
		FGridLevelObjectData Relay = MakeObject194(RelayId, EGridLevelObjectType::Logic);
		Relay.Logic.NodeType = EGridLogicNodeType::Relay;
		Fixture.Level->Objects.Add(Relay);
	}

	const FGuid FinalId(19, 4, 51, 1);
	FGridLevelObjectData FinalSet = MakeObject194(FinalId, EGridLevelObjectType::Logic);
	FinalSet.Logic.NodeType = EGridLogicNodeType::SetBool;
	FinalSet.Logic.VariableId = TEXT("Gate");
	FinalSet.Logic.bBoolValue = true;
	Fixture.Level->Objects.Add(FinalSet);

	FGridObjectLink First;
	First.SourceObjectId = SourceId;
	First.SourceEvent = EGridObjectEvent::Activated;
	First.TargetObjectId = RelayIds[0];
	First.Command = EGridObjectCommand::LogicExecute;
	Fixture.Level->Links.Add(First);

	for (int32 Index = 0; Index + 1 < RelayIds.Num(); ++Index)
	{
		FGridObjectLink Link;
		Link.SourceObjectId = RelayIds[Index];
		Link.SourceEvent = EGridObjectEvent::Activated;
		Link.TargetObjectId = RelayIds[Index + 1];
		Link.Command = EGridObjectCommand::LogicExecute;
		Fixture.Level->Links.Add(Link);
	}

	FGridObjectLink FinalLink;
	FinalLink.SourceObjectId = RelayIds.Last();
	FinalLink.SourceEvent = EGridObjectEvent::Activated;
	FinalLink.TargetObjectId = FinalId;
	FinalLink.Command = EGridObjectCommand::LogicExecute;
	Fixture.Level->Links.Add(FinalLink);

	if (!Fixture.FinalizeLevel(*this))
	{
		return false;
	}

	Fixture.Activation->ExecuteLinksFromObjectForEvent(SourceId, EGridObjectEvent::Activated);

	FGridLevelRuntimeState* State = Fixture.Runtime->GetOrCreateRuntimeStateForCurrentLevel();
	FString Error;
	bool bGate = false;
	TestTrue(TEXT("Gate remains readable after budget stop"), State && GridLevelVariableStore::TryGetBool(*Fixture.Level, *State, TEXT("Gate"), bGate, Error));
	TestFalse(TEXT("Shared runtime action budget stops chain before final mutation"), bGate);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
