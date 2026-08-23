#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridTypes.h"
#include "EditorTools/GridEditorLinkPolicy.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridEditorMON2044RecruitmentLinkPolicyTest,
    "Grimrock.MON20.4.RecruitmentUI.EditorLinkPolicy",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridEditorMON2044RecruitmentLinkPolicyTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;

    FGridLevelObjectData Companion;
    Companion.ObjectId = FGuid (20, 4, 4, 100);
    Companion.Type = EGridLevelObjectType::StoryCompanion;

    const TArray<EGridObjectCommand> Commands =
        GridEditorLinkPolicy::GetSupportedCommandsForTarget (Companion);
    TestEqual (
        TEXT ("Story companion exposes exactly one target command"),
        Commands.Num (),
        1);
    TestTrue (
        TEXT ("Story companion exposes OfferRecruitment"),
        Commands.Contains (EGridObjectCommand::OfferRecruitment));
    TestTrue (
        TEXT ("OfferRecruitment has Gameplay runtime support"),
        GridEditorLinkPolicy::GetCommandRuntimeSupport (
            Companion,
            EGridObjectCommand::OfferRecruitment) ==
        EGridEditorCommandRuntimeSupport::Gameplay);
    TestTrue (
        TEXT ("Story companion rejects generic Toggle"),
        GridEditorLinkPolicy::GetCommandRuntimeSupport (
            Companion,
            EGridObjectCommand::Toggle) ==
        EGridEditorCommandRuntimeSupport::Unsupported);
    TestTrue (
        TEXT ("Story companion can receive editor links"),
        GridEditorLinkPolicy::CanObjectReceiveCommands (Companion));
    TestFalse (
        TEXT ("Story companion is a target only and emits no events by default"),
        GridEditorLinkPolicy::CanObjectEmitEvents (Companion));

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
