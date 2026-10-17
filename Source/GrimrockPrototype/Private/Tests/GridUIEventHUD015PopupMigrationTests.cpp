#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridEventFeedSubsystem.h"
#include "Runtime/GridLevelRuntimeActor.h"

namespace
{
	struct FGridEventHUD015WorldScope
	{
		UWorld* World = nullptr;

		FGridEventHUD015WorldScope()
		{
			const UWorld::InitializationValues InitializationValues = UWorld::InitializationValues()
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
				FName(*FString::Printf(TEXT("EVENTHUD015_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
				nullptr,
				true,
				ERHIFeatureLevel::Num,
				&InitializationValues);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FGridEventHUD015WorldScope()
		{
			if (World)
			{
				World->DestroyWorld(false);
				if (GEngine)
				{
					GEngine->DestroyWorldContext(World);
				}
				World = nullptr;
			}
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIEventHUD015ReadableMigrationTest, "Grimrock.UI.EventHUD01.PopupMigration.Readable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIEventHUD015ReadableMigrationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridEventHUD015WorldScope Scope;
	if (!TestNotNull(TEXT("Test world exists"), Scope.World))
	{
		return false;
	}

	AGridLevelRuntimeActor* Runtime = Scope.World->SpawnActor<AGridLevelRuntimeActor>();
	UGridEventFeedSubsystem* Feed = Scope.World->GetSubsystem<UGridEventFeedSubsystem>();
	if (!TestNotNull(TEXT("Runtime actor exists"), Runtime) || !TestNotNull(TEXT("Event Feed subsystem exists"), Feed))
	{
		return false;
	}

	Runtime->ShowReadableMessage(FText::FromString(TEXT("Seul celui qui porte la marque pourra franchir le seuil.")));

	TestFalse(TEXT("Readable world text no longer creates a modal readable state"), Runtime->HasActiveReadableMessage());
	TestEqual(TEXT("Readable compatibility façade publishes exactly one feed entry"), Feed->GetEntries().Num(), 1);
	if (Feed->GetEntries().Num() == 1)
	{
		const FGridHudEventEntry& Entry = Feed->GetEntries()[0];
		TestEqual(TEXT("Readable entry uses dungeon category"), Entry.Category, EGridHudEventCategory::Dungeon);
		TestEqual(TEXT("Readable entry is informational"), Entry.Semantic, EGridHudEventSemantic::Information);
		TestEqual(TEXT("Readable entry is explicitly player-facing"), Entry.Visibility, EGridEventVisibility::Always);
		TestEqual(TEXT("Readable entry keeps its text"), Entry.Message.ToString(),
			FString(TEXT("Seul celui qui porte la marque pourra franchir le seuil.")));
	}

	TestFalse(TEXT("There is no popup left to dismiss after feed migration"), Runtime->DismissReadableMessage());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIEventHUD015InteractionMigrationTest, "Grimrock.UI.EventHUD01.PopupMigration.Interaction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIEventHUD015InteractionMigrationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridEventHUD015WorldScope Scope;
	if (!TestNotNull(TEXT("Test world exists"), Scope.World))
	{
		return false;
	}

	AGridLevelRuntimeActor* Runtime = Scope.World->SpawnActor<AGridLevelRuntimeActor>();
	UGridEventFeedSubsystem* Feed = Scope.World->GetSubsystem<UGridEventFeedSubsystem>();
	if (!TestNotNull(TEXT("Runtime actor exists"), Runtime) || !TestNotNull(TEXT("Event Feed subsystem exists"), Feed))
	{
		return false;
	}

	Runtime->ShowInteractionFeedback(FText::FromString(TEXT("Impossible d'utiliser cet objet.")), 0.1f);

	TestEqual(TEXT("Interaction feedback publishes exactly one feed entry"), Feed->GetEntries().Num(), 1);
	if (Feed->GetEntries().Num() == 1)
	{
		const FGridHudEventEntry& Entry = Feed->GetEntries()[0];
		TestEqual(TEXT("Interaction feedback remains in Events category"), Entry.Category, EGridHudEventCategory::Dungeon);
		TestEqual(TEXT("Interaction feedback has interaction semantic"), Entry.Semantic, EGridHudEventSemantic::Interaction);
		TestEqual(TEXT("Interaction feedback is explicitly player-facing"), Entry.Visibility, EGridEventVisibility::Always);
		TestEqual(TEXT("Interaction feedback keeps its text"), Entry.Message.ToString(), FString(TEXT("Impossible d'utiliser cet objet.")));
	}

	Runtime->HideInteractionFeedback();
	TestEqual(TEXT("Hiding legacy interaction popup does not delete feed history"), Feed->GetEntries().Num(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIEventHUD015CombatPopupBoundaryTest, "Grimrock.UI.EventHUD01.PopupMigration.CombatBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIEventHUD015CombatPopupBoundaryTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridEventHUD015WorldScope Scope;
	if (!TestNotNull(TEXT("Test world exists"), Scope.World))
	{
		return false;
	}

	AGridLevelRuntimeActor* Runtime = Scope.World->SpawnActor<AGridLevelRuntimeActor>();
	UGridEventFeedSubsystem* Feed = Scope.World->GetSubsystem<UGridEventFeedSubsystem>();
	if (!TestNotNull(TEXT("Runtime actor exists"), Runtime) || !TestNotNull(TEXT("Event Feed subsystem exists"), Feed))
	{
		return false;
	}

	FGridPlayerAttackFeedbackRequest CombatFeedback;
	CombatFeedback.bAccepted = false;
	CombatFeedback.PrimaryText = FText::FromString(TEXT("L'attaque est impossible."));
	Runtime->ShowCombatFeedback(CombatFeedback);

	TestEqual(TEXT("EVENTHUD01.8 migrates rejected combat feedback into the Event Feed"), Feed->GetEntries().Num(), 1);
	if (Feed->GetEntries().Num() == 1)
	{
		TestEqual(TEXT("Rejected attack uses Combat category"), Feed->GetEntries()[0].Category, EGridHudEventCategory::Combat);
		TestEqual(TEXT("Rejected attack keeps an interaction semantic"), Feed->GetEntries()[0].Semantic, EGridHudEventSemantic::Interaction);
	}
	return true;
}

#endif
