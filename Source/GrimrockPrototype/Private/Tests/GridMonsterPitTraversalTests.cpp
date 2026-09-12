#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterMovementComponent.h"
#include "Runtime/Monsters/GridMonsterTraversalUtils.h"

namespace GridMonsterPitTraversalTests
{
	struct FTestWorld
	{
		UWorld* World = nullptr;

		FTestWorld()
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
				FName(*FString::Printf(TEXT("MonsterPitTraversal_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true,
				ERHIFeatureLevel::Num, &Values);
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterPitTraversalTest, "Grimrock.Monsters.PitTraversal.OpenPitBlocksClosedPitAllows",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterPitTraversalTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMonsterPitTraversalTests;

	FTestWorld TestWorld;
	if (!TestNotNull(TEXT("Test world exists"), TestWorld.World))
	{
		return false;
	}

	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	if (!TestNotNull(TEXT("Runtime exists"), Runtime))
	{
		return false;
	}

	UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Runtime);
	Level->Width = 3;
	Level->Height = 1;
	Level->CellSize = 200.0f;
	Level->EnsureCellCount();
	for (FGridLevelCellData& Cell : Level->Cells)
	{
		Cell.CellType = EGridCellType::Floor;
		Cell.bBlocksOccupancy = false;
		Cell.NorthWall = EGridWallType::None;
		Cell.EastWall = EGridWallType::None;
		Cell.SouthWall = EGridWallType::None;
		Cell.WestWall = EGridWallType::None;
	}

	FGridWorldObjectInstance Pit;
	Pit.InstanceId = FGuid::NewGuid();
	Pit.Type = EGridLevelObjectType::Pit;
	Pit.CellX = 1;
	Pit.CellY = 0;
	Pit.WallSide = EGridEdge::None;
	Pit.bInitiallyEnabled = true;
	Pit.InstanceConfig.Pit.bInitiallyOpen = true;
	const FGuid PitId = Pit.InstanceId;
	Level->WorldObjectInstances.Add(Pit);
	Runtime->LevelAsset = Level;

	UGridMonsterDefinitionAsset* MonsterDefinition = NewObject<UGridMonsterDefinitionAsset>(Runtime);
	MonsterDefinition->MonsterId = TEXT("Monster_PitTraversal_Test");
	MonsterDefinition->DisplayName = FText::FromString(TEXT("Pit Traversal Test Monster"));
	MonsterDefinition->CategoryId = TEXT("Test");
	MonsterDefinition->GridFootprint = FIntPoint(1, 1);
	MonsterDefinition->MoveDuration = 1.0f;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Runtime;
	AGridMonsterActor* Monster = TestWorld.World->SpawnActor<AGridMonsterActor>(
		AGridMonsterActor::StaticClass(), Runtime->GetCellCenterWorld(0, 0), FRotator::ZeroRotator, SpawnParams);
	if (!TestNotNull(TEXT("Monster exists"), Monster) ||
		!TestTrue(TEXT("Monster initializes"), Monster->InitializeMonster(MonsterDefinition, FGuid::NewGuid(), FIntPoint(0, 0), EGridEdge::East, NAME_None)))
	{
		return false;
	}

	UGridMonsterMovementComponent* Movement = NewObject<UGridMonsterMovementComponent>(Monster, TEXT("PitTraversalMovement"));
	Movement->bAutoInitialize = false;
	Movement->bInferCellFromActorLocation = false;
	Monster->AddInstanceComponent(Movement);
	Movement->RegisterComponent();
	if (!TestTrue(TEXT("Movement initializes"), Movement->InitializeMovement(Runtime)))
	{
		return false;
	}

	TestTrue(TEXT("Open pit cell is a monster traversal blocker"), GridMonsterTraversalUtils::IsOpenPitCell(Runtime, FIntPoint(1, 0)));
	TestFalse(TEXT("Monster cannot step onto an open pit"), Movement->TryMove(EGridEdge::East));

	TestTrue(TEXT("Pit can close"), Runtime->SetPitOpen(PitId, false, false));
	TestFalse(TEXT("Fully closed pit is no longer a monster traversal blocker"), GridMonsterTraversalUtils::IsOpenPitCell(Runtime, FIntPoint(1, 0)));
	TestTrue(TEXT("Monster can step onto the fully closed trapdoor"), Movement->TryMove(EGridEdge::East));
	Movement->CancelCurrentAction();

	return true;
}

#endif