#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridItemActor.h"
#include "GridCanonicalLootTestScope.h"
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

	FGridCanonicalLootTestScope Scope;
	if (!TestTrue(TEXT("Isolated item system registered"), Scope.AttachWithWeights(TestWorld.World, {},
		[&](UGridItemSystemAsset& Root)
		{
			auto* Definition = NewObject<UGridNonArmamentDefinitionAsset>(&Root);
			Definition->ItemId = TEXT("Gem_Blue_Physics_Test");
			Definition->WeightKg = 0.10f;
			Definition->bUseItemWeightAsWorldPhysicsMass = true;
			Definition->WorldPhysicsInitialTiltDegrees = 2.0f;
			Definition->DisplayName = FText::FromName(Definition->ItemId);
			Root.ItemDefinitions.Add(Definition);
			auto* NoTilt = NewObject<UGridNonArmamentDefinitionAsset>(&Root);
			NoTilt->ItemId = TEXT("WorldPhysics_NoTilt");
			NoTilt->DisplayName = FText::FromName(NoTilt->ItemId);
			NoTilt->WeightKg = 1.f;
			Root.ItemDefinitions.Add(NoTilt);
		})))
	{
		return false;
	}

	const FGuid RuntimeId = FGuid::NewGuid();
	AGridItemActor* FirstActor = TestWorld.World->SpawnActor<AGridItemActor>();
	if (!TestNotNull(TEXT("First item actor exists"), FirstActor))
	{
		return false;
	}
	FGridItemInstance Instance;
	Instance.ItemDefinitionId = TEXT("Gem_Blue_Physics_Test");
	Instance.RuntimeObjectId = RuntimeId;
	Instance.Quantity = 1;
	Instance.OwnerType = EGridItemOwnerType::World;
	if (!TestTrue(TEXT("Tilt fixture initializes"), FirstActor->InitializeFromItemInstance(Instance, Scope.Registry))) return false;
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
	if (!TestTrue(TEXT("Same identity initializes"), SameIdActor->InitializeFromItemInstance(Instance, Scope.Registry))) return false;
	SameIdActor->ApplyWorldPhysicsInitialNudge();
	TestTrue(TEXT("Fresh-placement tilt direction is deterministic for the same runtime identity"),
		SameIdActor->GetActorQuat().Equals(AfterNudge, 0.0001f));

	AGridItemActor* ZeroTiltActor = TestWorld.World->SpawnActor<AGridItemActor>();
	Instance.ItemDefinitionId = TEXT("WorldPhysics_NoTilt");
	Instance.RuntimeObjectId = FGuid::NewGuid();
	if (!TestNotNull(TEXT("Zero tilt actor"), ZeroTiltActor) ||
		!TestTrue(TEXT("Zero tilt fixture initializes"), ZeroTiltActor->InitializeFromItemInstance(Instance, Scope.Registry))) return false;
	const FQuat ZeroTiltRotation = ZeroTiltActor->GetActorQuat();
	ZeroTiltActor->ApplyWorldPhysicsInitialNudge();
	TestTrue(TEXT("Rotation remains unchanged when initial tilt is zero"), ZeroTiltActor->GetActorQuat().Equals(ZeroTiltRotation));

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

	return true;
}

#endif
