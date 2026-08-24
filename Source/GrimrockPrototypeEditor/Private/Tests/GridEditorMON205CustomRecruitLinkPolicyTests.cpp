#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridTypes.h"
#include "EditorTools/GridEditorLinkPolicy.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridEditorMON2055CustomRecruitLinkPolicyTest,
    "Grimrock.MON20.5.CustomRecruit.EditorLinkPolicy",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridEditorMON2055CustomRecruitLinkPolicyTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;

    FGridLevelObjectData Recruiter;
    Recruiter.ObjectId = FGuid (20, 5, 5, 100);
    Recruiter.Type = EGridLevelObjectType::CustomRecruiter;

    const TArray<EGridObjectCommand> Commands =
        GridEditorLinkPolicy::GetSupportedCommandsForTarget (Recruiter);
    TestEqual (
        TEXT ("Custom recruiter exposes exactly one target command"),
        Commands.Num (),
        1);
    TestTrue (
        TEXT ("Custom recruiter exposes OpenCustomRecruit"),
        Commands.Contains (EGridObjectCommand::OpenCustomRecruit));
    TestTrue (
        TEXT ("OpenCustomRecruit has Gameplay runtime support"),
        GridEditorLinkPolicy::GetCommandRuntimeSupport (
            Recruiter,
            EGridObjectCommand::OpenCustomRecruit) ==
        EGridEditorCommandRuntimeSupport::Gameplay);
    TestTrue (
        TEXT ("Custom recruiter rejects generic Toggle"),
        GridEditorLinkPolicy::GetCommandRuntimeSupport (
            Recruiter,
            EGridObjectCommand::Toggle) ==
        EGridEditorCommandRuntimeSupport::Unsupported);
    TestTrue (
        TEXT ("Custom recruiter can receive editor links"),
        GridEditorLinkPolicy::CanObjectReceiveCommands (Recruiter));
    TestFalse (
        TEXT ("Custom recruiter is a target only and emits no events"),
        GridEditorLinkPolicy::CanObjectEmitEvents (Recruiter));

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
