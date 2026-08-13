#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridTypes.h"
#include "EditorTools/GridEditorLinkPolicy.h"

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

#endif
