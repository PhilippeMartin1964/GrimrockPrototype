#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/GrimrockPlayerController.h"

namespace
{
	struct FGridUISplit02TestWorld
	{
		UWorld* World = nullptr;

		FGridUISplit02TestWorld()
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
				FName(*FString::Printf(TEXT("UISplit02World_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true,
				ERHIFeatureLevel::Num, &InitializationValues);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FGridUISplit02TestWorld()
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUISplit02WorldInteractionPolicyTest, "Grimrock.UI.Split02.WorldInteractionPolicy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUISplit02WorldInteractionPolicyTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridUISplit02TestWorld TestWorld;
	AGrimrockPlayerController* Controller =
		TestWorld.World ? TestWorld.World->SpawnActor<AGrimrockPlayerController>() : nullptr;
	AGrimrockPartyPawn* Party = TestWorld.World ? TestWorld.World->SpawnActor<AGrimrockPartyPawn>() : nullptr;

	if (!TestNotNull(TEXT("Controller exists"), Controller) || !TestNotNull(TEXT("Party exists"), Party))
	{
		return false;
	}

	Controller->bInventoryUiOpen = false;
	Party->bInventoryWorkspaceVisible = false;
	TestFalse(TEXT("Closed UI never blocks world interaction"),
		Controller->ShouldBlockWorldInteractionForInventoryUi(Party, false));

	Controller->bInventoryUiOpen = true;
	Party->bInventoryWorkspaceVisible = false;
	TestTrue(TEXT("Legacy full-page inventory/menu remains modal"),
		Controller->ShouldBlockWorldInteractionForInventoryUi(Party, false));

	Party->bInventoryWorkspaceVisible = true;
	TestFalse(TEXT("Split CharacterSheet + InventoryBag overlay keeps world interaction available"),
		Controller->ShouldBlockWorldInteractionForInventoryUi(Party, false));

	TestFalse(TEXT("Held cursor item remains routable to the world"),
		Controller->ShouldBlockWorldInteractionForInventoryUi(Party, true));

	return true;
}

#endif
