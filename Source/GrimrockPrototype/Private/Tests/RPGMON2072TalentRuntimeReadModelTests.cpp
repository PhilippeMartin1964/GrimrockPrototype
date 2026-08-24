#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "RPG/RPGClassAsset.h"
#include "RPG/RPGClassProgressionTransactionService.h"
#include "RPG/RPGTalentRuntimeService.h"
#include "Runtime/GridPartyInventoryComponent.h"

namespace RPGMON2072TalentRuntimeReadModelTests
{
    const FName TalentA = TEXT ("Talent_A");
    const FName TalentB = TEXT ("Talent_B");
    const FName TalentC = TEXT ("Talent_C");
    const FName TalentExpensive = TEXT ("Talent_Expensive");

    struct FRuntimeStateGuard
    {
        FRuntimeStateGuard ()
        {
            FRPGClassProgressionTransactionService::ResetRuntimeState ();
        }

        ~FRuntimeStateGuard ()
        {
            FRPGClassProgressionTransactionService::ResetRuntimeState ();
        }
    };

    URPGClassAsset* MakeTalentClass (UObject* Outer)
    {
        URPGClassAsset* ClassDefinition = NewObject<URPGClassAsset> (Outer);
        ClassDefinition->ClassId = TEXT ("MON2072_Fighter");
        ClassDefinition->DisplayName = FText::FromString (TEXT ("MON20.7 Fighter"));
        ClassDefinition->HealthAtLevelOne = 20;

        FRPGClassProgressionLevelGrant Level2;
        Level2.Level = 2;
        Level2.ChoicePointsGranted = 1;
        ClassDefinition->ProgressionLevelGrants.Add (Level2);

        FRPGClassProgressionLevelGrant Level3;
        Level3.Level = 3;
        Level3.ChoicePointsGranted = 1;
        ClassDefinition->ProgressionLevelGrants.Add (Level3);

        FRPGClassProgressionLevelGrant Level4;
        Level4.Level = 4;
        Level4.ChoicePointsGranted = 2;
        ClassDefinition->ProgressionLevelGrants.Add (Level4);

        FRPGClassProgressionChoiceDefinition ChoiceA;
        ChoiceA.ChoiceId = TalentA;
        ChoiceA.DisplayName = FText::FromString (TEXT ("Talent A"));
        ChoiceA.Description = FText::FromString (TEXT ("Premier talent."));
        ChoiceA.MinimumLevel = 2;
        ChoiceA.PointCost = 1;
        ClassDefinition->ProgressionChoices.Add (ChoiceA);

        FRPGClassProgressionChoiceDefinition ChoiceB;
        ChoiceB.ChoiceId = TalentB;
        ChoiceB.DisplayName = FText::FromString (TEXT ("Talent B"));
        ChoiceB.Description = FText::FromString (TEXT ("Talent dépendant de A."));
        ChoiceB.MinimumLevel = 3;
        ChoiceB.PointCost = 1;
        ChoiceB.PrerequisiteChoiceIds.Add (TalentA);
        ClassDefinition->ProgressionChoices.Add (ChoiceB);

        FRPGClassProgressionChoiceDefinition ChoiceC;
        ChoiceC.ChoiceId = TalentC;
        ChoiceC.DisplayName = FText::FromString (TEXT ("Talent C"));
        ChoiceC.MinimumLevel = 4;
        ChoiceC.PointCost = 2;
        ChoiceC.PrerequisiteChoiceIds.Add (TalentB);
        ClassDefinition->ProgressionChoices.Add (ChoiceC);

        FRPGClassProgressionChoiceDefinition Expensive;
        Expensive.ChoiceId = TalentExpensive;
        Expensive.DisplayName = FText::FromString (TEXT ("Talent coûteux"));
        Expensive.MinimumLevel = 2;
        Expensive.PointCost = 3;
        ClassDefinition->ProgressionChoices.Add (Expensive);

        return ClassDefinition;
    }

    FGridCharacterInventoryState MakeCharacter (
        URPGClassAsset* ClassDefinition,
        int32 Level,
        const TCHAR* Name)
    {
        FGridCharacterInventoryState Character;
        Character.CharacterId = FGuid::NewGuid ();
        Character.DisplayName = FText::FromString (Name);
        Character.ClassId = ClassDefinition->ClassId;
        Character.ClassDisplayName = ClassDefinition->DisplayName;
        Character.ClassDefinition = ClassDefinition;
        Character.Level = Level;
        return Character;
    }

    UGridPartyInventoryComponent* MakeParty (
        int32 Level,
        URPGClassAsset*& OutClassDefinition)
    {
        UGridPartyInventoryComponent* Party =
            NewObject<UGridPartyInventoryComponent> ();
        Party->PartyInventoryState = FGridPartyInventoryState ();
        OutClassDefinition = MakeTalentClass (Party);
        Party->PartyInventoryState.ActiveCharacters.Add (
            MakeCharacter (OutClassDefinition, Level, TEXT ("A")));
        Party->PartyInventoryState.ActiveCharacters.Add (
            MakeCharacter (OutClassDefinition, Level, TEXT ("B")));
        Party->PartyInventoryState.ActiveEquipment.SetNum (2);
        Party->PartyInventoryState.SelectedCharacterIndex = 0;
        return Party;
    }

    bool CommitTalents (
        UGridPartyInventoryComponent* Party,
        int32 CharacterIndex,
        std::initializer_list<FName> ChoiceIds)
    {
        TArray<FName> Requested;
        for (const FName ChoiceId : ChoiceIds)
        {
            Requested.Add (ChoiceId);
        }

        FRPGClassProgressionCommitResult Result;
        return FRPGClassProgressionTransactionService::TryCommitChoices (
            Party,
            CharacterIndex,
            Requested,
            Result);
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON2072ExplicitSelectedTalentsTest,
    "Grimrock.MON20.7.Talents.ExplicitSelectedTalents",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2072ExplicitSelectedTalentsTest::RunTest (const FString& Parameters)
{
    (void)Parameters;
    using namespace RPGMON2072TalentRuntimeReadModelTests;
    FRuntimeStateGuard Guard;

    URPGClassAsset* ClassDefinition = nullptr;
    UGridPartyInventoryComponent* Party = MakeParty (3, ClassDefinition);
    TestTrue (TEXT ("Talent A commits"), CommitTalents (Party, 0, { TalentA }));

    TArray<FRPGTalentRuntimeView> Talents;
    TestTrue (
        TEXT ("Explicit selected talents are readable"),
        FRPGTalentRuntimeService::TryGetSelectedTalents (Party, 0, Talents));
    TestEqual (TEXT ("Exactly one talent is selected"), Talents.Num (), 1);
    if (Talents.Num () == 1)
    {
        TestEqual (TEXT ("Selected talent identity is preserved"), Talents[0].ChoiceId, TalentA);
        TestTrue (TEXT ("Selected talent view is marked selected"), Talents[0].bSelected);
        TestTrue (
            TEXT ("Selected talent reports AlreadySelected through MON15 rules"),
            Talents[0].AvailabilityReason ==
                ERPGClassProgressionChoiceAvailabilityReason::AlreadySelected);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON2072SelectedCharacterAuthorityTest,
    "Grimrock.MON20.7.Talents.SelectedCharacterAuthority",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2072SelectedCharacterAuthorityTest::RunTest (const FString& Parameters)
{
    (void)Parameters;
    using namespace RPGMON2072TalentRuntimeReadModelTests;
    FRuntimeStateGuard Guard;

    URPGClassAsset* ClassDefinition = nullptr;
    UGridPartyInventoryComponent* Party = MakeParty (3, ClassDefinition);
    TestTrue (TEXT ("Character 1 selection succeeds"), Party->SetSelectedCharacterIndex (1));
    TestTrue (
        TEXT ("Talent A+B commit for selected character"),
        CommitTalents (Party, 1, { TalentA, TalentB }));

    TArray<FRPGTalentRuntimeView> SelectedCharacterTalents;
    TestTrue (
        TEXT ("Selected-character talent query succeeds"),
        FRPGTalentRuntimeService::TryGetSelectedCharacterTalents (
            Party,
            SelectedCharacterTalents));
    TestEqual (
        TEXT ("Selected character owns both committed talents"),
        SelectedCharacterTalents.Num (),
        2);

    TArray<FRPGTalentRuntimeView> CharacterZeroTalents;
    TestTrue (
        TEXT ("Explicit character zero query succeeds"),
        FRPGTalentRuntimeService::TryGetSelectedTalents (
            Party,
            0,
            CharacterZeroTalents));
    TestEqual (
        TEXT ("Character zero remains independent"),
        CharacterZeroTalents.Num (),
        0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON2072HasTalentTest,
    "Grimrock.MON20.7.Talents.HasTalent",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2072HasTalentTest::RunTest (const FString& Parameters)
{
    (void)Parameters;
    using namespace RPGMON2072TalentRuntimeReadModelTests;
    FRuntimeStateGuard Guard;

    URPGClassAsset* ClassDefinition = nullptr;
    UGridPartyInventoryComponent* Party = MakeParty (3, ClassDefinition);
    TestTrue (TEXT ("Talent A commits"), CommitTalents (Party, 0, { TalentA }));

    bool bHasTalent = false;
    TestTrue (
        TEXT ("Known acquired talent query succeeds"),
        FRPGTalentRuntimeService::HasTalent (Party, 0, TalentA, bHasTalent));
    TestTrue (TEXT ("Acquired talent is reported"), bHasTalent);

    bHasTalent = true;
    TestTrue (
        TEXT ("Known unacquired talent query succeeds"),
        FRPGTalentRuntimeService::HasTalent (Party, 0, TalentB, bHasTalent));
    TestFalse (TEXT ("Unacquired talent is not reported"), bHasTalent);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON2072TalentPointBalanceTest,
    "Grimrock.MON20.7.Talents.PointBalance",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2072TalentPointBalanceTest::RunTest (const FString& Parameters)
{
    (void)Parameters;
    using namespace RPGMON2072TalentRuntimeReadModelTests;
    FRuntimeStateGuard Guard;

    URPGClassAsset* ClassDefinition = nullptr;
    UGridPartyInventoryComponent* Party = MakeParty (3, ClassDefinition);
    TestTrue (TEXT ("Talent A commits"), CommitTalents (Party, 0, { TalentA }));

    FRPGTalentPointBalance Balance;
    TestTrue (
        TEXT ("Talent point balance query succeeds"),
        FRPGTalentRuntimeService::TryGetTalentPointBalance (Party, 0, Balance));
    TestEqual (TEXT ("MON15 grants two choice points at level 3"), Balance.GrantedPoints, 2);
    TestEqual (TEXT ("Talent A spends one point"), Balance.SpentPoints, 1);
    TestEqual (TEXT ("One point remains"), Balance.RemainingPoints, 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON2072AvailableBeforeSelectionTest,
    "Grimrock.MON20.7.Talents.AvailableBeforeSelection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2072AvailableBeforeSelectionTest::RunTest (const FString& Parameters)
{
    (void)Parameters;
    using namespace RPGMON2072TalentRuntimeReadModelTests;
    FRuntimeStateGuard Guard;

    URPGClassAsset* ClassDefinition = nullptr;
    UGridPartyInventoryComponent* Party = MakeParty (3, ClassDefinition);

    TArray<FRPGTalentRuntimeView> Available;
    TestTrue (
        TEXT ("Available talent query succeeds"),
        FRPGTalentRuntimeService::TryGetAvailableTalents (Party, 0, Available));
    TestEqual (TEXT ("Only Talent A is currently available"), Available.Num (), 1);
    if (Available.Num () == 1)
    {
        TestEqual (TEXT ("Talent A is exposed"), Available[0].ChoiceId, TalentA);
        TestFalse (TEXT ("Available talent is not selected"), Available[0].bSelected);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON2072AvailableAfterPrerequisiteTest,
    "Grimrock.MON20.7.Talents.AvailableAfterPrerequisite",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2072AvailableAfterPrerequisiteTest::RunTest (const FString& Parameters)
{
    (void)Parameters;
    using namespace RPGMON2072TalentRuntimeReadModelTests;
    FRuntimeStateGuard Guard;

    URPGClassAsset* ClassDefinition = nullptr;
    UGridPartyInventoryComponent* Party = MakeParty (3, ClassDefinition);
    TestTrue (TEXT ("Talent A commits"), CommitTalents (Party, 0, { TalentA }));

    TArray<FRPGTalentRuntimeView> Available;
    TestTrue (
        TEXT ("Available talent query succeeds after prerequisite"),
        FRPGTalentRuntimeService::TryGetAvailableTalents (Party, 0, Available));
    TestEqual (TEXT ("Only Talent B becomes available"), Available.Num (), 1);
    if (Available.Num () == 1)
    {
        TestEqual (TEXT ("Talent B is exposed"), Available[0].ChoiceId, TalentB);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON2072SelectedCharacterFacadeTest,
    "Grimrock.MON20.7.Talents.SelectedCharacterFacade",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2072SelectedCharacterFacadeTest::RunTest (const FString& Parameters)
{
    (void)Parameters;
    using namespace RPGMON2072TalentRuntimeReadModelTests;
    FRuntimeStateGuard Guard;

    URPGClassAsset* ClassDefinition = nullptr;
    UGridPartyInventoryComponent* Party = MakeParty (3, ClassDefinition);
    TestTrue (TEXT ("Character 1 selection succeeds"), Party->SetSelectedCharacterIndex (1));
    TestTrue (TEXT ("Talent A commits on character 1"), CommitTalents (Party, 1, { TalentA }));

    bool bHasTalent = false;
    TestTrue (
        TEXT ("Selected-character HasTalent succeeds"),
        FRPGTalentRuntimeService::HasSelectedCharacterTalent (
            Party,
            TalentA,
            bHasTalent));
    TestTrue (TEXT ("Selected character owns Talent A"), bHasTalent);

    FRPGTalentPointBalance Balance;
    TestTrue (
        TEXT ("Selected-character balance succeeds"),
        FRPGTalentRuntimeService::TryGetSelectedCharacterTalentPointBalance (
            Party,
            Balance));
    TestEqual (TEXT ("Selected character has one point remaining"), Balance.RemainingPoints, 1);

    TArray<FRPGTalentRuntimeView> Available;
    TestTrue (
        TEXT ("Selected-character available talents query succeeds"),
        FRPGTalentRuntimeService::TryGetSelectedCharacterAvailableTalents (
            Party,
            Available));
    TestEqual (TEXT ("Selected character can now select Talent B"), Available.Num (), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON2072InvalidIndexNoMutationTest,
    "Grimrock.MON20.7.Talents.InvalidIndexNoMutation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2072InvalidIndexNoMutationTest::RunTest (const FString& Parameters)
{
    (void)Parameters;
    using namespace RPGMON2072TalentRuntimeReadModelTests;
    FRuntimeStateGuard Guard;

    URPGClassAsset* ClassDefinition = nullptr;
    UGridPartyInventoryComponent* Party = MakeParty (3, ClassDefinition);
    TestTrue (TEXT ("Talent A commits"), CommitTalents (Party, 0, { TalentA }));

    TArray<FName> Before;
    TestTrue (
        TEXT ("Authoritative MON15 selection is readable before invalid calls"),
        FRPGClassProgressionTransactionService::TryGetSelectedChoiceIds (
            Party,
            0,
            Before));

    TArray<FRPGTalentRuntimeView> Talents;
    Talents.Add (FRPGTalentRuntimeView ());
    TestFalse (
        TEXT ("Invalid explicit index is rejected"),
        FRPGTalentRuntimeService::TryGetSelectedTalents (Party, 99, Talents));
    TestEqual (TEXT ("Failed list query clears output"), Talents.Num (), 0);

    FRPGTalentPointBalance Balance;
    Balance.GrantedPoints = 99;
    Balance.SpentPoints = 99;
    Balance.RemainingPoints = 99;
    TestFalse (
        TEXT ("Invalid balance index is rejected"),
        FRPGTalentRuntimeService::TryGetTalentPointBalance (Party, 99, Balance));
    TestEqual (TEXT ("Failed balance query resets granted points"), Balance.GrantedPoints, 0);
    TestEqual (TEXT ("Failed balance query resets spent points"), Balance.SpentPoints, 0);
    TestEqual (TEXT ("Failed balance query resets remaining points"), Balance.RemainingPoints, 0);

    bool bHasTalent = true;
    TestFalse (
        TEXT ("Invalid HasTalent index is rejected"),
        FRPGTalentRuntimeService::HasTalent (Party, 99, TalentA, bHasTalent));
    TestFalse (TEXT ("Failed HasTalent resets output"), bHasTalent);

    Party->PartyInventoryState.SelectedCharacterIndex = 99;
    TestFalse (
        TEXT ("Invalid selected character is rejected"),
        FRPGTalentRuntimeService::TryGetSelectedCharacterTalents (
            Party,
            Talents));

    TArray<FName> After;
    TestTrue (
        TEXT ("Authoritative MON15 selection remains readable"),
        FRPGClassProgressionTransactionService::TryGetSelectedChoiceIds (
            Party,
            0,
            After));
    TestEqual (TEXT ("Invalid read calls do not mutate the selection"), After, Before);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
