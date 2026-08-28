#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "UObject/Class.h"

namespace
{
	struct FGridTD052RuntimeDiagnosticsWorld
	{
		UWorld* World = nullptr;

		FGridTD052RuntimeDiagnosticsWorld()
		{
			const UWorld::InitializationValues InitializationValues = UWorld::InitializationValues()
																		  .AllowAudioPlayback(false)
																		  .RequiresHitProxies(false)
																		  .CreatePhysicsScene(false)
																		  .CreateNavigation(false)
																		  .CreateAISystem(false)
																		  .ShouldSimulatePhysics(false)
																		  .SetTransactional(false);

			World = UWorld::CreateWorld(EWorldType::Game, false,
				FName(*FString::Printf(TEXT("TD052RuntimeDiagnostics_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true,
				ERHIFeatureLevel::Num, &InitializationValues);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FGridTD052RuntimeDiagnosticsWorld()
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

	UGridLevelAsset* GridTD052MakeDiagnosticsLevel(UObject* Outer)
	{
		UGridLevelAsset* LevelAsset = NewObject<UGridLevelAsset>(Outer, TEXT("TD052_Level"));
		LevelAsset->Width = 2;
		LevelAsset->Height = 2;
		LevelAsset->CellSize = 200.0f;
		LevelAsset->StartCellX = 1;
		LevelAsset->StartCellY = 0;
		LevelAsset->StartFacing = EGridEdge::East;
		LevelAsset->EnsureCellCount();

		for (FGridLevelCellData& Cell : LevelAsset->Cells)
		{
			Cell.CellType = EGridCellType::Floor;
			Cell.bBlocksOccupancy = false;
			Cell.bHasCeiling = false;
		}

		return LevelAsset;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD052RuntimeActorDiagnosticsContractTest, "Grimrock.TechnicalDebt.TD05_2.RuntimeActorDiagnostics.Contract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD052RuntimeActorDiagnosticsContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridTD052RuntimeDiagnosticsWorld TestWorld;
	TestNotNull(TEXT("The transient runtime diagnostics world is created"), TestWorld.World);
	if (!TestWorld.World)
	{
		return false;
	}

	AGridLevelRuntimeActor* RuntimeActor = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	TestNotNull(TEXT("The runtime actor is created"), RuntimeActor);
	if (!RuntimeActor)
	{
		return false;
	}

	const UFunction* RuntimeSummaryFunction = RuntimeActor->FindFunction(TEXT("GetRuntimeDebugSummary"));
	const UFunction* LevelDiagnosticsFunction = RuntimeActor->FindFunction(TEXT("GetLevelAssetDiagnostics"));
	const UFunction* PIEReadinessFunction = RuntimeActor->FindFunction(TEXT("GetPIEReadinessDiagnostics"));
	TestNotNull(TEXT("GetRuntimeDebugSummary remains reflected"), RuntimeSummaryFunction);
	TestNotNull(TEXT("GetLevelAssetDiagnostics remains reflected"), LevelDiagnosticsFunction);
	TestNotNull(TEXT("GetPIEReadinessDiagnostics remains reflected"), PIEReadinessFunction);
	TestTrue(TEXT("GetRuntimeDebugSummary remains BlueprintCallable"),
		RuntimeSummaryFunction && RuntimeSummaryFunction->HasAnyFunctionFlags(FUNC_BlueprintCallable));
	TestTrue(TEXT("GetLevelAssetDiagnostics remains BlueprintCallable"),
		LevelDiagnosticsFunction && LevelDiagnosticsFunction->HasAnyFunctionFlags(FUNC_BlueprintCallable));
	TestTrue(TEXT("GetPIEReadinessDiagnostics remains BlueprintCallable"),
		PIEReadinessFunction && PIEReadinessFunction->HasAnyFunctionFlags(FUNC_BlueprintCallable));

	UGridLevelAsset* LevelAsset = GridTD052MakeDiagnosticsLevel(RuntimeActor);
	TestNotNull(TEXT("The characterized level asset is created"), LevelAsset);
	if (!LevelAsset)
	{
		return false;
	}
	RuntimeActor->LevelAsset = LevelAsset;

	const FString RuntimeSummary = RuntimeActor->GetRuntimeDebugSummary();
	TestTrue(TEXT("Runtime summary keeps its heading and level identity"), RuntimeSummary.Contains(TEXT("Grid Runtime | Level=TD052_Level")));
	TestTrue(TEXT("Runtime summary exposes the runtime actor count"), RuntimeSummary.Contains(TEXT("Runtime Actors=0")));
	TestTrue(TEXT("Runtime summary exposes the spawned item count"), RuntimeSummary.Contains(TEXT("Spawned Items=0")));
	TestTrue(TEXT("Runtime summary exposes monster count and failures"), RuntimeSummary.Contains(TEXT("Spawned Monsters=0 Failures=0")));

	const FString LevelDiagnostics = RuntimeActor->GetLevelAssetDiagnostics();
	TestTrue(TEXT("Level diagnostics keep their heading"), LevelDiagnostics.Contains(TEXT("Grid LevelAsset Diagnostics")));
	TestTrue(TEXT("Level diagnostics expose the grid size"), LevelDiagnostics.Contains(TEXT("GridSize=2x2")));
	TestTrue(TEXT("Level diagnostics expose the cell size"), LevelDiagnostics.Contains(TEXT("CellSize=200.00")));
	TestTrue(TEXT("Level diagnostics expose the start pose"), LevelDiagnostics.Contains(TEXT("StartCell=(1,0) StartFacing=East StartCellValid=true")));
	TestTrue(TEXT("Level diagnostics expose cell count consistency"), LevelDiagnostics.Contains(TEXT("Cells=4 ExpectedCells=4")));
	TestTrue(TEXT("Level diagnostics expose cell classifications"), LevelDiagnostics.Contains(TEXT("NonEmptyCells=4 BlockingCells=0 CeilingCells=0")));
	TestTrue(TEXT("Level diagnostics expose object and transition counts"),
		LevelDiagnostics.Contains(TEXT("Objects=0 Links=0 TransitionObjects=0 HiddenFloorCells=0")));
	TestTrue(TEXT("A structurally consistent level reports OK"), LevelDiagnostics.Contains(TEXT("Status=OK")));

	const FString PIEReadiness = RuntimeActor->GetPIEReadinessDiagnostics();
	TestTrue(TEXT("PIE readiness keeps its heading"), PIEReadiness.Contains(TEXT("GridLevelRuntimeActor PIE Readiness")));
	TestTrue(TEXT("PIE readiness exposes the game world type"), PIEReadiness.Contains(TEXT("WorldType: Game")));
	TestTrue(TEXT("PIE readiness identifies a game world"), PIEReadiness.Contains(TEXT("IsGameWorld: true")));
	TestTrue(TEXT("PIE readiness exposes the valid start pose"), PIEReadiness.Contains(TEXT("Start: Cell=(1,0) Facing=East Valid=true")));
	TestTrue(TEXT("Missing presentation meshes preserve the readiness error contract"),
		PIEReadiness.Contains(TEXT("Status: ERROR - FloorMesh, WallMesh or CeilingMesh is missing.")));

	RuntimeActor->LevelAsset = nullptr;
	const FString MissingLevelDiagnostics = RuntimeActor->GetLevelAssetDiagnostics();
	TestTrue(TEXT("Missing LevelAsset preserves the level diagnostics error contract"),
		MissingLevelDiagnostics.Contains(TEXT("Status=ERROR: missing LevelAsset reference.")));

	const FString MissingLevelPIEReadiness = RuntimeActor->GetPIEReadinessDiagnostics();
	TestTrue(
		TEXT("Missing LevelAsset preserves the PIE readiness error contract"), MissingLevelPIEReadiness.Contains(TEXT("Status: ERROR - LevelAsset is null.")));

	return true;
}

#endif
