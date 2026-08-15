#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "RPG/RPGCharacterRulesLibrary.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGClassProgressionService.h"
#include "RPG/RPGLevelUpService.h"
#include "Runtime/Combat/GridCombatActionCatalog.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Save/GrimrockPartySaveGame.h"

namespace
{
    URPGClassAsset* MakeMON154Class (UObject* Outer)
    {
        URPGClassAsset* ClassDefinition =
            NewObject<URPGClassAsset> (Outer);
        ClassDefinition->ClassId = TEXT ("MON154_Fighter");
        ClassDefinition->DisplayName =
            FText::FromString (TEXT ("MON15.4 Fighter"));
        ClassDefinition->BaseAttributes =
            FRPGAttributes { 12, 12, 12, 10, 10, 10 };
        ClassDefinition->HealthAtLevelOne = 20;
        ClassDefinition->HealthPerLevel = 5;
        ClassDefinition->ManaAtLevelOne = 5;
        ClassDefinition->ManaPerLevel = 2;

        FRPGClassProgressionLevelGrant Level2;
        Level2.Level = 2;
        Level2.ChoicePointsGranted = 1;
        Level2.GrantedRequirementIds.Add (TEXT ("Feature_Level2"));
        ClassDefinition->ProgressionLevelGrants.Add (Level2);

        FRPGClassProgressionLevelGrant Level3;
        Level3.Level = 3;
        Level3.ChoicePointsGranted = 1;
        ClassDefinition->ProgressionLevelGrants.Add (Level3);

        FRPGClassProgressionLevelGrant Level4;
        Level4.Level = 4;
        Level4.ChoicePointsGranted = 2;
        Level4.GrantedRequirementIds.Add (TEXT ("Feature_Level4"));
        ClassDefinition->ProgressionLevelGrants.Add (Level4);

        FRPGClassProgressionChoiceDefinition ChoiceA;
        ChoiceA.ChoiceId = TEXT ("Choice_A");
        ChoiceA.DisplayName = FText::FromString (TEXT ("Choice A"));
        ChoiceA.MinimumLevel = 2;
        ChoiceA.PointCost = 1;
        ChoiceA.GrantedRequirementIds.Add (TEXT ("Feature_A"));
        ClassDefinition->ProgressionChoices.Add (ChoiceA);

        FRPGClassProgressionChoiceDefinition ChoiceB;
        ChoiceB.ChoiceId = TEXT ("Choice_B");
        ChoiceB.DisplayName = FText::FromString (TEXT ("Choice B"));
        ChoiceB.MinimumLevel = 3;
        ChoiceB.PointCost = 1;
        ChoiceB.PrerequisiteChoiceIds.Add (TEXT ("Choice_A"));
        ChoiceB.GrantedRequirementIds.Add (TEXT ("Feature_B"));
        ClassDefinition->ProgressionChoices.Add (ChoiceB);

        FRPGClassProgressionChoiceDefinition ChoiceC;
        ChoiceC.ChoiceId = TEXT ("Choice_C");
        ChoiceC.DisplayName = FText::FromString (TEXT ("Choice C"));
        ChoiceC.MinimumLevel = 4;
        ChoiceC.PointCost = 2;
        ChoiceC.PrerequisiteChoiceIds.Add (TEXT ("Choice_B"));
        ChoiceC.GrantedRequirementIds.Add (TEXT ("Feature_C"));
        ClassDefinition->ProgressionChoices.Add (ChoiceC);

        FRPGClassProgressionChoiceDefinition Expensive;
        Expensive.ChoiceId = TEXT ("Choice_Expensive");
        Expensive.DisplayName = FText::FromString (TEXT ("Expensive"));
        Expensive.MinimumLevel = 2;
        Expensive.PointCost = 3;
        ClassDefinition->ProgressionChoices.Add (Expensive);

        return ClassDefinition;
    }

    FGridCombatActionDefinition MakeMON154EffectAction (
        FName ActionId,
        FName RequirementId)
    {
        FGridCombatActionDefinition Definition;
        Definition.ActionId = ActionId;
        Definition.DisplayName = FText::FromName (ActionId);
        Definition.ActionType = EGridCombatActionType::Ability;
        Definition.SourcePolicy = EGridCombatActionSourcePolicy::Ability;
        Definition.TargetingPolicy = EGridCombatTargetingPolicy::Self;
        Definition.ResolutionProfile =
            EGridCombatActionResolutionProfile::Effect;
        Definition.ActionPointCost = 1;
        Definition.EffectProfile.RestoreHealth = 1;
        if (!RequirementId.IsNone ())
        {
            Definition.Requirements.Add (RequirementId);
        }
        return Definition;
    }

    FGridCombatActionCatalogContext MakeMON154CatalogContext (
        const TSet<FName>& SatisfiedRequirements)
    {
        FGridCombatActionCatalogContext Context;
        Context.CharacterIndex = 0;
        Context.CharacterId = FGuid::NewGuid ();
        Context.bCombatActive = true;
        Context.bCharacterDefeated = false;
        Context.bActiveCombatant = true;
        Context.bPartyBusy = false;
        Context.bEnableClassActionExecutors = true;
        Context.RemainingActionPoints = 4;
        Context.CurrentHealth = 5;
        Context.MaximumHealth = 10;
        Context.CurrentMana = 10;
        Context.MaximumMana = 10;
        Context.SatisfiedRequirements = SatisfiedRequirements;
        return Context;
    }

    FGridAvailableCombatAction BuildMON154AvailableAction (
        const URPGClassAsset* ClassDefinition,
        const FGridCombatActionDefinition& Definition,
        const TSet<FName>& SatisfiedRequirements)
    {
        FGridCombatActionContribution Contribution;
        Contribution.Definition = Definition;
        Contribution.SourceDefinitionId = ClassDefinition
            ? ClassDefinition->ClassId
            : NAME_None;
        Contribution.AvailableSourceQuantity = 1;

        TArray<FGridAvailableCombatAction> Actions;
        FGridCombatActionCatalog::Build (
            MakeMON154CatalogContext (SatisfiedRequirements),
            { Contribution },
            Actions);
        return Actions.Num () == 1
            ? Actions[0]
            : FGridAvailableCombatAction ();
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON154DefinitionValidationTest,
    "Grimrock.RPG.MON15.4.DefinitionValidation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON154DefinitionValidationTest::RunTest (const FString& Parameters)
{
    (void)Parameters;

    URPGClassAsset* Valid = MakeMON154Class (GetTransientPackage ());
    TestTrue (TEXT ("Valid progression definition is accepted"),
        Valid->IsValidDefinition ());

    URPGClassAsset* DuplicateLevel =
        MakeMON154Class (GetTransientPackage ());
    const FRPGClassProgressionLevelGrant DuplicateGrant =
        DuplicateLevel->ProgressionLevelGrants[0];
    DuplicateLevel->ProgressionLevelGrants.Add (DuplicateGrant);
    TestFalse (TEXT ("Duplicate grant level is rejected"),
        DuplicateLevel->IsValidDefinition ());

    URPGClassAsset* UnknownPrerequisite =
        MakeMON154Class (GetTransientPackage ());
    UnknownPrerequisite->ProgressionChoices[1].PrerequisiteChoiceIds.Add (
        TEXT ("Choice_Missing"));
    TestFalse (TEXT ("Unknown prerequisite is rejected"),
        UnknownPrerequisite->IsValidDefinition ());

    URPGClassAsset* Cycle = MakeMON154Class (GetTransientPackage ());
    Cycle->ProgressionChoices[0].PrerequisiteChoiceIds.Add (
        TEXT ("Choice_C"));
    TestFalse (TEXT ("Choice dependency cycle is rejected"),
        Cycle->IsValidDefinition ());

    URPGClassAsset* NoOpGrant = MakeMON154Class (GetTransientPackage ());
    FRPGClassProgressionLevelGrant EmptyGrant;
    EmptyGrant.Level = 5;
    NoOpGrant->ProgressionLevelGrants.Add (EmptyGrant);
    TestFalse (TEXT ("No-op level grant is rejected"),
        NoOpGrant->IsValidDefinition ());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON154ChoicePointAccountingTest,
    "Grimrock.RPG.MON15.4.ChoicePointAccounting",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON154ChoicePointAccountingTest::RunTest (const FString& Parameters)
{
    (void)Parameters;
    URPGClassAsset* ClassDefinition =
        MakeMON154Class (GetTransientPackage ());

    TestEqual (TEXT ("Level one grants zero choice points"),
        FRPGClassProgressionService::GetTotalChoicePointsGranted (
            ClassDefinition, 1),
        0);
    TestEqual (TEXT ("Level two grants one choice point"),
        FRPGClassProgressionService::GetTotalChoicePointsGranted (
            ClassDefinition, 2),
        1);
    TestEqual (TEXT ("Level three grants two cumulative points"),
        FRPGClassProgressionService::GetTotalChoicePointsGranted (
            ClassDefinition, 3),
        2);
    TestEqual (TEXT ("Level four grants four cumulative points"),
        FRPGClassProgressionService::GetTotalChoicePointsGranted (
            ClassDefinition, 4),
        4);

    TSet<FName> Selected;
    Selected.Add (TEXT ("Choice_A"));
    Selected.Add (TEXT ("Choice_B"));
    Selected.Add (TEXT ("Choice_C"));
    int32 Granted = 0;
    int32 Spent = 0;
    int32 Remaining = 0;
    TestTrue (TEXT ("A valid hypothetical selection has a balance"),
        FRPGClassProgressionService::TryGetChoicePointBalance (
            ClassDefinition, 4, Selected, Granted, Spent, Remaining));
    TestEqual (TEXT ("Four points granted"), Granted, 4);
    TestEqual (TEXT ("Four points spent"), Spent, 4);
    TestEqual (TEXT ("No point remains"), Remaining, 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON154SelectionRulesTest,
    "Grimrock.RPG.MON15.4.SelectionRules",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON154SelectionRulesTest::RunTest (const FString& Parameters)
{
    (void)Parameters;
    URPGClassAsset* ClassDefinition =
        MakeMON154Class (GetTransientPackage ());
    TSet<FName> Selected;

    TestTrue (TEXT ("Choice A is available at level two"),
        FRPGClassProgressionService::GetChoiceAvailability (
            ClassDefinition, 2, Selected, TEXT ("Choice_A")) ==
        ERPGClassProgressionChoiceAvailabilityReason::None);
    TestTrue (TEXT ("Choice B is level-gated at level two"),
        FRPGClassProgressionService::GetChoiceAvailability (
            ClassDefinition, 2, Selected, TEXT ("Choice_B")) ==
        ERPGClassProgressionChoiceAvailabilityReason::LevelTooLow);
    TestTrue (TEXT ("Choice B requires Choice A at level three"),
        FRPGClassProgressionService::GetChoiceAvailability (
            ClassDefinition, 3, Selected, TEXT ("Choice_B")) ==
        ERPGClassProgressionChoiceAvailabilityReason::MissingPrerequisite);
    TestTrue (TEXT ("Expensive choice is rejected for insufficient points"),
        FRPGClassProgressionService::GetChoiceAvailability (
            ClassDefinition, 2, Selected, TEXT ("Choice_Expensive")) ==
        ERPGClassProgressionChoiceAvailabilityReason::InsufficientChoicePoints);

    Selected.Add (TEXT ("Choice_A"));
    TestTrue (TEXT ("Choice B becomes available after its prerequisite"),
        FRPGClassProgressionService::GetChoiceAvailability (
            ClassDefinition, 3, Selected, TEXT ("Choice_B")) ==
        ERPGClassProgressionChoiceAvailabilityReason::None);
    TestTrue (TEXT ("Already-selected choice is rejected"),
        FRPGClassProgressionService::GetChoiceAvailability (
            ClassDefinition, 3, Selected, TEXT ("Choice_A")) ==
        ERPGClassProgressionChoiceAvailabilityReason::AlreadySelected);

    TSet<FName> InvalidSelection;
    InvalidSelection.Add (TEXT ("Choice_B"));
    int32 Granted = 0;
    int32 Spent = 0;
    int32 Remaining = 0;
    TestFalse (TEXT ("A selection missing its prerequisite is invalid"),
        FRPGClassProgressionService::TryGetChoicePointBalance (
            ClassDefinition,
            3,
            InvalidSelection,
            Granted,
            Spent,
            Remaining));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON154RequirementProjectionTest,
    "Grimrock.RPG.MON15.4.RequirementProjection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON154RequirementProjectionTest::RunTest (const FString& Parameters)
{
    (void)Parameters;
    URPGClassAsset* ClassDefinition =
        MakeMON154Class (GetTransientPackage ());
    TSet<FName> Requirements;

    TestTrue (TEXT ("Level one requirements resolve"),
        FRPGClassProgressionService::CollectAutomaticSatisfiedRequirements (
            ClassDefinition, 1, Requirements));
    TestTrue (TEXT ("Class identity is always satisfied"),
        Requirements.Contains (ClassDefinition->ClassId));
    TestFalse (TEXT ("Level two feature is locked at level one"),
        Requirements.Contains (TEXT ("Feature_Level2")));

    TestTrue (TEXT ("Level two requirements resolve"),
        FRPGClassProgressionService::CollectAutomaticSatisfiedRequirements (
            ClassDefinition, 2, Requirements));
    TestTrue (TEXT ("Level two automatic feature is granted"),
        Requirements.Contains (TEXT ("Feature_Level2")));

    TSet<FName> Selected;
    Selected.Add (TEXT ("Choice_A"));
    Selected.Add (TEXT ("Choice_B"));
    TestTrue (TEXT ("Hypothetical selected requirements resolve"),
        FRPGClassProgressionService::CollectSatisfiedRequirements (
            ClassDefinition, 3, Selected, Requirements));
    TestTrue (TEXT ("Choice A id is a requirement tag"),
        Requirements.Contains (TEXT ("Choice_A")));
    TestTrue (TEXT ("Choice B id is a requirement tag"),
        Requirements.Contains (TEXT ("Choice_B")));
    TestTrue (TEXT ("Choice A grants its feature tag"),
        Requirements.Contains (TEXT ("Feature_A")));
    TestTrue (TEXT ("Choice B grants its feature tag"),
        Requirements.Contains (TEXT ("Feature_B")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON154CombatActionUnlockProjectionTest,
    "Grimrock.RPG.MON15.4.CombatActionUnlockProjection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON154CombatActionUnlockProjectionTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    URPGClassAsset* ClassDefinition =
        MakeMON154Class (GetTransientPackage ());
    const FGridCombatActionDefinition LevelAction =
        MakeMON154EffectAction (
            TEXT ("Ability_Level2"),
            TEXT ("Feature_Level2"));
    ClassDefinition->CombatActions.Add (LevelAction);
    TestTrue (TEXT ("Class remains valid with progression-gated action"),
        ClassDefinition->IsValidDefinition ());

    TSet<FName> Requirements;
    TestTrue (TEXT ("Level one projection succeeds"),
        FRPGClassProgressionService::CollectAutomaticSatisfiedRequirements (
            ClassDefinition, 1, Requirements));
    FGridAvailableCombatAction Available =
        BuildMON154AvailableAction (
            ClassDefinition,
            LevelAction,
            Requirements);
    TestFalse (TEXT ("Level-gated ability is disabled before unlock"),
        Available.bEnabled);
    TestTrue (TEXT ("Locked ability reports MissingRequirement"),
        Available.AvailabilityReason ==
            EGridCombatActionAvailabilityReason::MissingRequirement);

    TestTrue (TEXT ("Level two projection succeeds"),
        FRPGClassProgressionService::CollectAutomaticSatisfiedRequirements (
            ClassDefinition, 2, Requirements));
    Available = BuildMON154AvailableAction (
        ClassDefinition,
        LevelAction,
        Requirements);
    TestTrue (TEXT ("Level-gated ability becomes available"),
        Available.bEnabled);

    const FGridCombatActionDefinition ChoiceAction =
        MakeMON154EffectAction (
            TEXT ("Ability_ChoiceA"),
            TEXT ("Choice_A"));
    TSet<FName> Selected;
    Selected.Add (TEXT ("Choice_A"));
    TestTrue (TEXT ("Choice projection succeeds"),
        FRPGClassProgressionService::CollectSatisfiedRequirements (
            ClassDefinition, 2, Selected, Requirements));
    Available = BuildMON154AvailableAction (
        ClassDefinition,
        ChoiceAction,
        Requirements);
    TestTrue (TEXT ("Choice-gated ability becomes available for selected candidate"),
        Available.bEnabled);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON154LevelUpIntegrationTest,
    "Grimrock.RPG.MON15.4.LevelUpIntegration",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON154LevelUpIntegrationTest::RunTest (const FString& Parameters)
{
    (void)Parameters;
    UGridPartyInventoryComponent* Component =
        NewObject<UGridPartyInventoryComponent> ();
    URPGClassAsset* ClassDefinition = MakeMON154Class (Component);

    FGridCharacterInventoryState Character;
    Character.CharacterId = FGuid::NewGuid ();
    Character.ClassId = ClassDefinition->ClassId;
    Character.ClassDefinition = ClassDefinition;
    Character.Level = 1;
    Character.Experience = 1000;
    Character.Attributes = ClassDefinition->BaseAttributes;
    Character.DerivedStats =
        URPGCharacterRulesLibrary::CalculateDerivedStats (
            Character.Attributes,
            ClassDefinition,
            Character.Level);
    Component->PartyInventoryState.ActiveCharacters.Add (Character);

    TestTrue (TEXT ("MON15.3 level-up applies"),
        FRPGLevelUpService::ApplyPendingLevelUp (
            Component, 0, false));
    TestEqual (TEXT ("Character reaches level two"),
        Component->PartyInventoryState.ActiveCharacters[0].Level,
        2);
    TestEqual (TEXT ("Level two now derives one class choice point"),
        FRPGClassProgressionService::GetTotalChoicePointsGranted (
            ClassDefinition,
            Component->PartyInventoryState.ActiveCharacters[0].Level),
        1);

    TSet<FName> Requirements;
    TestTrue (TEXT ("New level automatically projects progression requirements"),
        FRPGClassProgressionService::CollectAutomaticSatisfiedRequirements (
            ClassDefinition,
            Component->PartyInventoryState.ActiveCharacters[0].Level,
            Requirements));
    TestTrue (TEXT ("Level-up exposes the level-two feature"),
        Requirements.Contains (TEXT ("Feature_Level2")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON154LegacyCompatibilityTest,
    "Grimrock.RPG.MON15.4.LegacyCompatibility",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON154LegacyCompatibilityTest::RunTest (const FString& Parameters)
{
    (void)Parameters;
    URPGClassAsset* ClassDefinition =
        NewObject<URPGClassAsset> (GetTransientPackage ());
    ClassDefinition->ClassId = TEXT ("MON154_LegacyClass");
    ClassDefinition->HealthAtLevelOne = 10;
    ClassDefinition->CombatActions.Add (
        MakeMON154EffectAction (TEXT ("Legacy_Ability"), NAME_None));

    TestTrue (TEXT ("Class with no MON15.4 progression remains valid"),
        ClassDefinition->IsValidDefinition ());
    TestEqual (TEXT ("Legacy class grants no progression points"),
        FRPGClassProgressionService::GetTotalChoicePointsGranted (
            ClassDefinition, 10),
        0);

    TSet<FName> Requirements;
    TestTrue (TEXT ("Legacy class requirement projection succeeds"),
        FRPGClassProgressionService::CollectAutomaticSatisfiedRequirements (
            ClassDefinition, 10, Requirements));
    TestTrue (TEXT ("Legacy class identity remains available"),
        Requirements.Contains (ClassDefinition->ClassId));
    TestEqual (TEXT ("MON15.4 does not bump SaveVersion"),
        UGrimrockPartySaveGame::CurrentSaveVersion,
        3);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS