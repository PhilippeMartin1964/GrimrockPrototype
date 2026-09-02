#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/BoxComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterDeathComponent.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "UObject/UnrealType.h"

namespace GridMonsterDeathCollision01
{
	UGridMonsterDefinitionAsset* MakeValidDefinition()
	{
		UGridMonsterDefinitionAsset* Definition = NewObject<UGridMonsterDefinitionAsset>(GetTransientPackage());
		if (!Definition)
		{
			return nullptr;
		}
		Definition->MonsterId = TEXT("MON_DEATH_COLLISION01");
		Definition->DisplayName = FText::FromString(TEXT("Obstacle Aware Death Test"));
		Definition->CategoryId = TEXT("Test");
		Definition->MaxHealth = 1;
		Definition->ActionPointsPerTurn = 1;
		Definition->DeathExpectedDuration = 1.0f;
		return Definition;
	}

	struct FTestWorld
	{
		UWorld* World = nullptr;

		FTestWorld()
		{
			const UWorld::InitializationValues Values = UWorld::InitializationValues()
				.AllowAudioPlayback(false)
				.RequiresHitProxies(false)
				.CreatePhysicsScene(true)
				.CreateNavigation(false)
				.CreateAISystem(false)
				.ShouldSimulatePhysics(false)
				.SetTransactional(false);
			World = UWorld::CreateWorld(EWorldType::Game, false,
				FName(*FString::Printf(TEXT("MONDeathCollision01_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
				nullptr, true, ERHIFeatureLevel::Num, &Values);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FTestWorld()
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterDeathCollision01DefinitionTest,
	"Grimrock.Monsters.MON_DEATH_COLLISION01.DefinitionContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterDeathCollision01DefinitionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UGridMonsterDefinitionAsset* Definition = GridMonsterDeathCollision01::MakeValidDefinition();
	if (!TestNotNull(TEXT("Definition exists"), Definition))
	{
		return false;
	}

	TestFalse(TEXT("Obstacle-aware death is opt-in by default"), Definition->bEnableObstacleAwareDeath);
	TestTrue(TEXT("Default fall direction is backwards"), Definition->DeathFallLocalDirection.Equals(FVector(-1.0f, 0.0f, 0.0f)));

	Definition->bEnableObstacleAwareDeath = true;
	FString Error;
	TestTrue(TEXT("Default obstacle-aware death settings are valid"), Definition->ValidateDefinition(Error));

	Definition->DeathFallLocalDirection = FVector::ZeroVector;
	Error.Reset();
	TestFalse(TEXT("Zero fall direction is rejected"), Definition->ValidateDefinition(Error));
	TestTrue(TEXT("Fall direction validation names the field"), Error.Contains(TEXT("DeathFallLocalDirection")));

	Definition->DeathFallLocalDirection = FVector(-1.0f, 0.0f, 0.0f);
	Definition->DeathObstacleProbeHalfHeight = Definition->DeathObstacleProbeRadius - 1.0f;
	Error.Reset();
	TestFalse(TEXT("Capsule half-height smaller than radius is rejected"), Definition->ValidateDefinition(Error));
	TestTrue(TEXT("Probe validation reports obstacle dimensions"), Error.Contains(TEXT("probe dimensions")));

	Definition->DeathObstacleProbeHalfHeight = 60.0f;
	Definition->DeathRagdollBackwardSpeed = -1.0f;
	Error.Reset();
	TestFalse(TEXT("Negative ragdoll speed is rejected"), Definition->ValidateDefinition(Error));
	TestTrue(TEXT("Ragdoll validation reports speed contract"), Error.Contains(TEXT("ragdoll speeds")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterDeathCollision01ApiTest,
	"Grimrock.Monsters.MON_DEATH_COLLISION01.ApiContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterDeathCollision01ApiTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UClass* DefinitionClass = UGridMonsterDefinitionAsset::StaticClass();
	UClass* DeathClass = UGridMonsterDeathComponent::StaticClass();
	if (!TestNotNull(TEXT("Definition class exists"), DefinitionClass) || !TestNotNull(TEXT("Death component class exists"), DeathClass))
	{
		return false;
	}

	const FName DefinitionProperties[] = {
		GET_MEMBER_NAME_CHECKED(UGridMonsterDefinitionAsset, bEnableObstacleAwareDeath),
		GET_MEMBER_NAME_CHECKED(UGridMonsterDefinitionAsset, DeathFallLocalDirection),
		GET_MEMBER_NAME_CHECKED(UGridMonsterDefinitionAsset, DeathObstacleProbeDistance),
		GET_MEMBER_NAME_CHECKED(UGridMonsterDefinitionAsset, DeathObstacleProbeRadius),
		GET_MEMBER_NAME_CHECKED(UGridMonsterDefinitionAsset, DeathObstacleProbeHalfHeight),
		GET_MEMBER_NAME_CHECKED(UGridMonsterDefinitionAsset, DeathRagdollBackwardSpeed),
		GET_MEMBER_NAME_CHECKED(UGridMonsterDefinitionAsset, DeathRagdollDownwardSpeed),
		GET_MEMBER_NAME_CHECKED(UGridMonsterDefinitionAsset, DeathRagdollAngularSpeedDegrees)
	};
	for (const FName PropertyName : DefinitionProperties)
	{
		TestNotNull(FString::Printf(TEXT("Definition exposes %s"), *PropertyName.ToString()), FindFProperty<FProperty>(DefinitionClass, PropertyName));
	}

	TestNotNull(TEXT("Death component exposes obstacle detection state"),
		FindFProperty<FProperty>(DeathClass, GET_MEMBER_NAME_CHECKED(UGridMonsterDeathComponent, bDeathObstacleDetected)));
	TestNotNull(TEXT("Death component exposes ragdoll state"),
		FindFProperty<FProperty>(DeathClass, GET_MEMBER_NAME_CHECKED(UGridMonsterDeathComponent, bDeathRagdollActive)));
	TestNotNull(TEXT("Death component exposes transient collision guard diagnostics"),
		FindFProperty<FProperty>(DeathClass, GET_MEMBER_NAME_CHECKED(UGridMonsterDeathComponent, DeathCollisionGuardCount)));

	const FName RequiredFunctions[] = {
		TEXT("ResolveDeathFallWorldDirection"),
		TEXT("ProbeDeathObstacle"),
		TEXT("ResetDeathRagdollPresentation")
	};
	for (const FName FunctionName : RequiredFunctions)
	{
		TestNotNull(FString::Printf(TEXT("Death component exposes %s"), *FunctionName.ToString()), DeathClass->FindFunctionByName(FunctionName));
	}

	const UGridMonsterDeathComponent* CDO = GetDefault<UGridMonsterDeathComponent>();
	TestNotNull(TEXT("Death component CDO exists"), CDO);
	if (CDO)
	{
		TestFalse(TEXT("Obstacle-aware death adds no permanent component tick"), CDO->PrimaryComponentTick.bCanEverTick);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterDeathCollision01DirectionTest,
	"Grimrock.Monsters.MON_DEATH_COLLISION01.DirectionContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterDeathCollision01DirectionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	GridMonsterDeathCollision01::FTestWorld TestWorld;
	if (!TestNotNull(TEXT("Test world exists"), TestWorld.World))
	{
		return false;
	}

	AGridMonsterActor* Monster = TestWorld.World->SpawnActor<AGridMonsterActor>();
	UGridMonsterDefinitionAsset* Definition = GridMonsterDeathCollision01::MakeValidDefinition();
	if (!TestNotNull(TEXT("Monster exists"), Monster) || !TestNotNull(TEXT("Definition exists"), Definition))
	{
		return false;
	}

	Definition->bEnableObstacleAwareDeath = true;
	Definition->DeathFallLocalDirection = FVector(-1.0f, 0.0f, 0.0f);
	Monster->MonsterDefinition = Definition;
	Monster->DeathComponent->InitializeDeathComponent(nullptr);

	Monster->SetActorRotation(FRotator::ZeroRotator);
	TestTrue(TEXT("Yaw 0 resolves backwards to world -X"),
		Monster->DeathComponent->ResolveDeathFallWorldDirection().Equals(FVector(-1.0f, 0.0f, 0.0f), 0.001f));

	Monster->SetActorRotation(FRotator(0.0f, 90.0f, 0.0f));
	TestTrue(TEXT("Yaw 90 rotates backwards direction to world -Y"),
		Monster->DeathComponent->ResolveDeathFallWorldDirection().Equals(FVector(0.0f, -1.0f, 0.0f), 0.001f));
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterDeathCollision01QueryOnlyObstacleTest,
	"Grimrock.Monsters.MON_DEATH_COLLISION01.QueryOnlyObstacleContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterDeathCollision01QueryOnlyObstacleTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	GridMonsterDeathCollision01::FTestWorld TestWorld;
	if (!TestNotNull(TEXT("Test world exists"), TestWorld.World))
	{
		return false;
	}

	AGridMonsterActor* Monster = TestWorld.World->SpawnActor<AGridMonsterActor>();
	UGridMonsterDefinitionAsset* Definition = GridMonsterDeathCollision01::MakeValidDefinition();
	if (!TestNotNull(TEXT("Monster exists"), Monster) || !TestNotNull(TEXT("Definition exists"), Definition))
	{
		return false;
	}

	Definition->bEnableObstacleAwareDeath = true;
	Definition->DeathFallLocalDirection = FVector(-1.0f, 0.0f, 0.0f);
	Definition->DeathObstacleProbeDistance = 120.0f;
	Definition->DeathObstacleProbeRadius = 20.0f;
	Definition->DeathObstacleProbeHalfHeight = 50.0f;
	Monster->MonsterDefinition = Definition;
	Monster->SetActorLocation(FVector::ZeroVector);
	Monster->SetActorRotation(FRotator::ZeroRotator);
	Monster->DeathComponent->InitializeDeathComponent(nullptr);

	AActor* Blocker = TestWorld.World->SpawnActor<AActor>();
	if (!TestNotNull(TEXT("Query-only blocker actor exists"), Blocker))
	{
		return false;
	}

	UBoxComponent* Box = NewObject<UBoxComponent>(Blocker, TEXT("QueryOnlyDeathWall"));
	if (!TestNotNull(TEXT("Query-only blocker component exists"), Box))
	{
		return false;
	}

	Blocker->SetRootComponent(Box);
	Blocker->AddInstanceComponent(Box);
	Box->InitBoxExtent(FVector(5.0f, 100.0f, 100.0f));
	Box->SetCollisionObjectType(ECC_WorldStatic);
	Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Box->SetCollisionResponseToAllChannels(ECR_Block);
	Box->RegisterComponentWithWorld(TestWorld.World);
	Blocker->SetActorLocation(FVector(-80.0f, 0.0f, 55.0f));
	TestWorld.World->UpdateWorldComponents(true, false);

	FHitResult Hit;
	TestTrue(TEXT("A QueryOnly wall is still detected as a death obstacle"), Monster->DeathComponent->ProbeDeathObstacle(Hit));
	TestEqual(TEXT("Detected obstacle remains QueryOnly"), Box->GetCollisionEnabled(), ECollisionEnabled::QueryOnly);
	TestTrue(TEXT("The QueryOnly wall component is the reported obstacle"), Hit.GetComponent() == Box);
	return true;
}

#endif
