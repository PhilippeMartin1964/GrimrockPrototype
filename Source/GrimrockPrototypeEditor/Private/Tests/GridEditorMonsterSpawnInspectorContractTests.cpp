#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "UObject/Class.h"

namespace
{
	struct FGridMonsterInspectorTestWorld
	{
		UWorld* World = nullptr;

		FGridMonsterInspectorTestWorld()
		{
			const UWorld::InitializationValues Values = UWorld::InitializationValues()
				.AllowAudioPlayback(false)
				.RequiresHitProxies(false)
				.CreatePhysicsScene(false)
				.CreateNavigation(false)
				.CreateAISystem(false)
				.ShouldSimulatePhysics(false)
				.SetTransactional(false);

			World = UWorld::CreateWorld(EWorldType::EditorPreview, false,
				FName(*FString::Printf(TEXT("MonsterInspectorWorld_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true,
				ERHIFeatureLevel::Num, &Values);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::EditorPreview);
				Context.SetCurrentWorld(World);
			}
		}

		~FGridMonsterInspectorTestWorld()
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterSpawnInspectorAuthoringContractTest,
	"Grimrock.Editor.MonsterSpawn.InspectorAuthoringContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterSpawnInspectorAuthoringContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridMonsterInspectorTestWorld TestWorld;
	TestNotNull(TEXT("The editor preview world is created"), TestWorld.World);
	if (!TestWorld.World)
	{
		return false;
	}

	AGridLevelEditorActor* EditorActor = TestWorld.World->SpawnActor<AGridLevelEditorActor>();
	TestNotNull(TEXT("The Grid Editor actor is spawned"), EditorActor);
	if (!EditorActor)
	{
		return false;
	}

	UGridLevelAsset* LevelAsset = NewObject<UGridLevelAsset>(EditorActor);
	LevelAsset->Width = 4;
	LevelAsset->Height = 4;
	LevelAsset->EnsureCellCount();
	for (FGridLevelCellData& Cell : LevelAsset->Cells)
	{
		Cell.CellType = EGridCellType::Floor;
		Cell.bBlocksOccupancy = false;
	}
	EditorActor->LevelAsset = LevelAsset;

	UGridMonsterDefinitionAsset* Definition = NewObject<UGridMonsterDefinitionAsset>(EditorActor);
	Definition->MonsterId = TEXT("MON_InspectorAmbush");
	Definition->SightRangeCells = 2;
	Definition->HearingRangeCells = 0;
	Definition->PrimaryAIProfile = EGridMonsterAIProfile::Ambush;
	Definition->bSharesAggroWithGroup = false;

	FGridMonsterSpawnInstance Spawn;
	Spawn.SpawnId = FGuid::NewGuid();
	Spawn.MonsterDefinition = Definition;
	Spawn.CellX = 1;
	Spawn.CellY = 1;
	Spawn.Facing = EGridEdge::North;
	Spawn.InitialMonsterState = EGridMonsterState::Idle;
	Spawn.bSpawnAtStart = true;
	LevelAsset->MonsterSpawns.Add(Spawn);

	TestTrue(TEXT("The MonsterSpawn can be selected"), EditorActor->SelectObjectById(Spawn.SpawnId));

	const UFunction* InitialStateFunction = EditorActor->FindFunction(TEXT("SetSelectedObjectInitialMonsterState"));
	TestNotNull(TEXT("The focused InitialMonsterState editor action is reflected"), InitialStateFunction);
	TestTrue(TEXT("The InitialMonsterState editor action is BlueprintCallable"),
		InitialStateFunction && InitialStateFunction->HasAnyFunctionFlags(FUNC_BlueprintCallable));

	TestTrue(TEXT("The selected spawn can be authored as Dormant"),
		EditorActor->SetSelectedObjectInitialMonsterState(EGridMonsterState::Dormant));
	TestEqual(TEXT("Dormant is stored on the typed placement"), LevelAsset->MonsterSpawns[0].InitialMonsterState, EGridMonsterState::Dormant);

	TestFalse(TEXT("Runtime-only Alert is rejected as a fresh authored state"),
		EditorActor->SetSelectedObjectInitialMonsterState(EGridMonsterState::Alert));
	TestEqual(TEXT("Rejected state never overwrites Dormant"), LevelAsset->MonsterSpawns[0].InitialMonsterState, EGridMonsterState::Dormant);

	TestEqual(TEXT("Sight remains definition-owned"), Definition->SightRangeCells, 2);
	TestEqual(TEXT("Hearing remains definition-owned"), Definition->HearingRangeCells, 0);
	TestEqual(TEXT("The intended Ambush AI profile remains definition-owned"), Definition->PrimaryAIProfile, EGridMonsterAIProfile::Ambush);

	FGridMonsterPatrolWaypoint A;
	A.Cell = FIntPoint(1, 1);
	FGridMonsterPatrolWaypoint B;
	B.Cell = FIntPoint(2, 1);
	LevelAsset->MonsterSpawns[0].PatrolWaypoints = { A, B };
	TestTrue(TEXT("The existing inspector patrol contract accepts PingPong once two waypoints exist"),
		EditorActor->SetSelectedMonsterPatrolMode(EGridMonsterPatrolMode::PingPong));
	TestEqual(TEXT("Patrol mode is stored on the typed MonsterSpawn"), LevelAsset->MonsterSpawns[0].PatrolMode, EGridMonsterPatrolMode::PingPong);

	return true;
}

#endif
