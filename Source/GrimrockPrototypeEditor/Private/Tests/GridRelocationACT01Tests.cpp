#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridActivationComponent.h"
#include "Runtime/GridLevelRuntimeActor.h"

namespace
{
	struct FRelocationACT01TestWorld
	{
		UWorld* World = nullptr;

		FRelocationACT01TestWorld()
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
				FName(*FString::Printf(TEXT("RELOCACT01TestWorld_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true,
				ERHIFeatureLevel::Num, &Values);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FRelocationACT01TestWorld()
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridRelocationACT01CommandsTest, "Grimrock.Relocation.ACT01.Commands", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridRelocationACT01CommandsTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FRelocationACT01TestWorld Fixture;
	if (!TestNotNull(TEXT("World"), Fixture.World))
	{
		return false;
	}

	AGridLevelRuntimeActor* Runtime = Fixture.World->SpawnActor<AGridLevelRuntimeActor>();
	if (!TestNotNull(TEXT("Runtime"), Runtime))
	{
		return false;
	}

	UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Runtime);
	Runtime->LevelAsset = Level;

	UGridActivationComponent* Activation = Runtime->FindComponentByClass<UGridActivationComponent>();
	if (!TestNotNull(TEXT("Activation component"), Activation))
	{
		return false;
	}
	Activation->Initialize(Runtime);
	Activation->ResetRuntimeState();

	FGridWorldObjectInstance SourceButton;
	SourceButton.InstanceId = FGuid::NewGuid();
	SourceButton.Type = EGridLevelObjectType::Button;
	SourceButton.CellX = 0;
	SourceButton.CellY = 0;

	FGridWorldObjectInstance Relocation;
	Relocation.InstanceId = FGuid::NewGuid();
	Relocation.Type = EGridLevelObjectType::Relocation;
	Relocation.CellX = 1;
	Relocation.CellY = 0;
	Relocation.InstanceConfig.bRelocationInitiallyEnabled = false;

	Level->WorldObjectInstances = { SourceButton, Relocation };

	FGridObjectLink Link;
	Link.SourceObjectId = SourceButton.InstanceId;
	Link.TargetObjectId = Relocation.InstanceId;
	Link.SourceEvent = EGridObjectEvent::Activated;
	Link.Command = EGridObjectCommand::Activate;
	Level->Links = { Link };

	Activation->RegisterInitialObjectState(Relocation);
	Activation->RebuildIndexes();
	TestFalse(TEXT("Relocation starts inactive"), Activation->IsObjectActive(Relocation.InstanceId));

	TestTrue(TEXT("Activate command is applied"), Activation->ExecuteLinksFromObjectForEvent(SourceButton.InstanceId, EGridObjectEvent::Activated));
	TestTrue(TEXT("Activate command enables relocation"), Activation->IsObjectActive(Relocation.InstanceId));

	Level->Links[0].Command = EGridObjectCommand::Deactivate;
	TestTrue(TEXT("Deactivate command is applied"), Activation->ExecuteLinksFromObjectForEvent(SourceButton.InstanceId, EGridObjectEvent::Activated));
	TestFalse(TEXT("Deactivate command disables relocation"), Activation->IsObjectActive(Relocation.InstanceId));

	Level->Links[0].Command = EGridObjectCommand::Toggle;
	TestTrue(TEXT("First Toggle command is applied"), Activation->ExecuteLinksFromObjectForEvent(SourceButton.InstanceId, EGridObjectEvent::Activated));
	TestTrue(TEXT("First Toggle enables relocation"), Activation->IsObjectActive(Relocation.InstanceId));
	TestTrue(TEXT("Second Toggle command is applied"), Activation->ExecuteLinksFromObjectForEvent(SourceButton.InstanceId, EGridObjectEvent::Activated));
	TestFalse(TEXT("Second Toggle disables relocation"), Activation->IsObjectActive(Relocation.InstanceId));

	return true;
}

#endif
