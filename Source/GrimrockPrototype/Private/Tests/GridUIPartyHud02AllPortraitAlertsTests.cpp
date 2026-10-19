#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/Image.h"
#include "UI/GridCombatActionPanelWidget.h"
#include "UI/GridCombatHudWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGridPartyHud02CombatPortraitAlertsTest,
    "Grimrock.UI.PartyHud02.CombatPortraitAlerts",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridPartyHud02CombatPortraitAlertsTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    UGridCombatActionPanelWidget* PartyPanel = NewObject<UGridCombatActionPanelWidget>();
    UGridCombatHudInitiativeSlotWidget* InitiativeSlot = NewObject<UGridCombatHudInitiativeSlotWidget>();
    if (!TestNotNull(TEXT("Combat member panel exists"), PartyPanel) ||
        !TestNotNull(TEXT("Combat initiative slot exists"), InitiativeSlot))
    {
        return false;
    }

    PartyPanel->Image_LevelUpAlert = NewObject<UImage>(PartyPanel);
    PartyPanel->Image_WeightAlert = NewObject<UImage>(PartyPanel);
    InitiativeSlot->Image_LevelUpAlert = NewObject<UImage>(InitiativeSlot);
    InitiativeSlot->Image_WeightAlert = NewObject<UImage>(InitiativeSlot);

    FGridCombatHudPartyMemberView PartyView;
    PartyView.bPresent = true;
    PartyView.bHasUnspentProgressionPoints = true;
    PartyView.bOverloaded = true;
    PartyPanel->SetView(PartyView);
    TestEqual(TEXT("Party combat panel shows progression badge"),
        PartyPanel->Image_LevelUpAlert->GetVisibility(), ESlateVisibility::HitTestInvisible);
    TestEqual(TEXT("Party combat panel shows overload badge"),
        PartyPanel->Image_WeightAlert->GetVisibility(), ESlateVisibility::HitTestInvisible);

    PartyView.bHasUnspentProgressionPoints = false;
    PartyPanel->SetView(PartyView);
    TestEqual(TEXT("Spending points hides combat '+' independently"),
        PartyPanel->Image_LevelUpAlert->GetVisibility(), ESlateVisibility::Collapsed);
    TestEqual(TEXT("Spending points preserves combat overload"),
        PartyPanel->Image_WeightAlert->GetVisibility(), ESlateVisibility::HitTestInvisible);

    PartyView.bPresent = false;
    PartyPanel->SetView(PartyView);
    TestEqual(TEXT("Absent member hides combat weight alert"),
        PartyPanel->Image_WeightAlert->GetVisibility(), ESlateVisibility::Collapsed);

    FGridCombatHudInitiativeView InitiativeView;
    InitiativeView.Combatant.Side = EGridCombatantSide::Party;
    InitiativeView.Combatant.CharacterIndex = 0;
    InitiativeView.bHasUnspentProgressionPoints = true;
    InitiativeView.bOverloaded = true;
    InitiativeSlot->InitializeInitiativeSlot(InitiativeView);
    TestEqual(TEXT("Party initiative slot shows progression badge"),
        InitiativeSlot->Image_LevelUpAlert->GetVisibility(), ESlateVisibility::HitTestInvisible);
    TestEqual(TEXT("Party initiative slot shows overload badge"),
        InitiativeSlot->Image_WeightAlert->GetVisibility(), ESlateVisibility::HitTestInvisible);

    // Initiative previews mix monsters and party members. Monster portraits
    // must not receive the player-only alert overlays, even with stale flags.
    InitiativeView.Combatant.Side = EGridCombatantSide::Monster;
    InitiativeSlot->InitializeInitiativeSlot(InitiativeView);
    TestEqual(TEXT("Monster initiative slot never shows progression badge"),
        InitiativeSlot->Image_LevelUpAlert->GetVisibility(), ESlateVisibility::Collapsed);
    TestEqual(TEXT("Monster initiative slot never shows overload badge"),
        InitiativeSlot->Image_WeightAlert->GetVisibility(), ESlateVisibility::Collapsed);

    InitiativeView.Combatant.Side = EGridCombatantSide::Party;
    InitiativeView.bHasUnspentProgressionPoints = false;
    InitiativeView.bOverloaded = false;
    InitiativeSlot->InitializeInitiativeSlot(InitiativeView);
    TestEqual(TEXT("Spent party initiative points hide '+'"),
        InitiativeSlot->Image_LevelUpAlert->GetVisibility(), ESlateVisibility::Collapsed);
    TestEqual(TEXT("Relieved party initiative member hides weight alert"),
        InitiativeSlot->Image_WeightAlert->GetVisibility(), ESlateVisibility::Collapsed);
    return true;
}

#endif
