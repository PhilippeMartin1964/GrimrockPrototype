#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "GridDoorTestUtils.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridDoorActor.h"
#include "Runtime/GridDoorSystemComponent.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridSecretDoorActor.h"
#include "Runtime/Map/GridMapExplorationState.h"
#include "UObject/UnrealType.h"

namespace GridMapMON2164Tests
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
				FName(*FString::Printf(TEXT("MapMON2164World_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true,
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

	UGridLevelAsset* MakeLevel(AGridLevelRuntimeActor* Runtime)
	{
		UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Runtime);
		Level->Width = FGridMapExplorationState::GridSize;
		Level->Height = FGridMapExplorationState::GridSize;
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
		Runtime->LevelAsset = Level;
		return Level;
	}

	FGridWorldObjectInstance AddDoor(UGridLevelAsset* Level, bool bInitiallyOpen, FName DefinitionId = NAME_None)
	{
		FGridWorldObjectInstance Door;
		Door.InstanceId = FGuid::NewGuid();
		Door.Type = EGridLevelObjectType::Door;
		Door.WorldObjectDefinitionId = DefinitionId;
		Door.CellX = 10;
		Door.CellY = 10;
		Door.WallSide = EGridEdge::North;
		Door.InstanceConfig.bDoorInitiallyOpen = bInitiallyOpen;
		Level->WorldObjectInstances.Add(Door);
		return Door;
	}

	UGridDoorSystemComponent* PrepareDoorSystem(AGridLevelRuntimeActor* Runtime)
	{
		UGridDoorSystemComponent* DoorSystem = Runtime ? Runtime->FindComponentByClass<UGridDoorSystemComponent>() : nullptr;
		if (DoorSystem)
		{
			DoorSystem->Initialize(Runtime);
			DoorSystem->RebuildIndexes();
		}
		return DoorSystem;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMapMON2164StateTest, "Grimrock.Map.MON21_6_4.SecretDiscovery.StateIdentityAndReset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMapMON2164StateTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridMapExplorationState State;
	bool bNewlyDiscovered = true;
	TestFalse(TEXT("Invalid secret identity is rejected"), State.TryMarkSecretDiscovered(FGuid(), bNewlyDiscovered));
	TestFalse(TEXT("Rejected identity is not new"), bNewlyDiscovered);

	const FGuid SecretId = FGuid::NewGuid();
	TestTrue(TEXT("Valid secret identity is accepted"), State.TryMarkSecretDiscovered(SecretId, bNewlyDiscovered));
	TestTrue(TEXT("First discovery is new"), bNewlyDiscovered);
	TestTrue(TEXT("Secret is readable as discovered"), State.IsSecretDiscovered(SecretId));
	TestEqual(TEXT("Exactly one secret is known"), State.GetDiscoveredSecretCount(), 1);

	bNewlyDiscovered = true;
	TestTrue(TEXT("Repeated discovery is idempotently accepted"), State.TryMarkSecretDiscovered(SecretId, bNewlyDiscovered));
	TestFalse(TEXT("Repeated discovery is not new"), bNewlyDiscovered);
	TestEqual(TEXT("Repeated discovery does not duplicate identity"), State.GetDiscoveredSecretCount(), 1);

	State.Reset();
	TestFalse(TEXT("Reset forgets secret knowledge"), State.IsSecretDiscovered(SecretId));
	TestEqual(TEXT("Reset clears discovered secrets"), State.GetDiscoveredSecretCount(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMapMON2164OpeningTest, "Grimrock.Map.MON21_6_4.SecretDiscovery.FullOpenThenCloseRemainsKnown",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMapMON2164OpeningTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMapMON2164Tests;

	FTestWorld TestWorld;
	TestNotNull(TEXT("Transient world exists"), TestWorld.World);
	if (!TestWorld.World)
	{
		return false;
	}

	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	TestNotNull(TEXT("Runtime actor exists"), Runtime);
	if (!Runtime)
	{
		return false;
	}

	UGridLevelAsset* Level = MakeLevel(Runtime);
	const FGridWorldObjectInstance DoorData = AddDoor(Level, false);
	UGridDoorSystemComponent* DoorSystem = PrepareDoorSystem(Runtime);
	TestNotNull(TEXT("Door system exists"), DoorSystem);
	if (!DoorSystem)
	{
		return false;
	}

	AGridSecretDoorActor* Door = TestWorld.World->SpawnActor<AGridSecretDoorActor>();
	TestNotNull(TEXT("Secret door actor exists"), Door);
	if (!Door)
	{
		return false;
	}
	GridDoorTestUtils::InitializeDoorFromMotion(Door, DoorData, TestWorld.World, 1.0f);
	DoorSystem->RegisterDoorObject(FGridRuntimeWorldObjectData(DoorData), Door);

	TestTrue(TEXT("Secret door accepts open command"), Runtime->OpenDoorOnEdge(10, 10, EGridEdge::North));
	const FGridLevelRuntimeState* BeforeFinish = Runtime->FindRuntimeStateForCurrentLevel();
	TestFalse(TEXT("Opening command alone does not discover secret"),
		BeforeFinish && BeforeFinish->MapExploration.IsSecretDiscovered(DoorData.InstanceId));

	Door->Tick(0.5f);
	const FGridLevelRuntimeState* MidOpen = Runtime->FindRuntimeStateForCurrentLevel();
	TestFalse(TEXT("Half-open secret is not yet discovered"), MidOpen && MidOpen->MapExploration.IsSecretDiscovered(DoorData.InstanceId));

	Door->Tick(0.5f);
	const FGridLevelRuntimeState* FullyOpen = Runtime->FindRuntimeStateForCurrentLevel();
	TestTrue(TEXT("Fully exposed secret becomes discovered"), FullyOpen && FullyOpen->MapExploration.IsSecretDiscovered(DoorData.InstanceId));

	TestTrue(TEXT("Secret door accepts close command"), Runtime->CloseDoorOnEdge(10, 10, EGridEdge::North));
	Door->Tick(1.0f);
	const FGridLevelRuntimeState* ClosedAgain = Runtime->FindRuntimeStateForCurrentLevel();
	TestTrue(TEXT("Discovered secret remains known after closing"),
		ClosedAgain && ClosedAgain->MapExploration.IsSecretDiscovered(DoorData.InstanceId));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMapMON2164InitialOpenTest, "Grimrock.Map.MON21_6_4.SecretDiscovery.InitiallyOpenIsKnown",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMapMON2164InitialOpenTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMapMON2164Tests;

	FTestWorld TestWorld;
	if (!TestWorld.World)
	{
		AddError(TEXT("Transient world was not created"));
		return false;
	}
	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	if (!Runtime)
	{
		AddError(TEXT("Runtime actor was not spawned"));
		return false;
	}

	UGridLevelAsset* Level = MakeLevel(Runtime);
	const FName DefinitionId(TEXT("MON2164_CustomSecretDoor"));
	UGridWorldObjectDefinitionAsset* Definition = NewObject<UGridWorldObjectDefinitionAsset>(Runtime);
	Definition->DefinitionId = DefinitionId;
	Definition->SupportedType = EGridLevelObjectType::Door;
	Definition->RuntimeActorClass = AGridSecretDoorActor::StaticClass();
	Runtime->WorldObjectDefinitions.Add(Definition);

	const FGridWorldObjectInstance DoorData = AddDoor(Level, true, DefinitionId);
	UGridDoorSystemComponent* DoorSystem = PrepareDoorSystem(Runtime);
	if (!DoorSystem)
	{
		AddError(TEXT("Secret door fixture is incomplete"));
		return false;
	}

	TestTrue(TEXT("Custom definition is classified as a secret door without relying on its DefinitionId"),
		DoorSystem->IsSecretDoorOnEdge(10, 10, EGridEdge::North));

	// No live actor is registered: discovery must also use the same data-driven definition.
	DoorSystem->RegisterDoorObject(FGridRuntimeWorldObjectData(DoorData), nullptr);

	const FGridLevelRuntimeState* State = Runtime->FindRuntimeStateForCurrentLevel();
	TestTrue(TEXT("An initially open secret is discovered from its runtime actor definition"),
		State && State->MapExploration.IsSecretDiscovered(DoorData.InstanceId));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMapMON2164NormalDoorAndSaveBoundaryTest, "Grimrock.Map.MON21_6_4.SecretDiscovery.NormalDoorAndSaveBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMapMON2164NormalDoorAndSaveBoundaryTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMapMON2164Tests;

	FTestWorld TestWorld;
	if (!TestWorld.World)
	{
		AddError(TEXT("Transient world was not created"));
		return false;
	}
	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	if (!Runtime)
	{
		AddError(TEXT("Runtime actor was not spawned"));
		return false;
	}

	UGridLevelAsset* Level = MakeLevel(Runtime);
	const FGridWorldObjectInstance DoorData = AddDoor(Level, false);
	UGridDoorSystemComponent* DoorSystem = PrepareDoorSystem(Runtime);
	AGridDoorActor* Door = TestWorld.World->SpawnActor<AGridDoorActor>();
	if (!DoorSystem || !Door)
	{
		AddError(TEXT("Normal door fixture is incomplete"));
		return false;
	}
	GridDoorTestUtils::InitializeDoorFromMotion(Door, DoorData, TestWorld.World, 1.0f);
	DoorSystem->RegisterDoorObject(FGridRuntimeWorldObjectData(DoorData), Door);
	Runtime->OpenDoorOnEdge(10, 10, EGridEdge::North);
	Door->Tick(1.0f);

	const FGridLevelRuntimeState* State = Runtime->FindRuntimeStateForCurrentLevel();
	TestFalse(TEXT("Normal door opening never creates secret knowledge"),
		State && State->MapExploration.IsSecretDiscovered(DoorData.InstanceId));

	UScriptStruct* ExplorationStruct = FGridMapExplorationState::StaticStruct();
	const FProperty* SecretProperty = ExplorationStruct ? ExplorationStruct->FindPropertyByName(TEXT("DiscoveredSecretObjectIds")) : nullptr;
	TestNotNull(TEXT("Exploration state owns discovered secret identities"), SecretProperty);
	TestTrue(TEXT("Secret knowledge is SaveGame-persistent from MON21.6.5"),
		SecretProperty && SecretProperty->HasAnyPropertyFlags(CPF_SaveGame));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
