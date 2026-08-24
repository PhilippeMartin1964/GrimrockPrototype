#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Runtime/GrimrockPartyPawn.h"
#include "UI/RPGCharacterCreationWidget.h"
#include "UObject/Class.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON205CustomRecruitRuntimeContractTest,
    "Grimrock.MON20.5.CustomRecruit.RuntimeContract",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FRPGMON205CustomRecruitRuntimeContractTest::RunTest (
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
        TEXT ("Custom recruit reuses CharacterCreationWidgetClass"),
        PawnClass->FindPropertyByName (
            GET_MEMBER_NAME_CHECKED (
                AGrimrockPartyPawn,
                CharacterCreationWidgetClass)));
    TestNotNull (
        TEXT ("Custom recruit reuses CharacterCreationWidgetInstance"),
        PawnClass->FindPropertyByName (
            GET_MEMBER_NAME_CHECKED (
                AGrimrockPartyPawn,
                CharacterCreationWidgetInstance)));
    TestNotNull (
        TEXT ("Custom recruit reuses character creation modal state"),
        PawnClass->FindPropertyByName (
            GET_MEMBER_NAME_CHECKED (
                AGrimrockPartyPawn,
                bCharacterCreationModalActive)));

    TestNotNull (
        TEXT ("Runtime exposes ShowCustomRecruitCharacterCreationWidget"),
        PawnClass->FindFunctionByName (
            TEXT ("ShowCustomRecruitCharacterCreationWidget")));
    TestNotNull (
        TEXT ("Runtime exposes CloseCustomRecruitCharacterCreationWidget"),
        PawnClass->FindFunctionByName (
            TEXT ("CloseCustomRecruitCharacterCreationWidget")));
    TestNotNull (
        TEXT ("Runtime exposes IsCustomRecruitCharacterCreationModalActive"),
        PawnClass->FindFunctionByName (
            TEXT ("IsCustomRecruitCharacterCreationModalActive")));
    TestNotNull (
        TEXT ("Runtime exposes GetCustomRecruitCharacterCreationWidget"),
        PawnClass->FindFunctionByName (
            TEXT ("GetCustomRecruitCharacterCreationWidget")));

    TestNull (
        TEXT ("No parallel CustomRecruit widget class property was introduced"),
        PawnClass->FindPropertyByName (TEXT ("CustomRecruitWidgetClass")));
    TestNull (
        TEXT ("No parallel CustomRecruit widget instance property was introduced"),
        PawnClass->FindPropertyByName (TEXT ("CustomRecruitWidgetInstance")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON205CustomRecruitRuntimeDefaultStateTest,
    "Grimrock.MON20.5.CustomRecruit.RuntimeDefaultState",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FRPGMON205CustomRecruitRuntimeDefaultStateTest::RunTest (
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

    TestFalse (
        TEXT ("Character creation modal is inactive by default"),
        DefaultPawn->bCharacterCreationModalActive);
    TestNull (
        TEXT ("No character creation widget instance exists on the CDO"),
        DefaultPawn->CharacterCreationWidgetInstance.Get ());
    TestFalse (
        TEXT ("Custom recruit modal is inactive by default"),
        DefaultPawn->IsCustomRecruitCharacterCreationModalActive ());
    TestNull (
        TEXT ("Custom recruit widget getter is null by default"),
        DefaultPawn->GetCustomRecruitCharacterCreationWidget ());
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
