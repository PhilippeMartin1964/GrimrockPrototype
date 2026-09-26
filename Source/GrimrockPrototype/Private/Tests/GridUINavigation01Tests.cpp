#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/HorizontalBox.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "UI/GridCombatHudWidget.h"
#include "UI/GridCharacterSheetWidget.h"
#include "UI/GridInventoryBagWidget.h"
#include "UI/GridInventoryUiTypes.h"
#include "UI/GrimrockMenuWidget.h"

namespace
{
	struct FGridUINavigation01TestWorld
	{
		UWorld* World = nullptr;

		FGridUINavigation01TestWorld()
		{
			const UWorld::InitializationValues InitializationValues = UWorld::InitializationValues()
				.AllowAudioPlayback(false)
				.RequiresHitProxies(false)
				.CreatePhysicsScene(false)
				.CreateNavigation(false)
				.CreateAISystem(false)
				.ShouldSimulatePhysics(false)
				.SetTransactional(false);

			World = UWorld::CreateWorld(EWorldType::Game, false,
				FName(*FString::Printf(TEXT("UINavigation01World_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true,
				ERHIFeatureLevel::Num, &InitializationValues);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FGridUINavigation01TestWorld()
		{
			if (!World)
			{
				return;
			}

			World->DestroyWorld(false);
			if (GEngine)
			{
				GEngine->DestroyWorldContext(World);
			}
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUINavigation01PersistentBarTest, "Grimrock.UI.Navigation01.PersistentBottomBar",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUINavigation01PersistentBarTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridCombatHudWidget* Hud = NewObject<UGridCombatHudWidget>();
	UHorizontalBox* NavigationPanel = NewObject<UHorizontalBox>(Hud);
	TestNotNull(TEXT("HUD can be created"), Hud);
	TestNotNull(TEXT("Navigation panel can be created"), NavigationPanel);
	if (!Hud || !NavigationPanel)
	{
		return false;
	}

	NavigationPanel->SetVisibility(ESlateVisibility::Collapsed);
	Hud->Panel_GlobalNavigation = NavigationPanel;
	Hud->RefreshFromSources();
	TestEqual(TEXT("Global navigation is visible even with combat inactive"), NavigationPanel->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUINavigation01PageToggleTest, "Grimrock.UI.Navigation01.PageToggleContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUINavigation01PageToggleTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridUINavigation01TestWorld TestWorld;
	TestNotNull(TEXT("Transient world exists"), TestWorld.World);
	if (!TestWorld.World)
	{
		return false;
	}

	AGrimrockPartyPawn* Party = TestWorld.World->SpawnActor<AGrimrockPartyPawn>();
	TestNotNull(TEXT("Party pawn exists"), Party);
	if (!Party)
	{
		return false;
	}

	UGrimrockMenuWidget* Menu = NewObject<UGrimrockMenuWidget>(Party);
	TestNotNull(TEXT("Menu state object exists"), Menu);
	if (!Menu)
	{
		return false;
	}
	Party->MenuWidgetInstance = Menu;

	auto PrepareVisibleTab = [Party, Menu](EInventoryTopTab Tab)
	{
		Menu->CurrentTopTab = Tab;
		Menu->SetVisibility(ESlateVisibility::Visible);
		Party->bInventoryWidgetVisible = true;
	};

	UGridCharacterSheetWidget* Sheet = NewObject<UGridCharacterSheetWidget>(Party);
	UGridInventoryBagWidget* Bag = NewObject<UGridInventoryBagWidget>(Party);
	TestNotNull(TEXT("Split character sheet state object exists"), Sheet);
	TestNotNull(TEXT("Split inventory bag state object exists"), Bag);
	if (!Sheet || !Bag)
	{
		return false;
	}
	Sheet->SetVisibility(ESlateVisibility::Visible);
	Bag->SetVisibility(ESlateVisibility::Visible);
	Party->CharacterSheetWidgetInstance = Sheet;
	Party->InventoryBagWidgetInstance = Bag;
	Party->bInventoryWorkspaceVisible = true;
	Party->bInventoryWidgetVisible = true;
	Party->ToggleInventoryWidget();
	TestFalse(TEXT("I closes the canonical split inventory workspace when both windows are visible"), Party->bInventoryWidgetVisible);

	PrepareVisibleTab(EInventoryTopTab::Skills);
	Party->ToggleSkillsWidget();
	TestFalse(TEXT("K closes Skills when Skills is already active"), Party->bInventoryWidgetVisible);

	PrepareVisibleTab(EInventoryTopTab::Recipes);
	Party->ToggleCraftingWidget();
	TestFalse(TEXT("G closes Crafting/Recipes when already active"), Party->bInventoryWidgetVisible);

	PrepareVisibleTab(EInventoryTopTab::Map);
	Party->ToggleMapWidget();
	TestFalse(TEXT("M closes Map when Map is already active"), Party->bInventoryWidgetVisible);

	PrepareVisibleTab(EInventoryTopTab::Journal);
	Party->ToggleJournalWidget();
	TestFalse(TEXT("J closes Journal when Journal is already active"), Party->bInventoryWidgetVisible);

	PrepareVisibleTab(EInventoryTopTab::Codex);
	Party->ToggleHelpWidget();
	TestFalse(TEXT("H closes Help/Codex when already active"), Party->bInventoryWidgetVisible);

	Sheet->SetVisibility(ESlateVisibility::Visible);
	Bag->SetVisibility(ESlateVisibility::Visible);
	Party->bInventoryWorkspaceVisible = true;
	Party->bInventoryWidgetVisible = true;
	Party->HandleGlobalEscape();
	TestFalse(TEXT("Global ESC closes the canonical split inventory workspace before requesting the in-game main menu"), Party->bInventoryWidgetVisible);

	return true;
}

#endif
