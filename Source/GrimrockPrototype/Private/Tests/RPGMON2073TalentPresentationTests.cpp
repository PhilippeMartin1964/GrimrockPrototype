#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "RPGMON155TestHelpers.h"

namespace RPGMON2073TalentPresentationTests
{
    const FRPGLevelUpChoiceView* FindChoice (
        const FRPGLevelUpView& View,
        FName ChoiceId)
    {
        return View.Choices.FindByPredicate (
            [ChoiceId] (const FRPGLevelUpChoiceView& Choice)
            {
                return Choice.ChoiceId == ChoiceId;
            });
    }

    URPGLevelUpWidget* MakeWidget (
        UGridPartyInventoryComponent* Component,
        int32 PreviousLevel = 2,
        int32 NewLevel = 3)
    {
        URPGLevelUpWidget* Widget = NewObject<URPGLevelUpWidget> (Component);
        Widget->InitializeLevelUpWidget (
            Component,
            0,
            PreviousLevel,
            NewLevel);
        return Widget;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON2073PresentationVocabularyTest,
    "Grimrock.MON20.7.Talents.PresentationVocabulary",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2073PresentationVocabularyTest::RunTest (const FString& Parameters)
{
    (void)Parameters;
    using namespace RPGMON2073TalentPresentationTests;
    FMON155RuntimeStateGuard RuntimeGuard;
    URPGClassAsset* ClassDefinition = nullptr;
    UGridPartyInventoryComponent* Component =
        MakeMON155Inventory (3, 3000, ClassDefinition);
    URPGLevelUpWidget* Widget = MakeWidget (Component);

    TestEqual (TEXT ("Talent section vocabulary is explicit"),
        Widget->View.Presentation.TalentSectionTitle.ToString (),
        FString (TEXT ("TALENTS DE CLASSE")));
    TestEqual (TEXT ("Talent points vocabulary is explicit"),
        Widget->View.Presentation.TalentPointsLabel.ToString (),
        FString (TEXT ("Points de talent")));
    TestTrue (TEXT ("Empty talent message is exposed to Blueprint view"),
        !Widget->View.Presentation.EmptyTalentsMessage.IsEmpty ());
    TestTrue (TEXT ("Talent selection prompt is exposed to Blueprint view"),
        !Widget->View.Presentation.SelectionPrompt.IsEmpty ());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON2073PresentationChoiceIdentityTest,
    "Grimrock.MON20.7.Talents.PresentationChoiceIdentity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2073PresentationChoiceIdentityTest::RunTest (const FString& Parameters)
{
    (void)Parameters;
    using namespace RPGMON2073TalentPresentationTests;
    FMON155RuntimeStateGuard RuntimeGuard;
    URPGClassAsset* ClassDefinition = nullptr;
    UGridPartyInventoryComponent* Component =
        MakeMON155Inventory (3, 3000, ClassDefinition);
    URPGLevelUpWidget* Widget = MakeWidget (Component);

    TestNotNull (TEXT ("Choice A keeps its MON15 ChoiceId"),
        FindChoice (Widget->View, TEXT ("Choice_A")));
    TestNotNull (TEXT ("Choice B keeps its MON15 ChoiceId"),
        FindChoice (Widget->View, TEXT ("Choice_B")));
    TestNotNull (TEXT ("Choice C keeps its MON15 ChoiceId"),
        FindChoice (Widget->View, TEXT ("Choice_C")));
    TestNotNull (TEXT ("Expensive choice keeps its MON15 ChoiceId"),
        FindChoice (Widget->View, TEXT ("Choice_Expensive")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON2073PresentationAvailableTalentTest,
    "Grimrock.MON20.7.Talents.PresentationAvailableTalent",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2073PresentationAvailableTalentTest::RunTest (const FString& Parameters)
{
    (void)Parameters;
    using namespace RPGMON2073TalentPresentationTests;
    FMON155RuntimeStateGuard RuntimeGuard;
    URPGClassAsset* ClassDefinition = nullptr;
    UGridPartyInventoryComponent* Component =
        MakeMON155Inventory (3, 3000, ClassDefinition);
    URPGLevelUpWidget* Widget = MakeWidget (Component);

    const FRPGLevelUpChoiceView* ChoiceA = FindChoice (Widget->View, TEXT ("Choice_A"));
    const FRPGLevelUpChoiceView* ChoiceB = FindChoice (Widget->View, TEXT ("Choice_B"));
    TestNotNull (TEXT ("Choice A exists"), ChoiceA);
    TestNotNull (TEXT ("Choice B exists"), ChoiceB);
    if (ChoiceA)
    {
        TestTrue (TEXT ("Choice A is presented as available talent"), ChoiceA->bAvailable);
        TestFalse (TEXT ("Available talent is not committed"), ChoiceA->bCommitted);
        TestFalse (TEXT ("Available talent is not pending"), ChoiceA->bPending);
    }
    if (ChoiceB)
    {
        TestFalse (TEXT ("Choice B is blocked before prerequisite"), ChoiceB->bAvailable);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON2073PresentationPendingTalentTest,
    "Grimrock.MON20.7.Talents.PresentationPendingTalent",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2073PresentationPendingTalentTest::RunTest (const FString& Parameters)
{
    (void)Parameters;
    using namespace RPGMON2073TalentPresentationTests;
    FMON155RuntimeStateGuard RuntimeGuard;
    URPGClassAsset* ClassDefinition = nullptr;
    UGridPartyInventoryComponent* Component =
        MakeMON155Inventory (3, 3000, ClassDefinition);
    URPGLevelUpWidget* Widget = MakeWidget (Component);

    TestTrue (TEXT ("Talent A stages through historical MON15 API"),
        Widget->StageOrUnstageChoice (TEXT ("Choice_A")));

    const FRPGLevelUpChoiceView* ChoiceA = FindChoice (Widget->View, TEXT ("Choice_A"));
    const FRPGLevelUpChoiceView* ChoiceB = FindChoice (Widget->View, TEXT ("Choice_B"));
    if (ChoiceA)
    {
        TestTrue (TEXT ("Staged talent is pending"), ChoiceA->bPending);
        TestFalse (TEXT ("Staged talent is not yet committed"), ChoiceA->bCommitted);
    }
    if (ChoiceB)
    {
        TestTrue (TEXT ("Pending prerequisite makes B selectable in same batch"),
            ChoiceB->bAvailable);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON2073PresentationCommittedTalentTest,
    "Grimrock.MON20.7.Talents.PresentationCommittedTalent",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2073PresentationCommittedTalentTest::RunTest (const FString& Parameters)
{
    (void)Parameters;
    using namespace RPGMON2073TalentPresentationTests;
    FMON155RuntimeStateGuard RuntimeGuard;
    URPGClassAsset* ClassDefinition = nullptr;
    UGridPartyInventoryComponent* Component =
        MakeMON155Inventory (3, 3000, ClassDefinition);

    FRPGClassProgressionCommitResult Commit;
    TestTrue (TEXT ("Talent A commits through MON15 transaction"),
        FRPGClassProgressionTransactionService::TryCommitChoices (
            Component, 0, { TEXT ("Choice_A") }, Commit));

    URPGLevelUpWidget* Widget = MakeWidget (Component);
    const FRPGLevelUpChoiceView* ChoiceA = FindChoice (Widget->View, TEXT ("Choice_A"));
    const FRPGLevelUpChoiceView* ChoiceB = FindChoice (Widget->View, TEXT ("Choice_B"));
    if (ChoiceA)
    {
        TestTrue (TEXT ("Committed talent is presented acquired"), ChoiceA->bCommitted);
        TestFalse (TEXT ("Committed talent is not pending"), ChoiceA->bPending);
    }
    if (ChoiceB)
    {
        TestTrue (TEXT ("Committed prerequisite makes B available"), ChoiceB->bAvailable);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON2073PresentationPointBudgetTest,
    "Grimrock.MON20.7.Talents.PresentationPointBudget",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2073PresentationPointBudgetTest::RunTest (const FString& Parameters)
{
    (void)Parameters;
    using namespace RPGMON2073TalentPresentationTests;
    FMON155RuntimeStateGuard RuntimeGuard;
    URPGClassAsset* ClassDefinition = nullptr;
    UGridPartyInventoryComponent* Component =
        MakeMON155Inventory (3, 3000, ClassDefinition);
    URPGLevelUpWidget* Widget = MakeWidget (Component);

    TestEqual (TEXT ("Level three grants two talent/progression points"),
        Widget->View.GrantedChoicePoints, 2);
    TestEqual (TEXT ("No talent point starts spent"),
        Widget->View.SpentChoicePoints, 0);
    TestEqual (TEXT ("Two talent points start available"),
        Widget->View.RemainingChoicePoints, 2);

    TestTrue (TEXT ("Talent A stages"),
        Widget->StageOrUnstageChoice (TEXT ("Choice_A")));
    TestEqual (TEXT ("Staging spends one point in the presentation"),
        Widget->View.SpentChoicePoints, 1);
    TestEqual (TEXT ("One point remains after staging"),
        Widget->View.RemainingChoicePoints, 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON2073PresentationConfirmTransactionTest,
    "Grimrock.MON20.7.Talents.PresentationConfirmTransaction",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2073PresentationConfirmTransactionTest::RunTest (const FString& Parameters)
{
    (void)Parameters;
    using namespace RPGMON2073TalentPresentationTests;
    FMON155RuntimeStateGuard RuntimeGuard;
    URPGClassAsset* ClassDefinition = nullptr;
    UGridPartyInventoryComponent* Component =
        MakeMON155Inventory (3, 3000, ClassDefinition);
    URPGLevelUpWidget* Widget = MakeWidget (Component);

    TestTrue (TEXT ("Talent A stages"),
        Widget->StageOrUnstageChoice (TEXT ("Choice_A")));
    TestTrue (TEXT ("Talent B stages after pending prerequisite"),
        Widget->StageOrUnstageChoice (TEXT ("Choice_B")));
    TestTrue (TEXT ("Confirm still delegates to MON15 transaction"),
        Widget->ConfirmSelection ());

    TArray<FName> Selected;
    TestTrue (TEXT ("Authoritative selection remains readable"),
        FRPGClassProgressionTransactionService::TryGetSelectedChoiceIds (
            Component, 0, Selected));
    TestEqual (TEXT ("Two talents committed"), Selected.Num (), 2);
    TestTrue (TEXT ("Choice A retained as authoritative identity"),
        Selected.Contains (TEXT ("Choice_A")));
    TestTrue (TEXT ("Choice B retained as authoritative identity"),
        Selected.Contains (TEXT ("Choice_B")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON2073PresentationCancelNoMutationTest,
    "Grimrock.MON20.7.Talents.PresentationCancelNoMutation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2073PresentationCancelNoMutationTest::RunTest (const FString& Parameters)
{
    (void)Parameters;
    using namespace RPGMON2073TalentPresentationTests;
    FMON155RuntimeStateGuard RuntimeGuard;
    URPGClassAsset* ClassDefinition = nullptr;
    UGridPartyInventoryComponent* Component =
        MakeMON155Inventory (3, 3000, ClassDefinition);
    URPGLevelUpWidget* Widget = MakeWidget (Component);

    TestTrue (TEXT ("Talent A stages before cancel"),
        Widget->StageOrUnstageChoice (TEXT ("Choice_A")));
    Widget->CancelSelection ();

    TArray<FName> Selected;
    TestTrue (TEXT ("Authoritative selection remains readable after cancel"),
        FRPGClassProgressionTransactionService::TryGetSelectedChoiceIds (
            Component, 0, Selected));
    TestTrue (TEXT ("Cancel commits no talent"), Selected.IsEmpty ());
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
