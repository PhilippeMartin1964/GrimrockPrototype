#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "RPG/RPGTalentRuntimeService.h"
#include "RPGMON155TestHelpers.h"
#include "Save/GrimrockPartySaveGame.h"

namespace RPGMON2074TalentPersistenceRegressionTests
{
    const FName TalentA = TEXT ("Choice_A");
    const FName TalentB = TEXT ("Choice_B");
    const FName TalentARequirement = TEXT ("Feature_A");
    const FName AutomaticLevelRequirement = TEXT ("Feature_Level2");

    bool CommitTalentA (
        UGridPartyInventoryComponent* Component,
        int32 CharacterIndex)
    {
        FRPGClassProgressionCommitResult Result;
        return FRPGClassProgressionTransactionService::TryCommitChoices (
            Component,
            CharacterIndex,
            { TalentA },
            Result);
    }

    bool CaptureProgression (
        UGridPartyInventoryComponent* Component,
        TArray<FRPGCharacterProgressionSaveState>& OutStates)
    {
        if (!Component)
        {
            return false;
        }

        FText Error;
        return FRPGClassProgressionTransactionService::CapturePersistentState (
            Component->PartyInventoryState,
            OutStates,
            Error);
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON2074RequirementBeforeTalentTest,
    "Grimrock.MON20.7.Talents.RequirementBeforeTalent",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2074RequirementBeforeTalentTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    using namespace RPGMON2074TalentPersistenceRegressionTests;
    FMON155RuntimeStateGuard RuntimeGuard;

    URPGClassAsset* ClassDefinition = nullptr;
    UGridPartyInventoryComponent* Component =
        MakeMON155Inventory (3, 3000, ClassDefinition);

    TestTrue (
        TEXT ("Initial MON15 projection refresh succeeds"),
        FRPGClassProgressionTransactionService::RefreshCharacterProjection (
            Component,
            0));

    const TSet<FName> Requirements =
        GetMON155RuntimeRequirements (Component, 0);
    TestTrue (
        TEXT ("Automatic level requirement is projected before talents"),
        Requirements.Contains (AutomaticLevelRequirement));
    TestFalse (
        TEXT ("Talent ChoiceId is absent before acquisition"),
        Requirements.Contains (TalentA));
    TestFalse (
        TEXT ("Talent granted requirement is absent before acquisition"),
        Requirements.Contains (TalentARequirement));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON2074RequirementAfterTalentTest,
    "Grimrock.MON20.7.Talents.RequirementAfterTalent",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2074RequirementAfterTalentTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    using namespace RPGMON2074TalentPersistenceRegressionTests;
    FMON155RuntimeStateGuard RuntimeGuard;

    URPGClassAsset* ClassDefinition = nullptr;
    UGridPartyInventoryComponent* Component =
        MakeMON155Inventory (3, 3000, ClassDefinition);

    TestTrue (TEXT ("Talent A commits"), CommitTalentA (Component, 0));

    const TSet<FName> Requirements =
        GetMON155RuntimeRequirements (Component, 0);
    TestTrue (
        TEXT ("Automatic level requirement remains projected"),
        Requirements.Contains (AutomaticLevelRequirement));
    TestTrue (
        TEXT ("Acquired ChoiceId is projected as a requirement"),
        Requirements.Contains (TalentA));
    TestTrue (
        TEXT ("GrantedRequirementIds are projected immediately"),
        Requirements.Contains (TalentARequirement));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON2074RequirementCharacterIsolationTest,
    "Grimrock.MON20.7.Talents.RequirementCharacterIsolation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2074RequirementCharacterIsolationTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    using namespace RPGMON2074TalentPersistenceRegressionTests;
    FMON155RuntimeStateGuard RuntimeGuard;

    URPGClassAsset* ClassDefinition = nullptr;
    UGridPartyInventoryComponent* Component =
        MakeMON155Inventory (3, 3000, ClassDefinition);
    Component->PartyInventoryState.ActiveCharacters.Add (
        MakeMON155Character (
            ClassDefinition,
            3,
            3000,
            TEXT ("Mina")));
    Component->PartyInventoryState.ActiveEquipment.SetNum (2);

    TestTrue (
        TEXT ("Talent A commits only for character zero"),
        CommitTalentA (Component, 0));
    TestTrue (
        TEXT ("Character one projection refresh succeeds"),
        FRPGClassProgressionTransactionService::RefreshCharacterProjection (
            Component,
            1));

    const TSet<FName> CharacterZeroRequirements =
        GetMON155RuntimeRequirements (Component, 0);
    const TSet<FName> CharacterOneRequirements =
        GetMON155RuntimeRequirements (Component, 1);

    TestTrue (
        TEXT ("Character zero owns Talent A projection"),
        CharacterZeroRequirements.Contains (TalentA));
    TestTrue (
        TEXT ("Character zero owns Feature A projection"),
        CharacterZeroRequirements.Contains (TalentARequirement));
    TestFalse (
        TEXT ("Character one does not inherit Talent A"),
        CharacterOneRequirements.Contains (TalentA));
    TestFalse (
        TEXT ("Character one does not inherit Feature A"),
        CharacterOneRequirements.Contains (TalentARequirement));
    TestTrue (
        TEXT ("Character one still owns automatic level requirement"),
        CharacterOneRequirements.Contains (AutomaticLevelRequirement));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON2074CaptureUsesMON15SnapshotTest,
    "Grimrock.MON20.7.Talents.CaptureUsesMON15Snapshot",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2074CaptureUsesMON15SnapshotTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    using namespace RPGMON2074TalentPersistenceRegressionTests;
    FMON155RuntimeStateGuard RuntimeGuard;

    URPGClassAsset* ClassDefinition = nullptr;
    UGridPartyInventoryComponent* Component =
        MakeMON155Inventory (3, 3000, ClassDefinition);
    TestTrue (TEXT ("Talent A commits"), CommitTalentA (Component, 0));

    TArray<FRPGCharacterProgressionSaveState> SavedStates;
    TestTrue (
        TEXT ("MON15.6 progression snapshot captures"),
        CaptureProgression (Component, SavedStates));
    TestEqual (
        TEXT ("One active character produces one progression snapshot"),
        SavedStates.Num (),
        1);
    if (SavedStates.Num () == 1)
    {
        TestEqual (
            TEXT ("Snapshot keeps stable CharacterId"),
            SavedStates[0].CharacterId,
            Component->PartyInventoryState.ActiveCharacters[0].CharacterId);
        TestEqual (
            TEXT ("Exactly one talent ChoiceId is persisted"),
            SavedStates[0].SelectedChoiceIds.Num (),
            1);
        TestTrue (
            TEXT ("Persisted talent uses the original ChoiceId"),
            SavedStates[0].SelectedChoiceIds.Contains (TalentA));
    }
    TestEqual (
        TEXT ("MON20.7 does not bump SaveGame version"),
        UGrimrockPartySaveGame::CurrentSaveVersion,
        7);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON2074RestoreTalentReadModelTest,
    "Grimrock.MON20.7.Talents.RestoreTalentReadModel",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2074RestoreTalentReadModelTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    using namespace RPGMON2074TalentPersistenceRegressionTests;
    FMON155RuntimeStateGuard RuntimeGuard;

    URPGClassAsset* ClassDefinition = nullptr;
    UGridPartyInventoryComponent* Component =
        MakeMON155Inventory (3, 3000, ClassDefinition);
    TestTrue (TEXT ("Talent A commits"), CommitTalentA (Component, 0));

    TArray<FRPGCharacterProgressionSaveState> SavedStates;
    TestTrue (
        TEXT ("Progression snapshot captures before reset"),
        CaptureProgression (Component, SavedStates));

    FRPGClassProgressionTransactionService::ResetRuntimeState ();
    FText RestoreError;
    TestTrue (
        TEXT ("MON15.6 progression snapshot restores"),
        FRPGClassProgressionTransactionService::RestorePersistentState (
            Component->PartyInventoryState,
            SavedStates,
            RestoreError));

    bool bHasTalent = false;
    TestTrue (
        TEXT ("MON20.7 read facade remains usable after restore"),
        FRPGTalentRuntimeService::HasTalent (
            Component,
            0,
            TalentA,
            bHasTalent));
    TestTrue (
        TEXT ("Restored ChoiceId is still exposed as acquired talent"),
        bHasTalent);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON2074RestoreDetachedRequirementsTest,
    "Grimrock.MON20.7.Talents.RestoreDetachedRequirements",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2074RestoreDetachedRequirementsTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    using namespace RPGMON2074TalentPersistenceRegressionTests;
    FMON155RuntimeStateGuard RuntimeGuard;

    URPGClassAsset* ClassDefinition = nullptr;
    UGridPartyInventoryComponent* Component =
        MakeMON155Inventory (3, 3000, ClassDefinition);
    TestTrue (TEXT ("Talent A commits"), CommitTalentA (Component, 0));

    TArray<FRPGCharacterProgressionSaveState> SavedStates;
    TestTrue (
        TEXT ("Progression snapshot captures before reset"),
        CaptureProgression (Component, SavedStates));

    const FGuid CharacterId =
        Component->PartyInventoryState.ActiveCharacters[0].CharacterId;
    FRPGClassProgressionTransactionService::ResetRuntimeState ();
    FText RestoreError;
    TestTrue (
        TEXT ("Detached progression projection restores"),
        FRPGClassProgressionTransactionService::RestorePersistentState (
            Component->PartyInventoryState,
            SavedStates,
            RestoreError));

    TSet<FName> Requirements;
    FRPGClassProgressionTransactionService::AppendRuntimeSatisfiedRequirements (
        CharacterId,
        Requirements);
    TestTrue (
        TEXT ("Detached restore immediately projects automatic requirement"),
        Requirements.Contains (AutomaticLevelRequirement));
    TestTrue (
        TEXT ("Detached restore immediately projects ChoiceId"),
        Requirements.Contains (TalentA));
    TestTrue (
        TEXT ("Detached restore immediately projects GrantedRequirementIds"),
        Requirements.Contains (TalentARequirement));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON2074InvalidRestoreAtomicTest,
    "Grimrock.MON20.7.Talents.InvalidRestoreAtomic",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2074InvalidRestoreAtomicTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    using namespace RPGMON2074TalentPersistenceRegressionTests;
    FMON155RuntimeStateGuard RuntimeGuard;

    URPGClassAsset* ClassDefinition = nullptr;
    UGridPartyInventoryComponent* Component =
        MakeMON155Inventory (3, 3000, ClassDefinition);
    TestTrue (TEXT ("Talent A commits"), CommitTalentA (Component, 0));

    FRPGCharacterProgressionSaveState InvalidState;
    InvalidState.CharacterId =
        Component->PartyInventoryState.ActiveCharacters[0].CharacterId;
    InvalidState.SelectedChoiceIds = { TalentB };

    FText RestoreError;
    TestFalse (
        TEXT ("Snapshot missing Talent B prerequisite is rejected"),
        FRPGClassProgressionTransactionService::RestorePersistentState (
            Component->PartyInventoryState,
            { InvalidState },
            RestoreError));

    bool bHasTalentA = false;
    TestTrue (
        TEXT ("Existing runtime state remains readable after rejected restore"),
        FRPGTalentRuntimeService::HasTalent (
            Component,
            0,
            TalentA,
            bHasTalentA));
    TestTrue (
        TEXT ("Rejected restore does not erase existing Talent A"),
        bHasTalentA);

    const TSet<FName> Requirements =
        GetMON155RuntimeRequirements (Component, 0);
    TestTrue (
        TEXT ("Rejected restore preserves Talent A requirement projection"),
        Requirements.Contains (TalentA));
    TestTrue (
        TEXT ("Rejected restore preserves Feature A projection"),
        Requirements.Contains (TalentARequirement));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON2074RestoreByCharacterIdTest,
    "Grimrock.MON20.7.Talents.RestoreByCharacterId",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2074RestoreByCharacterIdTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    using namespace RPGMON2074TalentPersistenceRegressionTests;
    FMON155RuntimeStateGuard RuntimeGuard;

    URPGClassAsset* ClassDefinition = nullptr;
    UGridPartyInventoryComponent* Component =
        MakeMON155Inventory (3, 3000, ClassDefinition);
    Component->PartyInventoryState.ActiveCharacters.Add (
        MakeMON155Character (
            ClassDefinition,
            3,
            3000,
            TEXT ("Mina")));
    Component->PartyInventoryState.ActiveEquipment.SetNum (2);

    TestTrue (
        TEXT ("Character zero commits Talent A"),
        CommitTalentA (Component, 0));

    FRPGClassProgressionCommitResult CharacterOneResult;
    TestTrue (
        TEXT ("Character one commits prerequisite chain A+B"),
        FRPGClassProgressionTransactionService::TryCommitChoices (
            Component,
            1,
            { TalentA, TalentB },
            CharacterOneResult));

    TArray<FRPGCharacterProgressionSaveState> SavedStates;
    TestTrue (
        TEXT ("Two-character progression snapshot captures"),
        CaptureProgression (Component, SavedStates));
    TestEqual (
        TEXT ("Two active characters produce two snapshots"),
        SavedStates.Num (),
        2);
    if (SavedStates.Num () != 2)
    {
        return false;
    }

    SavedStates.Swap (0, 1);
    FRPGClassProgressionTransactionService::ResetRuntimeState ();
    FText RestoreError;
    TestTrue (
        TEXT ("Restore is keyed by CharacterId, not snapshot array order"),
        FRPGClassProgressionTransactionService::RestorePersistentState (
            Component->PartyInventoryState,
            SavedStates,
            RestoreError));

    TArray<FName> CharacterZeroTalents;
    TArray<FName> CharacterOneTalents;
    TestTrue (
        TEXT ("Character zero restored selection is readable"),
        FRPGClassProgressionTransactionService::TryGetSelectedChoiceIds (
            Component,
            0,
            CharacterZeroTalents));
    TestTrue (
        TEXT ("Character one restored selection is readable"),
        FRPGClassProgressionTransactionService::TryGetSelectedChoiceIds (
            Component,
            1,
            CharacterOneTalents));

    TestEqual (
        TEXT ("Character zero restores exactly one talent"),
        CharacterZeroTalents.Num (),
        1);
    TestTrue (
        TEXT ("Character zero restores Talent A"),
        CharacterZeroTalents.Contains (TalentA));
    TestEqual (
        TEXT ("Character one restores two talents"),
        CharacterOneTalents.Num (),
        2);
    TestTrue (
        TEXT ("Character one restores Talent A"),
        CharacterOneTalents.Contains (TalentA));
    TestTrue (
        TEXT ("Character one restores Talent B"),
        CharacterOneTalents.Contains (TalentB));
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
