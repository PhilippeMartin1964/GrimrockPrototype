#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridButtonActor.h"
#include "Runtime/GridRuntimeObjectActor.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundWave.h"

namespace
{
	struct FGridObjectAudioTestWorld
	{
		UWorld* World = nullptr;

		FGridObjectAudioTestWorld()
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
				FName(*FString::Printf(TEXT("GenericObjectAudioWorld_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true,
				ERHIFeatureLevel::Num, &Values);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FGridObjectAudioTestWorld()
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridObjectGenericAudioContractTest,
	"Grimrock.Runtime.Objects.GenericAudioContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridObjectGenericAudioContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridObjectAudioTestWorld TestWorld;
	TestNotNull(TEXT("The transient generic-audio world exists"), TestWorld.World);
	if (!TestWorld.World)
	{
		return false;
	}

	// Prove the contract is not door-specific: a Button definition can define and
	// resolve its canonical Activated event through the shared runtime base class.
	UGridWorldObjectDefinitionAsset* ButtonDefinition = NewObject<UGridWorldObjectDefinitionAsset>(TestWorld.World);
	ButtonDefinition->DefinitionId = TEXT("Button_GenericAudio_Test");
	ButtonDefinition->SupportedType = EGridLevelObjectType::Button;
	USoundAttenuation* SharedAttenuation = NewObject<USoundAttenuation>(ButtonDefinition);
	ButtonDefinition->DefaultAudioAttenuation = SharedAttenuation;

	USoundWave* ActivatedSoundA = NewObject<USoundWave>(ButtonDefinition);
	USoundWave* ActivatedSoundB = NewObject<USoundWave>(ButtonDefinition);
	FGridObjectAudioEvent ActivatedEvent;
	ActivatedEvent.Volume = 0.65f;
	ActivatedEvent.PitchVariation = 0.0f;
	ActivatedEvent.Sounds.Add(ActivatedSoundA);
	ActivatedEvent.Sounds.Add(ActivatedSoundB);
	ButtonDefinition->AudioEvents.Add(TEXT("Activated"), ActivatedEvent);

	AGridRuntimeObjectActor* RuntimeObject = TestWorld.World->SpawnActor<AGridRuntimeObjectActor>();
	TestNotNull(TEXT("The generic runtime object exists"), RuntimeObject);
	if (!RuntimeObject)
	{
		return false;
	}
	RuntimeObject->ConfigureObjectAudio(ButtonDefinition);

	TestTrue(TEXT("A non-door runtime object uses the definition's single attenuation"), RuntimeObject->DefaultObjectAudioAttenuation == SharedAttenuation);
	TestTrue(TEXT("A Button runtime object exposes its configured Activated event"), RuntimeObject->HasObjectAudioEvent(TEXT("Activated")));
	TestFalse(TEXT("An undeclared event is not invented"), RuntimeObject->HasObjectAudioEvent(TEXT("Open")));

	const FGridObjectAudioPlaybackResult First = RuntimeObject->PlayObjectAudioEventDetailed(TEXT("Activated"), false, 1.25f);
	const FGridObjectAudioPlaybackResult Second = RuntimeObject->PlayObjectAudioEventDetailed(TEXT("Activated"), false);
	TestTrue(TEXT("First generic playback request resolves"), First.bRequested);
	TestTrue(TEXT("Second generic playback request resolves"), Second.bRequested);
	TestTrue(TEXT("Generic event variants advance deterministically"), First.Sound == ActivatedSoundA && Second.Sound == ActivatedSoundB);
	TestTrue(TEXT("Detailed playback preserves an explicit StartTime"), FMath::IsNearlyEqual(First.StartTimeSeconds, 1.25f));
	TestTrue(TEXT("Default detailed playback starts at zero"), FMath::IsNearlyEqual(Second.StartTimeSeconds, 0.0f));
	TestTrue(TEXT("Mechanical-safe zero pitch variation preserves pitch 1.0"), FMath::IsNearlyEqual(First.Pitch, 1.0f));

	// BUTTON-AUDIO01: prove the specialized Button actor actually emits the same
	// Activated audio vocabulary as Events & Actions when TriggerPress executes.
	AGridButtonActor* ButtonActor = TestWorld.World->SpawnActor<AGridButtonActor>();
	TestNotNull(TEXT("The audio-enabled Button actor exists"), ButtonActor);
	if (!ButtonActor)
	{
		return false;
	}
	ButtonActor->ConfigureObjectAudio(ButtonDefinition);
	ButtonActor->TriggerPress();
	const FGridObjectAudioPlaybackResult AfterTrigger = ButtonActor->PlayObjectAudioEventDetailed(TEXT("Activated"), false);
	TestTrue(TEXT("TriggerPress consumes the first Activated audio variant"), AfterTrigger.Sound == ActivatedSoundB);

	// Backward compatibility: already-saved door definitions using the historical
	// fields still resolve as generic Open/Close until they are resaved/migrated.
	UGridWorldObjectDefinitionAsset* LegacyDoor = NewObject<UGridWorldObjectDefinitionAsset>(TestWorld.World);
	LegacyDoor->SupportedType = EGridLevelObjectType::Door;
	USoundWave* LegacyOpen = NewObject<USoundWave>(LegacyDoor);
	LegacyDoor->DoorOpenSounds.Add(LegacyOpen);
	LegacyDoor->DoorAudioVolume = 0.75f;

	AGridRuntimeObjectActor* LegacyRuntimeObject = TestWorld.World->SpawnActor<AGridRuntimeObjectActor>();
	TestNotNull(TEXT("The legacy compatibility runtime object exists"), LegacyRuntimeObject);
	if (!LegacyRuntimeObject)
	{
		return false;
	}
	LegacyRuntimeObject->ConfigureObjectAudio(LegacyDoor);
	TestTrue(TEXT("Legacy door Open data transparently resolves through generic audio"), LegacyRuntimeObject->HasObjectAudioEvent(TEXT("Open")));

	const FGridObjectAudioPlaybackResult LegacyPlayback = LegacyRuntimeObject->PlayObjectAudioEventDetailed(TEXT("Open"), false);
	TestTrue(TEXT("Legacy door playback request survives migration"), LegacyPlayback.bRequested);
	TestTrue(TEXT("Legacy door sound survives migration"), LegacyPlayback.Sound == LegacyOpen);

	return true;
}

#endif
