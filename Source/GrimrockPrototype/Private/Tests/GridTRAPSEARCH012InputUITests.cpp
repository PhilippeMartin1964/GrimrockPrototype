#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridEventFeedSubsystem.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/GrimrockPlayerController.h"
#include "UI/GridPersistentHudWidget.h"
#include "UI/UILocalizationText.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTRAPSEARCH012Contract,
    "Grimrock.TRAP_SEARCH01.2.InputAndHudContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTRAPSEARCH012Contract::RunTest(const FString&)
{
    const UClass* ControllerClass = AGrimrockPlayerController::StaticClass();
    TestNotNull(TEXT("Shared player command is Blueprint-callable"),
        ControllerClass->FindFunctionByName(TEXT("RequestActiveTrapSearch")));
    TestNotNull(TEXT("Pure exploration eligibility query exists"),
        ControllerClass->FindFunctionByName(TEXT("CanRequestActiveTrapSearch")));
    const FProperty* SearchButton = UGridPersistentHudWidget::StaticClass()
        ->FindPropertyByName(TEXT("Button_SearchSurroundings"));
    if (TestNotNull(TEXT("Designer has an optional native search button contract"), SearchButton))
        TestTrue(TEXT("Button is optional until the WBP has been authored"),
            SearchButton->HasMetaData(TEXT("BindWidgetOptional")));
    return true;
}

namespace TRAPSEARCH012
{
    struct FWorld
    {
        UWorld* World = nullptr;
        FWorld()
        {
            const UWorld::InitializationValues Values = UWorld::InitializationValues()
                .AllowAudioPlayback(false).RequiresHitProxies(false)
                .CreatePhysicsScene(false).CreateNavigation(false)
                .CreateAISystem(false).ShouldSimulatePhysics(false)
                .SetTransactional(false);
            World = UWorld::CreateWorld(EWorldType::Game, false,
                FName(*FString::Printf(TEXT("TRAPSEARCH012_%s"),
                    *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true,
                ERHIFeatureLevel::Num, &Values);
            if (World && GEngine)
            {
                FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
                Context.SetCurrentWorld(World);
            }
            if (World) World->InitializeActorsForPlay(FURL());
        }
        ~FWorld()
        {
            if (!World) return;
            World->DestroyWorld(false);
            if (GEngine) GEngine->DestroyWorldContext(World);
        }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTRAPSEARCH012Guards,
    "Grimrock.TRAP_SEARCH01.2.RequestGuardsAndNeutralFeedback",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTRAPSEARCH012Guards::RunTest(const FString&)
{
    TRAPSEARCH012::FWorld F;
    if (!TestNotNull(TEXT("Game world"), F.World)) return false;
    AGrimrockPlayerController* Controller = F.World->SpawnActor<AGrimrockPlayerController>();
    AGrimrockPartyPawn* Party = F.World->SpawnActor<AGrimrockPartyPawn>();
    AGridLevelRuntimeActor* Runtime = F.World->SpawnActor<AGridLevelRuntimeActor>();
    if (!TestNotNull(TEXT("Controller"), Controller) || !TestNotNull(TEXT("Party"), Party) ||
        !TestNotNull(TEXT("Runtime"), Runtime)) return false;
    TestFalse(TEXT("No pawn blocks the search"), Controller->CanRequestActiveTrapSearch());
    Controller->Possess(Party);
    Party->LevelRuntimeActor = Runtime;
    UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Runtime);
    Level->EnsureCellCount();
    Runtime->LevelAsset = Level;
    TestTrue(TEXT("Empty exploration permits deliberate search"), Controller->CanRequestActiveTrapSearch());

    Party->bInventoryWidgetVisible = true;
    TestFalse(TEXT("Inventory owns input"), Controller->CanRequestActiveTrapSearch());
    Party->bInventoryWidgetVisible = false;
    Party->bCharacterCreationModalActive = true;
    TestFalse(TEXT("Character creation owns input"), Controller->CanRequestActiveTrapSearch());
    Party->bCharacterCreationModalActive = false;
    Controller->SetInventoryUiOpen(true);
    TestFalse(TEXT("Controller inventory flag blocks search"), Controller->CanRequestActiveTrapSearch());
    Controller->SetInventoryUiOpen(false);
    Runtime->bIsExecutingRelocation = true;
    TestFalse(TEXT("Relocation blocks search"), Controller->CanRequestActiveTrapSearch());
    Runtime->bIsExecutingRelocation = false;

    UGridTurnManagerComponent* Turns = Runtime->FindComponentByClass<UGridTurnManagerComponent>();
    if (!Turns)
    {
        Turns = NewObject<UGridTurnManagerComponent>(Runtime);
        Turns->RegisterComponent();
    }
    if (!TestNotNull(TEXT("Turn manager"), Turns)) return false;
    Turns->bCombatActive = true;
    TestFalse(TEXT("Combat blocks search"), Controller->CanRequestActiveTrapSearch());
    Turns->bCombatActive = false;

    // These strings must have been projected from UI.csv by the mandatory
    // UIStringTableAuthoring commandlet before running this integration test.
    const FText Message = FUILocalizationText::Get(TEXT("Exploration.Search.Performed"));
    const FText Tooltip = FUILocalizationText::Get(TEXT("Navigation.Tooltip.SearchSurroundings"));
    if (!TestFalse(TEXT("Neutral search message is in real ST_UI"), Message.IsEmpty()) ||
        !TestFalse(TEXT("Search tooltip is in real ST_UI"), Tooltip.IsEmpty())) return false;

    UGridEventFeedSubsystem* Feed = F.World->GetSubsystem<UGridEventFeedSubsystem>();
    if (!TestNotNull(TEXT("Event Feed"), Feed)) return false;
    TestTrue(TEXT("Empty-world search is accepted"), Controller->RequestActiveTrapSearch());
    TestTrue(TEXT("Search always produces only a neutral acknowledgement when nothing is found"),
        Feed->GetEntries().ContainsByPredicate([](const FGridHudEventEntry& E)
        {
            return E.EventCode == FName(TEXT("Dungeon.Trap.ActiveSearch")) &&
                   !E.Message.IsEmpty() &&
                   !E.Message.ToString().Contains(TEXT("piège"));
        }));
    TestTrue(TEXT("Empty-world scan never invents a trap"),
        Runtime->GetOrCreateRuntimeStateForCurrentLevel()->DetectedTriggerTrapIds.IsEmpty());
    TestTrue(TEXT("Empty-world scan records no fake attempts"),
        Runtime->GetOrCreateRuntimeStateForCurrentLevel()->TrapActiveSearchAttemptedRangeMask.IsEmpty());

    const int32 EventsBeforeRefusal = Feed->GetEntries().Num();
    Party->bInventoryWidgetVisible = true;
    TestFalse(TEXT("Shared UI command refuses inventory mode"), Controller->RequestActiveTrapSearch());
    TestEqual(TEXT("Refusal does not publish an additional event"),
        Feed->GetEntries().Num(), EventsBeforeRefusal);
    return true;
}
#endif
