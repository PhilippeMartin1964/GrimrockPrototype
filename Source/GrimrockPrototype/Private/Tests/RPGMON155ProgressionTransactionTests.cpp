#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "RPGMON155TestHelpers.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON155AtomicBatchCommitTest,
    "Grimrock.RPG.MON15.5.AtomicBatchCommit",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON155AtomicBatchCommitTest::RunTest (const FString& Parameters)
{
    (void)Parameters;
    FMON155RuntimeStateGuard RuntimeGuard;
    URPGClassAsset* ClassDefinition = nullptr;
    UGridPartyInventoryComponent* Component =
        MakeMON155Inventory (3, 3000, ClassDefinition);

    FRPGClassProgressionCommitResult Result;
    const TArray<FName> Batch = { TEXT ("Choice_A"), TEXT ("Choice_B") };
    TestTrue (TEXT ("A valid prerequisite chain commits atomically"),
        FRPGClassProgressionTransactionService::TryCommitChoices (
            Component,
            0,
            Batch,
            Result));
    TestTrue (TEXT ("The result is marked committed"), Result.bCommitted);
    TestEqual (TEXT ("Two points are granted at level three"),
        Result.GrantedPoints, 2);
    TestEqual (TEXT ("Two points are spent after the batch"),
        Result.SpentPointsAfter, 2);
    TestEqual (TEXT ("No progression point remains"),
        Result.RemainingPoints, 0);

    TArray<FName> Selected;
    TestTrue (TEXT ("Committed choices can be read"),
        FRPGClassProgressionTransactionService::TryGetSelectedChoiceIds (
            Component, 0, Selected));
    TestEqual (TEXT ("Both choices are committed"), Selected.Num (), 2);
    TestTrue (TEXT ("Choice A is committed"),
        Selected.Contains (TEXT ("Choice_A")));
    TestTrue (TEXT ("Choice B is committed"),
        Selected.Contains (TEXT ("Choice_B")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON155AtomicFailureTest,
    "Grimrock.RPG.MON15.5.AtomicFailure",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON155AtomicFailureTest::RunTest (const FString& Parameters)
{
    (void)Parameters;
    FMON155RuntimeStateGuard RuntimeGuard;
    URPGClassAsset* ClassDefinition = nullptr;
    UGridPartyInventoryComponent* Component =
        MakeMON155Inventory (3, 3000, ClassDefinition);

    const FGuid CharacterId =
        Component->PartyInventoryState.ActiveCharacters[0].CharacterId;
    FRPGClassProgressionCommitResult Result;
    const TArray<FName> InvalidBatch = {
        TEXT ("Choice_A"),
        TEXT ("Choice_Expensive")
    };
    TestFalse (TEXT ("An over-budget batch is rejected"),
        FRPGClassProgressionTransactionService::TryCommitChoices (
            Component,
            0,
            InvalidBatch,
            Result));
    TestTrue (TEXT ("The rejection reports insufficient points"),
        Result.RejectReason ==
            ERPGClassProgressionCommitRejectReason::InsufficientChoicePoints);

    TArray<FName> Selected;
    TestTrue (TEXT ("Runtime progression state remains readable"),
        FRPGClassProgressionTransactionService::TryGetSelectedChoiceIds (
            Component, 0, Selected));
    TestTrue (TEXT ("No partial choice was committed"), Selected.IsEmpty ());

    TSet<FName> Requirements;
    FRPGClassProgressionTransactionService::AppendRuntimeSatisfiedRequirements (
        CharacterId,
        Requirements);
    TestFalse (TEXT ("Rejected Choice A grants no requirement"),
        Requirements.Contains (TEXT ("Choice_A")));
    TestTrue (TEXT ("Automatic level grant remains projected"),
        Requirements.Contains (TEXT ("Feature_Level2")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON155CharacterIsolationTest,
    "Grimrock.RPG.MON15.5.CharacterIsolation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON155CharacterIsolationTest::RunTest (const FString& Parameters)
{
    (void)Parameters;
    FMON155RuntimeStateGuard RuntimeGuard;
    URPGClassAsset* ClassDefinition = nullptr;
    UGridPartyInventoryComponent* Component =
        MakeMON155Inventory (2, 1000, ClassDefinition);
    Component->PartyInventoryState.ActiveCharacters.Add (
        MakeMON155Character (
            ClassDefinition,
            2,
            1000,
            TEXT ("Mina")));
    Component->PartyInventoryState.ActiveEquipment.SetNum (2);

    FRPGClassProgressionCommitResult Result;
    TestTrue (TEXT ("Character zero commits Choice A"),
        FRPGClassProgressionTransactionService::TryCommitChoices (
            Component,
            0,
            { TEXT ("Choice_A") },
            Result));

    TArray<FName> CharacterZeroChoices;
    TArray<FName> CharacterOneChoices;
    TestTrue (TEXT ("Character zero state is readable"),
        FRPGClassProgressionTransactionService::TryGetSelectedChoiceIds (
            Component, 0, CharacterZeroChoices));
    TestTrue (TEXT ("Character one state is readable"),
        FRPGClassProgressionTransactionService::TryGetSelectedChoiceIds (
            Component, 1, CharacterOneChoices));
    TestTrue (TEXT ("Character zero owns Choice A"),
        CharacterZeroChoices.Contains (TEXT ("Choice_A")));
    TestTrue (TEXT ("Character one remains untouched"),
        CharacterOneChoices.IsEmpty ());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON155TransientBoundaryTest,
    "Grimrock.RPG.MON15.5.TransientPersistenceBoundary",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON155TransientBoundaryTest::RunTest (const FString& Parameters)
{
    (void)Parameters;
    FMON155RuntimeStateGuard RuntimeGuard;
    URPGClassAsset* ClassDefinition = nullptr;
    UGridPartyInventoryComponent* Component =
        MakeMON155Inventory (2, 1000, ClassDefinition);

    FRPGClassProgressionCommitResult Result;
    TestTrue (TEXT ("Choice A commits before the transient boundary check"),
        FRPGClassProgressionTransactionService::TryCommitChoices (
            Component,
            0,
            { TEXT ("Choice_A") },
            Result));

    UGrimrockPartySaveGame* Save = NewObject<UGrimrockPartySaveGame> ();
    Save->PartyInventoryState = Component->PartyInventoryState;
    TestEqual (TEXT ("MON15.5 keeps SaveVersion three"),
        UGrimrockPartySaveGame::CurrentSaveVersion, 3);

    FRPGClassProgressionTransactionService::ResetRuntimeState (Component);
    TArray<FName> SelectedAfterRuntimeReset;
    TestTrue (TEXT ("Projection rebuilds after transient runtime reset"),
        FRPGClassProgressionTransactionService::TryGetSelectedChoiceIds (
            Component,
            0,
            SelectedAfterRuntimeReset));
    TestTrue (TEXT ("MON15.5 choices are intentionally not persisted yet"),
        SelectedAfterRuntimeReset.IsEmpty ());
    TestEqual (TEXT ("The ordinary party snapshot is otherwise unchanged"),
        Save->PartyInventoryState.ActiveCharacters.Num (), 1);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS