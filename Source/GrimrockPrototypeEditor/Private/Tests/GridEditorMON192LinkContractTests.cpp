#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridTypes.h"
#include "EditorTools/GridEditorLinkPolicy.h"

#include <initializer_list>

namespace
{
    FGridLevelObjectData MakeObject (EGridLevelObjectType Type)
    {
        FGridLevelObjectData Object;
        Object.Type = Type;
        return Object;
    }

    template<typename TEnum>
    bool ContainsExactly (
        const TArray<TEnum>& Actual,
        std::initializer_list<TEnum> Expected)
    {
        if (Actual.Num () != static_cast<int32> (Expected.size ()))
        {
            return false;
        }

        for (const TEnum Value : Expected)
        {
            if (!Actual.Contains (Value))
            {
                return false;
            }
        }

        return true;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridEditorMON192LinkPolicyMatrixTest,
    "Grimrock.MON19.2.Editor.LinkPolicyMatrix",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridEditorMON192LinkPolicyMatrixTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;

    const FGridLevelObjectData Button = MakeObject (EGridLevelObjectType::Button);
    const FGridLevelObjectData Lever = MakeObject (EGridLevelObjectType::Lever);
    const FGridLevelObjectData PressurePlate = MakeObject (EGridLevelObjectType::PressurePlate);
    const FGridLevelObjectData Trigger = MakeObject (EGridLevelObjectType::Trigger);
    const FGridLevelObjectData Receptacle = MakeObject (EGridLevelObjectType::Receptacle);
    const FGridLevelObjectData MonsterSpawn = MakeObject (EGridLevelObjectType::MonsterSpawn);
    const FGridLevelObjectData Door = MakeObject (EGridLevelObjectType::Door);
    const FGridLevelObjectData Teleporter = MakeObject (EGridLevelObjectType::Teleporter);
    const FGridLevelObjectData Light = MakeObject (EGridLevelObjectType::Light);

    TestTrue (
        TEXT ("Button emits exactly Activated"),
        ContainsExactly (
            GridEditorLinkPolicy::GetSupportedEventsForSource (Button),
            {EGridObjectEvent::Activated}));
    TestTrue (
        TEXT ("Lever emits exactly Activated and Deactivated"),
        ContainsExactly (
            GridEditorLinkPolicy::GetSupportedEventsForSource (Lever),
            {EGridObjectEvent::Activated, EGridObjectEvent::Deactivated}));
    TestTrue (
        TEXT ("PressurePlate emits exactly Activated and Deactivated"),
        ContainsExactly (
            GridEditorLinkPolicy::GetSupportedEventsForSource (PressurePlate),
            {EGridObjectEvent::Activated, EGridObjectEvent::Deactivated}));
    TestTrue (
        TEXT ("Trigger emits exactly Activated and Deactivated"),
        ContainsExactly (
            GridEditorLinkPolicy::GetSupportedEventsForSource (Trigger),
            {EGridObjectEvent::Activated, EGridObjectEvent::Deactivated}));
    TestTrue (
        TEXT ("Receptacle emits exactly its three item events"),
        ContainsExactly (
            GridEditorLinkPolicy::GetSupportedEventsForSource (Receptacle),
            {
                EGridObjectEvent::ItemInserted,
                EGridObjectEvent::ItemRemoved,
                EGridObjectEvent::ItemChanged
            }));
    TestTrue (
        TEXT ("MonsterSpawn emits exactly its six lifecycle events"),
        ContainsExactly (
            GridEditorLinkPolicy::GetSupportedEventsForSource (MonsterSpawn),
            {
                EGridObjectEvent::MonsterDied,
                EGridObjectEvent::MonsterSpawned,
                EGridObjectEvent::MonsterDespawned,
                EGridObjectEvent::MonsterTeleported,
                EGridObjectEvent::EncounterWaveStarted,
                EGridObjectEvent::EncounterCompleted
            }));
    TestFalse (
        TEXT ("Door is not an event source in the current editor contract"),
        GridEditorLinkPolicy::CanObjectEmitEvents (Door));

    const TArray<EGridObjectCommand> DoorCommands =
        GridEditorLinkPolicy::GetSupportedCommandsForTarget (Door);
    TestTrue (
        TEXT ("Door exposes exactly five connector commands"),
        ContainsExactly (
            DoorCommands,
            {
                EGridObjectCommand::Open,
                EGridObjectCommand::Close,
                EGridObjectCommand::Toggle,
                EGridObjectCommand::Activate,
                EGridObjectCommand::Deactivate
            }));

    const TArray<EGridObjectCommand> ReceptacleCommands =
        GridEditorLinkPolicy::GetSupportedCommandsForTarget (Receptacle);
    TestTrue (
        TEXT ("Receptacle exposes exactly four specialized commands"),
        ContainsExactly (
            ReceptacleCommands,
            {
                EGridObjectCommand::ReceptacleConsumeItem,
                EGridObjectCommand::ReceptacleConsumeAllItems,
                EGridObjectCommand::ReceptacleEnableRemoval,
                EGridObjectCommand::ReceptacleDisableRemoval
            }));

    const TArray<EGridObjectCommand> MonsterCommands =
        GridEditorLinkPolicy::GetSupportedCommandsForTarget (MonsterSpawn);
    TestEqual (
        TEXT ("MonsterSpawn keeps the nine MON13 commands"),
        MonsterCommands.Num (),
        9);

    for (const EGridObjectCommand Command : DoorCommands)
    {
        TestTrue (
            TEXT ("Every official Door command has real gameplay support"),
            GridEditorLinkPolicy::GetCommandRuntimeSupport (Door, Command) ==
                EGridEditorCommandRuntimeSupport::Gameplay);
    }

    for (const EGridObjectCommand Command : ReceptacleCommands)
    {
        TestTrue (
            TEXT ("Every official Receptacle command has real gameplay support"),
            GridEditorLinkPolicy::GetCommandRuntimeSupport (Receptacle, Command) ==
                EGridEditorCommandRuntimeSupport::Gameplay);
    }

    for (const EGridObjectCommand Command : MonsterCommands)
    {
        TestTrue (
            TEXT ("Every official MonsterSpawn command has real gameplay support"),
            GridEditorLinkPolicy::GetCommandRuntimeSupport (MonsterSpawn, Command) ==
                EGridEditorCommandRuntimeSupport::Gameplay);
    }

    for (const EGridObjectCommand Command :
        GridEditorLinkPolicy::GetSupportedCommandsForTarget (Teleporter))
    {
        TestTrue (
            TEXT ("Teleporter connector commands are explicitly classified StateOnly"),
            GridEditorLinkPolicy::GetCommandRuntimeSupport (Teleporter, Command) ==
                EGridEditorCommandRuntimeSupport::StateOnly);
    }

    for (const EGridObjectCommand Command :
        GridEditorLinkPolicy::GetSupportedCommandsForTarget (Light))
    {
        TestTrue (
            TEXT ("Light connector commands are explicitly classified StateOnly"),
            GridEditorLinkPolicy::GetCommandRuntimeSupport (Light, Command) ==
                EGridEditorCommandRuntimeSupport::StateOnly);
    }

    TestTrue (
        TEXT ("Lever generic activation has real runtime behavior"),
        GridEditorLinkPolicy::GetCommandRuntimeSupport (
            Lever,
            EGridObjectCommand::Activate) ==
                EGridEditorCommandRuntimeSupport::Gameplay);
    TestTrue (
        TEXT ("PressurePlate generic Toggle has real runtime behavior"),
        GridEditorLinkPolicy::GetCommandRuntimeSupport (
            PressurePlate,
            EGridObjectCommand::Toggle) ==
                EGridEditorCommandRuntimeSupport::Gameplay);
    TestTrue (
        TEXT ("Unsupported command remains distinguishable"),
        GridEditorLinkPolicy::GetCommandRuntimeSupport (
            Door,
            EGridObjectCommand::ShowMessage) ==
                EGridEditorCommandRuntimeSupport::Unsupported);

    const TArray<EGridObjectCondition> ReceptacleConditions =
        GridEditorLinkPolicy::GetSupportedConditionsForTarget (Receptacle);
    TestTrue (
        TEXT ("Receptacle exposes its eight historical conditions plus two level-variable conditions"),
        ContainsExactly (
            ReceptacleConditions,
            {
                EGridObjectCondition::None,
                EGridObjectCondition::LevelVariableBoolEquals,
                EGridObjectCondition::LevelVariableIntCompare,
                EGridObjectCondition::ReceptacleIsEmpty,
                EGridObjectCondition::ReceptacleHasAnyItem,
                EGridObjectCondition::ReceptacleContainsItemDefinition,
                EGridObjectCondition::ReceptacleContainsItemTag,
                EGridObjectCondition::ReceptacleContainsItemType,
                EGridObjectCondition::ReceptacleItemCountAtLeast,
                EGridObjectCondition::ReceptacleWeightAtLeast
            }));

    TestTrue (
        TEXT ("Door exposes None plus both level-variable conditions"),
        ContainsExactly (
            GridEditorLinkPolicy::GetSupportedConditionsForTarget (Door),
            {
                EGridObjectCondition::None,
                EGridObjectCondition::LevelVariableBoolEquals,
                EGridObjectCondition::LevelVariableIntCompare
            }));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridEditorMON192ConditionalLinkIdentityTest,
    "Grimrock.MON19.2.Editor.ConditionalLinkIdentity",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridEditorMON192ConditionalLinkIdentityTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;

    FGridObjectLink Base;
    Base.SourceObjectId = FGuid::NewGuid ();
    Base.TargetObjectId = FGuid::NewGuid ();
    Base.SourceEvent = EGridObjectEvent::Activated;
    Base.Command = EGridObjectCommand::ReceptacleConsumeItem;
    Base.Condition = EGridObjectCondition::ReceptacleContainsItemDefinition;
    Base.ConditionItemDefinitionId = TEXT ("Item_RedGem");
    Base.ConditionCount = 1;
    Base.ConditionWeight = 2.5f;

    const FGridObjectLink ExactCopy = Base;
    TestTrue (
        TEXT ("Exact persistent connector data is equivalent"),
        GridEditorLinkPolicy::AreLinksExactlyEquivalent (Base, ExactCopy));

    FGridObjectLink DifferentCondition = Base;
    DifferentCondition.Condition = EGridObjectCondition::ReceptacleHasAnyItem;
    TestFalse (
        TEXT ("Same historical quadruplet with another condition is distinct"),
        GridEditorLinkPolicy::AreLinksExactlyEquivalent (Base, DifferentCondition));

    FGridObjectLink DifferentDefinition = Base;
    DifferentDefinition.ConditionItemDefinitionId = TEXT ("Item_BlueGem");
    TestFalse (
        TEXT ("A condition parameter participates in connector identity"),
        GridEditorLinkPolicy::AreLinksExactlyEquivalent (Base, DifferentDefinition));

    FGridObjectLink DifferentCount = Base;
    DifferentCount.ConditionCount = 2;
    TestFalse (
        TEXT ("ConditionCount participates in connector identity"),
        GridEditorLinkPolicy::AreLinksExactlyEquivalent (Base, DifferentCount));

    FGridObjectLink DifferentWeight = Base;
    DifferentWeight.ConditionWeight = 3.0f;
    TestFalse (
        TEXT ("ConditionWeight participates in connector identity"),
        GridEditorLinkPolicy::AreLinksExactlyEquivalent (Base, DifferentWeight));

    FGridObjectLink Inverted = Base;
    Inverted.bInvertCondition = true;
    TestFalse (
        TEXT ("Condition inversion participates in connector identity"),
        GridEditorLinkPolicy::AreLinksExactlyEquivalent (Base, Inverted));

    return true;
}

#endif
