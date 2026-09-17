#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridDungeonAsset.h"
#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridActivationComponent.h"
#include "Runtime/GridLevelRuntimeActor.h"

namespace
{
	struct FGridLevelFade01TestWorld
	{
		UWorld* World = nullptr;

		FGridLevelFade01TestWorld()
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
				FName(*FString::Printf(TEXT("LEVELFADE01TestWorld_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true,
				ERHIFeatureLevel::Num, &Values);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FGridLevelFade01TestWorld()
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

	UGridLevelAsset* MakeLevelFade01Floor(UObject* Outer)
	{
		UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Outer);
		Level->Width = 4;
		Level->Height = 4;
		Level->CellSize = 200.0f;
		Level->EnsureCellCount();
		for (FGridLevelCellData& Cell : Level->Cells)
		{
			Cell.CellType = EGridCellType::Floor;
			Cell.bBlocksOccupancy = false;
		}
		return Level;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridLevelFade01RelocationPersistenceTest,
	"Grimrock.Relocation.LEVEL_FADE01.RelocationActivationPersistsAcrossRevisit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridLevelFade01RelocationPersistenceTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridLevelFade01TestWorld TestWorld;
	if (!TestNotNull(TEXT("World"), TestWorld.World))
	{
		return false;
	}

	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	if (!TestNotNull(TEXT("Runtime"), Runtime))
	{
		return false;
	}

	UGridDungeonAsset* Dungeon = NewObject<UGridDungeonAsset>(Runtime);
	UGridLevelAsset* Level = MakeLevelFade01Floor(Dungeon);
	const FName LevelId(TEXT("LEVEL_FADE01_Level"));

	FGridDungeonLevelEntry Entry;
	Entry.LevelId = LevelId;
	Entry.LevelAsset = Level;
	Entry.bEnabled = true;
	Dungeon->Levels = { Entry };
	Dungeon->DefaultLevelId = LevelId;

	FGridWorldObjectInstance Relocation;
	Relocation.InstanceId = FGuid::NewGuid();
	Relocation.Type = EGridLevelObjectType::Relocation;
	Relocation.CellX = 1;
	Relocation.CellY = 1;
	Relocation.InstanceConfig.bRelocationInitiallyEnabled = true;
	Relocation.InstanceConfig.Relocation.TargetCellX = 2;
	Relocation.InstanceConfig.Relocation.TargetCellY = 2;
	Relocation.InstanceConfig.Relocation.TargetFacing = EGridEdge::North;
	Level->WorldObjectInstances = { Relocation };

	Runtime->DungeonAsset = Dungeon;
	Runtime->LevelAsset = Level;
	Runtime->CurrentDungeonLevelId = LevelId;

	UGridActivationComponent* Activation = Runtime->FindComponentByClass<UGridActivationComponent>();
	if (!TestNotNull(TEXT("Activation component"), Activation))
	{
		return false;
	}

	Activation->Initialize(Runtime);
	Activation->ResetRuntimeState();
	Activation->RegisterInitialObjectState(Relocation);

	TestTrue(TEXT("Relocation starts active"), Activation->IsObjectActive(Relocation.InstanceId));

	FGridRelocationBehaviorParams Resolved;
	TestTrue(TEXT("Active relocation resolves before capture"), Runtime->FindRelocationAtCell(1, 1, Resolved));
	TestTrue(TEXT("Capture current level state"), Runtime->CaptureCurrentLevelRuntimeState());

	const FGridLevelRuntimeState* SavedState = Runtime->FindRuntimeStateForCurrentLevel();
	TestTrue(TEXT("Captured state contains relocation activation"), SavedState && SavedState->InteractiveObjects.Contains(Relocation.InstanceId));

	Activation->SetActiveObjectIds({});
	TestFalse(TEXT("Test simulates rebuild clearing active ids"), Activation->IsObjectActive(Relocation.InstanceId));

	TestTrue(TEXT("Apply visited level state"), Runtime->ApplyCurrentLevelRuntimeState());
	TestTrue(TEXT("Relocation active after first revisit"), Activation->IsObjectActive(Relocation.InstanceId));
	TestTrue(TEXT("Relocation resolves after first revisit"), Runtime->FindRelocationAtCell(1, 1, Resolved));

	TestTrue(TEXT("Capture second visit"), Runtime->CaptureCurrentLevelRuntimeState());
	Activation->SetActiveObjectIds({});
	TestTrue(TEXT("Apply second revisit"), Runtime->ApplyCurrentLevelRuntimeState());
	TestTrue(TEXT("Relocation remains active across repeated revisits"), Activation->IsObjectActive(Relocation.InstanceId));
	TestTrue(TEXT("Relocation still resolves across repeated revisits"), Runtime->FindRelocationAtCell(1, 1, Resolved));

	return true;
}

#endif
