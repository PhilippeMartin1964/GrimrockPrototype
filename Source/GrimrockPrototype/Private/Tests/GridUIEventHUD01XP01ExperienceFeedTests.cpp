#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "RPG/RPGExperienceRewardService.h"
#include "Runtime/GridEventFeedPublisher.h"
#include "Runtime/GridEventFeedSubsystem.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "UI/GridEventFeedWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGridUIEventHUD01XP01ExperiencePublication,
    "Grimrock.UI.EventHUD01.XP01.ExperiencePublication",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIEventHUD01XP01ExperiencePublication::RunTest(const FString&)
{
    const UWorld::InitializationValues Values = UWorld::InitializationValues()
        .AllowAudioPlayback(false).RequiresHitProxies(false)
        .CreatePhysicsScene(false).CreateNavigation(false)
        .CreateAISystem(false).ShouldSimulatePhysics(false)
        .SetTransactional(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false,
        FName(*FString::Printf(TEXT("XP01_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
        nullptr, true, ERHIFeatureLevel::Num, &Values);
    if (!TestNotNull(TEXT("XP test world exists"), World)) return false;
    if (GEngine)
    {
        FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
        Context.SetCurrentWorld(World);
    }
    auto Cleanup = [&]()
    {
        World->DestroyWorld(false);
        if (GEngine) GEngine->DestroyWorldContext(World);
    };

    UGridEventFeedSubsystem* Feed = World->GetSubsystem<UGridEventFeedSubsystem>();
    AGrimrockPartyPawn* Pawn = World->SpawnActor<AGrimrockPartyPawn>();
    if (!TestNotNull(TEXT("Event Feed subsystem"), Feed) ||
        !TestNotNull(TEXT("Party pawn"), Pawn) ||
        !TestNotNull(TEXT("Party inventory"), Pawn ? Pawn->PartyInventoryComponent.Get() : nullptr))
    {
        Cleanup();
        return false;
    }
    UGridPartyInventoryComponent* Inventory = Pawn->PartyInventoryComponent;
    Inventory->PartyInventoryState.ActiveCharacters.Reset();
    for (int32 i = 0; i < 4; ++i)
    {
        FGridCharacterInventoryState& Character =
            Inventory->PartyInventoryState.ActiveCharacters.AddDefaulted_GetRef();
        Character.CharacterId = FGuid::NewGuid();
        Character.DisplayName = FText::FromString(FString::Printf(TEXT("Member %d"), i));
        Character.Level = 1;
        Character.Experience = 0;
        Character.Resources.CurrentHealth = i == 3 ? 0 : 10;
    }

    const FGridEventFeedSemanticPalette Palette;
    const FLinearColor ExpectedXP = FLinearColor::FromSRGBColor(FColor(0x72, 0xFF, 0x42));
    TestTrue(TEXT("Experience semantic resolves to bright green #72FF42"),
        UGridEventFeedWidget::ResolveSemanticColor(EGridHudEventSemantic::Experience, Palette)
            .Equals(ExpectedXP, 0.001f));
    TestFalse(TEXT("Experience colour is not poison"),
        Palette.Experience.Equals(Palette.Poison));
    TestFalse(TEXT("Experience colour is not healing"),
        Palette.Experience.Equals(Palette.Healing));

    TestEqual(TEXT("Zero XP is not applied"),
        FRPGExperienceRewardService::AwardToActiveParty(Inventory, 0), 0);
    TestEqual(TEXT("Negative XP is not applied"),
        FRPGExperienceRewardService::AwardToActiveParty(Inventory, -5), 0);
    TestEqual(TEXT("No zero-XP feed entry"), Feed->GetEntries().Num(), 0);
    TestFalse(TEXT("Publisher rejects a fake 0 XP notification"),
        GridEventFeedPublisher::PublishPartyExperienceAward(Inventory, 0, 4, 0));

    // One service call = one logical notification, not four per-character rows.
    TestEqual(TEXT("Equal shared reward is entirely applied"),
        FRPGExperienceRewardService::AwardToActiveParty(Inventory, 20), 20);
    TestEqual(TEXT("Exactly one XP event for four beneficiaries"),
        Feed->GetEntries().Num(), 1);
    if (Feed->GetEntries().Num() == 1)
    {
        const FGridHudEventEntry& Entry = Feed->GetEntries()[0];
        TestEqual(TEXT("Experience uses dedicated green semantic"),
            Entry.Semantic, EGridHudEventSemantic::Experience);
        TestEqual(TEXT("XP event is an Events feed item"),
            Entry.Category, EGridHudEventCategory::Dungeon);
        TestEqual(TEXT("Committed XP has canonical event code"),
            Entry.EventCode, FName(TEXT("RPG.Experience.Awarded")));
        TestEqual(TEXT("Committed XP cannot be hidden by dungeon visibility"),
            Entry.Visibility, EGridEventVisibility::Always);
        TestTrue(TEXT("XP message shows applied group reward"),
            Entry.Message.ToString().Contains(TEXT("+20 XP")));
        TestTrue(TEXT("Equal shares are visible in one line"),
            Entry.Message.ToString().Contains(TEXT("+5 XP")));
    }
    for (const FGridCharacterInventoryState& Character :
        Inventory->PartyInventoryState.ActiveCharacters)
    {
        TestEqual(TEXT("XP actually arrived in all active members including unconscious"),
            Character.Experience, 5);
    }

    TestTrue(TEXT("Active dungeon puzzle consequence group can open"),
        GridEventFeedPublisher::BeginDungeonEventGroup(
            World, FName(TEXT("Dungeon.Test.Activated")),
            FText::FromString(TEXT("Un mécanisme s'ouvre.")),
            EGridHudEventSemantic::Mechanism, EGridEventVisibility::Always, true));

    TestEqual(TEXT("Uneven group reward is applied"),
        FRPGExperienceRewardService::AwardToActiveParty(Inventory, 10), 10);
    TestEqual(TEXT("XP bypasses active group rather than becoming an orange clause"),
        Feed->GetEntries().Num(), 2);
    if (Feed->GetEntries().Num() == 2)
    {
        const FGridHudEventEntry& Entry = Feed->GetEntries()[1];
        TestEqual(TEXT("Grouped event still preserves XP semantic"),
            Entry.Semantic, EGridHudEventSemantic::Experience);
        TestTrue(TEXT("Uneven share is honestly described"),
            Entry.Message.ToString().Contains(TEXT("répartis entre 4 personnages")));
    }
    TestTrue(TEXT("Mechanism group closes independently"),
        GridEventFeedPublisher::EndDungeonEventGroup(World));
    TestEqual(TEXT("Two XP transactions and one mechanism yield three entries"),
        Feed->GetEntries().Num(), 3);
    if (Feed->GetEntries().Num() == 3)
    {
        TestEqual(TEXT("Mechanism keeps its distinct semantic"),
            Feed->GetEntries()[2].Semantic, EGridHudEventSemantic::Mechanism);
    }
    TestEqual(TEXT("Group member 0 gets first remainder point"),
        Inventory->PartyInventoryState.ActiveCharacters[0].Experience, 8);
    TestEqual(TEXT("Group member 1 gets second remainder point"),
        Inventory->PartyInventoryState.ActiveCharacters[1].Experience, 8);
    TestEqual(TEXT("Group member 2 gets base share"),
        Inventory->PartyInventoryState.ActiveCharacters[2].Experience, 7);
    TestEqual(TEXT("Group member 3 gets base share"),
        Inventory->PartyInventoryState.ActiveCharacters[3].Experience, 7);

    Cleanup();
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
