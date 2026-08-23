#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Core/GridLevelVariableTypes.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GridLuaScriptTypes.h"
#include "Runtime/GridActivationComponent.h"
#include "Runtime/GridDoorActor.h"
#include "Runtime/GridDoorSystemComponent.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridLevelVariableStore.h"

namespace GridMON198Tests
{
    struct FMON198TestWorld
    {
        UWorld* World = nullptr;

        FMON198TestWorld ()
        {
            const UWorld::InitializationValues Values =
                UWorld::InitializationValues ()
                    .AllowAudioPlayback (false)
                    .RequiresHitProxies (false)
                    .CreatePhysicsScene (false)
                    .CreateNavigation (false)
                    .CreateAISystem (false)
                    .ShouldSimulatePhysics (false)
                    .SetTransactional (false);

            World = UWorld::CreateWorld (
                EWorldType::Game,
                false,
                FName (*FString::Printf (
                    TEXT ("MON198_%s"),
                    *FGuid::NewGuid ().ToString (EGuidFormats::Digits))),
                nullptr,
                true,
                ERHIFeatureLevel::Num,
                &Values);

            if (World && GEngine)
            {
                FWorldContext& Context =
                    GEngine->CreateNewWorldContext (EWorldType::Game);
                Context.SetCurrentWorld (World);
            }
        }

        ~FMON198TestWorld ()
        {
            if (!World)
            {
                return;
            }
            World->DestroyWorld (false);
            if (GEngine)
            {
                GEngine->DestroyWorldContext (World);
            }
        }
    };

    FGridLevelVariableDefinition MakeIntVariable (
        FName VariableId,
        int32 DefaultValue = 0)
    {
        FGridLevelVariableDefinition Definition;
        Definition.VariableId = VariableId;
        Definition.Type = EGridLevelVariableType::Int32;
        Definition.DefaultInt32Value = DefaultValue;
        return Definition;
    }

    FGridLevelObjectData MakeLogicNode (
        EGridLogicNodeType NodeType,
        FGuid ObjectId,
        FName VariableId = NAME_None)
    {
        FGridLevelObjectData Node;
        Node.ObjectId = ObjectId;
        Node.Type = EGridLevelObjectType::Logic;
        Node.Logic.NodeType = NodeType;
        Node.Logic.VariableId = VariableId;
        return Node;
    }

    UGridLevelAsset* MakeBaseLevel (UObject* Outer)
    {
        UGridLevelAsset* Level = NewObject<UGridLevelAsset> (Outer);
        Level->Width = 2;
        Level->Height = 1;
        Level->EnsureCellCount ();
        for (FGridLevelCellData& Cell : Level->Cells)
        {
            Cell.CellType = EGridCellType::Floor;
        }
        return Level;
    }

    FGridLevelObjectData MakeDoor (
        FGuid ObjectId,
        FName LogicId = TEXT ("SecretDoor"))
    {
        FGridLevelObjectData Door;
        Door.ObjectId = ObjectId;
        Door.LogicId = LogicId;
        Door.Type = EGridLevelObjectType::Door;
        Door.CellX = 0;
        Door.CellY = 0;
        Door.Edge = EGridEdge::East;
        Door.bInitiallyActive = false;
        return Door;
    }

    bool PrepareRuntime (
        UWorld& World,
        AGridLevelRuntimeActor*& OutRuntime,
        UGridLevelAsset*& OutLevel,
        UGridActivationComponent*& OutActivation)
    {
        OutRuntime = World.SpawnActor<AGridLevelRuntimeActor> ();
        if (!OutRuntime)
        {
            return false;
        }

        OutLevel = MakeBaseLevel (OutRuntime);
        OutRuntime->LevelAsset = OutLevel;
        OutRuntime->CurrentDungeonLevelId = TEXT ("MON198");

        OutActivation =
            OutRuntime->FindComponentByClass<UGridActivationComponent> ();
        return OutActivation != nullptr;
    }

    bool PrepareClosedDoor (
        UWorld& World,
        AGridLevelRuntimeActor& Runtime,
        const FGridLevelObjectData& DoorData)
    {
        UGridDoorSystemComponent* DoorSystem =
            Runtime.FindComponentByClass<UGridDoorSystemComponent> ();
        if (!DoorSystem)
        {
            return false;
        }

        DoorSystem->Initialize (&Runtime);
        DoorSystem->RebuildIndexes ();

        AGridDoorActor* DoorActor = World.SpawnActor<AGridDoorActor> ();
        if (!DoorActor)
        {
            return false;
        }

        DoorActor->InitializeDoor (
            DoorData,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            false);
        DoorSystem->RegisterDoorObject (DoorData, DoorActor);
        return !Runtime.IsDoorOpenOnEdge (
            DoorData.CellX,
            DoorData.CellY,
            DoorData.Edge);
    }

    FGridObjectLink MakeLink (
        FGuid SourceId,
        EGridObjectEvent SourceEvent,
        FGuid TargetId,
        EGridObjectCommand Command)
    {
        FGridObjectLink Link;
        Link.SourceObjectId = SourceId;
        Link.SourceEvent = SourceEvent;
        Link.TargetObjectId = TargetId;
        Link.Command = Command;
        return Link;
    }

    FGridObjectLink MakeLuaLink (
        FGuid SourceId,
        EGridObjectEvent SourceEvent,
        FName ScriptId,
        FName CallbackName)
    {
        FGridObjectLink Link;
        Link.SourceObjectId = SourceId;
        Link.SourceEvent = SourceEvent;
        Link.Command = EGridObjectCommand::LuaCallback;
        Link.LuaScriptId = ScriptId;
        Link.LuaCallbackName = CallbackName;
        return Link;
    }

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMON198PuzzleADirectLeverDoorTest,
    "Grimrock.MON19.8.ProductionPuzzles.A_DirectLeverDoor",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMON198PuzzleADirectLeverDoorTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    FMON198TestWorld TestWorld;
    if (!TestWorld.World)
    {
        AddError (TEXT ("Unable to create MON19.8 Puzzle A world."));
        return false;
    }

    AGridLevelRuntimeActor* Runtime = nullptr;
    UGridLevelAsset* Level = nullptr;
    UGridActivationComponent* Activation = nullptr;
    if (!PrepareRuntime (*TestWorld.World, Runtime, Level, Activation))
    {
        AddError (TEXT ("Unable to prepare MON19.8 Puzzle A runtime."));
        return false;
    }

    const FGuid LeverId (19, 8, 1, 1);
    FGridLevelObjectData Lever;
    Lever.ObjectId = LeverId;
    Lever.Type = EGridLevelObjectType::Lever;
    Level->Objects.Add (Lever);

    const FGridLevelObjectData Door = MakeDoor (FGuid (19, 8, 1, 2));
    Level->Objects.Add (Door);
    Level->Links.Add (MakeLink (
        LeverId,
        EGridObjectEvent::Activated,
        Door.ObjectId,
        EGridObjectCommand::Open));

    Activation->Initialize (Runtime);
    Activation->RebuildIndexes ();
    if (!PrepareClosedDoor (*TestWorld.World, *Runtime, Door))
    {
        AddError (TEXT ("MON19.8 Puzzle A door fixture is not closed."));
        return false;
    }

    TestTrue (
        TEXT ("Lever Activated dispatches direct Door.Open"),
        Runtime->ExecuteLinksFromRuntimeObject (
            LeverId,
            EGridObjectEvent::Activated));
    TestTrue (
        TEXT ("Direct data-driven puzzle opens the door without Lua"),
        Runtime->IsDoorOpenOnEdge (Door.CellX, Door.CellY, Door.Edge));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMON198PuzzleBLogicCounterThresholdTest,
    "Grimrock.MON19.8.ProductionPuzzles.B_LogicCounterThreshold",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMON198PuzzleBLogicCounterThresholdTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    FMON198TestWorld TestWorld;
    if (!TestWorld.World)
    {
        AddError (TEXT ("Unable to create MON19.8 Puzzle B world."));
        return false;
    }

    AGridLevelRuntimeActor* Runtime = nullptr;
    UGridLevelAsset* Level = nullptr;
    UGridActivationComponent* Activation = nullptr;
    if (!PrepareRuntime (*TestWorld.World, Runtime, Level, Activation))
    {
        AddError (TEXT ("Unable to prepare MON19.8 Puzzle B runtime."));
        return false;
    }

    Level->LevelVariables.Add (MakeIntVariable (TEXT ("RuneCount"), 0));

    const FGuid LeverAId (19, 8, 2, 1);
    FGridLevelObjectData LeverA;
    LeverA.ObjectId = LeverAId;
    LeverA.Type = EGridLevelObjectType::Lever;
    Level->Objects.Add (LeverA);

    const FGuid LeverBId (19, 8, 2, 2);
    FGridLevelObjectData LeverB;
    LeverB.ObjectId = LeverBId;
    LeverB.Type = EGridLevelObjectType::Lever;
    Level->Objects.Add (LeverB);

    const FGuid AddId (19, 8, 2, 3);
    FGridLevelObjectData Add = MakeLogicNode (
        EGridLogicNodeType::AddInt,
        AddId,
        TEXT ("RuneCount"));
    Add.Logic.IntValue = 1;
    Level->Objects.Add (Add);

    const FGuid CompareId (19, 8, 2, 4);
    FGridLevelObjectData Compare = MakeLogicNode (
        EGridLogicNodeType::CompareInt,
        CompareId,
        TEXT ("RuneCount"));
    Compare.Logic.IntComparison = EGridLogicIntComparison::GreaterOrEqual;
    Compare.Logic.IntValue = 2;
    Level->Objects.Add (Compare);

    const FGridLevelObjectData Door = MakeDoor (FGuid (19, 8, 2, 5));
    Level->Objects.Add (Door);

    Level->Links.Add (MakeLink (
        LeverAId,
        EGridObjectEvent::Activated,
        AddId,
        EGridObjectCommand::LogicExecute));
    Level->Links.Add (MakeLink (
        LeverBId,
        EGridObjectEvent::Activated,
        AddId,
        EGridObjectCommand::LogicExecute));
    Level->Links.Add (MakeLink (
        AddId,
        EGridObjectEvent::Activated,
        CompareId,
        EGridObjectCommand::LogicExecute));
    Level->Links.Add (MakeLink (
        CompareId,
        EGridObjectEvent::Activated,
        Door.ObjectId,
        EGridObjectCommand::Open));

    Activation->Initialize (Runtime);
    Activation->RebuildIndexes ();
    if (!PrepareClosedDoor (*TestWorld.World, *Runtime, Door))
    {
        AddError (TEXT ("MON19.8 Puzzle B door fixture is not closed."));
        return false;
    }

    TestTrue (
        TEXT ("First lever executes the logic chain"),
        Runtime->ExecuteLinksFromRuntimeObject (
            LeverAId,
            EGridObjectEvent::Activated));

    FGridLevelRuntimeState* State =
        Runtime->GetOrCreateRuntimeStateForCurrentLevel ();
    if (!State)
    {
        AddError (TEXT ("MON19.8 Puzzle B runtime state is missing."));
        return false;
    }

    FString Error;
    int32 RuneCount = 0;
    TestTrue (
        TEXT ("RuneCount reads after first lever"),
        GridLevelVariableStore::TryGetInt32 (
            *Level,
            *State,
            TEXT ("RuneCount"),
            RuneCount,
            Error));
    TestEqual (TEXT ("First lever increments RuneCount once"), RuneCount, 1);
    TestFalse (
        TEXT ("Threshold is not reached after first lever"),
        Runtime->IsDoorOpenOnEdge (Door.CellX, Door.CellY, Door.Edge));

    TestTrue (
        TEXT ("Second lever executes the same data-driven logic chain"),
        Runtime->ExecuteLinksFromRuntimeObject (
            LeverBId,
            EGridObjectEvent::Activated));
    TestTrue (
        TEXT ("RuneCount reads after second lever"),
        GridLevelVariableStore::TryGetInt32 (
            *Level,
            *State,
            TEXT ("RuneCount"),
            RuneCount,
            Error));
    TestEqual (TEXT ("Second lever reaches the threshold"), RuneCount, 2);
    TestTrue (
        TEXT ("CompareInt Activated opens the door at threshold"),
        Runtime->IsDoorOpenOnEdge (Door.CellX, Door.CellY, Door.Edge));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMON198PuzzleCLuaConditionalTest,
    "Grimrock.MON19.8.ProductionPuzzles.C_LuaConditional",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMON198PuzzleCLuaConditionalTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    FMON198TestWorld TestWorld;
    if (!TestWorld.World)
    {
        AddError (TEXT ("Unable to create MON19.8 Puzzle C world."));
        return false;
    }

    AGridLevelRuntimeActor* Runtime = nullptr;
    UGridLevelAsset* Level = nullptr;
    UGridActivationComponent* Activation = nullptr;
    if (!PrepareRuntime (*TestWorld.World, Runtime, Level, Activation))
    {
        AddError (TEXT ("Unable to prepare MON19.8 Puzzle C runtime."));
        return false;
    }

    Level->LevelVariables.Add (MakeIntVariable (TEXT ("RuneCount"), 0));

    const FGuid TriggerId (19, 8, 3, 1);
    FGridLevelObjectData Trigger;
    Trigger.ObjectId = TriggerId;
    Trigger.Type = EGridLevelObjectType::Trigger;
    Level->Objects.Add (Trigger);

    const FGridLevelObjectData Door = MakeDoor (FGuid (19, 8, 3, 2));
    Level->Objects.Add (Door);

    FGridLuaScriptSource Script;
    Script.ScriptId = TEXT ("ConditionalPuzzle");
    Script.bEnabled = true;
    Script.Source =
        TEXT ("persistent = { RuneCount = 0 }\n")
        TEXT ("function on_trigger(event)\n")
        TEXT ("  if persistent.RuneCount >= 2 then\n")
        TEXT ("    local ok, err = grid.command('SecretDoor', 'Open')\n")
        TEXT ("    assert(ok, err)\n")
        TEXT ("  end\n")
        TEXT ("end\n");
    Level->LuaScripts.Add (Script);
    Level->Links.Add (MakeLuaLink (
        TriggerId,
        EGridObjectEvent::Activated,
        TEXT ("ConditionalPuzzle"),
        TEXT ("on_trigger")));

    Activation->Initialize (Runtime);
    Activation->RebuildIndexes ();
    if (!PrepareClosedDoor (*TestWorld.World, *Runtime, Door))
    {
        AddError (TEXT ("MON19.8 Puzzle C door fixture is not closed."));
        return false;
    }

    FString LuaError;
    if (!Activation->ReloadLuaRuntime (&LuaError))
    {
        AddError (FString::Printf (
            TEXT ("MON19.8 Puzzle C Lua runtime failed to load: %s"),
            *LuaError));
        return false;
    }

    FGridLevelRuntimeState* State =
        Runtime->GetOrCreateRuntimeStateForCurrentLevel ();
    if (!State)
    {
        AddError (TEXT ("MON19.8 Puzzle C runtime state is missing."));
        return false;
    }

    FString Error;
    TestTrue (
        TEXT ("RuneCount can be set below threshold"),
        GridLevelVariableStore::SetInt32 (
            *Level,
            *State,
            TEXT ("RuneCount"),
            1,
            Error));
    TestTrue (
        TEXT ("Lua callback succeeds below threshold"),
        Runtime->ExecuteLinksFromRuntimeObject (
            TriggerId,
            EGridObjectEvent::Activated));
    TestFalse (
        TEXT ("Lua leaves door closed below threshold"),
        Runtime->IsDoorOpenOnEdge (Door.CellX, Door.CellY, Door.Edge));

    TestTrue (
        TEXT ("RuneCount can be set at threshold"),
        GridLevelVariableStore::SetInt32 (
            *Level,
            *State,
            TEXT ("RuneCount"),
            2,
            Error));
    TestTrue (
        TEXT ("Lua callback succeeds at threshold"),
        Runtime->ExecuteLinksFromRuntimeObject (
            TriggerId,
            EGridObjectEvent::Activated));
    TestTrue (
        TEXT ("Lua reads persistent state and requests normal Door.Open"),
        Runtime->IsDoorOpenOnEdge (Door.CellX, Door.CellY, Door.Edge));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMON198PuzzleDEncounterLuaDoorTest,
    "Grimrock.MON19.8.ProductionPuzzles.D_EncounterLuaDoor",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMON198PuzzleDEncounterLuaDoorTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    FMON198TestWorld TestWorld;
    if (!TestWorld.World)
    {
        AddError (TEXT ("Unable to create MON19.8 Puzzle D world."));
        return false;
    }

    AGridLevelRuntimeActor* Runtime = nullptr;
    UGridLevelAsset* Level = nullptr;
    UGridActivationComponent* Activation = nullptr;
    if (!PrepareRuntime (*TestWorld.World, Runtime, Level, Activation))
    {
        AddError (TEXT ("Unable to prepare MON19.8 Puzzle D runtime."));
        return false;
    }

    const FGuid EncounterAnchorId (19, 8, 4, 1);
    FGridLevelObjectData EncounterAnchor;
    EncounterAnchor.ObjectId = EncounterAnchorId;
    EncounterAnchor.Type = EGridLevelObjectType::MonsterSpawn;
    Level->Objects.Add (EncounterAnchor);

    const FGridLevelObjectData Door = MakeDoor (FGuid (19, 8, 4, 2));
    Level->Objects.Add (Door);

    FGridLuaScriptSource Script;
    Script.ScriptId = TEXT ("EncounterBridge");
    Script.bEnabled = true;
    Script.Source =
        TEXT ("function on_encounter_completed(event)\n")
        TEXT ("  local ok, err = grid.command('SecretDoor', 'Open')\n")
        TEXT ("  assert(ok, err)\n")
        TEXT ("end\n");
    Level->LuaScripts.Add (Script);
    Level->Links.Add (MakeLuaLink (
        EncounterAnchorId,
        EGridObjectEvent::EncounterCompleted,
        TEXT ("EncounterBridge"),
        TEXT ("on_encounter_completed")));

    Activation->Initialize (Runtime);
    Activation->RebuildIndexes ();
    if (!PrepareClosedDoor (*TestWorld.World, *Runtime, Door))
    {
        AddError (TEXT ("MON19.8 Puzzle D door fixture is not closed."));
        return false;
    }

    FString LuaError;
    if (!Activation->ReloadLuaRuntime (&LuaError))
    {
        AddError (FString::Printf (
            TEXT ("MON19.8 Puzzle D Lua runtime failed to load: %s"),
            *LuaError));
        return false;
    }

    TestTrue (
        TEXT ("EncounterCompleted dispatch reaches Lua bridge"),
        Runtime->ExecuteLinksFromRuntimeObject (
            EncounterAnchorId,
            EGridObjectEvent::EncounterCompleted));
    TestTrue (
        TEXT ("Encounter Lua bridge opens the LogicId door"),
        Runtime->IsDoorOpenOnEdge (Door.CellX, Door.CellY, Door.Edge));
    return true;
}

} // namespace GridMON198Tests

#endif // WITH_DEV_AUTOMATION_TESTS