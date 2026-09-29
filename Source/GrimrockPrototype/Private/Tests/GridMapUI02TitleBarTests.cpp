#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "UI/GridMapWidget.h"

namespace
{
	struct FGridMapUI02TestWorld
	{
		UWorld* World = nullptr;

		FGridMapUI02TestWorld()
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
				FName(*FString::Printf(TEXT("MapUI02World_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
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

		~FGridMapUI02TestWorld()
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMapUI02CloseButtonContractTest, "Grimrock.UI.MapUI02.CloseButtonContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMapUI02CloseButtonContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UClass* MapClass = UGridMapWidget::StaticClass();
	UClass* PartyClass = AGrimrockPartyPawn::StaticClass();
	if (!TestNotNull(TEXT("Map widget class exists"), MapClass) || !TestNotNull(TEXT("Party class exists"), PartyClass))
	{
		return false;
	}

	TestNotNull(TEXT("Map exposes canonical Button_CloseMap binding"),
		FindFProperty<FProperty>(MapClass, FName(TEXT("Button_CloseMap"))));
	TestNotNull(TEXT("Party exposes public canonical Map close path"),
		PartyClass->FindFunctionByName(FName(TEXT("HideMapWidget"))));
	TestNotNull(TEXT("Party exposes reflected close-button handler"),
		PartyClass->FindFunctionByName(FName(TEXT("HandleMapWindowCloseClicked"))));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMapUI02CloseTransitionTest, "Grimrock.UI.MapUI02.CloseTransition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMapUI02CloseTransitionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridMapUI02TestWorld TestWorld;
	if (!TestNotNull(TEXT("Transient world exists"), TestWorld.World))
	{
		return false;
	}

	AGrimrockPartyPawn* Party = TestWorld.World->SpawnActor<AGrimrockPartyPawn>();
	UGridMapWidget* Map = NewObject<UGridMapWidget>(Party);
	if (!TestNotNull(TEXT("Party exists"), Party) || !TestNotNull(TEXT("Map state exists"), Map))
	{
		return false;
	}

	Party->MapWidgetInstance = Map;
	Party->bInventoryWidgetVisible = true;
	Map->SetVisibility(ESlateVisibility::Visible);

	Party->HideMapWidget();

	TestFalse(TEXT("Canonical Map close path collapses standalone Map"), Party->IsMapWidgetVisible());
	TestFalse(TEXT("Canonical Map close path releases major UI state"), Party->bInventoryWidgetVisible);

	return true;
}

#endif
