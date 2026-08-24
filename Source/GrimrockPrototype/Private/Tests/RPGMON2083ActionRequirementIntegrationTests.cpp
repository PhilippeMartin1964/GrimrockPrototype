#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "RPG/RPGSkillAsset.h"
#include "RPG/RPGSkillRequirementProjectionService.h"
#include "RPG/RPGSkillService.h"
#include "Runtime/Combat/GridCombatActionCatalog.h"
#include "Runtime/GridInventoryTypes.h"

namespace
{
    FGridCombatActionCatalogContext MakeMON2083Context ()
    {
        FGridCombatActionCatalogContext Context;
        Context.CharacterIndex = 0;
        Context.CharacterId = FGuid::NewGuid ();
        Context.bCombatActive = true;
        Context.bCharacterDefeated = false;
        Context.bActiveCombatant = true;
        Context.bPartyBusy = false;
        Context.bEnableQuickItemExecutors = true;
        Context.bEnableClassActionExecutors = true;
        Context.RemainingActionPoints = 6;
        Context.CurrentHealth = 10;
        Context.MaximumHealth = 10;
        Context.CurrentMana = 10;
        Context.MaximumMana = 10;
        return Context;
    }

    FGridCombatActionContribution MakeMON2083Action (
        FName ActionId,
        const TArray<FName>& Requirements)
    {
        FGridCombatActionContribution Contribution;
        Contribution.Definition =
            FGridCombatActionCatalog::MakeUnarmedAttackDefinition (2);
        Contribution.Definition.ActionId = ActionId;
        Contribution.Definition.OffensiveProfile.AttackId = ActionId;
        Contribution.Definition.Requirements = Requirements;
        Contribution.SourceDefinitionId = NAME_None;
        Contribution.AvailableSourceQuantity = 0;
        return Contribution;
    }

    const FGridAvailableCombatAction* BuildMON2083SingleAction (
        const FGridCombatActionCatalogContext& Context,
        const FGridCombatActionContribution& Contribution,
        TArray<FGridAvailableCombatAction>& OutActions)
    {
        TArray<FGridCombatActionContribution> Contributions;
        Contributions.Add (Contribution);
        FGridCombatActionCatalog::Build (
            Context,
            Contributions,
            OutActions);
        return OutActions.Num () == 1 ? &OutActions[0] : nullptr;
    }

    URPGSkillAsset* MakeMON2083Skill (
        UObject* Outer,
        FName SkillId = TEXT ("Skill_Lockpicking"),
        int32 MaxRank = 5)
    {
        URPGSkillAsset* Skill = NewObject<URPGSkillAsset> (Outer);
        Skill->SkillId = SkillId;
        Skill->DisplayName = FText::FromString (TEXT ("Crochetage"));
        Skill->Description =
            FText::FromString (TEXT ("Ouvrir les serrures mécaniques."));
        Skill->GoverningAttribute =
            ERPGSkillGoverningAttribute::Dexterity;
        Skill->MaxRank = MaxRank;
        Skill->bAllowUntrainedChecks = true;
        return Skill;
    }

    FRPGSkillRequirementGrant MakeMON2083Grant (
        int32 MinimumRank,
        FName RequirementId)
    {
        FRPGSkillRequirementGrant Grant;
        Grant.MinimumRank = MinimumRank;
        Grant.GrantedRequirementIds.Add (RequirementId);
        return Grant;
    }

    FGridCharacterInventoryState MakeMON2083Character ()
    {
        FGridCharacterInventoryState Character;
        Character.CharacterId = FGuid::NewGuid ();
        Character.DisplayName =
            FText::FromString (TEXT ("MON20.8.3 Tester"));
        return Character;
    }

    bool SetMON2083Rank (
        FGridCharacterInventoryState& Character,
        URPGSkillAsset* Skill,
        int32 Rank)
    {
        FRPGSkillMutationResult Result;
        return FRPGSkillService::TrySetSkillRank (
            Character,
            Skill,
            Rank,
            Result);
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON2083MissingRequirementDiagnosticTest,
    "Grimrock.MON20.8.ActionRequirements.MissingRequirementDiagnostic",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2083MissingRequirementDiagnosticTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;

    const FGridCombatActionCatalogContext Context = MakeMON2083Context ();
    const FGridCombatActionContribution Contribution = MakeMON2083Action (
        TEXT ("Action_LockpickStrike"),
        { TEXT ("Skill_Lockpicking") });
    TArray<FGridAvailableCombatAction> Actions;
    const FGridAvailableCombatAction* Action =
        BuildMON2083SingleAction (Context, Contribution, Actions);

    TestNotNull (TEXT ("Action is projected"), Action);
    if (!Action)
    {
        return false;
    }
    TestFalse (TEXT ("Missing requirement disables action"), Action->bEnabled);
    TestEqual (TEXT ("Availability reason is MissingRequirement"),
        Action->AvailabilityReason,
        EGridCombatActionAvailabilityReason::MissingRequirement);
    TestEqual (TEXT ("Exactly one missing requirement is exposed"),
        Action->MissingRequirements.Num (), 1);
    TestTrue (TEXT ("Missing SkillId is exposed"),
        Action->MissingRequirements.Contains (TEXT ("Skill_Lockpicking")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON2083MissingRequirementsDeterministicOrderTest,
    "Grimrock.MON20.8.ActionRequirements.MissingRequirementsDeterministicOrder",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2083MissingRequirementsDeterministicOrderTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;

    const FGridCombatActionCatalogContext Context = MakeMON2083Context ();
    const FGridCombatActionContribution Contribution = MakeMON2083Action (
        TEXT ("Action_MultiGate"),
        {
            TEXT ("Req_Zeta"),
            TEXT ("Req_Alpha"),
            TEXT ("Req_Zeta"),
            TEXT ("Req_Mid")
        });
    TArray<FGridAvailableCombatAction> Actions;
    const FGridAvailableCombatAction* Action =
        BuildMON2083SingleAction (Context, Contribution, Actions);

    TestNotNull (TEXT ("Action is projected"), Action);
    if (!Action)
    {
        return false;
    }
    TestEqual (TEXT ("Missing requirements are deduplicated"),
        Action->MissingRequirements.Num (), 3);
    if (Action->MissingRequirements.Num () == 3)
    {
        TestEqual (TEXT ("First diagnostic is deterministic"),
            Action->MissingRequirements[0], FName (TEXT ("Req_Alpha")));
        TestEqual (TEXT ("Second diagnostic is deterministic"),
            Action->MissingRequirements[1], FName (TEXT ("Req_Mid")));
        TestEqual (TEXT ("Third diagnostic is deterministic"),
            Action->MissingRequirements[2], FName (TEXT ("Req_Zeta")));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON2083RequirementsSatisfiedTest,
    "Grimrock.MON20.8.ActionRequirements.RequirementsSatisfied",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2083RequirementsSatisfiedTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;

    FGridCombatActionCatalogContext Context = MakeMON2083Context ();
    Context.SatisfiedRequirements.Add (TEXT ("Req_Trained"));
    const FGridCombatActionContribution Contribution = MakeMON2083Action (
        TEXT ("Action_Ready"),
        { TEXT ("Req_Trained") });
    TArray<FGridAvailableCombatAction> Actions;
    const FGridAvailableCombatAction* Action =
        BuildMON2083SingleAction (Context, Contribution, Actions);

    TestNotNull (TEXT ("Action is projected"), Action);
    if (!Action)
    {
        return false;
    }
    TestTrue (TEXT ("Satisfied requirement enables action"), Action->bEnabled);
    TestEqual (TEXT ("Enabled action has no missing requirements"),
        Action->MissingRequirements.Num (), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON2083SkillIdUnlocksActionTest,
    "Grimrock.MON20.8.ActionRequirements.SkillIdUnlocksAction",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2083SkillIdUnlocksActionTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;

    FGridCharacterInventoryState Character = MakeMON2083Character ();
    URPGSkillAsset* Skill = MakeMON2083Skill (GetTransientPackage ());
    TestTrue (TEXT ("Rank one setup succeeds"),
        SetMON2083Rank (Character, Skill, 1));

    FGridCombatActionCatalogContext Context = MakeMON2083Context ();
    FString Error;
    TestTrue (TEXT ("Skill projection succeeds"),
        FRPGSkillRequirementProjectionService::AppendSatisfiedRequirements (
            Character,
            [Skill] (FName SkillId)
            {
                return SkillId == Skill->SkillId ? Skill : nullptr;
            },
            Context.SatisfiedRequirements,
            Error));

    const FGridCombatActionContribution Contribution = MakeMON2083Action (
        TEXT ("Action_TrainingGate"),
        { Skill->SkillId });
    TArray<FGridAvailableCombatAction> Actions;
    const FGridAvailableCombatAction* Action =
        BuildMON2083SingleAction (Context, Contribution, Actions);

    TestNotNull (TEXT ("Action is projected"), Action);
    if (!Action)
    {
        return false;
    }
    TestTrue (TEXT ("Positive skill rank unlocks SkillId-gated action"),
        Action->bEnabled);
    TestTrue (TEXT ("No missing SkillId remains"),
        Action->MissingRequirements.IsEmpty ());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON2083SkillThresholdUnlocksActionTest,
    "Grimrock.MON20.8.ActionRequirements.SkillThresholdUnlocksAction",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2083SkillThresholdUnlocksActionTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;

    FGridCharacterInventoryState Character = MakeMON2083Character ();
    URPGSkillAsset* Skill = MakeMON2083Skill (GetTransientPackage ());
    Skill->RequirementGrants.Add (
        MakeMON2083Grant (2, TEXT ("Req_Lockpicking_Advanced")));
    TestTrue (TEXT ("Rank two setup succeeds"),
        SetMON2083Rank (Character, Skill, 2));

    FGridCombatActionCatalogContext Context = MakeMON2083Context ();
    FString Error;
    TestTrue (TEXT ("Threshold projection succeeds"),
        FRPGSkillRequirementProjectionService::AppendSatisfiedRequirements (
            Character,
            [Skill] (FName SkillId)
            {
                return SkillId == Skill->SkillId ? Skill : nullptr;
            },
            Context.SatisfiedRequirements,
            Error));

    const FGridCombatActionContribution Contribution = MakeMON2083Action (
        TEXT ("Action_AdvancedLockpick"),
        { TEXT ("Req_Lockpicking_Advanced") });
    TArray<FGridAvailableCombatAction> Actions;
    const FGridAvailableCombatAction* Action =
        BuildMON2083SingleAction (Context, Contribution, Actions);

    TestNotNull (TEXT ("Action is projected"), Action);
    if (!Action)
    {
        return false;
    }
    TestTrue (TEXT ("Reached threshold unlocks action"), Action->bEnabled);
    TestTrue (TEXT ("Threshold diagnostic is empty"),
        Action->MissingRequirements.IsEmpty ());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON2083SkillAndExistingRequirementsComposeTest,
    "Grimrock.MON20.8.ActionRequirements.SkillAndExistingRequirementsCompose",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2083SkillAndExistingRequirementsComposeTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;

    FGridCharacterInventoryState Character = MakeMON2083Character ();
    URPGSkillAsset* Skill = MakeMON2083Skill (GetTransientPackage ());
    TestTrue (TEXT ("Skill setup succeeds"),
        SetMON2083Rank (Character, Skill, 1));

    FGridCombatActionCatalogContext Context = MakeMON2083Context ();
    Context.SatisfiedRequirements.Add (TEXT ("Class_Rogue"));
    Context.SatisfiedRequirements.Add (TEXT ("ItemTag_Dagger"));
    FString Error;
    TestTrue (TEXT ("Skill projection preserves and extends existing gates"),
        FRPGSkillRequirementProjectionService::AppendSatisfiedRequirements (
            Character,
            [Skill] (FName SkillId)
            {
                return SkillId == Skill->SkillId ? Skill : nullptr;
            },
            Context.SatisfiedRequirements,
            Error));

    const FGridCombatActionContribution Contribution = MakeMON2083Action (
        TEXT ("Action_ComposedGate"),
        {
            TEXT ("Class_Rogue"),
            TEXT ("ItemTag_Dagger"),
            TEXT ("Skill_Lockpicking")
        });
    TArray<FGridAvailableCombatAction> Actions;
    const FGridAvailableCombatAction* Action =
        BuildMON2083SingleAction (Context, Contribution, Actions);

    TestNotNull (TEXT ("Action is projected"), Action);
    if (!Action)
    {
        return false;
    }
    TestTrue (TEXT ("Class, equipment and Skill requirements compose"),
        Action->bEnabled);
    TestEqual (TEXT ("No composed requirement is missing"),
        Action->MissingRequirements.Num (), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON2083SkillProjectionFailureAtomicTest,
    "Grimrock.MON20.8.ActionRequirements.SkillProjectionFailureAtomic",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2083SkillProjectionFailureAtomicTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;

    FGridCharacterInventoryState Character = MakeMON2083Character ();
    URPGSkillAsset* Skill = MakeMON2083Skill (
        GetTransientPackage (),
        TEXT ("Skill_Lockpicking"),
        3);
    FRPGSkillRank InvalidRank;
    InvalidRank.SkillId = Skill->SkillId;
    InvalidRank.Rank = 4;
    Character.SkillRanks.Add (InvalidRank);

    FGridCombatActionCatalogContext Context = MakeMON2083Context ();
    Context.SatisfiedRequirements.Add (TEXT ("Class_Rogue"));
    FString Error;
    TestFalse (TEXT ("Invalid Skill projection is rejected"),
        FRPGSkillRequirementProjectionService::AppendSatisfiedRequirements (
            Character,
            [Skill] (FName SkillId)
            {
                return SkillId == Skill->SkillId ? Skill : nullptr;
            },
            Context.SatisfiedRequirements,
            Error));
    TestTrue (TEXT ("Existing class requirement survives failure"),
        Context.SatisfiedRequirements.Contains (TEXT ("Class_Rogue")));
    TestFalse (TEXT ("Invalid Skill requirement is not leaked"),
        Context.SatisfiedRequirements.Contains (Skill->SkillId));

    TArray<FGridCombatActionContribution> Contributions;
    Contributions.Add (MakeMON2083Action (
        TEXT ("Action_ClassOnly"),
        { TEXT ("Class_Rogue") }));
    Contributions.Add (MakeMON2083Action (
        TEXT ("Action_SkillOnly"),
        { Skill->SkillId }));
    TArray<FGridAvailableCombatAction> Actions;
    FGridCombatActionCatalog::Build (Context, Contributions, Actions);

    TestEqual (TEXT ("Both actions remain inspectable"), Actions.Num (), 2);
    if (Actions.Num () == 2)
    {
        TestTrue (TEXT ("Unrelated class-gated action remains enabled"),
            Actions[0].bEnabled);
        TestFalse (TEXT ("Skill-gated action fails closed"),
            Actions[1].bEnabled);
        TestTrue (TEXT ("Skill failure is diagnosed by SkillId"),
            Actions[1].MissingRequirements.Contains (Skill->SkillId));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON2083NonRequirementFailureKeepsDiagnosticTest,
    "Grimrock.MON20.8.ActionRequirements.NonRequirementFailureKeepsDiagnostic",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2083NonRequirementFailureKeepsDiagnosticTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;

    FGridCombatActionCatalogContext Context = MakeMON2083Context ();
    Context.RemainingActionPoints = 0;
    const FGridCombatActionContribution Contribution = MakeMON2083Action (
        TEXT ("Action_APAndSkillGate"),
        { TEXT ("Skill_Lockpicking") });
    TArray<FGridAvailableCombatAction> Actions;
    const FGridAvailableCombatAction* Action =
        BuildMON2083SingleAction (Context, Contribution, Actions);

    TestNotNull (TEXT ("Action is projected"), Action);
    if (!Action)
    {
        return false;
    }
    TestEqual (TEXT ("Primary availability reason keeps existing gate order"),
        Action->AvailabilityReason,
        EGridCombatActionAvailabilityReason::InsufficientActionPoints);
    TestTrue (TEXT ("Missing requirements remain structurally diagnosable"),
        Action->MissingRequirements.Contains (TEXT ("Skill_Lockpicking")));
    return true;
}

#endif
