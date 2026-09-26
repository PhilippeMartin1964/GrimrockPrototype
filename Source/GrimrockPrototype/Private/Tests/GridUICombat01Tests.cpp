#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "UI/GridCharacterSheetWidget.h"
#include "UI/GridInventoryBagWidget.h"
#include "UI/GrimrockMenuWidget.h"

namespace
{
	struct FGridUICombat01TestWorld
	{
		UWorld* World = nullptr;

		FGridUICombat01TestWorld()
		{
			const UWorld::InitializationValues Values = UWorld::InitializationValues()
				.AllowAudioPlayback(false)
				.RequiresHitProxies(false)
				.CreatePhysicsScene(false)
				.CreateNavigation(false)
				.CreateAISystem(false)
				.ShouldSimulatePhysics(false)
				.SetTransactional(false);

			World = UWorld::CreateWorld(EWorldType::Game, false,
				FName(*FString::Printf(TEXT("UICombat01World_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true,
				ERHIFeatureLevel::Num, &Values);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FGridUICombat01TestWorld()
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

	struct FGridUICombat01Fixture
	{
		FGridUICombat01TestWorld TestWorld;
		AGridLevelRuntimeActor* Runtime = nullptr;
		AGrimrockPartyPawn* Party = nullptr;
		UGridTurnManagerComponent* TurnManager = nullptr;

		bool Initialize()
		{
			if (!TestWorld.World)
			{
				return false;
			}

			Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
			Party = TestWorld.World->SpawnActor<AGrimrockPartyPawn>();
			if (!Runtime || !Party)
			{
				return false;
			}
			Party->LevelRuntimeActor = Runtime;

			TurnManager = NewObject<UGridTurnManagerComponent>(Runtime, TEXT("UICombat01TurnManager"));
			Runtime->AddInstanceComponent(TurnManager);
			TurnManager->RegisterComponent();

			Party->MenuWidgetInstance = NewObject<UGrimrockMenuWidget>(Party);
			Party->CharacterSheetWidgetInstance = NewObject<UGridCharacterSheetWidget>(Party);
			Party->InventoryBagWidgetInstance = NewObject<UGridInventoryBagWidget>(Party);
			return TurnManager && Party->MenuWidgetInstance && Party->CharacterSheetWidgetInstance && Party->InventoryBagWidgetInstance;
		}

		void SetMajorUiVisible()
		{
			Party->MenuWidgetInstance->SetVisibility(ESlateVisibility::Visible);
			Party->CharacterSheetWidgetInstance->SetVisibility(ESlateVisibility::Visible);
			Party->InventoryBagWidgetInstance->SetVisibility(ESlateVisibility::Visible);
			Party->bInventoryWorkspaceVisible = true;
			Party->bInventoryWidgetVisible = true;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUICombat01CloseNonCombatUiTest, "Grimrock.UI.Combat01.CloseNonCombatUi",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUICombat01CloseNonCombatUiTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridUICombat01Fixture Fixture;
	TestTrue(TEXT("Fixture initializes"), Fixture.Initialize());
	if (!Fixture.Party)
	{
		return false;
	}

	Fixture.SetMajorUiVisible();
	Fixture.Party->bAutoSaveOnInventoryClose = true;
	Fixture.Party->CloseNonCombatUiForCombat();

	TestEqual(TEXT("Page shell is collapsed"), Fixture.Party->MenuWidgetInstance->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("Character sheet is collapsed"), Fixture.Party->CharacterSheetWidgetInstance->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("Inventory bag is collapsed"), Fixture.Party->InventoryBagWidgetInstance->GetVisibility(), ESlateVisibility::Collapsed);
	TestFalse(TEXT("Split inventory workspace flag is cleared"), Fixture.Party->bInventoryWorkspaceVisible);
	TestFalse(TEXT("Major UI ownership flag is cleared"), Fixture.Party->bInventoryWidgetVisible);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUICombat01CombatLifecycleTest, "Grimrock.UI.Combat01.CombatLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUICombat01CombatLifecycleTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridUICombat01Fixture Fixture;
	TestTrue(TEXT("Fixture initializes"), Fixture.Initialize());
	if (!Fixture.Party || !Fixture.TurnManager)
	{
		return false;
	}

	// ShowCombatActionPanelWidget binds the UI lifecycle before it attempts HUD creation,
	// so this transient test does not require a PlayerController or UMG class.
	Fixture.Party->ShowCombatActionPanelWidget();
	Fixture.SetMajorUiVisible();
	Fixture.TurnManager->bCombatActive = true;

	TestTrue(TEXT("Combat blocks non-combat major UI"), Fixture.Party->IsNonCombatUiBlockedByCombat());
	Fixture.TurnManager->OnPhaseChanged.Broadcast(EGridCombatPhase::StartingCombat);

	TestFalse(TEXT("Combat phase closes major UI"), Fixture.Party->bInventoryWidgetVisible);
	TestEqual(TEXT("Combat phase collapses page shell"), Fixture.Party->MenuWidgetInstance->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("Combat phase collapses character sheet"), Fixture.Party->CharacterSheetWidgetInstance->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("Combat phase collapses inventory bag"), Fixture.Party->InventoryBagWidgetInstance->GetVisibility(), ESlateVisibility::Collapsed);

	Fixture.Party->ToggleInventoryWidget();
	Fixture.Party->ToggleSkillsWidget();
	TestFalse(TEXT("Navigation commands cannot reopen major UI during combat"), Fixture.Party->bInventoryWidgetVisible);

	Fixture.TurnManager->bCombatActive = false;
	TestFalse(TEXT("Major UI lock releases when combat authority becomes inactive"), Fixture.Party->IsNonCombatUiBlockedByCombat());
	return true;
}

#endif
