#if WITH_DEV_AUTOMATION_TESTS

#include "Core/GridLevelAsset.h"
#include "Core/GridLevelVariableTypes.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GridLuaScriptTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Runtime/GridActivationComponent.h"
#include "Runtime/GridDungeonRuntimeState.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridLevelVariableStore.h"
#include "Save/GrimrockPartySaveGame.h"

namespace
{
	const FGuid MON195SourceId(19, 5, 1, 1);

	FGridLevelVariableDefinition MakeBoolVariable195(FName Id, bool bDefault = false)
	{
		FGridLevelVariableDefinition Definition;
		Definition.VariableId = Id;
		Definition.Type = EGridLevelVariableType::Bool;
		Definition.bDefaultBoolValue = bDefault;
		return Definition;
	}

	FGridLevelVariableDefinition MakeIntVariable195(FName Id, int32 DefaultValue = 0)
	{
		FGridLevelVariableDefinition Definition;
		Definition.VariableId = Id;
		Definition.Type = EGridLevelVariableType::Int32;
		Definition.DefaultInt32Value = DefaultValue;
		return Definition;
	}

	FGridLuaScriptSource MakeLuaScript195(const FString& Source)
	{
		FGridLuaScriptSource Script;
		Script.ScriptId = TEXT("Persistence");
		Script.bEnabled = true;
		Script.Source = Source;
		return Script;
	}

	UGridLevelAsset* MakeLevel195(UObject* Outer, const FString& ScriptSource)
	{
		UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Outer);
		Level->Width = 1;
		Level->Height = 1;
		Level->EnsureCellCount();
		Level->Cells[0].CellType = EGridCellType::Floor;
		Level->LevelVariables = { MakeBoolVariable195(TEXT("Gate"), false), MakeIntVariable195(TEXT("Count"), 10),
			MakeIntVariable195(TEXT("SessionObserved"), 0) };

		FGridLevelObjectData Source;
		Source.ObjectId = MON195SourceId;
		Source.Type = EGridLevelObjectType::Trigger;
		Level->Objects.Add(Source);

		FGridObjectLink Link;
		Link.SourceObjectId = MON195SourceId;
		Link.SourceEvent = EGridObjectEvent::Activated;
		Link.Command = EGridObjectCommand::LuaCallback;
		Link.LuaScriptId = TEXT("Persistence");
		Link.LuaCallbackName = TEXT("on_trigger");
		Level->Links.Add(Link);

		Level->LuaScripts.Add(MakeLuaScript195(ScriptSource));
		return Level;
	}

	struct FGridMON195TestWorld
	{
		UWorld* World = nullptr;

		FGridMON195TestWorld()
		{
			const UWorld::InitializationValues Values = UWorld::InitializationValues()
															.AllowAudioPlayback(false)
															.RequiresHitProxies(false)
															.CreatePhysicsScene(false)
															.CreateNavigation(false)
															.CreateAISystem(false)
															.ShouldSimulatePhysics(false)
															.SetTransactional(false);
			World = UWorld::CreateWorld(EWorldType::Game, false, FName(*FString::Printf(TEXT("MON195_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
				nullptr, true, ERHIFeatureLevel::Num, &Values);
			if (!World || !GEngine)
			{
				return;
			}

			FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
			Context.SetCurrentWorld(World);
		}

		~FGridMON195TestWorld()
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

	AGridLevelRuntimeActor* SpawnRuntime195(
		FAutomationTestBase& Test, UWorld* World, UGridLevelAsset* Level, const FGridDungeonRuntimeState* RestoredDungeonState = nullptr)
	{
		AGridLevelRuntimeActor* Runtime = World ? World->SpawnActor<AGridLevelRuntimeActor>() : nullptr;
		if (!Runtime)
		{
			Test.AddError(TEXT("Unable to spawn MON19.5 runtime actor."));
			return nullptr;
		}

		Runtime->LevelAsset = Level;
		if (RestoredDungeonState)
		{
			Runtime->DungeonRuntimeState = *RestoredDungeonState;
		}
		Runtime->RebuildLevel();
		return Runtime;
	}

	bool Execute195(AGridLevelRuntimeActor* Runtime)
	{
		return Runtime && Runtime->ExecuteLinksFromRuntimeObject(MON195SourceId, EGridObjectEvent::Activated);
	}

	bool ReadInt195(AGridLevelRuntimeActor* Runtime, FName VariableId, int32& OutValue, FString& OutError)
	{
		if (!Runtime || !Runtime->LevelAsset)
		{
			return false;
		}
		FGridLevelRuntimeState* State = Runtime->GetOrCreateRuntimeStateForCurrentLevel();
		return State && GridLevelVariableStore::TryGetInt32(*Runtime->LevelAsset, *State, VariableId, OutValue, OutError);
	}

	bool ReadBool195(AGridLevelRuntimeActor* Runtime, FName VariableId, bool& OutValue, FString& OutError)
	{
		if (!Runtime || !Runtime->LevelAsset)
		{
			return false;
		}
		FGridLevelRuntimeState* State = Runtime->GetOrCreateRuntimeStateForCurrentLevel();
		return State && GridLevelVariableStore::TryGetBool(*Runtime->LevelAsset, *State, VariableId, OutValue, OutError);
	}

	UGrimrockPartySaveGame* RoundTripDungeonState195(FAutomationTestBase& Test, const FGridDungeonRuntimeState& DungeonState)
	{
		UGrimrockPartySaveGame* Source = NewObject<UGrimrockPartySaveGame>(GetTransientPackage());
		Source->DungeonRuntimeState = DungeonState;

		TArray<uint8> SaveBytes;
		if (!Test.TestTrue(TEXT("MON19.5 SaveGame serializes through the real UE SaveGame archive"), UGameplayStatics::SaveGameToMemory(Source, SaveBytes)))
		{
			return nullptr;
		}

		UGrimrockPartySaveGame* Loaded = Cast<UGrimrockPartySaveGame>(UGameplayStatics::LoadGameFromMemory(SaveBytes));
		Test.TestNotNull(TEXT("MON19.5 SaveGame deserializes through the real UE SaveGame archive"), Loaded);
		return Loaded;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON195SaveRestoreFreshVmTest, "Grimrock.MON19.5.LuaPersistence.SaveRoundTripFreshVm",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON195SaveRestoreFreshVmTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON195TestWorld TestWorld;
	if (!TestWorld.World)
	{
		AddError(TEXT("Unable to create MON19.5 test world."));
		return false;
	}

	const FString SourceCode = TEXT("session_counter = 0\n") TEXT("function on_trigger(event)\n") TEXT("  session_counter = session_counter + 1\n")
		TEXT("  local count, err = grid.vars.get_int('Count')\n") TEXT("  assert(err == nil, err)\n") TEXT("  local ok\n")
			TEXT("  ok, err = grid.vars.set_int('Count', count + 1)\n") TEXT("  assert(ok, err)\n")
				TEXT("  ok, err = grid.vars.set_int('SessionObserved', session_counter)\n") TEXT("  assert(ok, err)\n") TEXT("end\n");

	UGridLevelAsset* Level = MakeLevel195(GetTransientPackage(), SourceCode);
	AGridLevelRuntimeActor* SourceRuntime = SpawnRuntime195(*this, TestWorld.World, Level);
	if (!SourceRuntime)
	{
		return false;
	}

	TestTrue(TEXT("First Lua callback executes"), Execute195(SourceRuntime));
	TestTrue(TEXT("Second Lua callback executes"), Execute195(SourceRuntime));

	FString Error;
	int32 Count = 0;
	int32 SessionObserved = 0;
	TestTrue(TEXT("Count is readable before save"), ReadInt195(SourceRuntime, TEXT("Count"), Count, Error));
	TestEqual(TEXT("Typed Count reached 12 before save"), Count, 12);
	TestTrue(TEXT("SessionObserved is readable before save"), ReadInt195(SourceRuntime, TEXT("SessionObserved"), SessionObserved, Error));
	TestEqual(TEXT("Plain Lua global reached two before save"), SessionObserved, 2);

	TestTrue(TEXT("Current runtime state captures before SaveGame"), SourceRuntime->CaptureCurrentLevelRuntimeState());
	UGrimrockPartySaveGame* Loaded = RoundTripDungeonState195(*this, SourceRuntime->DungeonRuntimeState);
	if (!Loaded)
	{
		return false;
	}
	TestEqual(TEXT("MON19.5 round-trip uses the current SaveVersion"), Loaded->SaveVersion, UGrimrockPartySaveGame::CurrentSaveVersion);

	AGridLevelRuntimeActor* RestoredRuntime = SpawnRuntime195(*this, TestWorld.World, Level, &Loaded->DungeonRuntimeState);
	if (!RestoredRuntime)
	{
		return false;
	}
	TestTrue(TEXT("Saved runtime snapshot applies to rebuilt level"), RestoredRuntime->ApplyCurrentLevelRuntimeState());
	TestTrue(TEXT("Fresh VM executes after restore"), Execute195(RestoredRuntime));

	Count = 0;
	SessionObserved = 0;
	TestTrue(TEXT("Restored Count is readable"), ReadInt195(RestoredRuntime, TEXT("Count"), Count, Error));
	TestEqual(TEXT("Fresh VM continues from persisted typed Count"), Count, 13);
	TestTrue(TEXT("Restored session observation is readable"), ReadInt195(RestoredRuntime, TEXT("SessionObserved"), SessionObserved, Error));
	TestEqual(TEXT("Plain Lua global is recreated from source instead of SaveGame"), SessionObserved, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON195CurrentSourceWinsTest, "Grimrock.MON19.5.LuaPersistence.CurrentScriptSourceWins",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON195CurrentSourceWinsTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON195TestWorld TestWorld;
	if (!TestWorld.World)
	{
		AddError(TEXT("Unable to create MON19.5 source-wins test world."));
		return false;
	}

	const FString VersionOne =
		TEXT("function on_trigger(event)\n") TEXT("  local count, err = grid.vars.get_int('Count')\n") TEXT("  assert(err == nil, err)\n") TEXT("  local ok\n")
			TEXT("  ok, err = grid.vars.set_int('Count', count + 1)\n") TEXT("  assert(ok, err)\n") TEXT("end\n");
	UGridLevelAsset* Level = MakeLevel195(GetTransientPackage(), VersionOne);
	AGridLevelRuntimeActor* SourceRuntime = SpawnRuntime195(*this, TestWorld.World, Level);
	if (!SourceRuntime)
	{
		return false;
	}

	TestTrue(TEXT("Version-one callback executes"), Execute195(SourceRuntime));
	TestTrue(TEXT("Runtime state captures version-one result"), SourceRuntime->CaptureCurrentLevelRuntimeState());
	UGrimrockPartySaveGame* Loaded = RoundTripDungeonState195(*this, SourceRuntime->DungeonRuntimeState);
	if (!Loaded)
	{
		return false;
	}

	const FString VersionTwo = TEXT("function on_trigger(event)\n") TEXT("  local count, err = grid.vars.get_int('Count')\n")
		TEXT("  assert(err == nil, err)\n") TEXT("  local ok\n") TEXT("  ok, err = grid.vars.set_int('Count', count + 100)\n") TEXT("  assert(ok, err)\n")
			TEXT("  ok, err = grid.vars.set_bool('Gate', true)\n") TEXT("  assert(ok, err)\n") TEXT("end\n");
	Level->LuaScripts[0].Source = VersionTwo;

	AGridLevelRuntimeActor* RestoredRuntime = SpawnRuntime195(*this, TestWorld.World, Level, &Loaded->DungeonRuntimeState);
	if (!RestoredRuntime)
	{
		return false;
	}
	TestTrue(TEXT("Saved runtime snapshot applies before current script"), RestoredRuntime->ApplyCurrentLevelRuntimeState());
	TestTrue(TEXT("Current version-two callback executes"), Execute195(RestoredRuntime));

	FString Error;
	int32 Count = 0;
	bool bGate = false;
	TestTrue(TEXT("Count is readable after current source callback"), ReadInt195(RestoredRuntime, TEXT("Count"), Count, Error));
	TestEqual(TEXT("Current LevelAsset script source wins over saved-time source"), Count, 111);
	TestTrue(TEXT("Gate is readable after current source callback"), ReadBool195(RestoredRuntime, TEXT("Gate"), bGate, Error));
	TestTrue(TEXT("Only version-two behavior sets Gate"), bGate);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON195InvalidCurrentSourceDropsVmTest, "Grimrock.MON19.5.LuaPersistence.InvalidCurrentSourceDropsStaleVm",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON195InvalidCurrentSourceDropsVmTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON195TestWorld TestWorld;
	if (!TestWorld.World)
	{
		AddError(TEXT("Unable to create MON19.5 invalid-source test world."));
		return false;
	}

	const FString ValidSource =
		TEXT("function on_trigger(event)\n") TEXT("  local ok, err = grid.vars.set_bool('Gate', true)\n") TEXT("  assert(ok, err)\n") TEXT("end\n");
	UGridLevelAsset* Level = MakeLevel195(GetTransientPackage(), ValidSource);
	AGridLevelRuntimeActor* Runtime = SpawnRuntime195(*this, TestWorld.World, Level);
	if (!Runtime)
	{
		return false;
	}

	TestTrue(TEXT("Valid pre-restore VM executes"), Execute195(Runtime));
	FGridLevelRuntimeState* State = Runtime->GetOrCreateRuntimeStateForCurrentLevel();
	TestNotNull(TEXT("Runtime state exists"), State);
	if (!State)
	{
		return false;
	}
	FString Error;
	TestTrue(TEXT("Gate can be reset before invalid authoritative rebuild"), GridLevelVariableStore::SetBool(*Level, *State, TEXT("Gate"), false, Error));

	Level->LuaScripts[0].Source = TEXT("function on_trigger(\n");
	Runtime->RebuildLevel();

	TestFalse(TEXT("Invalid current source cannot execute stale pre-rebuild callback"), Execute195(Runtime));

	bool bGate = true;
	TestTrue(TEXT("Gate remains readable after failed Lua rebuild"), ReadBool195(Runtime, TEXT("Gate"), bGate, Error));
	TestFalse(TEXT("Stale VM never mutates gameplay state after invalid rebuild"), bGate);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON195SaveVersionContractTest, "Grimrock.MON19.5.LuaPersistence.SaveVersionContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON195SaveVersionContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	TestTrue(TEXT("Lua persistence keeps MON19.2.2 SaveVersion seven or later"), UGrimrockPartySaveGame::CurrentSaveVersion >= 7);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
