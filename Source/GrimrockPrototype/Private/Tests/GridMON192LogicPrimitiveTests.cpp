#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Core/GridLevelVariableTypes.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridActivationComponent.h"
#include "Runtime/GridDungeonRuntimeState.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridLevelVariableStore.h"
#include "Runtime/GridLogicRuntime.h"

namespace GridMON192LogicPrimitiveTests
{
    FGridLevelVariableDefinition MakeBoolVariable (
        FName Id,
        bool bDefault = false)
    {
        FGridLevelVariableDefinition Definition;
        Definition.VariableId = Id;
        Definition.Type = EGridLevelVariableType::Bool;
        Definition.bDefaultBoolValue = bDefault;
        return Definition;
    }

    FGridLevelVariableDefinition MakeIntVariable (
        FName Id,
        int32 DefaultValue = 0)
    {
        FGridLevelVariableDefinition Definition;
        Definition.VariableId = Id;
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

    UGridLevelAsset* MakeLogicLevel (UObject* Outer)
    {
        UGridLevelAsset* Level = NewObject<UGridLevelAsset> (Outer);
        Level->Width = 1;
        Level->Height = 1;
        Level->EnsureCellCount ();
        Level->Cells[0].CellType = EGridCellType::Floor;
        Level->LevelVariables = {
            MakeBoolVariable (TEXT ("Gate"), false),
            MakeBoolVariable (TEXT ("Latch"), false),
            MakeIntVariable (TEXT ("Count"), 2)
        };
        return Level;
    }

    bool Execute (
        UGridLevelAsset& Level,
        const FGridLevelObjectData& Node,
        FGridLevelRuntimeState& State,
        FGridLogicExecutionResult& Result,
        EGridObjectCommand Command = EGridObjectCommand::LogicExecute)
    {
        return GridLogicRuntime::ExecuteNode (
            Level,
            Node,
            State,
            Command,
            Result);
    }

    struct FMON1923TestWorld
    {
        UWorld* World = nullptr;

        FMON1923TestWorld ()
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
                    TEXT ("MON1923_%s"),
                    *FGuid::NewGuid ().ToString (EGuidFormats::Digits))),
                nullptr,
                true,
                ERHIFeatureLevel::Num,
                &Values);
            if (!World || !GEngine)
            {
                return;
            }

            FWorldContext& Context =
                GEngine->CreateNewWorldContext (EWorldType::Game);
            Context.SetCurrentWorld (World);
        }

        ~FMON1923TestWorld ()
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
}

using namespace GridMON192LogicPrimitiveTests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMON1923MutationPrimitivesTest,
    "Grimrock.MON19.2.Runtime.Logic.MutationPrimitives",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMON1923MutationPrimitivesTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    UGridLevelAsset* Level = MakeLogicLevel (GetTransientPackage ());
    FGridLevelRuntimeState State;
    FGridLogicExecutionResult Result;
    FString Error;

    FGridLevelObjectData SetBool = MakeLogicNode (
        EGridLogicNodeType::SetBool,
        FGuid (19, 2, 3, 1),
        TEXT ("Gate"));
    SetBool.Logic.bBoolValue = true;
    TestTrue (TEXT ("SetBool executes"), Execute (*Level, SetBool, State, Result));
    TestTrue (TEXT ("SetBool emits Activated"),
        Result.bEmitEvent && Result.EmittedEvent == EGridObjectEvent::Activated);
    bool bGate = false;
    TestTrue (TEXT ("Gate reads after SetBool"),
        GridLevelVariableStore::TryGetBool (
            *Level, State, TEXT ("Gate"), bGate, Error));
    TestTrue (TEXT ("SetBool writes true"), bGate);

    FGridLevelObjectData ToggleBool = MakeLogicNode (
        EGridLogicNodeType::ToggleBool,
        FGuid (19, 2, 3, 2),
        TEXT ("Gate"));
    TestTrue (TEXT ("ToggleBool executes"),
        Execute (*Level, ToggleBool, State, Result));
    GridLevelVariableStore::TryGetBool (
        *Level, State, TEXT ("Gate"), bGate, Error);
    TestFalse (TEXT ("ToggleBool writes false"), bGate);

    FGridLevelObjectData SetInt = MakeLogicNode (
        EGridLogicNodeType::SetInt,
        FGuid (19, 2, 3, 3),
        TEXT ("Count"));
    SetInt.Logic.IntValue = 10;
    TestTrue (TEXT ("SetInt executes"),
        Execute (*Level, SetInt, State, Result));

    FGridLevelObjectData AddInt = MakeLogicNode (
        EGridLogicNodeType::AddInt,
        FGuid (19, 2, 3, 4),
        TEXT ("Count"));
    AddInt.Logic.IntValue = 5;
    TestTrue (TEXT ("AddInt executes"),
        Execute (*Level, AddInt, State, Result));

    FGridLevelObjectData SubtractInt = MakeLogicNode (
        EGridLogicNodeType::SubtractInt,
        FGuid (19, 2, 3, 5),
        TEXT ("Count"));
    SubtractInt.Logic.IntValue = 3;
    TestTrue (TEXT ("SubtractInt executes"),
        Execute (*Level, SubtractInt, State, Result));

    int32 Count = 0;
    TestTrue (TEXT ("Count reads after mutations"),
        GridLevelVariableStore::TryGetInt32 (
            *Level, State, TEXT ("Count"), Count, Error));
    TestEqual (TEXT ("Set/Add/Sub compose deterministically"), Count, 12);

    FGridLevelObjectData Reset = MakeLogicNode (
        EGridLogicNodeType::ResetVariable,
        FGuid (19, 2, 3, 6),
        TEXT ("Count"));
    TestTrue (TEXT ("ResetVariable executes"),
        Execute (*Level, Reset, State, Result));
    GridLevelVariableStore::TryGetInt32 (
        *Level, State, TEXT ("Count"), Count, Error);
    TestEqual (TEXT ("ResetVariable restores declaration default"), Count, 2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMON1923ComparisonPrimitivesTest,
    "Grimrock.MON19.2.Runtime.Logic.ComparisonPrimitives",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMON1923ComparisonPrimitivesTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    UGridLevelAsset* Level = MakeLogicLevel (GetTransientPackage ());
    FGridLevelRuntimeState State;
    FGridLogicExecutionResult Result;
    FString Error;

    FGridLevelObjectData CompareBool = MakeLogicNode (
        EGridLogicNodeType::CompareBool,
        FGuid (19, 2, 3, 10),
        TEXT ("Gate"));
    CompareBool.Logic.bBoolValue = false;
    TestTrue (TEXT ("CompareBool executes"),
        Execute (*Level, CompareBool, State, Result));
    TestEqual (TEXT ("Bool equality emits Activated"),
        Result.EmittedEvent,
        EGridObjectEvent::Activated);

    GridLevelVariableStore::SetBool (
        *Level, State, TEXT ("Gate"), true, Error);
    TestTrue (TEXT ("CompareBool false branch executes"),
        Execute (*Level, CompareBool, State, Result));
    TestEqual (TEXT ("Bool inequality emits Deactivated"),
        Result.EmittedEvent,
        EGridObjectEvent::Deactivated);

    FGridLevelObjectData CompareInt = MakeLogicNode (
        EGridLogicNodeType::CompareInt,
        FGuid (19, 2, 3, 11),
        TEXT ("Count"));
    CompareInt.Logic.IntComparison =
        EGridLogicIntComparison::GreaterOrEqual;
    CompareInt.Logic.IntValue = 2;
    TestTrue (TEXT ("CompareInt true branch executes"),
        Execute (*Level, CompareInt, State, Result));
    TestEqual (TEXT ("Int threshold true emits Activated"),
        Result.EmittedEvent,
        EGridObjectEvent::Activated);

    CompareInt.Logic.IntValue = 3;
    TestTrue (TEXT ("CompareInt false branch executes"),
        Execute (*Level, CompareInt, State, Result));
    TestEqual (TEXT ("Int threshold false emits Deactivated"),
        Result.EmittedEvent,
        EGridObjectEvent::Deactivated);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMON1923LatchRelayTest,
    "Grimrock.MON19.2.Runtime.Logic.LatchAndRelay",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMON1923LatchRelayTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    UGridLevelAsset* Level = MakeLogicLevel (GetTransientPackage ());
    FGridLevelRuntimeState State;
    FGridLogicExecutionResult Result;
    FString Error;

    FGridLevelObjectData Relay = MakeLogicNode (
        EGridLogicNodeType::Relay,
        FGuid (19, 2, 3, 20));
    TestTrue (TEXT ("Relay executes"),
        Execute (*Level, Relay, State, Result));
    TestTrue (TEXT ("Relay always emits Activated"),
        Result.bEmitEvent &&
        Result.EmittedEvent == EGridObjectEvent::Activated);

    FGridLevelObjectData Latch = MakeLogicNode (
        EGridLogicNodeType::Latch,
        FGuid (19, 2, 3, 21),
        TEXT ("Latch"));
    TestTrue (TEXT ("First latch execute succeeds"),
        Execute (*Level, Latch, State, Result));
    TestTrue (TEXT ("First latch execute emits"),
        Result.bEmitEvent &&
        Result.EmittedEvent == EGridObjectEvent::Activated);

    TestTrue (TEXT ("Second latch execute succeeds"),
        Execute (*Level, Latch, State, Result));
    TestFalse (TEXT ("Latched node is silent on repeated execute"),
        Result.bEmitEvent);

    bool bLatched = false;
    GridLevelVariableStore::TryGetBool (
        *Level, State, TEXT ("Latch"), bLatched, Error);
    TestTrue (TEXT ("Latch state is stored in persistent Bool"), bLatched);

    TestTrue (TEXT ("Logic Reset succeeds for latch"),
        Execute (
            *Level,
            Latch,
            State,
            Result,
            EGridObjectCommand::LogicReset));
    TestTrue (TEXT ("Latch reset emits Deactivated"),
        Result.bEmitEvent &&
        Result.EmittedEvent == EGridObjectEvent::Deactivated);
    GridLevelVariableStore::TryGetBool (
        *Level, State, TEXT ("Latch"), bLatched, Error);
    TestFalse (TEXT ("Latch reset clears persistent Bool"), bLatched);

    TestTrue (TEXT ("Latch can fire again after reset"),
        Execute (*Level, Latch, State, Result));
    TestTrue (TEXT ("Re-armed latch emits again"), Result.bEmitEvent);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMON1923ValidationOverflowTest,
    "Grimrock.MON19.2.Runtime.Logic.ValidationAndOverflow",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMON1923ValidationOverflowTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    UGridLevelAsset* Level = MakeLogicLevel (GetTransientPackage ());
    FGridLevelRuntimeState State;
    FGridLogicExecutionResult Result;
    FString Error;

    FGridLevelObjectData WrongType = MakeLogicNode (
        EGridLogicNodeType::AddInt,
        FGuid (19, 2, 3, 30),
        TEXT ("Gate"));
    TestFalse (TEXT ("Int primitive rejects Bool declaration"),
        GridLogicRuntime::ValidateNode (*Level, WrongType, Error));
    TestTrue (TEXT ("Wrong-type validation reports a reason"),
        !Error.IsEmpty ());

    FGridLevelObjectData MissingVariable = MakeLogicNode (
        EGridLogicNodeType::SetBool,
        FGuid (19, 2, 3, 31),
        TEXT ("Missing"));
    TestFalse (TEXT ("Undeclared variable is rejected"),
        GridLogicRuntime::ValidateNode (
            *Level, MissingVariable, Error));

    FGridLevelObjectData VisualLogic = MakeLogicNode (
        EGridLogicNodeType::Relay,
        FGuid (19, 2, 3, 34));
    VisualLogic.ArchetypeId = TEXT ("ShouldNotSpawn");
    TestFalse (TEXT ("Logic nodes reject runtime archetypes"),
        GridLogicRuntime::ValidateNode (*Level, VisualLogic, Error));

    FGridLevelObjectData InvalidComparison = MakeLogicNode (
        EGridLogicNodeType::CompareInt,
        FGuid (19, 2, 3, 35),
        TEXT ("Count"));
    InvalidComparison.Logic.IntComparison =
        static_cast<EGridLogicIntComparison> (255);
    TestFalse (TEXT ("Invalid Int comparison is rejected"),
        GridLogicRuntime::ValidateNode (*Level, InvalidComparison, Error));

    TestTrue (TEXT ("Count can be set to MAX_int32"),
        GridLevelVariableStore::SetInt32 (
            *Level,
            State,
            TEXT ("Count"),
            MAX_int32,
            Error));
    FGridLevelObjectData Overflow = MakeLogicNode (
        EGridLogicNodeType::AddInt,
        FGuid (19, 2, 3, 32),
        TEXT ("Count"));
    Overflow.Logic.IntValue = 1;
    TestFalse (TEXT ("AddInt rejects signed overflow"),
        Execute (*Level, Overflow, State, Result));
    TestFalse (TEXT ("Overflow never emits an event"),
        Result.bEmitEvent);
    int32 Count = 0;
    GridLevelVariableStore::TryGetInt32 (
        *Level, State, TEXT ("Count"), Count, Error);
    TestEqual (TEXT ("Overflow failure is atomic"), Count, MAX_int32);

    FGridLevelObjectData Relay = MakeLogicNode (
        EGridLogicNodeType::Relay,
        FGuid (19, 2, 3, 33));
    TestFalse (TEXT ("Logic Reset is rejected for non-latch"),
        Execute (
            *Level,
            Relay,
            State,
            Result,
            EGridObjectCommand::LogicReset));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMON1923EventCommandChainTest,
    "Grimrock.MON19.2.Runtime.Logic.EventCommandChain",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMON1923EventCommandChainTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    FMON1923TestWorld TestWorld;
    if (!TestWorld.World)
    {
        AddError (TEXT ("Unable to create MON19.2.3 test world."));
        return false;
    }

    AGridLevelRuntimeActor* Runtime =
        TestWorld.World->SpawnActor<AGridLevelRuntimeActor> ();
    TestNotNull (TEXT ("Runtime actor spawns"), Runtime);
    if (!Runtime)
    {
        return false;
    }

    UGridLevelAsset* Level = MakeLogicLevel (Runtime);
    Runtime->LevelAsset = Level;
    Runtime->CurrentDungeonLevelId = TEXT ("MON1923");

    const FGuid SourceId (19, 2, 3, 40);
    FGridLevelObjectData Source;
    Source.ObjectId = SourceId;
    Source.Type = EGridLevelObjectType::Trigger;
    Level->Objects.Add (Source);

    const FGuid SetBoolId (19, 2, 3, 41);
    FGridLevelObjectData SetBool = MakeLogicNode (
        EGridLogicNodeType::SetBool,
        SetBoolId,
        TEXT ("Gate"));
    SetBool.Logic.bBoolValue = true;
    Level->Objects.Add (SetBool);

    const FGuid AddIntId (19, 2, 3, 42);
    FGridLevelObjectData AddInt = MakeLogicNode (
        EGridLogicNodeType::AddInt,
        AddIntId,
        TEXT ("Count"));
    AddInt.Logic.IntValue = 3;
    Level->Objects.Add (AddInt);

    FGridObjectLink FirstLink;
    FirstLink.SourceObjectId = SourceId;
    FirstLink.SourceEvent = EGridObjectEvent::Activated;
    FirstLink.TargetObjectId = SetBoolId;
    FirstLink.Command = EGridObjectCommand::LogicExecute;
    Level->Links.Add (FirstLink);

    FGridObjectLink SecondLink;
    SecondLink.SourceObjectId = SetBoolId;
    SecondLink.SourceEvent = EGridObjectEvent::Activated;
    SecondLink.TargetObjectId = AddIntId;
    SecondLink.Command = EGridObjectCommand::LogicExecute;
    Level->Links.Add (SecondLink);

    // A self-loop must be rejected before re-applying AddInt. This verifies
    // that data-only logic nodes cannot mutate twice while already dispatching.
    FGridObjectLink SelfLoop;
    SelfLoop.SourceObjectId = AddIntId;
    SelfLoop.SourceEvent = EGridObjectEvent::Activated;
    SelfLoop.TargetObjectId = AddIntId;
    SelfLoop.Command = EGridObjectCommand::LogicExecute;
    Level->Links.Add (SelfLoop);

    UGridActivationComponent* Activation =
        Runtime->FindComponentByClass<UGridActivationComponent> ();
    TestNotNull (TEXT ("Activation component exists"), Activation);
    if (!Activation)
    {
        return false;
    }
    Activation->Initialize (Runtime);
    Activation->RebuildIndexes ();

    TestTrue (TEXT ("Source event executes data-only logic chain"),
        Runtime->ExecuteLinksFromRuntimeObject (
            SourceId,
            EGridObjectEvent::Activated));

    FGridLevelRuntimeState* State =
        Runtime->GetOrCreateRuntimeStateForCurrentLevel ();
    TestNotNull (TEXT ("Logic chain created current-level runtime state"), State);
    if (!State)
    {
        return false;
    }

    FString Error;
    bool bGate = false;
    int32 Count = 0;
    TestTrue (TEXT ("Chained Bool is readable"),
        GridLevelVariableStore::TryGetBool (
            *Level, *State, TEXT ("Gate"), bGate, Error));
    TestTrue (TEXT ("First logic node mutated Bool"), bGate);
    TestTrue (TEXT ("Chained Int is readable"),
        GridLevelVariableStore::TryGetInt32 (
            *Level, *State, TEXT ("Count"), Count, Error));
    TestEqual (TEXT ("Logic chain applies AddInt once despite self-loop"), Count, 5);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
