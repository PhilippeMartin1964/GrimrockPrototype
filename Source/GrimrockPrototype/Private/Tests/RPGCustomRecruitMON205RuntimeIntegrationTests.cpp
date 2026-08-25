#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "UI/RPGCharacterCreationWidget.h"
#include "UObject/Class.h"

namespace RPGCustomRecruitMON205RuntimeIntegrationTests
{
	struct FRuntimeIntegrationTestWorld
	{
		UWorld* World = nullptr;

		FRuntimeIntegrationTestWorld()
		{
			const UWorld::InitializationValues Values = UWorld::InitializationValues()
															.AllowAudioPlayback(false)
															.RequiresHitProxies(false)
															.CreatePhysicsScene(false)
															.CreateNavigation(false)
															.CreateAISystem(false)
															.ShouldSimulatePhysics(false)
															.SetTransactional(false);

			World = UWorld::CreateWorld(EWorldType::Game, false, FName(*FString::Printf(TEXT("MON2056_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
				nullptr, true, ERHIFeatureLevel::Num, &Values);
			if (!World || !GEngine)
			{
				return;
			}

			FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
			Context.SetCurrentWorld(World);
		}

		~FRuntimeIntegrationTestWorld()
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

using namespace RPGCustomRecruitMON205RuntimeIntegrationTests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON205CustomRecruitRuntimeContractTest, "Grimrock.MON20.5.CustomRecruit.RuntimeContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON205CustomRecruitRuntimeContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UClass* PawnClass = AGrimrockPartyPawn::StaticClass();
	TestNotNull(TEXT("Party pawn class exists"), PawnClass);
	if (!PawnClass)
	{
		return false;
	}

	TestNotNull(TEXT("Custom recruit reuses CharacterCreationWidgetClass"),
		PawnClass->FindPropertyByName(GET_MEMBER_NAME_CHECKED(AGrimrockPartyPawn, CharacterCreationWidgetClass)));
	TestNotNull(TEXT("Custom recruit reuses CharacterCreationWidgetInstance"),
		PawnClass->FindPropertyByName(GET_MEMBER_NAME_CHECKED(AGrimrockPartyPawn, CharacterCreationWidgetInstance)));
	TestNotNull(TEXT("Custom recruit reuses character creation modal state"),
		PawnClass->FindPropertyByName(GET_MEMBER_NAME_CHECKED(AGrimrockPartyPawn, bCharacterCreationModalActive)));

	TestNotNull(
		TEXT("Runtime exposes ShowCustomRecruitCharacterCreationWidget"), PawnClass->FindFunctionByName(TEXT("ShowCustomRecruitCharacterCreationWidget")));
	TestNotNull(
		TEXT("Runtime exposes CloseCustomRecruitCharacterCreationWidget"), PawnClass->FindFunctionByName(TEXT("CloseCustomRecruitCharacterCreationWidget")));
	TestNotNull(TEXT("Runtime exposes IsCustomRecruitCharacterCreationModalActive"),
		PawnClass->FindFunctionByName(TEXT("IsCustomRecruitCharacterCreationModalActive")));
	TestNotNull(
		TEXT("Runtime exposes GetCustomRecruitCharacterCreationWidget"), PawnClass->FindFunctionByName(TEXT("GetCustomRecruitCharacterCreationWidget")));

	TestNull(TEXT("No parallel CustomRecruit widget class property was introduced"), PawnClass->FindPropertyByName(TEXT("CustomRecruitWidgetClass")));
	TestNull(TEXT("No parallel CustomRecruit widget instance property was introduced"), PawnClass->FindPropertyByName(TEXT("CustomRecruitWidgetInstance")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON205CustomRecruitRuntimeDefaultStateTest, "Grimrock.MON20.5.CustomRecruit.RuntimeDefaultState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON205CustomRecruitRuntimeDefaultStateTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const AGrimrockPartyPawn* DefaultPawn = GetDefault<AGrimrockPartyPawn>();
	TestNotNull(TEXT("Party pawn CDO exists"), DefaultPawn);
	if (!DefaultPawn)
	{
		return false;
	}

	TestFalse(TEXT("Character creation modal is inactive by default"), DefaultPawn->bCharacterCreationModalActive);
	TestNull(TEXT("No character creation widget instance exists on the CDO"), DefaultPawn->CharacterCreationWidgetInstance.Get());
	TestFalse(TEXT("Custom recruit modal is inactive by default"), DefaultPawn->IsCustomRecruitCharacterCreationModalActive());
	TestNull(TEXT("Custom recruit widget getter is null by default"), DefaultPawn->GetCustomRecruitCharacterCreationWidget());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2056CustomRecruitRuntimeCombatGateTest, "Grimrock.MON20.5.CustomRecruit.RuntimeCombatGate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2056CustomRecruitRuntimeCombatGateTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FRuntimeIntegrationTestWorld TestWorld;
	if (!TestWorld.World)
	{
		AddError(TEXT("Unable to create MON20.5.6 test world."));
		return false;
	}

	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	AGrimrockPartyPawn* Pawn = TestWorld.World->SpawnActor<AGrimrockPartyPawn>();
	TestNotNull(TEXT("Runtime actor spawns"), Runtime);
	TestNotNull(TEXT("Party pawn spawns"), Pawn);
	if (!Runtime || !Pawn || !Pawn->PartyInventoryComponent)
	{
		return false;
	}

	Pawn->LevelRuntimeActor = Runtime;
	FGridPartyInventoryState& PartyState = Pawn->PartyInventoryComponent->PartyInventoryState;
	PartyState.bInitialCharacterCreationCompleted = true;
	PartyState.MaxActiveCharacters = 4;
	PartyState.SelectedCharacterIndex = 0;
	PartyState.ActiveCharacters.SetNum(1);
	PartyState.ActiveCharacters[0].CharacterId = FGuid(20, 5, 6, 1);

	UGridTurnManagerComponent* TurnManager = NewObject<UGridTurnManagerComponent>(Runtime);
	TestNotNull(TEXT("Turn manager component is created"), TurnManager);
	if (!TurnManager)
	{
		return false;
	}

	Runtime->AddInstanceComponent(TurnManager);
	TurnManager->RegisterComponent();
	TurnManager->bCombatActive = true;
	TurnManager->CurrentPhase = EGridCombatPhase::PlayerPhase;
	TurnManager->RoundNumber = 3;

	TestFalse(TEXT("Custom recruit modal is rejected during active combat"), Pawn->ShowCustomRecruitCharacterCreationWidget());
	TestFalse(TEXT("Combat rejection leaves character creation modal inactive"), Pawn->bCharacterCreationModalActive);
	TestNull(TEXT("Combat rejection creates no character creation widget"), Pawn->CharacterCreationWidgetInstance.Get());

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
