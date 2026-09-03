#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridItemDefinitionAsset.h"

namespace
{
	struct FGridItemWorldPhysicsTestWorld
	{
		UWorld* World = nullptr;

		FGridItemWorldPhysicsTestWorld()
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
				EWorldType::Game, false,
				FName(*FString::Printf(TEXT("ITEM_PHYSICS01_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
				nullptr, true, ERHIFeatureLevel::Num, &Values);

			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FGridItemWorldPhysicsTestWorld()
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridItemWorldPhysicsSettlingTest,
	"Grimrock.Items.ITEM_PHYSICS01.WorldPickupSettling",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridItemWorldPhysicsSettlingTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridItemWorldPhysicsTestWorld TestWorld;
	if (!TestNotNull(TEXT("World physics test world exists"), TestWorld.World))
	{
		return false;
	}

	UGridItemDefinitionAsset* Definition = NewObject<UGridItemDefinitionAsset>(TestWorld.World);
	Definition->ItemDefinitionId = TEXT("Gem_Blue_Physics_Test");
	Definition->ItemType = EGridItemType::Gem;
	Definition->Weight = 0.10f;
	Definition->bUseItemWeightAsWorldPhysicsMass = true;
	Definition->WorldPhysicsInitialTiltDegrees = 2.0f;
	Definition->WorldPhysicsInitialAngularSpeedDegrees = 120.0f;
	TestTrue(TEXT("Opt-in gem world physics definition is valid"), Definition->IsValidDefinition());

	const FGuid RuntimeId = FGuid::NewGuid();
	AGridItemActor* FirstActor = TestWorld.World->SpawnActor<AGridItemActor>();
	if (!TestNotNull(TEXT("First item actor exists"), FirstActor))
	{
		return false;
	}
	FirstActor->InitializeFromItemDefinition(Definition, RuntimeId);
	const FQuat BeforeNudge = FirstActor->GetActorQuat();
	FirstActor->ApplyWorldPhysicsInitialNudge();
	const FQuat AfterNudge = FirstActor->GetActorQuat();
	const float AppliedTiltDegrees = FMath::RadiansToDegrees(BeforeNudge.AngularDistance(AfterNudge));
	TestTrue(TEXT("Configured fresh-placement tilt is applied"), FMath::IsNearlyEqual(AppliedTiltDegrees, 2.0f, 0.05f));

	AGridItemActor* SameIdActor = TestWorld.World->SpawnActor<AGridItemActor>();
	if (!TestNotNull(TEXT("Second item actor exists"), SameIdActor))
	{
		return false;
	}
	SameIdActor->InitializeFromItemDefinition(Definition, RuntimeId);
	SameIdActor->ApplyWorldPhysicsInitialNudge();
	TestTrue(TEXT("Fresh-placement tilt direction is deterministic for the same runtime identity"),
		SameIdActor->GetActorQuat().Equals(AfterNudge, 0.0001f));

	UGridItemDefinitionAsset* LegacyDefinition = NewObject<UGridItemDefinitionAsset>(TestWorld.World);
	LegacyDefinition->ItemDefinitionId = TEXT("Legacy_Item_No_Nudge");
	LegacyDefinition->Weight = 1.0f;
	AGridItemActor* LegacyActor = TestWorld.World->SpawnActor<AGridItemActor>();
	LegacyActor->InitializeFromItemDefinition(LegacyDefinition, FGuid::NewGuid());
	const FQuat LegacyRotation = LegacyActor->GetActorQuat();
	LegacyActor->ApplyWorldPhysicsInitialNudge();
	TestTrue(TEXT("Legacy definitions remain unchanged when initial tilt is zero"), LegacyActor->GetActorQuat().Equals(LegacyRotation));

	UGridItemDefinitionAsset* InvalidMassDefinition = NewObject<UGridItemDefinitionAsset>(TestWorld.World);
	InvalidMassDefinition->ItemDefinitionId = TEXT("Invalid_Zero_Mass");
	InvalidMassDefinition->Weight = 0.0f;
	InvalidMassDefinition->bUseItemWeightAsWorldPhysicsMass = true;
	TestFalse(TEXT("Mass override requires a positive finite item weight"), InvalidMassDefinition->IsValidDefinition());

	UGridItemDefinitionAsset* InvalidTiltDefinition = NewObject<UGridItemDefinitionAsset>(TestWorld.World);
	InvalidTiltDefinition->ItemDefinitionId = TEXT("Invalid_Tilt");
	InvalidTiltDefinition->Weight = 0.1f;
	InvalidTiltDefinition->WorldPhysicsInitialTiltDegrees = 50.0f;
	TestFalse(TEXT("Initial tilt outside the authored safety range is invalid"), InvalidTiltDefinition->IsValidDefinition());

	UGridItemDefinitionAsset* InvalidAngularSpeedDefinition = NewObject<UGridItemDefinitionAsset>(TestWorld.World);
	InvalidAngularSpeedDefinition->ItemDefinitionId = TEXT("Invalid_Angular_Speed");
	InvalidAngularSpeedDefinition->Weight = 0.1f;
	InvalidAngularSpeedDefinition->WorldPhysicsInitialAngularSpeedDegrees = 900.0f;
	TestFalse(TEXT("Initial angular speed outside the authored safety range is invalid"), InvalidAngularSpeedDefinition->IsValidDefinition());

	return true;
}

#endif
