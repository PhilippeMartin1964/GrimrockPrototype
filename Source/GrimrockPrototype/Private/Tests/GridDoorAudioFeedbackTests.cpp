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
	Archetype->DoorAudioVolume = 0.8f;
	Archetype->DoorAudioPitchVariation = 0.04f;
	Archetype->DoorOpenSounds.Add(NewObject<USoundWave>(Archetype));
	Archetype->DoorOpenSounds.Add(NewObject<USoundWave>(Archetype));
	Archetype->DoorCloseSounds.Add(NewObject<USoundWave>(Archetype));

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
	Door->ConfigureDoorAudio(Archetype);
	Door->bNativeDoorAudioPlaybackEnabled = false;

	TestEqual(TEXT("No opening request exists after initialization"), Door->DoorOpenAudioPlaybackRequestCount, 0);
	TestEqual(TEXT("No closing request exists after initialization"), Door->DoorCloseAudioPlaybackRequestCount, 0);

	Door->OpenDoor();
	TestTrue(TEXT("Open starts real animation"), Door->IsAnimating());
	TestEqual(TEXT("Open movement requests exactly one opening sound"), Door->DoorOpenAudioPlaybackRequestCount, 1);
	TestEqual(TEXT("Open movement does not request a closing sound"), Door->DoorCloseAudioPlaybackRequestCount, 0);

	Door->OpenDoor();
	TestEqual(TEXT("Repeated Open toward the active target does not replay audio"), Door->DoorOpenAudioPlaybackRequestCount, 1);

	Door->CloseDoor();
	TestTrue(TEXT("Close reverses the active motion"), Door->IsAnimating());
	TestEqual(TEXT("A genuine reversal requests exactly one closing sound"), Door->DoorCloseAudioPlaybackRequestCount, 1);

	Door->CloseDoor();
	TestEqual(TEXT("Repeated Close toward the active target does not replay audio"), Door->DoorCloseAudioPlaybackRequestCount, 1);

	Door->SnapDoorOpenState(true);
	Door->SnapDoorOpenState(false);
	TestEqual(TEXT("State restoration never replays opening audio"), Door->DoorOpenAudioPlaybackRequestCount, 1);
	TestEqual(TEXT("State restoration never replays closing audio"), Door->DoorCloseAudioPlaybackRequestCount, 1);

	return true;
}

#endif
