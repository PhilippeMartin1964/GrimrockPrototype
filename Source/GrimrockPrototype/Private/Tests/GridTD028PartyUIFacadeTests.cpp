#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/GrimrockPlayerController.h"

namespace
{
	struct FGridTD028TestWorld
	{
		UWorld* World = nullptr;

		FGridTD028TestWorld()
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
				FName(*FString::Printf(TEXT("TD028PartyUIFacadeWorld_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true,
				ERHIFeatureLevel::Num, &InitializationValues);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FGridTD028TestWorld()
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD028PartyUIFacadeModalBlockingContractTest,
	"Grimrock.TechnicalDebt.TD02_8.PartyUIFacade.ModalBlockingContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD028PartyUIFacadeModalBlockingContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridTD028TestWorld TestWorld;
	TestNotNull(TEXT("The transient world is created"), TestWorld.World);
	if (!TestWorld.World)
	{
		return false;
	}

	AGrimrockPartyPawn* Party = TestWorld.World->SpawnActor<AGrimrockPartyPawn>();
	TestNotNull(TEXT("The party pawn is spawned"), Party);
	if (!Party)
	{
		return false;
	}

	TestFalse(TEXT("Combat hotbar execution is available when no modal UI owns input"), Party->IsCombatHotbarExecutionBlocked());
	TestFalse(TEXT("Initial character creation is not modal by default"), Party->IsCharacterCreationModalActive());

	Party->bInventoryWidgetVisible = true;
	TestTrue(TEXT("The Grimrock menu blocks combat hotbar execution"), Party->IsCombatHotbarExecutionBlocked());
	Party->bInventoryWidgetVisible = false;
	TestFalse(TEXT("Closing the Grimrock menu releases the pawn-side hotbar block"), Party->IsCombatHotbarExecutionBlocked());

	Party->bCharacterCreationModalActive = true;
	TestTrue(TEXT("Character creation blocks combat hotbar execution"), Party->IsCombatHotbarExecutionBlocked());
	TestTrue(TEXT("The character creation modal accessor mirrors the authoritative pawn flag"), Party->IsCharacterCreationModalActive());
	Party->bCharacterCreationModalActive = false;
	TestFalse(TEXT("Closing character creation releases the pawn-side hotbar block"), Party->IsCombatHotbarExecutionBlocked());

	AGrimrockPlayerController* PlayerController = TestWorld.World->SpawnActor<AGrimrockPlayerController>();
	TestNotNull(TEXT("The Grimrock player controller is spawned"), PlayerController);
	if (!PlayerController)
	{
		return false;
	}

	PlayerController->Possess(Party);
	TestEqual(TEXT("The controller possesses the party pawn"), Party->GetController(), static_cast<AController*>(PlayerController));

	PlayerController->SetInventoryUiOpen(true);
	TestTrue(TEXT("The player controller UI ownership blocks combat hotbar execution"), Party->IsCombatHotbarExecutionBlocked());

	PlayerController->SetInventoryUiOpen(false);
	TestFalse(TEXT("Releasing player controller UI ownership restores hotbar execution"), Party->IsCombatHotbarExecutionBlocked());

	Party->bInventoryWidgetVisible = true;
	PlayerController->SetInventoryUiOpen(true);
	TestTrue(TEXT("Any active UI owner keeps combat hotbar execution blocked"), Party->IsCombatHotbarExecutionBlocked());

	PlayerController->SetInventoryUiOpen(false);
	Party->bInventoryWidgetVisible = false;
	TestFalse(TEXT("Hotbar execution is unblocked only when all UI owners release input"), Party->IsCombatHotbarExecutionBlocked());

	return true;
}

#endif
