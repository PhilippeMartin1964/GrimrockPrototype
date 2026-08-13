#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridTypes.h"
#include "Core/GridLevelAsset.h"
#include "EditorTools/GridEditorLinkPolicy.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridLevelRuntimeActor.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridEditorMON133MonsterSpawnLinkPolicyTest,
    "Grimrock.Monsters.MON13.3.EditorLinkPolicy",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridEditorMON133MonsterSpawnLinkPolicyTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;

    FGridLevelObjectData Trigger;
    Trigger.Type = EGridLevelObjectType::Trigger;
    const TArray<EGridObjectEvent> TriggerEvents =
        GridEditorLinkPolicy::GetSupportedEventsForSource (Trigger);
    TestTrue (TEXT ("Trigger remains a connector source"),
        GridEditorLinkPolicy::CanObjectEmitEvents (Trigger));
    TestTrue (TEXT ("Trigger exposes Activated"),
        TriggerEvents.Contains (EGridObjectEvent::Activated));

    FGridLevelObjectData MonsterSpawn;
    MonsterSpawn.Type = EGridLevelObjectType::MonsterSpawn;
    const TArray<EGridObjectCommand> MonsterCommands =
        GridEditorLinkPolicy::GetSupportedCommandsForTarget (
            MonsterSpawn);
    TestTrue (TEXT ("MonsterSpawn appears in Target Object"),
        GridEditorLinkPolicy::CanObjectReceiveCommands (
            MonsterSpawn));
    TestEqual (TEXT ("MonsterSpawn exposes every runtime command"),
        MonsterCommands.Num (),
        8);
    TestTrue (TEXT ("MonsterSpawn exposes Spawn"),
        MonsterCommands.Contains (EGridObjectCommand::Spawn));
    TestTrue (TEXT ("MonsterSpawn exposes Despawn"),
        MonsterCommands.Contains (EGridObjectCommand::Despawn));
    TestTrue (TEXT ("MonsterSpawn exposes Teleport"),
        MonsterCommands.Contains (EGridObjectCommand::Teleport));
    TestTrue (TEXT ("MonsterSpawn exposes Activate alias"),
        MonsterCommands.Contains (EGridObjectCommand::Activate));
    TestTrue (TEXT ("MonsterSpawn exposes Deactivate alias"),
        MonsterCommands.Contains (EGridObjectCommand::Deactivate));
    TestTrue (TEXT ("MonsterSpawn exposes Enable alias"),
        MonsterCommands.Contains (EGridObjectCommand::Enable));
    TestTrue (TEXT ("MonsterSpawn exposes Disable alias"),
        MonsterCommands.Contains (EGridObjectCommand::Disable));
    TestTrue (TEXT ("MonsterSpawn exposes Toggle alias"),
        MonsterCommands.Contains (EGridObjectCommand::Toggle));

    const TArray<EGridObjectEvent> MonsterEvents =
        GridEditorLinkPolicy::GetSupportedEventsForSource (
            MonsterSpawn);
    TestEqual (TEXT ("MonsterSpawn exposes every lifecycle event"),
        MonsterEvents.Num (),
        4);
    TestTrue (TEXT ("MonsterSpawn exposes MonsterDied"),
        MonsterEvents.Contains (EGridObjectEvent::MonsterDied));
    TestTrue (TEXT ("MonsterSpawn exposes MonsterSpawned"),
        MonsterEvents.Contains (EGridObjectEvent::MonsterSpawned));
    TestTrue (TEXT ("MonsterSpawn exposes MonsterDespawned"),
        MonsterEvents.Contains (EGridObjectEvent::MonsterDespawned));
    TestTrue (TEXT ("MonsterSpawn exposes MonsterTeleported"),
        MonsterEvents.Contains (EGridObjectEvent::MonsterTeleported));

    const TArray<EGridObjectEvent> DisplayOrder =
        GridEditorLinkPolicy::GetEventDisplayOrder ();
    TestTrue (TEXT ("Outgoing links display MonsterSpawned"),
        DisplayOrder.Contains (EGridObjectEvent::MonsterSpawned));
    TestTrue (TEXT ("Outgoing links display MonsterDespawned"),
        DisplayOrder.Contains (EGridObjectEvent::MonsterDespawned));
    TestTrue (TEXT ("Outgoing links display MonsterTeleported"),
        DisplayOrder.Contains (EGridObjectEvent::MonsterTeleported));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridEditorMON133FreshPIEPreparationTest,
    "Grimrock.Monsters.MON13.3.FreshPIEPreparation",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridEditorMON133FreshPIEPreparationTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;

    const UWorld::InitializationValues InitializationValues =
        UWorld::InitializationValues ()
            .AllowAudioPlayback (false)
            .RequiresHitProxies (false)
            .CreatePhysicsScene (false)
            .CreateNavigation (false)
            .CreateAISystem (false)
            .ShouldSimulatePhysics (false)
            .SetTransactional (false);
    UWorld* World = UWorld::CreateWorld (
        EWorldType::Editor,
        false,
        TEXT ("MON133FreshPIEPreparationWorld"),
        nullptr,
        true,
        ERHIFeatureLevel::Num,
        &InitializationValues);
    TestNotNull (TEXT ("Editor preparation world is created"), World);
    if (!World || !GEngine)
    {
        return false;
    }

    FWorldContext& Context =
        GEngine->CreateNewWorldContext (EWorldType::Editor);
    Context.SetCurrentWorld (World);

    AGridLevelRuntimeActor* Runtime =
        World->SpawnActor<AGridLevelRuntimeActor> ();
    AGridLevelEditorActor* EditorActor =
        World->SpawnActor<AGridLevelEditorActor> ();
    UGridLevelAsset* Level = NewObject<UGridLevelAsset> (EditorActor);
    Level->Width = 2;
    Level->Height = 2;
    Level->EnsureCellCount ();
    Level->Cells[0].CellType = EGridCellType::Floor;
    Level->StartCellX = 0;
    Level->StartCellY = 0;
    Level->StartFacing = EGridEdge::North;
    EditorActor->LevelAsset = Level;
    Runtime->LevelAsset = Level;

    FGridLevelRuntimeState PersistedLevelState;
    PersistedLevelState.LevelId = TEXT ("SingleLevel");
    PersistedLevelState.bHasBeenVisited = true;
    Runtime->DungeonRuntimeState.LevelStates.Add (
        PersistedLevelState.LevelId,
        PersistedLevelState);

    FString Error;
    TestTrue (TEXT ("PreparePIETestFromStartInternal succeeds"),
        EditorActor->PreparePIETestFromStartInternal (Error));
    TestTrue (TEXT ("PIE preparation marks fresh dungeon startup"),
        Runtime->bUseFreshDungeonStateOnBeginPlay);
    TestEqual (TEXT ("PIE preparation clears only transient dungeon state"),
        Runtime->DungeonRuntimeState.LevelStates.Num (),
        0);
    TestEqual (TEXT ("PIE preparation keeps the selected LevelAsset"),
        Runtime->LevelAsset.Get (),
        Level);

    World->DestroyWorld (false);
    GEngine->DestroyWorldContext (World);
    return true;
}

#endif
