#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "RPGMON155TestHelpers.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON155WidgetCancelTest,
    "Grimrock.RPG.MON15.5.WidgetCancelIsNonMutating",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON155WidgetCancelTest::RunTest (const FString& Parameters)
{
    (void)Parameters;
    FMON155RuntimeStateGuard RuntimeGuard;
    URPGClassAsset* ClassDefinition = nullptr;
    UGridPartyInventoryComponent* Component =
        MakeMON155Inventory (3, 3000, ClassDefinition);
    URPGLevelUpWidget* Widget = NewObject<URPGLevelUpWidget> (Component);

    TestTrue (TEXT ("The level-up widget initializes"),
        Widget->InitializeLevelUpWidget (Component, 0, 2, 3));
    TestTrue (TEXT ("Choice A can be staged"),
        Widget->StageOrUnstageChoice (TEXT ("Choice_A")));
    TestEqual (TEXT ("Exactly one pending choice is staged"),
        Widget->GetPendingChoiceIds ().Num (), 1);
    Widget->CancelSelection ();

    TArray<FName> Selected;
    TestTrue (TEXT ("Committed state remains readable after cancel"),
        FRPGClassProgressionTransactionService::TryGetSelectedChoiceIds (
            Component, 0, Selected));
    TestTrue (TEXT ("Cancel commits no staged choice"), Selected.IsEmpty ());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON155WidgetConfirmTest,
    "Grimrock.RPG.MON15.5.WidgetConfirmTransaction",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON155WidgetConfirmTest::RunTest (const FString& Parameters)
{
    (void)Parameters;
    FMON155RuntimeStateGuard RuntimeGuard;
    URPGClassAsset* ClassDefinition = nullptr;
    UGridPartyInventoryComponent* Component =
        MakeMON155Inventory (3, 3000, ClassDefinition);
    URPGLevelUpWidget* Widget = NewObject<URPGLevelUpWidget> (Component);

    TestTrue (TEXT ("The level-up widget initializes"),
        Widget->InitializeLevelUpWidget (Component, 0, 1, 3));
    TestEqual (TEXT ("The widget compares from level one"),
        Widget->View.PreviousLevel, 1);
    TestEqual (TEXT ("The widget compares to level three"),
        Widget->View.NewLevel, 3);
    TestTrue (TEXT ("The maximum health comparison increases"),
        Widget->View.NewMaxHealth > Widget->View.PreviousMaxHealth);
    TestTrue (TEXT ("The maximum mana comparison increases"),
        Widget->View.NewMaxMana > Widget->View.PreviousMaxMana);
    TestEqual (TEXT ("The level-three view exposes two points"),
        Widget->View.GrantedChoicePoints, 2);

    TestTrue (TEXT ("Choice A stages"),
        Widget->StageOrUnstageChoice (TEXT ("Choice_A")));
    TestTrue (TEXT ("Choice B can stage after pending prerequisite A"),
        Widget->StageOrUnstageChoice (TEXT ("Choice_B")));
    TestTrue (TEXT ("Confirm commits the whole staged selection"),
        Widget->ConfirmSelection ());

    TArray<FName> Selected;
    TestTrue (TEXT ("Committed widget state can be read"),
        FRPGClassProgressionTransactionService::TryGetSelectedChoiceIds (
            Component, 0, Selected));
    TestEqual (TEXT ("Two widget choices committed"), Selected.Num (), 2);
    TestTrue (TEXT ("Widget committed Choice A"),
        Selected.Contains (TEXT ("Choice_A")));
    TestTrue (TEXT ("Widget committed Choice B"),
        Selected.Contains (TEXT ("Choice_B")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON155CatalogUnlockTest,
    "Grimrock.RPG.MON15.5.CombatCatalogUnlock",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON155CatalogUnlockTest::RunTest (const FString& Parameters)
{
    (void)Parameters;
    FMON155RuntimeStateGuard RuntimeGuard;
    URPGClassAsset* ClassDefinition = nullptr;
    UGridPartyInventoryComponent* Component =
        MakeMON155Inventory (2, 1000, ClassDefinition);
    TestTrue (TEXT ("Initial progression projection is built"),
        FRPGClassProgressionTransactionService::RefreshCharacterProjection (
            Component, 0));

    FGridAvailableCombatAction Available =
        BuildMON155ChoiceActionAvailability (Component, ClassDefinition);
    TestFalse (TEXT ("Choice-gated action starts locked"),
        Available.bEnabled);
    TestTrue (TEXT ("The lock is a missing requirement"),
        Available.AvailabilityReason ==
            EGridCombatActionAvailabilityReason::MissingRequirement);

    FRPGClassProgressionCommitResult Result;
    TestTrue (TEXT ("Choice A commits"),
        FRPGClassProgressionTransactionService::TryCommitChoices (
            Component,
            0,
            { TEXT ("Choice_A") },
            Result));
    Available = BuildMON155ChoiceActionAvailability (
        Component,
        ClassDefinition);
    TestTrue (TEXT ("Committed Choice A unlocks the same catalog action"),
        Available.bEnabled);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON155LevelUpNotificationSourceTest,
    "Grimrock.RPG.MON15.5.LevelUpNotificationSource",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON155LevelUpNotificationSourceTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    FMON155RuntimeStateGuard RuntimeGuard;
    URPGClassAsset* ClassDefinition = nullptr;
    UGridPartyInventoryComponent* Component =
        MakeMON155Inventory (1, 1000, ClassDefinition);

    int32 EventCount = 0;
    UGridPartyInventoryComponent* EventComponent = nullptr;
    int32 EventCharacterIndex = INDEX_NONE;
    int32 EventPreviousLevel = 0;
    int32 EventNewLevel = 0;
    int32 EventLevelsGained = 0;
    const FDelegateHandle Handle =
        FRPGLevelUpService::OnCharacterLevelUpAppliedWithSource ().AddLambda (
            [&] (
                UGridPartyInventoryComponent* SourceComponent,
                int32 CharacterIndex,
                int32 PreviousLevel,
                int32 NewLevel,
                int32 LevelsGained)
            {
                ++EventCount;
                EventComponent = SourceComponent;
                EventCharacterIndex = CharacterIndex;
                EventPreviousLevel = PreviousLevel;
                EventNewLevel = NewLevel;
                EventLevelsGained = LevelsGained;
            });

    TestTrue (TEXT ("MON15.3 level-up still applies"),
        FRPGLevelUpService::ApplyPendingLevelUp (Component, 0, false));
    FRPGLevelUpService::OnCharacterLevelUpAppliedWithSource ().Remove (Handle);

    TestEqual (TEXT ("One source-aware level-up event fires"), EventCount, 1);
    TestTrue (TEXT ("The event identifies the source inventory"),
        EventComponent == Component);
    TestEqual (TEXT ("The event identifies character zero"),
        EventCharacterIndex, 0);
    TestEqual (TEXT ("The event reports previous level one"),
        EventPreviousLevel, 1);
    TestEqual (TEXT ("The event reports new level two"), EventNewLevel, 2);
    TestEqual (TEXT ("The event reports one level gained"),
        EventLevelsGained, 1);

    const TSet<FName> Requirements =
        GetMON155RuntimeRequirements (Component, 0);
    TestTrue (TEXT ("Level-up projects the level-two automatic feature"),
        Requirements.Contains (TEXT ("Feature_Level2")));
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS