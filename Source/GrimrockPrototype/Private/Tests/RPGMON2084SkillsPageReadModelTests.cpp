#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "RPG/RPGClassAsset.h"
#include "RPG/RPGClassProgressionTransactionService.h"
#include "RPG/RPGSkillAsset.h"
#include "RPG/RPGSkillService.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "UI/GridSkillsPageService.h"

namespace RPGMON2084SkillsPageReadModelTests
{
    const FName TalentA = TEXT ("Talent_A");

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

    URPGClassAsset* MakeClass (UObject* Outer)
    {
        URPGClassAsset* ClassDefinition = NewObject<URPGClassAsset> (Outer);
        ClassDefinition->ClassId = TEXT ("MON2084_Rogue");
        ClassDefinition->DisplayName = FText::FromString (TEXT ("Voleur"));
        ClassDefinition->HealthAtLevelOne = 16;

        FRPGClassProgressionLevelGrant Level2;
        Level2.Level = 2;
        Level2.ChoicePointsGranted = 1;
        ClassDefinition->ProgressionLevelGrants.Add (Level2);

        FRPGClassProgressionLevelGrant Level3;
        Level3.Level = 3;
        Level3.ChoicePointsGranted = 1;
        ClassDefinition->ProgressionLevelGrants.Add (Level3);

        FRPGClassProgressionChoiceDefinition Choice;
        Choice.ChoiceId = TalentA;
        Choice.DisplayName = FText::FromString (TEXT ("Doigts agiles"));
        Choice.Description = FText::FromString (TEXT ("Talent de test."));
        Choice.MinimumLevel = 2;
        Choice.PointCost = 1;
        ClassDefinition->ProgressionChoices.Add (Choice);
        return ClassDefinition;
    }

    FGridCharacterInventoryState MakeCharacter (
        URPGClassAsset* ClassDefinition,
        const TCHAR* Name)
    {
        FGridCharacterInventoryState Character;
        Character.CharacterId = FGuid::NewGuid ();
        Character.DisplayName = FText::FromString (Name);
        Character.ClassId = ClassDefinition->ClassId;
        Character.ClassDisplayName = ClassDefinition->DisplayName;
        Character.ClassDefinition = ClassDefinition;
        Character.Level = 3;
        return Character;
    }

    UGridPartyInventoryComponent* MakeParty (
        URPGClassAsset*& OutClassDefinition)
    {
        UGridPartyInventoryComponent* Party =
            NewObject<UGridPartyInventoryComponent> ();
        Party->PartyInventoryState = FGridPartyInventoryState ();
        OutClassDefinition = MakeClass (Party);
        Party->PartyInventoryState.ActiveCharacters.Add (
            MakeCharacter (OutClassDefinition, TEXT ("A")));
        Party->PartyInventoryState.ActiveCharacters.Add (
            MakeCharacter (OutClassDefinition, TEXT ("B")));
        Party->PartyInventoryState.ActiveEquipment.SetNum (2);
        Party->PartyInventoryState.SelectedCharacterIndex = 0;
        return Party;
    }

    URPGSkillAsset* MakeSkill (
        UObject* Outer,
        FName SkillId,
        const TCHAR* DisplayName)
    {
        URPGSkillAsset* Skill = NewObject<URPGSkillAsset> (Outer);
        Skill->SkillId = SkillId;
        Skill->DisplayName = FText::FromString (DisplayName);
        Skill->Description = FText::FromString (TEXT ("Compétence de test."));
        Skill->GoverningAttribute = ERPGSkillGoverningAttribute::Dexterity;
        Skill->MaxRank = 5;
        Skill->bAllowUntrainedChecks = true;
        return Skill;
    }

    bool SetRank (
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

    bool CommitTalent (
        UGridPartyInventoryComponent* Party,
        int32 CharacterIndex)
    {
        FRPGClassProgressionCommitResult Result;
        return FRPGClassProgressionTransactionService::TryCommitChoices (
            Party,
            CharacterIndex,
            { TalentA },
            Result);
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON2084SelectedCharacterIdentityTest,
    "Grimrock.MON20.8.SkillsPage.SelectedCharacterIdentity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2084SelectedCharacterIdentityTest::RunTest (const FString& Parameters)
{
    (void)Parameters;
    using namespace RPGMON2084SkillsPageReadModelTests;
    FRuntimeStateGuard Guard;
    URPGClassAsset* ClassDefinition = nullptr;
    UGridPartyInventoryComponent* Party = MakeParty (ClassDefinition);
    URPGSkillAsset* Skill = MakeSkill (Party, TEXT ("Skill_Lockpicking"), TEXT ("Crochetage"));

    FGridSkillsPageView View;
    TestTrue (TEXT ("Selected view builds"),
        FGridSkillsPageService::TryBuildSelectedCharacterView (
            Party, { Skill }, View));
    TestEqual (TEXT ("Selected index is authoritative"), View.CharacterIndex, 0);
    TestTrue (TEXT ("Selected CharacterId is preserved"),
        View.CharacterId == Party->PartyInventoryState.ActiveCharacters[0].CharacterId);
    TestTrue (TEXT ("Selected name is preserved"),
        View.CharacterName.EqualTo (FText::FromString (TEXT ("A"))));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON2084SkillRanksTest,
    "Grimrock.MON20.8.SkillsPage.SkillRanks",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2084SkillRanksTest::RunTest (const FString& Parameters)
{
    (void)Parameters;
    using namespace RPGMON2084SkillsPageReadModelTests;
    FRuntimeStateGuard Guard;
    URPGClassAsset* ClassDefinition = nullptr;
    UGridPartyInventoryComponent* Party = MakeParty (ClassDefinition);
    URPGSkillAsset* Lockpick = MakeSkill (Party, TEXT ("Skill_Lockpicking"), TEXT ("Crochetage"));
    URPGSkillAsset* Alchemy = MakeSkill (Party, TEXT ("Skill_Alchemy"), TEXT ("Alchimie"));
    TestTrue (TEXT ("Trained rank setup succeeds"),
        SetRank (Party->PartyInventoryState.ActiveCharacters[0], Lockpick, 3));

    FGridSkillsPageView View;
    TestTrue (TEXT ("View builds with trained and untrained Skills"),
        FGridSkillsPageService::TryBuildCharacterView (
            Party, 0, { Lockpick, Alchemy }, View));
    TestEqual (TEXT ("Both canonical Skills are visible"), View.Skills.Num (), 2);
    const FGridSkillEntryView* LockpickView = View.Skills.FindByPredicate (
        [] (const FGridSkillEntryView& Entry)
        {
            return Entry.SkillId == TEXT ("Skill_Lockpicking");
        });
    const FGridSkillEntryView* AlchemyView = View.Skills.FindByPredicate (
        [] (const FGridSkillEntryView& Entry)
        {
            return Entry.SkillId == TEXT ("Skill_Alchemy");
        });
    TestNotNull (TEXT ("Lockpicking view exists"), LockpickView);
    TestNotNull (TEXT ("Alchemy view exists"), AlchemyView);
    if (LockpickView)
    {
        TestEqual (TEXT ("Trained rank is projected"), LockpickView->Rank, 3);
        TestTrue (TEXT ("Trained flag is projected"), LockpickView->bTrained);
    }
    if (AlchemyView)
    {
        TestEqual (TEXT ("Untrained rank is zero"), AlchemyView->Rank, 0);
        TestFalse (TEXT ("Untrained flag remains false"), AlchemyView->bTrained);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON2084SkillOrderTest,
    "Grimrock.MON20.8.SkillsPage.DeterministicSkillOrder",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2084SkillOrderTest::RunTest (const FString& Parameters)
{
    (void)Parameters;
    using namespace RPGMON2084SkillsPageReadModelTests;
    FRuntimeStateGuard Guard;
    URPGClassAsset* ClassDefinition = nullptr;
    UGridPartyInventoryComponent* Party = MakeParty (ClassDefinition);
    URPGSkillAsset* Zeta = MakeSkill (Party, TEXT ("Skill_Zeta"), TEXT ("Zeta"));
    URPGSkillAsset* Alpha = MakeSkill (Party, TEXT ("Skill_Alpha"), TEXT ("Alpha"));

    FGridSkillsPageView View;
    TestTrue (TEXT ("Reverse input builds"),
        FGridSkillsPageService::TryBuildCharacterView (
            Party, 0, { Zeta, Alpha }, View));
    TestEqual (TEXT ("Two Skills are projected"), View.Skills.Num (), 2);
    if (View.Skills.Num () == 2)
    {
        TestEqual (TEXT ("Alpha sorts first"), View.Skills[0].SkillId, FName (TEXT ("Skill_Alpha")));
        TestEqual (TEXT ("Zeta sorts second"), View.Skills[1].SkillId, FName (TEXT ("Skill_Zeta")));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON2084TalentProjectionTest,
    "Grimrock.MON20.8.SkillsPage.TalentProjection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2084TalentProjectionTest::RunTest (const FString& Parameters)
{
    (void)Parameters;
    using namespace RPGMON2084SkillsPageReadModelTests;
    FRuntimeStateGuard Guard;
    URPGClassAsset* ClassDefinition = nullptr;
    UGridPartyInventoryComponent* Party = MakeParty (ClassDefinition);
    URPGSkillAsset* Skill = MakeSkill (Party, TEXT ("Skill_Lockpicking"), TEXT ("Crochetage"));
    TestTrue (TEXT ("Talent commit succeeds"), CommitTalent (Party, 0));

    FGridSkillsPageView View;
    TestTrue (TEXT ("View builds with acquired Talent"),
        FGridSkillsPageService::TryBuildCharacterView (
            Party, 0, { Skill }, View));
    TestEqual (TEXT ("One acquired Talent is visible"), View.Talents.Num (), 1);
    if (View.Talents.Num () == 1)
    {
        TestEqual (TEXT ("ChoiceId remains Talent identity"), View.Talents[0].ChoiceId, TalentA);
        TestTrue (TEXT ("Talent remains selected"), View.Talents[0].bSelected);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON2084TalentBalanceTest,
    "Grimrock.MON20.8.SkillsPage.TalentPointBalance",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2084TalentBalanceTest::RunTest (const FString& Parameters)
{
    (void)Parameters;
    using namespace RPGMON2084SkillsPageReadModelTests;
    FRuntimeStateGuard Guard;
    URPGClassAsset* ClassDefinition = nullptr;
    UGridPartyInventoryComponent* Party = MakeParty (ClassDefinition);
    URPGSkillAsset* Skill = MakeSkill (Party, TEXT ("Skill_Lockpicking"), TEXT ("Crochetage"));
    TestTrue (TEXT ("Talent commit succeeds"), CommitTalent (Party, 0));

    FGridSkillsPageView View;
    TestTrue (TEXT ("View builds with Talent balance"),
        FGridSkillsPageService::TryBuildCharacterView (
            Party, 0, { Skill }, View));
    TestEqual (TEXT ("Two Talent points are granted at level 3"), View.GrantedTalentPoints, 2);
    TestEqual (TEXT ("One Talent point is spent"), View.SpentTalentPoints, 1);
    TestEqual (TEXT ("One Talent point remains"), View.RemainingTalentPoints, 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON2084SelectedAuthorityTest,
    "Grimrock.MON20.8.SkillsPage.SelectedCharacterAuthority",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2084SelectedAuthorityTest::RunTest (const FString& Parameters)
{
    (void)Parameters;
    using namespace RPGMON2084SkillsPageReadModelTests;
    FRuntimeStateGuard Guard;
    URPGClassAsset* ClassDefinition = nullptr;
    UGridPartyInventoryComponent* Party = MakeParty (ClassDefinition);
    URPGSkillAsset* Skill = MakeSkill (Party, TEXT ("Skill_Lockpicking"), TEXT ("Crochetage"));
    TestTrue (TEXT ("Character one selection succeeds"), Party->SetSelectedCharacterIndex (1));
    TestTrue (TEXT ("Character one rank setup succeeds"),
        SetRank (Party->PartyInventoryState.ActiveCharacters[1], Skill, 4));

    FGridSkillsPageView View;
    TestTrue (TEXT ("Selected-character facade follows inventory selection"),
        FGridSkillsPageService::TryBuildSelectedCharacterView (
            Party, { Skill }, View));
    TestEqual (TEXT ("Character one is selected"), View.CharacterIndex, 1);
    TestEqual (TEXT ("Selected character rank is projected"), View.Skills[0].Rank, 4);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON2084DuplicateDefinitionAtomicTest,
    "Grimrock.MON20.8.SkillsPage.DuplicateDefinitionAtomic",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2084DuplicateDefinitionAtomicTest::RunTest (const FString& Parameters)
{
    (void)Parameters;
    using namespace RPGMON2084SkillsPageReadModelTests;
    FRuntimeStateGuard Guard;
    URPGClassAsset* ClassDefinition = nullptr;
    UGridPartyInventoryComponent* Party = MakeParty (ClassDefinition);
    URPGSkillAsset* SkillA = MakeSkill (Party, TEXT ("Skill_Duplicate"), TEXT ("A"));
    URPGSkillAsset* SkillB = MakeSkill (Party, TEXT ("Skill_Duplicate"), TEXT ("B"));

    FGridSkillsPageView View;
    View.CharacterIndex = 99;
    TestFalse (TEXT ("Duplicate canonical SkillId is rejected"),
        FGridSkillsPageService::TryBuildCharacterView (
            Party, 0, { SkillA, SkillB }, View));
    TestFalse (TEXT ("Failed view is reset atomically"), View.IsValid ());
    TestTrue (TEXT ("Failed view exposes no partial Skills"), View.Skills.IsEmpty ());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON2084MissingRankDefinitionAtomicTest,
    "Grimrock.MON20.8.SkillsPage.MissingRankDefinitionAtomic",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2084MissingRankDefinitionAtomicTest::RunTest (const FString& Parameters)
{
    (void)Parameters;
    using namespace RPGMON2084SkillsPageReadModelTests;
    FRuntimeStateGuard Guard;
    URPGClassAsset* ClassDefinition = nullptr;
    UGridPartyInventoryComponent* Party = MakeParty (ClassDefinition);
    URPGSkillAsset* Ranked = MakeSkill (Party, TEXT ("Skill_Ranked"), TEXT ("Ranked"));
    URPGSkillAsset* Other = MakeSkill (Party, TEXT ("Skill_Other"), TEXT ("Other"));
    TestTrue (TEXT ("Rank setup succeeds"),
        SetRank (Party->PartyInventoryState.ActiveCharacters[0], Ranked, 2));
    const TArray<FRPGSkillRank> Before =
        Party->PartyInventoryState.ActiveCharacters[0].SkillRanks;

    FGridSkillsPageView View;
    TestFalse (TEXT ("Missing canonical definition for ranked Skill is rejected"),
        FGridSkillsPageService::TryBuildCharacterView (
            Party, 0, { Other }, View));
    TestFalse (TEXT ("Failed view remains invalid"), View.IsValid ());
    TestEqual (TEXT ("Read model never mutates SkillRanks"),
        Party->PartyInventoryState.ActiveCharacters[0].SkillRanks.Num (),
        Before.Num ());
    if (Before.Num () == 1)
    {
        TestEqual (TEXT ("Original SkillId is preserved"),
            Party->PartyInventoryState.ActiveCharacters[0].SkillRanks[0].SkillId,
            Before[0].SkillId);
        TestEqual (TEXT ("Original rank is preserved"),
            Party->PartyInventoryState.ActiveCharacters[0].SkillRanks[0].Rank,
            Before[0].Rank);
    }
    return true;
}

#endif
