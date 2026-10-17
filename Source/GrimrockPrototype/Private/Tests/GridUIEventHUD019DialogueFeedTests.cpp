#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridEventFeedPublisher.h"
#include "Runtime/GridEventFeedSubsystem.h"

namespace
{
	struct FGridEventHUD019WorldScope
	{
		UWorld* World = nullptr;

		FGridEventHUD019WorldScope()
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
				FName(*FString::Printf(TEXT("EVENTHUD019_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
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

		~FGridEventHUD019WorldScope()
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIEventHUD019NpcSpokenLineTest, "Grimrock.UI.EventHUD01.DialogueFeed.NpcSpokenLine",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIEventHUD019NpcSpokenLineTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridEventHUD019WorldScope Scope;
	if (!TestNotNull(TEXT("World exists"), Scope.World))
	{
		return false;
	}

	UGridEventFeedSubsystem* Feed = Scope.World->GetSubsystem<UGridEventFeedSubsystem>();
	if (!TestNotNull(TEXT("Event Feed subsystem exists"), Feed))
	{
		return false;
	}

	const FName SourceId(TEXT("Guardian_01"));
	TestTrue(
		TEXT("Actually spoken NPC line is published"),
		GridEventFeedPublisher::PublishNpcSpokenLine(
			Scope.World,
			FText::FromString(TEXT("Gardien")),
			FText::FromString(TEXT("Aucun mortel n'est autorisé à franchir cette porte.")),
			SourceId));

	TestEqual(TEXT("Exactly one logical dialogue event is stored"), Feed->GetEntries().Num(), 1);
	if (Feed->GetEntries().Num() == 1)
	{
		const FGridHudEventEntry& Entry = Feed->GetEntries()[0];
		TestEqual(TEXT("NPC line uses Discussion category"), Entry.Category, EGridHudEventCategory::Dialogue);
		TestEqual(TEXT("NPC line uses Dialogue semantic"), Entry.Semantic, EGridHudEventSemantic::Dialogue);
		TestEqual(TEXT("NPC line is explicitly player-facing"), Entry.Visibility, EGridEventVisibility::Always);
		TestEqual(TEXT("NPC line uses stable spoken event code"), Entry.EventCode, FName(TEXT("Dialogue.NPC.Spoken")));
		TestEqual(TEXT("Speaker is preserved"), Entry.Header.ToString(), FString(TEXT("Gardien")));
		TestEqual(
			TEXT("Spoken content is preserved verbatim"),
			Entry.Message.ToString(),
			FString(TEXT("Aucun mortel n'est autorisé à franchir cette porte.")));
		TestEqual(TEXT("Source identity is preserved"), Entry.SourceId, SourceId);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIEventHUD019PlayerSpokenReplyTest, "Grimrock.UI.EventHUD01.DialogueFeed.PlayerSpokenReply",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIEventHUD019PlayerSpokenReplyTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridEventHUD019WorldScope Scope;
	if (!TestNotNull(TEXT("World exists"), Scope.World))
	{
		return false;
	}

	UGridEventFeedSubsystem* Feed = Scope.World->GetSubsystem<UGridEventFeedSubsystem>();
	if (!TestNotNull(TEXT("Event Feed subsystem exists"), Feed))
	{
		return false;
	}

	// Offered choices are presentation data only. Merely having them must not
	// publish anything; only the explicitly selected/spoken reply below does.
	const TArray<FText> OfferedChoices = {
		FText::FromString(TEXT("Nous ne cherchons pas le combat.")),
		FText::FromString(TEXT("Écarte-toi de notre chemin."))
	};
	TestEqual(TEXT("Presenting dialogue choices does not publish them"), Feed->GetEntries().Num(), 0);
	TestEqual(TEXT("Fixture exposes two offered choices"), OfferedChoices.Num(), 2);

	const FName SpeakerId(TEXT("Party.Elias"));
	TestTrue(
		TEXT("Selected and spoken player reply is published"),
		GridEventFeedPublisher::PublishPlayerSpokenReply(
			Scope.World,
			FText::FromString(TEXT("Elias")),
			OfferedChoices[0],
			SpeakerId));

	TestEqual(TEXT("Only the selected reply enters the feed"), Feed->GetEntries().Num(), 1);
	if (Feed->GetEntries().Num() == 1)
	{
		const FGridHudEventEntry& Entry = Feed->GetEntries()[0];
		TestEqual(TEXT("Player reply uses Discussion category"), Entry.Category, EGridHudEventCategory::Dialogue);
		TestEqual(TEXT("Player reply uses Dialogue semantic"), Entry.Semantic, EGridHudEventSemantic::Dialogue);
		TestEqual(TEXT("Player reply uses stable spoken event code"), Entry.EventCode, FName(TEXT("Dialogue.Player.Spoken")));
		TestEqual(TEXT("Player speaker is preserved"), Entry.Header.ToString(), FString(TEXT("Elias")));
		TestEqual(TEXT("Selected spoken reply is preserved"), Entry.Message.ToString(), OfferedChoices[0].ToString());
		TestNotEqual(TEXT("Unselected choice was not stored"), Entry.Message.ToString(), OfferedChoices[1].ToString());
		TestEqual(TEXT("Player speaker identity is preserved"), Entry.SourceId, SpeakerId);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIEventHUD019DialogueValidationTest, "Grimrock.UI.EventHUD01.DialogueFeed.Validation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIEventHUD019DialogueValidationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridEventHUD019WorldScope Scope;
	if (!TestNotNull(TEXT("World exists"), Scope.World))
	{
		return false;
	}

	UGridEventFeedSubsystem* Feed = Scope.World->GetSubsystem<UGridEventFeedSubsystem>();
	if (!TestNotNull(TEXT("Event Feed subsystem exists"), Feed))
	{
		return false;
	}

	TestFalse(
		TEXT("Anonymous NPC speech is rejected"),
		GridEventFeedPublisher::PublishNpcSpokenLine(
			Scope.World, FText::GetEmpty(), FText::FromString(TEXT("Texte")), NAME_None));
	TestFalse(
		TEXT("Empty NPC speech is rejected"),
		GridEventFeedPublisher::PublishNpcSpokenLine(
			Scope.World, FText::FromString(TEXT("Gardien")), FText::GetEmpty(), NAME_None));
	TestFalse(
		TEXT("Anonymous player reply is rejected"),
		GridEventFeedPublisher::PublishPlayerSpokenReply(
			Scope.World, FText::GetEmpty(), FText::FromString(TEXT("Réponse")), NAME_None));
	TestFalse(
		TEXT("Empty player reply is rejected"),
		GridEventFeedPublisher::PublishPlayerSpokenReply(
			Scope.World, FText::FromString(TEXT("Elias")), FText::GetEmpty(), NAME_None));

	TestEqual(TEXT("Rejected dialogue payloads never enter the feed"), Feed->GetEntries().Num(), 0);
	return true;
}

#endif
