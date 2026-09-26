#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterBehaviorComponent.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterMovementComponent.h"
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
		AGridMonsterActor* Monster = nullptr;

		bool Initialize(bool bWithCombatMonster = false)
		{
			if (!TestWorld.World)
			{
				return false;
			}

			Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
			Party = TestWorld.World->SpawnActor<AGrimrockPartyPawn>();
			if (!Runtime || !Party || !Party->PartyInventoryComponent)
			{
				return false;
			}
			Party->LevelRuntimeActor = Runtime;

			FGridCharacterInventoryState Character;
			Character.CharacterId = FGuid::NewGuid();
			Character.DisplayName = FText::FromString(TEXT("UI-COMBAT01 Party"));
			Character.DerivedStats.MaxHealth = 20;
			Character.Resources.CurrentHealth = 20;
			Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters = { Character };

			TurnManager = NewObject<UGridTurnManagerComponent>(Runtime, TEXT("UICombat01TurnManager"));
			TurnManager->bAutoInitialize = false;
			Runtime->AddInstanceComponent(TurnManager);
			TurnManager->RegisterComponent();
			if (!TurnManager->InitializeTurnManager(Runtime, Party))
			{
				return false;
			}

			Party->MenuWidgetInstance = NewObject<UGrimrockMenuWidget>(Party);
			Party->CharacterSheetWidgetInstance = NewObject<UGridCharacterSheetWidget>(Party);
			Party->InventoryBagWidgetInstance = NewObject<UGridInventoryBagWidget>(Party);
			if (!Party->MenuWidgetInstance || !Party->CharacterSheetWidgetInstance || !Party->InventoryBagWidgetInstance)
			{
				return false;
			}

			if (!bWithCombatMonster)
			{
				return true;
			}

			UGridMonsterDefinitionAsset* Definition = NewObject<UGridMonsterDefinitionAsset>(Runtime);
			Definition->MonsterId = TEXT("UI_COMBAT01_Monster");
			Definition->DisplayName = FText::FromString(TEXT("UI-COMBAT01 Monster"));
			Definition->CategoryId = TEXT("UI_COMBAT01");
			Definition->MaxHealth = 10;
			Definition->ActionPointsPerTurn = 2;
			Definition->DeathExpectedDuration = 1.0f;

			FActorSpawnParameters Params;
			Params.Owner = Runtime;
			Monster = TestWorld.World->SpawnActor<AGridMonsterActor>(
				AGridMonsterActor::StaticClass(), Runtime->GetCellCenterWorld(2, 2), FRotator::ZeroRotator, Params);
			if (!Monster || !Monster->InitializeMonster(Definition, FGuid::NewGuid(), FIntPoint(2, 2), EGridEdge::South))
			{
				return false;
			}

			UGridMonsterMovementComponent* Movement = NewObject<UGridMonsterMovementComponent>(Monster, TEXT("UICombat01Movement"));
			Movement->bAutoInitialize = false;
			Movement->bInferCellFromActorLocation = false;
			Monster->AddInstanceComponent(Movement);
			Movement->RegisterComponent();

			UGridMonsterBehaviorComponent* Behavior = NewObject<UGridMonsterBehaviorComponent>(Monster, TEXT("UICombat01Behavior"));
			Behavior->bAutoInitialize = false;
			Monster->AddInstanceComponent(Behavior);
			Behavior->RegisterComponent();

			return Movement->InitializeMovement(Runtime) && Behavior->InitializeBehavior(Runtime, Party) && Monster->CombatComponent &&
				Monster->CombatComponent->InitializeCombat(Party);
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
	TestTrue(TEXT("Fixture with combat monster initializes"), Fixture.Initialize(true));
	if (!Fixture.Party || !Fixture.TurnManager || !Fixture.Monster)
	{
		return false;
	}

	Fixture.SetMajorUiVisible();
	TestFalse(TEXT("Combat is initially inactive"), Fixture.TurnManager->bCombatActive);
	TestTrue(TEXT("Real combat start succeeds"), Fixture.TurnManager->StartCombatWithAllMonsters());
	TestTrue(TEXT("Combat becomes authoritative"), Fixture.TurnManager->bCombatActive);

	TestFalse(TEXT("Combat start closes major UI"), Fixture.Party->bInventoryWidgetVisible);
	TestEqual(TEXT("Combat start collapses page shell"), Fixture.Party->MenuWidgetInstance->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("Combat start collapses character sheet"), Fixture.Party->CharacterSheetWidgetInstance->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("Combat start collapses inventory bag"), Fixture.Party->InventoryBagWidgetInstance->GetVisibility(), ESlateVisibility::Collapsed);

	Fixture.Party->ToggleInventoryWidget();
	Fixture.Party->ToggleSkillsWidget();
	TestFalse(TEXT("Navigation commands cannot reopen major UI during combat"), Fixture.Party->bInventoryWidgetVisible);

	Fixture.TurnManager->AbortCombat();
	TestFalse(TEXT("Combat ends authoritatively"), Fixture.TurnManager->bCombatActive);
	TestFalse(TEXT("Major UI lock releases when combat becomes inactive"), Fixture.Party->IsNonCombatUiBlockedByCombat());
	return true;
}

#endif
