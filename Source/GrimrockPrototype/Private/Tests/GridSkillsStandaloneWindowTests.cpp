#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "UI/GridSkillsWidget.h"
#include "UI/GrimrockMenuWidget.h"

namespace
{
	struct FGridSkillsStandaloneTestWorld
	{
		UWorld* World = nullptr;

		FGridSkillsStandaloneTestWorld()
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
				FName(*FString::Printf(TEXT("SkillsStandaloneWorld_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
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

		~FGridSkillsStandaloneTestWorld()
		{
			if (!World) return;
			World->DestroyWorld(false);
			if (GEngine) GEngine->DestroyWorldContext(World);
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridSkillsStandaloneOwnershipContractTest,
	"Grimrock.UI.RPG03.StandaloneSkills.OwnershipContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridSkillsStandaloneOwnershipContractTest::RunTest(const FString&)
{
	UClass* PartyClass = AGrimrockPartyPawn::StaticClass();
	UClass* MenuClass = UGrimrockMenuWidget::StaticClass();
	if (!TestNotNull(TEXT("Party class exists"), PartyClass) || !TestNotNull(TEXT("Menu class exists"), MenuClass))
	{
		return false;
	}

	TestNotNull(TEXT("Party owns standalone SkillsWidgetClass"),
		FindFProperty<FProperty>(PartyClass, FName(TEXT("SkillsWidgetClass"))));
	TestNotNull(TEXT("Party owns standalone SkillsWidgetInstance"),
		FindFProperty<FProperty>(PartyClass, FName(TEXT("SkillsWidgetInstance"))));
	TestNotNull(TEXT("Party exposes standalone Skills visibility"),
		PartyClass->FindFunctionByName(FName(TEXT("IsSkillsWidgetVisible"))));
	TestNotNull(TEXT("Party exposes standalone Skills show path"),
		PartyClass->FindFunctionByName(FName(TEXT("ShowSkillsWidget"))));
	TestNotNull(TEXT("Party exposes standalone Skills hide path"),
		PartyClass->FindFunctionByName(FName(TEXT("HideSkillsWidget"))));

	TestNull(TEXT("Shared menu no longer binds Page_Skills"),
		FindFProperty<FProperty>(MenuClass, FName(TEXT("Page_Skills"))));
	TestNull(TEXT("Shared menu no longer exposes RefreshSkills"),
		MenuClass->FindFunctionByName(FName(TEXT("RefreshSkills"))));
	TestNull(TEXT("Shared menu no longer exposes GetSkillsWidget"),
		MenuClass->FindFunctionByName(FName(TEXT("GetSkillsWidget"))));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridSkillsStandaloneToggleCloseContractTest,
	"Grimrock.UI.RPG03.StandaloneSkills.ToggleCloseContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridSkillsStandaloneToggleCloseContractTest::RunTest(const FString&)
{
	FGridSkillsStandaloneTestWorld TestWorld;
	if (!TestNotNull(TEXT("Transient world exists"), TestWorld.World)) return false;

	AGrimrockPartyPawn* Party = TestWorld.World->SpawnActor<AGrimrockPartyPawn>();
	if (!TestNotNull(TEXT("Party pawn exists"), Party)) return false;

	UGridSkillsWidget* Skills = NewObject<UGridSkillsWidget>(Party);
	if (!TestNotNull(TEXT("Standalone Skills widget state exists"), Skills)) return false;

	Party->SkillsWidgetInstance = Skills;
	Skills->SetVisibility(ESlateVisibility::Visible);
	Party->bInventoryWidgetVisible = true;

	TestTrue(TEXT("Standalone Skills reports visible before K toggle"), Party->IsSkillsWidgetVisible());
	Party->ToggleSkillsWidget();
	TestFalse(TEXT("K closes standalone Skills"), Party->IsSkillsWidgetVisible());
	TestFalse(TEXT("Closing standalone Skills releases major UI state"), Party->bInventoryWidgetVisible);
	return true;
}

#endif
