#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridObjectArchetypeAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridRuntimeObjectActor.h"
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

	// Prove the contract is not door-specific: a Button archetype can define and
	// resolve an arbitrary Press event through the shared runtime base class.
	UGridObjectArchetypeAsset* ButtonArchetype = NewObject<UGridObjectArchetypeAsset>(TestWorld.World);
	ButtonArchetype->ArchetypeId = TEXT("Button_GenericAudio_Test");
	ButtonArchetype->SupportedType = EGridLevelObjectType::Button;

	USoundWave* PressSoundA = NewObject<USoundWave>(ButtonArchetype);
	USoundWave* PressSoundB = NewObject<USoundWave>(ButtonArchetype);
	FGridObjectAudioEvent PressEvent;
	PressEvent.Volume = 0.65f;
	PressEvent.PitchVariation = 0.0f;
	PressEvent.Sounds.Add(PressSoundA);
	PressEvent.Sounds.Add(PressSoundB);
	ButtonArchetype->AudioEvents.Add(TEXT("Press"), PressEvent);

	AGridRuntimeObjectActor* RuntimeObject = TestWorld.World->SpawnActor<AGridRuntimeObjectActor>();
	TestNotNull(TEXT("The generic runtime object exists"), RuntimeObject);
	if (!RuntimeObject)
	{
		return false;
	}
	RuntimeObject->ConfigureObjectAudio(ButtonArchetype);

	TestTrue(TEXT("A non-door runtime object exposes its configured Press event"), RuntimeObject->HasObjectAudioEvent(TEXT("Press")));
	TestFalse(TEXT("An undeclared event is not invented"), RuntimeObject->HasObjectAudioEvent(TEXT("Open")));

	const FGridObjectAudioPlaybackResult First = RuntimeObject->PlayObjectAudioEventDetailed(TEXT("Press"), false);
	const FGridObjectAudioPlaybackResult Second = RuntimeObject->PlayObjectAudioEventDetailed(TEXT("Press"), false);
	TestTrue(TEXT("First generic playback request resolves"), First.bRequested);
	TestTrue(TEXT("Second generic playback request resolves"), Second.bRequested);
	TestTrue(TEXT("Generic event variants advance deterministically"), First.Sound == PressSoundA && Second.Sound == PressSoundB);
	TestTrue(TEXT("Mechanical-safe zero pitch variation preserves pitch 1.0"), FMath::IsNearlyEqual(First.Pitch, 1.0f));

	// Backward compatibility: already-saved door archetypes using the historical
	// fields still resolve as generic Open/Close until they are resaved/migrated.
	UGridObjectArchetypeAsset* LegacyDoor = NewObject<UGridObjectArchetypeAsset>(TestWorld.World);
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
