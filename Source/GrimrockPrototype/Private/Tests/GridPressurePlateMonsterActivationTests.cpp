#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridActivationComponent.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterOccupancySubsystem.h"

namespace GridPressurePlateMonsterActivationTests
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
				FName(*FString::Printf(TEXT("PRESSUREMON01_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true,
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

	FGridWorldObjectInstance MakePlate(int32 X, bool bMonsterActivates)
	{
		FGridWorldObjectInstance Plate;
		Plate.InstanceId = FGuid::NewGuid();
		Plate.Type = EGridLevelObjectType::PressurePlate;
		Plate.WorldObjectDefinitionId = TEXT("PressurePlate_PRESSURE_MON01");
		Plate.CellX = X;
		Plate.CellY = 0;
		Plate.WallSide = EGridEdge::None;
		Plate.bInitiallyEnabled = true;
		Plate.InstanceConfig.InteractionOverrides.bOverridePressurePlateWeight = true;
		Plate.InstanceConfig.InteractionOverrides.PressurePlateWeight.bActivateWhenPartyPresent = false;
		Plate.InstanceConfig.InteractionOverrides.PressurePlateWeight.bActivateWhenMonsterPresent = bMonsterActivates;
		Plate.InstanceConfig.InteractionOverrides.PressurePlateWeight.bUseItemWeight = false;
		return Plate;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridPressurePlateMonsterActivationTest,
	"Grimrock.PressurePlate.PRESSURE_MON01.MonsterOccupancyActivation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridPressurePlateMonsterActivationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridPressurePlateMonsterActivationTests;

	TestFalse(TEXT("Monster activation remains opt-in by default"), FGridPressurePlateWeightParams().bActivateWhenMonsterPresent);

	FTestWorld TestWorld;
	if (!TestNotNull(TEXT("Test world exists"), TestWorld.World))
	{
		return false;
	}

	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	if (!TestNotNull(TEXT("Runtime exists"), Runtime) || !TestNotNull(TEXT("Activation component exists"), Runtime ? Runtime->ActivationComponent.Get() : nullptr))
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

	const FGridWorldObjectInstance MonsterPlate = MakePlate(1, true);
	const FGridWorldObjectInstance PassivePlate = MakePlate(2, false);
	const FGuid MonsterPlateId = MonsterPlate.InstanceId;
	const FGuid PassivePlateId = PassivePlate.InstanceId;
	Level->WorldObjectInstances = { MonsterPlate, PassivePlate };
	Runtime->LevelAsset = Level;

	UGridWorldObjectDefinitionAsset* PlateDefinition = NewObject<UGridWorldObjectDefinitionAsset>(Runtime);
	PlateDefinition->DefinitionId = TEXT("PressurePlate_PRESSURE_MON01");
	PlateDefinition->SupportedType = EGridLevelObjectType::PressurePlate;
	PlateDefinition->PlacementSurface = EGridObjectPlacementKind::Floor;
	Runtime->WorldObjectDefinitions.Add(PlateDefinition);

	Runtime->ActivationComponent->Initialize(Runtime);
	Runtime->ActivationComponent->RebuildIndexes();
	Runtime->ActivationComponent->RefreshAllPressurePlates();

	UGridMonsterDefinitionAsset* MonsterDefinition = NewObject<UGridMonsterDefinitionAsset>(Runtime);
	MonsterDefinition->MonsterId = TEXT("Monster_PRESSURE_MON01");
	MonsterDefinition->DisplayName = FText::FromString(TEXT("PRESSURE-MON01 Monster"));
	MonsterDefinition->CategoryId = TEXT("PRESSURE_MON01_Test");
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

	UGridMonsterOccupancySubsystem* Occupancy = TestWorld.World->GetSubsystem<UGridMonsterOccupancySubsystem>();
	if (!TestNotNull(TEXT("Monster occupancy subsystem exists"), Occupancy))
	{
		return false;
	}

	TestTrue(TEXT("Monster registers on the source cell"), Occupancy->RegisterMonster(Monster, FIntPoint(0, 0)));
	TestFalse(TEXT("Monster-enabled plate starts released"), Runtime->ActivationComponent->GetActiveObjectIds().Contains(MonsterPlateId));
	TestFalse(TEXT("Monster-disabled plate starts released"), Runtime->ActivationComponent->GetActiveObjectIds().Contains(PassivePlateId));

	TestTrue(TEXT("Monster can reserve the monster-enabled plate cell"), Occupancy->TryReserveCell(Monster, FIntPoint(1, 0)));
	TestFalse(TEXT("Reservation alone does not press the plate"), Runtime->ActivationComponent->GetActiveObjectIds().Contains(MonsterPlateId));

	TestTrue(TEXT("Committed monster move onto plate succeeds"), Occupancy->CommitMove(Monster, FIntPoint(0, 0), FIntPoint(1, 0)));
	TestTrue(TEXT("Committed monster occupancy presses enabled plate"), Runtime->ActivationComponent->GetActiveObjectIds().Contains(MonsterPlateId));

	TestTrue(TEXT("Monster can reserve the monster-disabled plate cell"), Occupancy->TryReserveCell(Monster, FIntPoint(2, 0)));
	TestTrue(TEXT("Source plate remains pressed until move commits"), Runtime->ActivationComponent->GetActiveObjectIds().Contains(MonsterPlateId));
	TestTrue(TEXT("Committed monster move off enabled plate succeeds"), Occupancy->CommitMove(Monster, FIntPoint(1, 0), FIntPoint(2, 0)));
	TestFalse(TEXT("Leaving monster-enabled plate releases it"), Runtime->ActivationComponent->GetActiveObjectIds().Contains(MonsterPlateId));
	TestFalse(TEXT("Monster occupancy does not press opt-out plate"), Runtime->ActivationComponent->GetActiveObjectIds().Contains(PassivePlateId));

	Occupancy->UnregisterMonster(Monster);
	TestFalse(TEXT("Unregister leaves opt-out plate released"), Runtime->ActivationComponent->GetActiveObjectIds().Contains(PassivePlateId));
	return true;
}

#endif
