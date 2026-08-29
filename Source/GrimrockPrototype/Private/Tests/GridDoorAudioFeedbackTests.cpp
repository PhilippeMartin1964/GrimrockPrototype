#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridObjectArchetypeAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridDoorActor.h"
#include "Sound/SoundWave.h"

namespace
{
	struct FGridDoorAudioTestWorld
	{
		UWorld* World = nullptr;

		FGridDoorAudioTestWorld()
		{
			const UWorld::InitializationValues Values = UWorld::InitializationValues()
				.AllowAudioPlayback(false)
				.RequiresHitProxies(false)
				.CreatePhysicsScene(false)
				.CreateNavigation(false)
				.CreateAISystem(false)
				.ShouldSimulatePhysics(false)
				.SetTransactional(false);

			World = UWorld::CreateWorld(EWorldType::Game, false,
				FName(*FString::Printf(TEXT("DoorAudioWorld_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true,
				ERHIFeatureLevel::Num, &Values);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FGridDoorAudioTestWorld()
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridDoorAudioFeedbackTest,
	"Grimrock.Runtime.Doors.AudioFeedback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridDoorAudioFeedbackTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridDoorAudioTestWorld TestWorld;
	TestNotNull(TEXT("The transient door-audio world exists"), TestWorld.World);
	if (!TestWorld.World)
	{
		return false;
	}

	UGridObjectArchetypeAsset* Archetype = NewObject<UGridObjectArchetypeAsset>(TestWorld.World);
	Archetype->ArchetypeId = TEXT("Door_Audio_Test");
	Archetype->SupportedType = EGridLevelObjectType::Door;
	FGridObjectAudioEvent OpenEvent;
	OpenEvent.Volume = 0.8f;
	OpenEvent.PitchVariation = 0.0f;
	OpenEvent.Sounds.Add(NewObject<USoundWave>(Archetype));
	OpenEvent.Sounds.Add(NewObject<USoundWave>(Archetype));
	Archetype->AudioEvents.Add(TEXT("Open"), OpenEvent);

	FGridObjectAudioEvent CloseEvent;
	CloseEvent.Volume = 0.8f;
	CloseEvent.PitchVariation = 0.0f;
	CloseEvent.Sounds.Add(NewObject<USoundWave>(Archetype));
	Archetype->AudioEvents.Add(TEXT("Close"), CloseEvent);

	AGridDoorActor* Door = TestWorld.World->SpawnActor<AGridDoorActor>();
	TestNotNull(TEXT("The door Actor exists"), Door);
	if (!Door)
	{
		return false;
	}

	FGridLevelObjectData Data;
	Data.ObjectId = FGuid::NewGuid();
	Data.Type = EGridLevelObjectType::Door;
	Data.CellX = 1;
	Data.CellY = 1;
	Data.Edge = EGridEdge::North;
	Data.Behavior.DoorAnimation.OpenHeight = 180.f;
	Data.Behavior.DoorAnimation.MoveDuration = 1.f;

	Door->InitializeDoor(Data, nullptr, nullptr, nullptr, nullptr, FVector::ZeroVector, FRotator::ZeroRotator, false);
	Door->ConfigureObjectAudio(Archetype);
	Door->bNativeDoorAudioPlaybackEnabled = false;

	TestEqual(TEXT("No opening request exists after initialization"), Door->DoorOpenAudioPlaybackRequestCount, 0);
	TestEqual(TEXT("No closing request exists after initialization"), Door->DoorCloseAudioPlaybackRequestCount, 0);
	TestEqual(TEXT("No audio stop exists after initialization"), Door->DoorAudioStopRequestCount, 0);
	TestFalse(TEXT("No movement audio is active after initialization"), Door->bDoorMotionAudioActive);

	Door->OpenDoor();
	TestTrue(TEXT("Open starts real animation"), Door->IsAnimating());
	TestEqual(TEXT("Open movement requests exactly one opening sound"), Door->DoorOpenAudioPlaybackRequestCount, 1);
	TestEqual(TEXT("Open movement does not request a closing sound"), Door->DoorCloseAudioPlaybackRequestCount, 0);
	TestTrue(TEXT("Opening movement owns an active audio voice"), Door->bDoorMotionAudioActive);
	TestTrue(TEXT("The active voice is marked as opening"), Door->bDoorMotionAudioOpening);

	Door->OpenDoor();
	TestEqual(TEXT("Repeated Open toward the active target does not replay audio"), Door->DoorOpenAudioPlaybackRequestCount, 1);

	// Let the door physically leave its closed endpoint before asking for the
	// reversal. An immediate Open -> Close in the same frame has no closing
	// travel to perform, so correctly produces no closing sound.
	Door->Tick(0.25f);
	TestTrue(TEXT("The opening is still in progress after partial travel"), Door->IsAnimating());

	Door->CloseDoor();
	TestTrue(TEXT("Close reverses the physically active motion"), Door->IsAnimating());
	TestEqual(TEXT("A genuine reversal requests exactly one closing sound"), Door->DoorCloseAudioPlaybackRequestCount, 1);
	TestEqual(TEXT("Reversal explicitly stops the previous opening voice"), Door->DoorAudioStopRequestCount, 1);
	TestTrue(TEXT("Closing movement owns the replacement audio voice"), Door->bDoorMotionAudioActive);
	TestFalse(TEXT("The replacement voice is marked as closing"), Door->bDoorMotionAudioOpening);

	Door->CloseDoor();
	TestEqual(TEXT("Repeated Close toward the active target does not replay audio"), Door->DoorCloseAudioPlaybackRequestCount, 1);

	Door->SnapDoorOpenState(true);
	TestEqual(TEXT("Snap stops the active closing voice"), Door->DoorAudioStopRequestCount, 2);
	TestFalse(TEXT("Snap leaves no movement voice active"), Door->bDoorMotionAudioActive);
	Door->SnapDoorOpenState(false);
	TestEqual(TEXT("A second silent Snap does not invent another stop"), Door->DoorAudioStopRequestCount, 2);
	TestEqual(TEXT("State restoration never replays opening audio"), Door->DoorOpenAudioPlaybackRequestCount, 1);
	TestEqual(TEXT("State restoration never replays closing audio"), Door->DoorCloseAudioPlaybackRequestCount, 1);

	// Pressure-plate style immediate reversal: Open starts a voice, but Close is
	// requested before the first movement Tick. The Open voice must be cut and
	// no Close voice should start because the door never physically left Closed.
	Door->OpenDoor();
	TestEqual(TEXT("Immediate-reversal setup requests a second opening voice"), Door->DoorOpenAudioPlaybackRequestCount, 2);
	TestTrue(TEXT("Immediate-reversal setup has active opening audio"), Door->bDoorMotionAudioActive);
	Door->CloseDoor();
	TestFalse(TEXT("Immediate reverse at the closed endpoint stops animation"), Door->IsAnimating());
	TestEqual(TEXT("Immediate reverse cuts the opening voice"), Door->DoorAudioStopRequestCount, 3);
	TestEqual(TEXT("No closing voice is started without closing travel"), Door->DoorCloseAudioPlaybackRequestCount, 1);
	TestFalse(TEXT("Immediate reverse leaves no orphaned audio"), Door->bDoorMotionAudioActive);

	// Normal completion releases movement authority without an artificial hard stop.
	// The test SoundWave has no authored duration, which intentionally exercises
	// the safe natural-completion path used when duration metadata is unavailable.
	Door->OpenDoor();
	TestTrue(TEXT("Natural-completion setup has active audio"), Door->bDoorMotionAudioActive);
	Door->Tick(2.0f);
	TestFalse(TEXT("Completed door motion is no longer animating"), Door->IsAnimating());
	TestFalse(TEXT("Completed door motion releases logical audio ownership"), Door->bDoorMotionAudioActive);
	TestEqual(TEXT("Normal endpoint does not add an interruption stop"), Door->DoorAudioStopRequestCount, 3);
	TestEqual(TEXT("Normal endpoint records natural completion"), Door->DoorAudioNaturalCompletionCount, 1);

	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridDoorMoveDurationContractTest,
	"Grimrock.Runtime.Doors.MoveDurationContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridDoorMoveDurationContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridDoorAudioTestWorld TestWorld;
	if (!TestWorld.World)
	{
		return false;
	}

	AGridDoorActor* Door = TestWorld.World->SpawnActor<AGridDoorActor>();
	TestNotNull(TEXT("The duration-contract door exists"), Door);
	if (!Door)
	{
		return false;
	}

	FGridLevelObjectData Data;
	Data.ObjectId = FGuid::NewGuid();
	Data.Type = EGridLevelObjectType::Door;
	Data.CellX = 2;
	Data.CellY = 2;
	Data.Edge = EGridEdge::East;
	Data.Behavior.DoorAnimation.OpenHeight = 180.f;
	Data.Behavior.DoorAnimation.MoveDuration = 5.0f;

	Door->InitializeDoor(Data, nullptr, nullptr, nullptr, nullptr, FVector::ZeroVector, FRotator::ZeroRotator, false);
	Door->bNativeDoorAudioPlaybackEnabled = false;

	TestTrue(TEXT("Runtime actor copies the placed instance MoveDuration=5.0"), FMath::IsNearlyEqual(Door->MoveDuration, 5.0f));

	Door->OpenDoor();
	TestTrue(TEXT("Full opening starts"), Door->IsAnimating());

	Door->Tick(4.9f);
	TestTrue(TEXT("A 5-second door is still moving at 4.9 seconds"), Door->IsAnimating());

	Door->Tick(0.11f);
	TestFalse(TEXT("A 5-second door is complete after crossing 5.0 seconds"), Door->IsAnimating());
	TestTrue(TEXT("The door is fully open after the five-second travel"), Door->IsFullyOpen());

	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridDoorNaturalTailContractTest,
	"Grimrock.Runtime.Doors.NaturalAudioTail",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridDoorNaturalTailContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridDoorAudioTestWorld TestWorld;
	if (!TestWorld.World)
	{
		return false;
	}

	UGridObjectArchetypeAsset* Archetype = NewObject<UGridObjectArchetypeAsset>(TestWorld.World);
	Archetype->ArchetypeId = TEXT("Door_NaturalTail_Test");
	Archetype->SupportedType = EGridLevelObjectType::Door;
	FGridObjectAudioEvent OpenEvent;
	OpenEvent.Sounds.Add(NewObject<USoundWave>(Archetype));
	Archetype->AudioEvents.Add(TEXT("Open"), OpenEvent);

	FGridObjectAudioEvent CloseEvent;
	CloseEvent.Sounds.Add(NewObject<USoundWave>(Archetype));
	Archetype->AudioEvents.Add(TEXT("Close"), CloseEvent);

	AGridDoorActor* Door = TestWorld.World->SpawnActor<AGridDoorActor>();
	TestNotNull(TEXT("Natural-tail door exists"), Door);
	if (!Door)
	{
		return false;
	}

	FGridLevelObjectData Data;
	Data.ObjectId = FGuid::NewGuid();
	Data.Type = EGridLevelObjectType::Door;
	Data.CellX = 3;
	Data.CellY = 3;
	Data.Edge = EGridEdge::North;
	Data.Behavior.DoorAnimation.OpenHeight = 180.f;
	Data.Behavior.DoorAnimation.MoveDuration = 1.0f;

	Door->InitializeDoor(Data, nullptr, nullptr, nullptr, nullptr, FVector::ZeroVector, FRotator::ZeroRotator, false);
	Door->ConfigureObjectAudio(Archetype);
	Door->bNativeDoorAudioPlaybackEnabled = false;

	Door->OpenDoor();
	TestTrue(TEXT("Opening owns movement audio before endpoint"), Door->bDoorMotionAudioActive);
	const int32 StopCountBeforeEndpoint = Door->DoorAudioStopRequestCount;

	Door->Tick(1.01f);
	TestTrue(TEXT("Door reaches its physical endpoint"), Door->IsFullyOpen());
	TestFalse(TEXT("Endpoint releases logical movement-audio ownership"), Door->bDoorMotionAudioActive);
	TestEqual(TEXT("Normal endpoint never issues an audio stop"), Door->DoorAudioStopRequestCount, StopCountBeforeEndpoint);
	TestEqual(TEXT("Normal endpoint records a natural audio tail"), Door->DoorAudioNaturalCompletionCount, 1);

	return true;
}

#endif
