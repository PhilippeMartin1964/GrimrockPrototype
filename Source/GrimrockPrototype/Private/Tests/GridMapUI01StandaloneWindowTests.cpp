#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "UI/GridMapWidget.h"
#include "UI/GrimrockMenuWidget.h"

namespace
{
	struct FGridMapUI01TestWorld
	{
		UWorld* World = nullptr;

		FGridMapUI01TestWorld()
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
				FName(*FString::Printf(TEXT("MapUI01World_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
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

		~FGridMapUI01TestWorld()
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMapUI01OwnershipContractTest, "Grimrock.UI.MapUI01.OwnershipContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMapUI01OwnershipContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UClass* PartyClass = AGrimrockPartyPawn::StaticClass();
	UClass* MenuClass = UGrimrockMenuWidget::StaticClass();
	if (!TestNotNull(TEXT("Party class exists"), PartyClass) || !TestNotNull(TEXT("Menu class exists"), MenuClass))
	{
		return false;
	}

	TestNotNull(TEXT("Party owns standalone MapWidgetClass"),
		FindFProperty<FProperty>(PartyClass, FName(TEXT("MapWidgetClass"))));
	TestNotNull(TEXT("Party owns standalone MapWidgetInstance"),
		FindFProperty<FProperty>(PartyClass, FName(TEXT("MapWidgetInstance"))));
	TestNotNull(TEXT("Party exposes standalone Map visibility"),
		PartyClass->FindFunctionByName(FName(TEXT("IsMapWidgetVisible"))));
	TestNotNull(TEXT("Party exposes standalone Map show path"),
		PartyClass->FindFunctionByName(FName(TEXT("ShowMapWidget"))));

	TestNull(TEXT("Shared menu no longer binds Page_Map"),
		FindFProperty<FProperty>(MenuClass, FName(TEXT("Page_Map"))));
	TestNull(TEXT("Shared menu no longer exposes RefreshMap"),
		MenuClass->FindFunctionByName(FName(TEXT("RefreshMap"))));
	TestNull(TEXT("Shared menu no longer exposes GetMapWidget"),
		MenuClass->FindFunctionByName(FName(TEXT("GetMapWidget"))));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMapUI01ToggleCloseContractTest, "Grimrock.UI.MapUI01.ToggleCloseContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMapUI01ToggleCloseContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridMapUI01TestWorld TestWorld;
	if (!TestNotNull(TEXT("Transient world exists"), TestWorld.World))
	{
		return false;
	}

	AGrimrockPartyPawn* Party = TestWorld.World->SpawnActor<AGrimrockPartyPawn>();
	if (!TestNotNull(TEXT("Party pawn exists"), Party))
	{
		return false;
	}

	UGridMapWidget* Map = NewObject<UGridMapWidget>(Party);
	if (!TestNotNull(TEXT("Standalone Map widget state exists"), Map))
	{
		return false;
	}

	Party->MapWidgetInstance = Map;
	Map->SetVisibility(ESlateVisibility::Visible);
	Party->bInventoryWidgetVisible = true;

	TestTrue(TEXT("Standalone Map reports visible before M toggle"), Party->IsMapWidgetVisible());
	Party->ToggleMapWidget();
	TestFalse(TEXT("M closes standalone Map"), Party->IsMapWidgetVisible());
	TestFalse(TEXT("Closing standalone Map releases major UI state"), Party->bInventoryWidgetVisible);

	return true;
}

#endif
