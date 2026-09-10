#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/StaticMeshComponent.h"
#include "Core/GridLevelPlacementTypes.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Core/GridWorldObjectInstanceVisual.h"
#include "Core/GridWorldObjectVisual.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Runtime/GridButtonActor.h"
#include "Runtime/GridRuntimeWorldObjectData.h"
#include "UObject/UnrealType.h"

namespace
{
	struct FRecoveryC2TestWorld
	{
		UWorld* World = nullptr;

		FRecoveryC2TestWorld()
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
				FName(*FString::Printf(TEXT("RecoveryC2_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
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

		~FRecoveryC2TestWorld()
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

	UStaticMeshComponent* FindStaticMeshComponentByName(AActor* Actor, FName ComponentName)
	{
		if (!Actor)
		{
			return nullptr;
		}

		TArray<UStaticMeshComponent*> Components;
		Actor->GetComponents<UStaticMeshComponent>(Components);
		for (UStaticMeshComponent* Component : Components)
		{
			if (Component && Component->GetFName() == ComponentName)
			{
				return Component;
			}
		}
		return nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRecoveryC2ReverseMotionDurationTest,
	"Grimrock.WorldObjects.RECOVERY01.C2.ReverseMotionDuration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRecoveryC2ReverseMotionDurationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	// 1) Generic reflected motion contract.
	UScriptStruct* MotionStruct = FGridWorldObjectMotion::StaticStruct();
	if (!TestNotNull(TEXT("Generic world-object motion struct exists"), MotionStruct))
	{
		return false;
	}

	TestNotNull(TEXT("Motion exposes Duration"), MotionStruct->FindPropertyByName(TEXT("Duration")));
	TestNotNull(TEXT("Motion exposes ReverseDuration"), MotionStruct->FindPropertyByName(TEXT("ReverseDuration")));

	FGridWorldObjectMotion Motion;
	Motion.Type = EGridWorldObjectMotionType::Translation;
	Motion.Axis = EGridWorldObjectMotionAxis::X;
	Motion.Amount = 6.0f;
	Motion.Duration = 0.08f;
	Motion.ReverseDuration = 0.0f;

	TestEqual(TEXT("Forward duration uses Duration"), Motion.GetDuration(false), 0.08f);
	TestEqual(TEXT("ReverseDuration <= 0 falls back to Duration"), Motion.GetDuration(true), 0.08f);

	Motion.ReverseDuration = 0.10f;
	TestEqual(TEXT("Positive ReverseDuration is used for Alpha 1->0"), Motion.GetDuration(true), 0.10f);
	TestEqual(TEXT("ReverseDuration never changes forward timing"), Motion.GetDuration(false), 0.08f);

	// 2) C1.1 sparse forward-duration overrides still define reverse timing when no explicit reverse duration exists.
	FGridWorldObjectMovingPart DefinitionPart;
	DefinitionPart.Mesh = NewObject<UStaticMesh>(GetTransientPackage());
	DefinitionPart.Motion = Motion;
	DefinitionPart.Motion.ReverseDuration = 0.0f;

	FGridWorldObjectMovingPartInstanceOverride DurationOverride;
	DurationOverride.PartIndex = 0;
	DurationOverride.bOverrideMotionDuration = true;
	DurationOverride.MotionDuration = 2.50f;

	const TArray<FGridWorldObjectMovingPartInstanceOverride> Overrides{DurationOverride};
	const FGridWorldObjectMovingPart ResolvedPart =
		GridWorldObjectInstanceVisual::ResolveMovingPart(DefinitionPart, Overrides, 0);

	TestEqual(TEXT("C1.1 forward override remains effective"), ResolvedPart.Motion.Duration, 2.50f);
	TestEqual(
		TEXT("Reverse fallback follows the C1.1 overridden forward duration"),
		ResolvedPart.Motion.GetDuration(true),
		2.50f);

	// 3) Runtime button consumes the generic asymmetric timing, with no button-specific animation struct restored.
	FRecoveryC2TestWorld TestWorld;
	if (!TestNotNull(TEXT("C2 transient world exists"), TestWorld.World))
	{
		return false;
	}

	UGridWorldObjectDefinitionAsset* Definition = NewObject<UGridWorldObjectDefinitionAsset>(TestWorld.World);
	Definition->DefinitionId = TEXT("RecoveryC2_Button");
	Definition->SupportedType = EGridLevelObjectType::Button;
	Definition->MovingParts.Part0.Mesh = NewObject<UStaticMesh>(Definition);
	Definition->MovingParts.Part0.LocalTransform = FTransform::Identity;
	Definition->MovingParts.Part0.Motion.Type = EGridWorldObjectMotionType::Translation;
	Definition->MovingParts.Part0.Motion.Axis = EGridWorldObjectMotionAxis::X;
	Definition->MovingParts.Part0.Motion.Amount = 6.0f;
	Definition->MovingParts.Part0.Motion.Duration = 0.08f;
	Definition->MovingParts.Part0.Motion.ReverseDuration = 0.10f;
	Definition->DefaultBehavior.ButtonAnimation.ButtonHoldTime = 0.15f;

	FGridWorldObjectInstance Instance;
	Instance.InstanceId = FGuid::NewGuid();
	Instance.Type = EGridLevelObjectType::Button;
	Instance.WorldObjectDefinitionId = Definition->DefinitionId;

	const FGridRuntimeWorldObjectData ObjectData(Instance);

	AGridButtonActor* Button = TestWorld.World->SpawnActor<AGridButtonActor>();
	if (!TestNotNull(TEXT("C2 runtime button exists"), Button))
	{
		return false;
	}

	Button->InitializeRuntimeMechanismVisuals(ObjectData, Definition, FTransform::Identity);
	Button->InitializeRuntimeWorldObject(ObjectData, nullptr, FTransform::Identity);

	TestEqual(TEXT("Button press uses forward 0.08s"), Button->PressDuration, 0.08f);
	TestEqual(TEXT("Button release uses reverse 0.10s"), Button->ReleaseDuration, 0.10f);
	TestEqual(TEXT("Button gameplay hold remains 0.15s"), Button->HoldTime, 0.15f);

	UStaticMeshComponent* MovingMesh = FindStaticMeshComponentByName(Button, TEXT("MovingMesh"));
	if (!TestNotNull(TEXT("Button moving mesh exists"), MovingMesh))
	{
		return false;
	}

	Button->TriggerPress();
	Button->Tick(0.08f);
	TestTrue(
		TEXT("Button reaches full 6cm press after 0.08s"),
		MovingMesh->GetRelativeLocation().Equals(FVector(6.0f, 0.0f, 0.0f), 0.01f));

	Button->Tick(0.15f);
	TestTrue(
		TEXT("Button remains fully pressed through hold time"),
		MovingMesh->GetRelativeLocation().Equals(FVector(6.0f, 0.0f, 0.0f), 0.01f));

	Button->Tick(0.05f);
	TestTrue(
		TEXT("Button is halfway released after 0.05 of 0.10s"),
		MovingMesh->GetRelativeLocation().Equals(FVector(3.0f, 0.0f, 0.0f), 0.01f));

	Button->Tick(0.05f);
	TestTrue(
		TEXT("Button returns to rest after full 0.10s reverse travel"),
		MovingMesh->GetRelativeLocation().Equals(FVector::ZeroVector, 0.01f));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
