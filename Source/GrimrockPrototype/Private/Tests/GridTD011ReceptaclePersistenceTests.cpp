#if WITH_DEV_AUTOMATION_TESTS

#include "Core/GridLevelAsset.h"
#include "Core/GridObjectArchetypeAsset.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "RPG/RPGSaveMigrationService.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridReceptacleActor.h"
#include "Save/GrimrockPartySaveGame.h"

namespace GridTD011Tests
{
	const FGuid ReceptacleObjectId(1, 1, 1, 1);
	const FName ReceptacleArchetypeId(TEXT("TD01_Receptacle"));

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
				FName(*FString::Printf(TEXT("TD011_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true,
				ERHIFeatureLevel::Num, &Values);
			if (!World || !GEngine)
			{
				return;
			}

			FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
			Context.SetCurrentWorld(World);
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

	struct FRuntimeFixture
	{
		UGridLevelAsset* Level = nullptr;
		UGridObjectArchetypeAsset* Archetype = nullptr;
		TObjectPtr<UStaticMesh> Mesh = nullptr;
	};

	FRuntimeFixture MakeFixture(UObject* Outer)
	{
		FRuntimeFixture Fixture;
		Fixture.Level = NewObject<UGridLevelAsset>(Outer);
		Fixture.Level->Width = 1;
		Fixture.Level->Height = 1;
		Fixture.Level->EnsureCellCount();
		Fixture.Level->Cells[0].CellType = EGridCellType::Floor;

		FGridLevelObjectData ReceptacleObject;
		ReceptacleObject.ObjectId = ReceptacleObjectId;
		ReceptacleObject.ArchetypeId = ReceptacleArchetypeId;
		ReceptacleObject.Type = EGridLevelObjectType::Receptacle;
		ReceptacleObject.CellX = 0;
		ReceptacleObject.CellY = 0;
		ReceptacleObject.Edge = EGridEdge::North;
		ReceptacleObject.bInitiallyEnabled = true;
		Fixture.Level->Objects.Add(ReceptacleObject);

		Fixture.Archetype = NewObject<UGridObjectArchetypeAsset>(Outer);
		Fixture.Archetype->ArchetypeId = ReceptacleArchetypeId;
		Fixture.Archetype->SupportedType = EGridLevelObjectType::Receptacle;
		Fixture.Archetype->PlacementKind = EGridObjectPlacementKind::Wall;
		Fixture.Archetype->bIsInteractable = true;
		Fixture.Archetype->RuntimeActorClass = AGridReceptacleActor::StaticClass();
		Fixture.Mesh = NewObject<UStaticMesh>(Outer);
		Fixture.Archetype->PreviewMesh = Fixture.Mesh;
		return Fixture;
	}

	AGridLevelRuntimeActor* SpawnRuntime(FAutomationTestBase& Test, UWorld* World, const FRuntimeFixture& Fixture,
		const FGridDungeonRuntimeState* RestoredState = nullptr)
	{
		AGridLevelRuntimeActor* Runtime = World ? World->SpawnActor<AGridLevelRuntimeActor>() : nullptr;
		if (!Runtime)
		{
			Test.AddError(TEXT("TD01.1 could not spawn GridLevelRuntimeActor."));
			return nullptr;
		}

		Runtime->LevelAsset = Fixture.Level;
		Runtime->ObjectArchetypes.Add(Fixture.Archetype);
		if (RestoredState)
		{
			Runtime->DungeonRuntimeState = *RestoredState;
		}
		Runtime->RebuildLevel();

		if (!Runtime->FindRuntimeObjectActor<AGridReceptacleActor>(ReceptacleObjectId))
		{
			Test.AddError(TEXT("TD01.1 could not spawn the transient receptacle fixture."));
			return nullptr;
		}
		return Runtime;
	}

	UGrimrockPartySaveGame* RoundTripDungeonState(FAutomationTestBase& Test, const FGridDungeonRuntimeState& DungeonState)
	{
		UGrimrockPartySaveGame* Source = NewObject<UGrimrockPartySaveGame>(GetTransientPackage());
		Source->DungeonRuntimeState = DungeonState;

		TArray<uint8> SaveBytes;
		if (!Test.TestTrue(TEXT("TD01.1 current SaveGame serializes"), UGameplayStatics::SaveGameToMemory(Source, SaveBytes)))
		{
			return nullptr;
		}

		UGrimrockPartySaveGame* Loaded = Cast<UGrimrockPartySaveGame>(UGameplayStatics::LoadGameFromMemory(SaveBytes));
		Test.TestNotNull(TEXT("TD01.1 current SaveGame deserializes"), Loaded);
		return Loaded;
	}
}

using namespace GridTD011Tests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD011DisabledRoundTripTest,
	"Grimrock.TechnicalDebt.TD01_1.ReceptaclePersistence.DisabledRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD011DisabledRoundTripTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FTestWorld TestWorld;
	if (!TestWorld.World)
	{
		AddError(TEXT("TD01.1 could not create a transient game world."));
		return false;
	}

	const FRuntimeFixture Fixture = MakeFixture(GetTransientPackage());
	AGridLevelRuntimeActor* SourceRuntime = SpawnRuntime(*this, TestWorld.World, Fixture);
	if (!SourceRuntime)
	{
		return false;
	}

	AGridReceptacleActor* SourceReceptacle = SourceRuntime->FindRuntimeObjectActor<AGridReceptacleActor>(ReceptacleObjectId);
	SourceReceptacle->SetCanRemoveItem(false);
	TestFalse(TEXT("Runtime receptacle is disabled before capture"), SourceReceptacle->bCanRemoveItem);
	TestTrue(TEXT("Disabled runtime state captures"), SourceRuntime->CaptureCurrentLevelRuntimeState());

	const FGridLevelRuntimeState* CapturedState = SourceRuntime->FindRuntimeStateForCurrentLevel();
	TestNotNull(TEXT("Captured level state exists"), CapturedState);
	const FGridRuntimeReceptacleState* CapturedReceptacle = CapturedState ? CapturedState->Receptacles.Find(ReceptacleObjectId) : nullptr;
	TestNotNull(TEXT("Captured receptacle state exists"), CapturedReceptacle);
	if (!CapturedReceptacle)
	{
		return false;
	}
	TestFalse(TEXT("Snapshot stores disabled removal permission"), CapturedReceptacle->bCanRemoveItem);

	UGrimrockPartySaveGame* Loaded = RoundTripDungeonState(*this, SourceRuntime->DungeonRuntimeState);
	if (!Loaded)
	{
		return false;
	}
	TestEqual(TEXT("Round-trip uses SaveGame v9"), Loaded->SaveVersion, 9);

	AGridLevelRuntimeActor* RestoredRuntime = SpawnRuntime(*this, TestWorld.World, Fixture, &Loaded->DungeonRuntimeState);
	if (!RestoredRuntime)
	{
		return false;
	}
	AGridReceptacleActor* RestoredReceptacle = RestoredRuntime->FindRuntimeObjectActor<AGridReceptacleActor>(ReceptacleObjectId);
	TestTrue(TEXT("Rebuild initializes legacy runtime default before apply"), RestoredReceptacle->bCanRemoveItem);
	TestTrue(TEXT("Saved runtime snapshot applies"), RestoredRuntime->ApplyCurrentLevelRuntimeState());
	TestFalse(TEXT("Disabled removal permission survives Save/Continue"), RestoredReceptacle->bCanRemoveItem);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD011EnabledRoundTripTest,
	"Grimrock.TechnicalDebt.TD01_1.ReceptaclePersistence.EnabledRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD011EnabledRoundTripTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FTestWorld TestWorld;
	if (!TestWorld.World)
	{
		AddError(TEXT("TD01.1 could not create a transient game world."));
		return false;
	}

	const FRuntimeFixture Fixture = MakeFixture(GetTransientPackage());
	AGridLevelRuntimeActor* Runtime = SpawnRuntime(*this, TestWorld.World, Fixture);
	if (!Runtime)
	{
		return false;
	}

	AGridReceptacleActor* Receptacle = Runtime->FindRuntimeObjectActor<AGridReceptacleActor>(ReceptacleObjectId);
	Receptacle->SetCanRemoveItem(false);
	Receptacle->SetCanRemoveItem(true);
	TestTrue(TEXT("Runtime receptacle is enabled before capture"), Receptacle->bCanRemoveItem);
	TestTrue(TEXT("Enabled runtime state captures"), Runtime->CaptureCurrentLevelRuntimeState());

	const FGridLevelRuntimeState* CapturedState = Runtime->FindRuntimeStateForCurrentLevel();
	const FGridRuntimeReceptacleState* CapturedReceptacle = CapturedState ? CapturedState->Receptacles.Find(ReceptacleObjectId) : nullptr;
	TestNotNull(TEXT("Captured enabled receptacle state exists"), CapturedReceptacle);
	if (!CapturedReceptacle)
	{
		return false;
	}
	TestTrue(TEXT("Snapshot stores enabled removal permission"), CapturedReceptacle->bCanRemoveItem);

	Receptacle->SetCanRemoveItem(false);
	TestFalse(TEXT("Runtime mutation proves restore is authoritative"), Receptacle->bCanRemoveItem);
	TestTrue(TEXT("Captured runtime snapshot reapplies"), Runtime->ApplyCurrentLevelRuntimeState());
	TestTrue(TEXT("Enabled removal permission restores from snapshot"), Receptacle->bCanRemoveItem);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD011V8MigrationTest,
	"Grimrock.TechnicalDebt.TD01_1.ReceptaclePersistence.V8Migration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD011V8MigrationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UGrimrockPartySaveGame* Save = NewObject<UGrimrockPartySaveGame>(GetTransientPackage());
	Save->SaveVersion = 8;

	FGridLevelRuntimeState LevelState;
	LevelState.LevelId = TEXT("SingleLevel");
	LevelState.bHasBeenVisited = true;
	FGridRuntimeReceptacleState LegacyReceptacle;
	LegacyReceptacle.ObjectId = ReceptacleObjectId;
	LegacyReceptacle.bCanRemoveItem = false;
	LevelState.Receptacles.Add(ReceptacleObjectId, LegacyReceptacle);
	Save->DungeonRuntimeState.LevelStates.Add(LevelState.LevelId, LevelState);

	FText Error;
	FRPGSaveMigrationReport Report;
	TestTrue(TEXT("SaveGame v8 migrates to current contract"), FRPGSaveMigrationService::PrepareLoadedSave(Save, Error, &Report));
	TestEqual(TEXT("TD01.1 current SaveGame contract is v9"), UGrimrockPartySaveGame::CurrentSaveVersion, 9);
	TestEqual(TEXT("Migrated legacy save is v9"), Save->SaveVersion, 9);
	TestEqual(TEXT("Migration report preserves source version"), Report.SourceVersion, 8);
	TestEqual(TEXT("Migration report targets v9"), Report.TargetVersion, 9);
	TestTrue(TEXT("Migration report marks v8 as migrated"), Report.bMigrated);

	const FGridLevelRuntimeState* MigratedLevel = Save->DungeonRuntimeState.LevelStates.Find(TEXT("SingleLevel"));
	const FGridRuntimeReceptacleState* MigratedReceptacle = MigratedLevel ? MigratedLevel->Receptacles.Find(ReceptacleObjectId) : nullptr;
	TestNotNull(TEXT("Migrated legacy receptacle remains present"), MigratedReceptacle);
	if (!MigratedReceptacle)
	{
		return false;
	}
	TestTrue(TEXT("Legacy v8 policy preserves historical removable behavior"), MigratedReceptacle->bCanRemoveItem);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
