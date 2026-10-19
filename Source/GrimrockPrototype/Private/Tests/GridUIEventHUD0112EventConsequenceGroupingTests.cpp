#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridEventFeedPublisher.h"
#include "Runtime/GridEventFeedSubsystem.h"

namespace
{
	struct FGridEventHUD0112WorldScope
	{
		UWorld* World = nullptr;

		FGridEventHUD0112WorldScope()
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
				FName(*FString::Printf(TEXT("EVENTHUD0112_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
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

		~FGridEventHUD0112WorldScope()
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

	bool BeginLeverGroup(UWorld* World, bool bObservable = true)
	{
		return GridEventFeedPublisher::BeginDungeonEventGroup(
			World,
			FName(TEXT("Dungeon.Lever.Activated")),
			FText::FromString(TEXT("Le levier est activé.")),
			EGridHudEventSemantic::Mechanism,
			bObservable ? EGridEventVisibility::Always : EGridEventVisibility::Observable,
			bObservable,
			FName(TEXT("Lever.Source")));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridUIEventHUD0112SingleConsequenceTest,
	"Grimrock.UI.EventHUD01.ConsequenceGrouping.SingleConsequence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIEventHUD0112SingleConsequenceTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridEventHUD0112WorldScope Scope;
	UGridEventFeedSubsystem* Feed = Scope.World ? Scope.World->GetSubsystem<UGridEventFeedSubsystem>() : nullptr;
	if (!TestNotNull(TEXT("Event Feed subsystem exists"), Feed))
	{
		return false;
	}

	TestTrue(TEXT("Root cause group opens"), BeginLeverGroup(Scope.World));
	TestEqual(TEXT("Root is buffered only after immediate consequences are known"), Feed->GetEntries().Num(), 0);

	TestTrue(
		TEXT("Immediate door consequence joins the active group"),
		GridEventFeedPublisher::PublishDungeonEvent(
			Scope.World,
			FName(TEXT("Dungeon.Door.Opened")),
			FText::FromString(TEXT("Une porte s'ouvre.")),
			EGridHudEventSemantic::Mechanism,
			EGridEventVisibility::Always,
			true,
			FName(TEXT("Door.Target"))));

	TestTrue(TEXT("Group closes as one logical entry"), GridEventFeedPublisher::EndDungeonEventGroup(Scope.World));
	TestEqual(TEXT("Exactly one event is stored"), Feed->GetEntries().Num(), 1);
	if (Feed->GetEntries().Num() == 1)
	{
		TestEqual(
			TEXT("Cause and consequence share one line"),
			Feed->GetEntries()[0].Message.ToString(),
			FString(TEXT("Le levier est activé → Une porte s'ouvre.")));
		TestFalse(TEXT("Grouping introduces no forced line break"), Feed->GetEntries()[0].Message.ToString().Contains(TEXT("\n")));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridUIEventHUD0112MultipleConsequencesTest,
	"Grimrock.UI.EventHUD01.ConsequenceGrouping.MultipleConsequences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIEventHUD0112MultipleConsequencesTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridEventHUD0112WorldScope Scope;
	UGridEventFeedSubsystem* Feed = Scope.World ? Scope.World->GetSubsystem<UGridEventFeedSubsystem>() : nullptr;
	if (!TestNotNull(TEXT("Event Feed subsystem exists"), Feed))
	{
		return false;
	}

	TestTrue(TEXT("Group opens"), BeginLeverGroup(Scope.World));
	GridEventFeedPublisher::PublishDungeonEvent(
		Scope.World, FName(TEXT("Dungeon.Door.Opened")), FText::FromString(TEXT("Une porte s'ouvre.")),
		EGridHudEventSemantic::Mechanism, EGridEventVisibility::Always, true);
	GridEventFeedPublisher::PublishDungeonEvent(
		Scope.World, FName(TEXT("Dungeon.Relocation.Activated")), FText::FromString(TEXT("Un téléporteur s'active.")),
		EGridHudEventSemantic::Mechanism, EGridEventVisibility::Always, true);
	TestTrue(TEXT("Group closes"), GridEventFeedPublisher::EndDungeonEventGroup(Scope.World));

	TestEqual(TEXT("Multiple consequences still consume one buffer slot"), Feed->GetEntries().Num(), 1);
	if (Feed->GetEntries().Num() == 1)
	{
		TestEqual(
			TEXT("Additional consequences use the middle-dot separator"),
			Feed->GetEntries()[0].Message.ToString(),
			FString(TEXT("Le levier est activé → Une porte s'ouvre · Un téléporteur s'active.")));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridUIEventHUD0112HiddenRootTest,
	"Grimrock.UI.EventHUD01.ConsequenceGrouping.HiddenRoot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIEventHUD0112HiddenRootTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridEventHUD0112WorldScope Scope;
	UGridEventFeedSubsystem* Feed = Scope.World ? Scope.World->GetSubsystem<UGridEventFeedSubsystem>() : nullptr;
	if (!TestNotNull(TEXT("Event Feed subsystem exists"), Feed))
	{
		return false;
	}

	TestTrue(TEXT("Unobservable source can still own a transient consequence group"), BeginLeverGroup(Scope.World, false));
	GridEventFeedPublisher::PublishDungeonEvent(
		Scope.World,
		FName(TEXT("Dungeon.Pit.Opened")),
		FText::FromString(TEXT("Vous entendez un mécanisme au loin.")),
		EGridHudEventSemantic::Danger,
		EGridEventVisibility::Always,
		true);
	TestTrue(TEXT("Perceptible consequence publishes group"), GridEventFeedPublisher::EndDungeonEventGroup(Scope.World));

	TestEqual(TEXT("One perceptible result is stored"), Feed->GetEntries().Num(), 1);
	if (Feed->GetEntries().Num() == 1)
	{
		TestEqual(
			TEXT("Invisible source is not leaked into the player-facing message"),
			Feed->GetEntries()[0].Message.ToString(),
			FString(TEXT("Vous entendez un mécanisme au loin.")));
		TestEqual(TEXT("Perceived consequence supplies the semantic when the cause is hidden"),
			Feed->GetEntries()[0].Semantic, EGridHudEventSemantic::Danger);
		TestEqual(TEXT("Hidden source identity is not leaked"), Feed->GetEntries()[0].SourceId, NAME_None);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridUIEventHUD0112DelayedConsequenceTest,
	"Grimrock.UI.EventHUD01.ConsequenceGrouping.DelayedConsequence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIEventHUD0112DelayedConsequenceTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridEventHUD0112WorldScope Scope;
	UGridEventFeedSubsystem* Feed = Scope.World ? Scope.World->GetSubsystem<UGridEventFeedSubsystem>() : nullptr;
	if (!TestNotNull(TEXT("Event Feed subsystem exists"), Feed))
	{
		return false;
	}

	TestTrue(TEXT("Group opens"), BeginLeverGroup(Scope.World));
	TestTrue(TEXT("Root-only group closes"), GridEventFeedPublisher::EndDungeonEventGroup(Scope.World));
	TestTrue(
		TEXT("Later asynchronous event publishes independently"),
		GridEventFeedPublisher::PublishDungeonEvent(
			Scope.World,
			FName(TEXT("Dungeon.Door.Opened")),
			FText::FromString(TEXT("Une porte s'ouvre.")),
			EGridHudEventSemantic::Mechanism,
			EGridEventVisibility::Always,
			true));

	TestEqual(TEXT("Delayed consequence remains a second logical event"), Feed->GetEntries().Num(), 2);
	if (Feed->GetEntries().Num() == 2)
	{
		TestEqual(TEXT("Root keeps first sequence"), Feed->GetEntries()[0].SequenceId, int64(1));
		TestEqual(TEXT("Delayed event gets next sequence"), Feed->GetEntries()[1].SequenceId, int64(2));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridUIEventHUD0112DoorPerceptionWordingTest,
	"Grimrock.UI.EventHUD01.ConsequenceGrouping.DoorPerceptionWording",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIEventHUD0112DoorPerceptionWordingTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const FName DoorOpened(TEXT("Dungeon.Door.Opened"));
	const FText LegacySpecific = FText::FromString(TEXT("La porte s'ouvre."));

	TestEqual(
		TEXT("Nearby visible door uses indefinite spatial wording"),
		GridEventFeedPublisher::ResolvePerceivedConsequenceText(
			DoorOpened, LegacySpecific, EGridDungeonConsequencePerception::VisibleNear).ToString(),
		FString(TEXT("Une porte devant vous s'ouvre.")));

	TestEqual(
		TEXT("Distant visible door is described as distant without false identity"),
		GridEventFeedPublisher::ResolvePerceivedConsequenceText(
			DoorOpened, LegacySpecific, EGridDungeonConsequencePerception::VisibleDistant).ToString(),
		FString(TEXT("Une porte s'ouvre au fond du couloir.")));

	TestEqual(
		TEXT("Non-visible audible door is described through hearing"),
		GridEventFeedPublisher::ResolvePerceivedConsequenceText(
			DoorOpened, LegacySpecific, EGridDungeonConsequencePerception::Audible).ToString(),
		FString(TEXT("Vous entendez une porte s'ouvrir au loin.")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridUIEventHUD0112SecretAntiSpoilerTest,
	"Grimrock.UI.EventHUD01.ConsequenceGrouping.SecretAntiSpoiler",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIEventHUD0112SecretAntiSpoilerTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const FName SecretOpened(TEXT("Dungeon.SecretDoor.Opened"));
	const FText Fallback = FText::FromString(TEXT("Une porte secrète s'ouvre."));

	const FString Audible = GridEventFeedPublisher::ResolvePerceivedConsequenceText(
		SecretOpened, Fallback, EGridDungeonConsequencePerception::Audible).ToString();
	TestEqual(TEXT("Audible secret consequence stays generic"), Audible, FString(TEXT("Vous entendez un mécanisme au loin.")));
	TestFalse(TEXT("Audible secret consequence never says secret"), Audible.Contains(TEXT("secret"), ESearchCase::IgnoreCase));
	TestFalse(TEXT("Audible secret consequence never identifies a door"), Audible.Contains(TEXT("porte"), ESearchCase::IgnoreCase));

	const FString Visible = GridEventFeedPublisher::ResolvePerceivedConsequenceText(
		SecretOpened, Fallback, EGridDungeonConsequencePerception::VisibleDistant).ToString();
	TestFalse(TEXT("Visible secret movement still avoids the word secret"), Visible.Contains(TEXT("secret"), ESearchCase::IgnoreCase));

	TestTrue(
		TEXT("Imperceptible secret consequence produces no player-facing text"),
		GridEventFeedPublisher::ResolvePerceivedConsequenceText(
			SecretOpened, Fallback, EGridDungeonConsequencePerception::Hidden).IsEmpty());
	return true;
}

#endif
