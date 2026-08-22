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

namespace
{
    FGridLevelVariableDefinition MakeBoolVariable1924 (
        FName Id,
        bool bDefault)
    {
        FGridLevelVariableDefinition Definition;
        Definition.VariableId = Id;
        Definition.Type = EGridLevelVariableType::Bool;
        Definition.bDefaultBoolValue = bDefault;
        return Definition;
    }

    FGridLevelVariableDefinition MakeIntVariable1924 (
        FName Id,
        int32 DefaultValue)
    {
        FGridLevelVariableDefinition Definition;
        Definition.VariableId = Id;
        Definition.Type = EGridLevelVariableType::Int32;
        Definition.DefaultInt32Value = DefaultValue;
        return Definition;
    }

    FGridLevelObjectData MakeAddIntLogicNode1924 (
        FGuid ObjectId,
        int32 Delta)
    {
        FGridLevelObjectData Node;
        Node.ObjectId = ObjectId;
        Node.Type = EGridLevelObjectType::Logic;
        Node.Logic.NodeType = EGridLogicNodeType::AddInt;
        Node.Logic.VariableId = TEXT ("Hits");
        Node.Logic.IntValue = Delta;
        return Node;
    }

    UGridLevelAsset* MakeVariableConditionLevel1924 (UObject* Outer)
    {
        UGridLevelAsset* Level = NewObject<UGridLevelAsset> (Outer);
        Level->Width = 1;
        Level->Height = 1;
        Level->EnsureCellCount ();
        Level->Cells[0].CellType = EGridCellType::Floor;
        Level->LevelVariables = {
            MakeBoolVariable1924 (TEXT ("Gate"), false),
            MakeIntVariable1924 (TEXT ("Count"), 2),
            MakeIntVariable1924 (TEXT ("Hits"), 0)
        };
        return Level;
    }

    struct FMON1924TestWorld
    {
        UWorld* World = nullptr;

        FMON1924TestWorld ()
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
                    TEXT ("MON1924_%s"),
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

        ~FMON1924TestWorld ()
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

    bool BuildVariableConditionRuntime1924 (
        FMON1924TestWorld& TestWorld,
        UGridLevelAsset*& OutLevel,
        AGridLevelRuntimeActor*& OutRuntime,
        FGuid& OutSourceId,
        FGuid& OutTargetId,
        int32 Delta)
    {
        if (!TestWorld.World)
        {
            return false;
        }

        OutRuntime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor> ();
        if (!OutRuntime)
        {
            return false;
        }

        OutLevel = MakeVariableConditionLevel1924 (OutRuntime);
        OutRuntime->LevelAsset = OutLevel;
        OutRuntime->CurrentDungeonLevelId = TEXT ("MON1924");

        OutSourceId = FGuid (19, 2, 4, 1);
        FGridLevelObjectData Source;
        Source.ObjectId = OutSourceId;
        Source.Type = EGridLevelObjectType::Trigger;
        OutLevel->Objects.Add (Source);

        OutTargetId = FGuid (19, 2, 4, 2);
        OutLevel->Objects.Add (MakeAddIntLogicNode1924 (OutTargetId, Delta));

        UGridActivationComponent* Activation =
            OutRuntime->FindComponentByClass<UGridActivationComponent> ();
        if (!Activation)
        {
            return false;
        }
        Activation->Initialize (OutRuntime);
        return true;
    }

    int32 ReadHits1924 (
        UGridLevelAsset& Level,
        AGridLevelRuntimeActor& Runtime,
        FAutomationTestBase& Test)
    {
        FGridLevelRuntimeState* State =
            Runtime.GetOrCreateRuntimeStateForCurrentLevel ();
        if (!State)
        {
            Test.AddError (TEXT ("Missing MON19.2.4 runtime state."));
            return INDEX_NONE;
        }

        FString Error;
        int32 Hits = 0;
        if (!GridLevelVariableStore::TryGetInt32 (
                Level,
                *State,
                TEXT ("Hits"),
                Hits,
                Error))
        {
            Test.AddError (FString::Printf (
                TEXT ("Unable to read Hits: %s"),
                *Error));
            return INDEX_NONE;
        }
        return Hits;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMON1924BoolVariableConditionTest,
    "Grimrock.MON19.2.Runtime.VariableConditions.BoolAndInvert",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMON1924BoolVariableConditionTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    FMON1924TestWorld TestWorld;
    UGridLevelAsset* Level = nullptr;
    AGridLevelRuntimeActor* Runtime = nullptr;
    FGuid SourceId;
    FGuid TargetId;
    if (!BuildVariableConditionRuntime1924 (
            TestWorld,
            Level,
            Runtime,
            SourceId,
            TargetId,
            1))
    {
        AddError (TEXT ("Unable to build MON19.2.4 Bool test runtime."));
        return false;
    }

    FGridObjectLink Link;
    Link.SourceObjectId = SourceId;
    Link.SourceEvent = EGridObjectEvent::Activated;
    Link.TargetObjectId = TargetId;
    Link.Command = EGridObjectCommand::LogicExecute;
    Link.Condition = EGridObjectCondition::LevelVariableBoolEquals;
    Link.ConditionVariableId = TEXT ("Gate");
    Link.ConditionBoolValue = true;
    Level->Links.Add (Link);

    UGridActivationComponent* Activation =
        Runtime->FindComponentByClass<UGridActivationComponent> ();
    Activation->RebuildIndexes ();

    TestFalse (TEXT ("False Gate rejects BoolEquals(true)"),
        Runtime->ExecuteLinksFromRuntimeObject (
            SourceId,
            EGridObjectEvent::Activated));
    TestEqual (TEXT ("Rejected Bool condition leaves target untouched"),
        ReadHits1924 (*Level, *Runtime, *this),
        0);

    FGridLevelRuntimeState* State =
        Runtime->GetOrCreateRuntimeStateForCurrentLevel ();
    FString Error;
    TestTrue (TEXT ("Gate can be set true"),
        GridLevelVariableStore::SetBool (
            *Level,
            *State,
            TEXT ("Gate"),
            true,
            Error));
    TestTrue (TEXT ("True Gate passes BoolEquals(true)"),
        Runtime->ExecuteLinksFromRuntimeObject (
            SourceId,
            EGridObjectEvent::Activated));
    TestEqual (TEXT ("Passing Bool condition executes target once"),
        ReadHits1924 (*Level, *Runtime, *this),
        1);

    Level->Links[0].bInvertCondition = true;
    TestFalse (TEXT ("Invert rejects an otherwise true Bool condition"),
        Runtime->ExecuteLinksFromRuntimeObject (
            SourceId,
            EGridObjectEvent::Activated));
    TestEqual (TEXT ("Rejected inverted condition does not mutate target"),
        ReadHits1924 (*Level, *Runtime, *this),
        1);

    TestTrue (TEXT ("Gate can be set false again"),
        GridLevelVariableStore::SetBool (
            *Level,
            *State,
            TEXT ("Gate"),
            false,
            Error));
    TestTrue (TEXT ("Invert passes when BoolEquals(true) is false"),
        Runtime->ExecuteLinksFromRuntimeObject (
            SourceId,
            EGridObjectEvent::Activated));
    TestEqual (TEXT ("Inverted Bool condition executes target once"),
        ReadHits1924 (*Level, *Runtime, *this),
        2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMON1924IntVariableConditionTest,
    "Grimrock.MON19.2.Runtime.VariableConditions.IntComparison",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMON1924IntVariableConditionTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    FMON1924TestWorld TestWorld;
    UGridLevelAsset* Level = nullptr;
    AGridLevelRuntimeActor* Runtime = nullptr;
    FGuid SourceId;
    FGuid TargetId;
    if (!BuildVariableConditionRuntime1924 (
            TestWorld,
            Level,
            Runtime,
            SourceId,
            TargetId,
            5))
    {
        AddError (TEXT ("Unable to build MON19.2.4 Int test runtime."));
        return false;
    }

    FGridObjectLink Link;
    Link.SourceObjectId = SourceId;
    Link.SourceEvent = EGridObjectEvent::Activated;
    Link.TargetObjectId = TargetId;
    Link.Command = EGridObjectCommand::LogicExecute;
    Link.Condition = EGridObjectCondition::LevelVariableIntCompare;
    Link.ConditionVariableId = TEXT ("Count");
    Link.ConditionIntComparison = EGridLogicIntComparison::GreaterOrEqual;
    Link.ConditionIntValue = 3;
    Level->Links.Add (Link);

    UGridActivationComponent* Activation =
        Runtime->FindComponentByClass<UGridActivationComponent> ();
    Activation->RebuildIndexes ();

    TestFalse (TEXT ("Count=2 rejects Count >= 3"),
        Runtime->ExecuteLinksFromRuntimeObject (
            SourceId,
            EGridObjectEvent::Activated));
    TestEqual (TEXT ("Rejected Int condition leaves target untouched"),
        ReadHits1924 (*Level, *Runtime, *this),
        0);

    FGridLevelRuntimeState* State =
        Runtime->GetOrCreateRuntimeStateForCurrentLevel ();
    FString Error;
    TestTrue (TEXT ("Count can be set to threshold"),
        GridLevelVariableStore::SetInt32 (
            *Level,
            *State,
            TEXT ("Count"),
            3,
            Error));
    TestTrue (TEXT ("Count=3 passes Count >= 3"),
        Runtime->ExecuteLinksFromRuntimeObject (
            SourceId,
            EGridObjectEvent::Activated));
    TestEqual (TEXT ("Passing Int condition executes target"),
        ReadHits1924 (*Level, *Runtime, *this),
        5);

    Level->Links[0].ConditionIntComparison = EGridLogicIntComparison::NotEqual;
    Level->Links[0].ConditionIntValue = 3;
    TestFalse (TEXT ("Count=3 rejects Count != 3"),
        Runtime->ExecuteLinksFromRuntimeObject (
            SourceId,
            EGridObjectEvent::Activated));
    TestEqual (TEXT ("Failed NotEqual comparison is atomic"),
        ReadHits1924 (*Level, *Runtime, *this),
        5);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMON1924MissingVariableConditionTest,
    "Grimrock.MON19.2.Runtime.VariableConditions.InvalidVariable",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMON1924MissingVariableConditionTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    FMON1924TestWorld TestWorld;
    UGridLevelAsset* Level = nullptr;
    AGridLevelRuntimeActor* Runtime = nullptr;
    FGuid SourceId;
    FGuid TargetId;
    if (!BuildVariableConditionRuntime1924 (
            TestWorld,
            Level,
            Runtime,
            SourceId,
            TargetId,
            1))
    {
        AddError (TEXT ("Unable to build MON19.2.4 invalid-variable runtime."));
        return false;
    }

    FGridObjectLink Link;
    Link.SourceObjectId = SourceId;
    Link.SourceEvent = EGridObjectEvent::Activated;
    Link.TargetObjectId = TargetId;
    Link.Command = EGridObjectCommand::LogicExecute;
    Link.Condition = EGridObjectCondition::LevelVariableBoolEquals;
    Link.ConditionVariableId = TEXT ("MissingVariable");
    Link.ConditionBoolValue = true;
    Level->Links.Add (Link);

    UGridActivationComponent* Activation =
        Runtime->FindComponentByClass<UGridActivationComponent> ();
    Activation->RebuildIndexes ();

    TestFalse (TEXT ("Undeclared condition variable rejects the link"),
        Runtime->ExecuteLinksFromRuntimeObject (
            SourceId,
            EGridObjectEvent::Activated));
    TestEqual (TEXT ("Invalid variable condition never executes target"),
        ReadHits1924 (*Level, *Runtime, *this),
        0);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
