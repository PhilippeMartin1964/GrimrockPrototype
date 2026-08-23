#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Runtime/GrimrockPartyPawn.h"
#include "UI/RPGStoryCompanionRecruitmentWidget.h"
#include "UObject/Class.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMON204RecruitmentRuntimeContractTest,
    "Grimrock.MON20.4.RecruitmentUI.RuntimeContract",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMON204RecruitmentRuntimeContractTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;

    UClass* PawnClass = AGrimrockPartyPawn::StaticClass ();
    TestNotNull (TEXT ("Party pawn class exists"), PawnClass);
    if (!PawnClass)
    {
        return false;
    }

    TestNotNull (
        TEXT ("Recruitment widget class is configurable on the party pawn"),
        PawnClass->FindPropertyByName (
            GET_MEMBER_NAME_CHECKED (
                AGrimrockPartyPawn,
                StoryCompanionRecruitmentWidgetClass)));
    TestNotNull (
        TEXT ("Recruitment widget instance is tracked on the party pawn"),
        PawnClass->FindPropertyByName (
            GET_MEMBER_NAME_CHECKED (
                AGrimrockPartyPawn,
                StoryCompanionRecruitmentWidgetInstance)));
    TestNotNull (
        TEXT ("Recruitment modal Z-order is configurable"),
        PawnClass->FindPropertyByName (
            GET_MEMBER_NAME_CHECKED (
                AGrimrockPartyPawn,
                StoryCompanionRecruitmentZOrder)));

    TestNotNull (
        TEXT ("Runtime exposes ShowStoryCompanionRecruitmentWidget"),
        PawnClass->FindFunctionByName (
            TEXT ("ShowStoryCompanionRecruitmentWidget")));
    TestNotNull (
        TEXT ("Runtime exposes CloseStoryCompanionRecruitmentWidget"),
        PawnClass->FindFunctionByName (
            TEXT ("CloseStoryCompanionRecruitmentWidget")));
    TestNotNull (
        TEXT ("Runtime exposes IsStoryCompanionRecruitmentModalActive"),
        PawnClass->FindFunctionByName (
            TEXT ("IsStoryCompanionRecruitmentModalActive")));
    TestNotNull (
        TEXT ("Runtime exposes GetStoryCompanionRecruitmentWidget"),
        PawnClass->FindFunctionByName (
            TEXT ("GetStoryCompanionRecruitmentWidget")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMON204RecruitmentRuntimeDefaultStateTest,
    "Grimrock.MON20.4.RecruitmentUI.RuntimeDefaultState",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMON204RecruitmentRuntimeDefaultStateTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;

    const AGrimrockPartyPawn* DefaultPawn =
        GetDefault<AGrimrockPartyPawn> ();
    TestNotNull (TEXT ("Party pawn CDO exists"), DefaultPawn);
    if (!DefaultPawn)
    {
        return false;
    }

    TestEqual (
        TEXT ("Recruitment modal default Z-order is 500"),
        DefaultPawn->StoryCompanionRecruitmentZOrder,
        500);
    TestTrue (
        TEXT ("Production WBP remains optional at the C++ default level"),
        !DefaultPawn->StoryCompanionRecruitmentWidgetClass);
    TestNull (
        TEXT ("No recruitment widget instance exists on the CDO"),
        DefaultPawn->StoryCompanionRecruitmentWidgetInstance.Get ());
    TestFalse (
        TEXT ("Recruitment modal is inactive by default"),
        DefaultPawn->IsStoryCompanionRecruitmentModalActive ());
    TestNull (
        TEXT ("Recruitment widget getter is null by default"),
        DefaultPawn->GetStoryCompanionRecruitmentWidget ());
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
