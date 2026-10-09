#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GrimrockPlayerController.h"

namespace
{
	struct FGridCursorStandalone01TestWorld
	{
		UWorld* World = nullptr;

		FGridCursorStandalone01TestWorld()
		{
			const UWorld::InitializationValues Values = UWorld::InitializationValues()
				.AllowAudioPlayback(false)
				.RequiresHitProxies(false)
				.CreatePhysicsScene(false)
				.CreateNavigation(false)
				.CreateAISystem(false)
				.ShouldSimulatePhysics(false)
				.SetTransactional(false);

			World = UWorld::CreateWorld(
				EWorldType::Game,
				false,
				FName(*FString::Printf(TEXT("CursorStandalone01World_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
				nullptr,
				true,
				ERHIFeatureLevel::Num,
				&Values);

			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FGridCursorStandalone01TestWorld()
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridCursorStandalone01SingleAuthorityTest,
	"Grimrock.UI.CursorStandalone01.SingleAuthority",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridCursorStandalone01SingleAuthorityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridCursorStandalone01TestWorld TestWorld;
	AGrimrockPlayerController* Controller =
		TestWorld.World ? TestWorld.World->SpawnActor<AGrimrockPlayerController>() : nullptr;
	if (!TestNotNull(TEXT("Player controller exists"), Controller))
	{
		return false;
	}

	Controller->CustomCursorWidget = NewObject<UUserWidget>(Controller);
	if (!TestNotNull(TEXT("Custom cursor widget exists"), Controller->CustomCursorWidget))
	{
		return false;
	}

	Controller->SetInventoryUiOpen(true);
	TestTrue(TEXT("Major gameplay UI ownership is active"), Controller->bInventoryUiOpen);
	TestFalse(TEXT("Windows cursor stays hidden while custom cursor owns gameplay UI"), Controller->bShowMouseCursor);
	TestEqual(TEXT("Default system cursor is disabled while custom cursor owns gameplay UI"),
		Controller->DefaultMouseCursor, EMouseCursor::None);
	TestEqual(TEXT("Current system cursor is disabled while custom cursor owns gameplay UI"),
		Controller->CurrentMouseCursor, EMouseCursor::None);
	TestEqual(TEXT("Custom cursor remains visible and never blocks UI hit tests"),
		Controller->CustomCursorWidget->GetVisibility(), ESlateVisibility::HitTestInvisible);

	Controller->SetInventoryUiOpen(false);
	TestFalse(TEXT("Closing gameplay UI releases modal ownership"), Controller->bInventoryUiOpen);
	TestFalse(TEXT("Windows cursor remains hidden after returning to gameplay"), Controller->bShowMouseCursor);
	TestEqual(TEXT("Custom cursor remains visible after returning to gameplay"),
		Controller->CustomCursorWidget->GetVisibility(), ESlateVisibility::HitTestInvisible);

	Controller->CustomCursorWidget = nullptr;
	Controller->SetInventoryUiOpen(true);
	TestTrue(TEXT("System cursor is the fallback when no custom cursor exists"), Controller->bShowMouseCursor);
	TestEqual(TEXT("System cursor fallback uses the default cursor"),
		Controller->CurrentMouseCursor, EMouseCursor::Default);

	return true;
}

#endif
