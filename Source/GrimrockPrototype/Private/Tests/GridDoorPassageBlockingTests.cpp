#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridDoorActor.h"
#include "Runtime/GridDoorSystemComponent.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridSecretDoorActor.h"

namespace
{
	struct FGridDoorPassageTestWorld
	{
		UWorld* World = nullptr;

		FGridDoorPassageTestWorld()
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
				FName(*FString::Printf(TEXT("DoorPassageBlockingWorld_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true,
				ERHIFeatureLevel::Num, &InitializationValues);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FGridDoorPassageTestWorld()
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

	UGridLevelAsset* MakeDoorPassageLevel(AGridLevelRuntimeActor* Runtime, FGridLevelObjectData& OutDoorData)
	{
		if (!Runtime)
		{
			return nullptr;
		}

		UGridLevelAsset* LevelAsset = NewObject<UGridLevelAsset>(Runtime);
		LevelAsset->Width = 3;
		LevelAsset->Height = 3;
		LevelAsset->CellSize = 200.f;
		LevelAsset->EnsureCellCount();
		for (FGridLevelCellData& Cell : LevelAsset->Cells)
		{
			Cell.CellType = EGridCellType::Floor;
			Cell.NorthWall = EGridWallType::None;
			Cell.EastWall = EGridWallType::None;
			Cell.SouthWall = EGridWallType::None;
			Cell.WestWall = EGridWallType::None;
			Cell.bBlocksOccupancy = false;
		}

		OutDoorData = FGridLevelObjectData();
		OutDoorData.ObjectId = FGuid::NewGuid();
		OutDoorData.Type = EGridLevelObjectType::Door;
		OutDoorData.CellX = 1;
		OutDoorData.CellY = 1;
		OutDoorData.Edge = EGridEdge::North;
		OutDoorData.bInitiallyActive = false;
		OutDoorData.Behavior.DoorAnimation.OpenHeight = 180.f;
		OutDoorData.Behavior.DoorAnimation.MoveDuration = 1.f;
		LevelAsset->Objects.Add(OutDoorData);
		Runtime->LevelAsset = LevelAsset;
		return LevelAsset;
	}

	bool ExerciseOpeningPassageBlocking(
		FAutomationTestBase& Test, UWorld* World, UClass* DoorClass, const TCHAR* DoorLabel)
	{
		AGridLevelRuntimeActor* Runtime = World ? World->SpawnActor<AGridLevelRuntimeActor>() : nullptr;
		Test.TestNotNull(*FString::Printf(TEXT("%s runtime is spawned"), DoorLabel), Runtime);
		if (!Runtime)
		{
			return false;
		}

		FGridLevelObjectData DoorData;
		if (!MakeDoorPassageLevel(Runtime, DoorData))
		{
			Test.AddError(FString::Printf(TEXT("%s level fixture could not be created"), DoorLabel));
			return false;
		}

		UGridDoorSystemComponent* DoorSystem = Runtime->FindComponentByClass<UGridDoorSystemComponent>();
		Test.TestNotNull(*FString::Printf(TEXT("%s door system exists"), DoorLabel), DoorSystem);
		if (!DoorSystem)
		{
			return false;
		}
		DoorSystem->Initialize(Runtime);
		DoorSystem->RebuildIndexes();

		AGridDoorActor* Door = World->SpawnActor<AGridDoorActor>(DoorClass);
		Test.TestNotNull(*FString::Printf(TEXT("%s actor is spawned"), DoorLabel), Door);
		if (!Door)
		{
			return false;
		}

		Door->InitializeDoor(DoorData, nullptr, nullptr, nullptr, nullptr, FVector::ZeroVector, FRotator::ZeroRotator, false);
		DoorSystem->RegisterDoorObject(DoorData, Door);

		Test.TestTrue(*FString::Printf(TEXT("%s starts fully closed"), DoorLabel), Door->IsFullyClosed());
		Test.TestTrue(*FString::Printf(TEXT("%s initially blocks party passage"), DoorLabel), DoorSystem->IsDoorPassageBlocked(1, 1, EGridEdge::North));
		Test.TestFalse(*FString::Printf(TEXT("%s initially rejects CanMove"), DoorLabel), Runtime->CanMove(1, 1, EGridEdge::North));

		Test.TestTrue(*FString::Printf(TEXT("%s accepts the open command"), DoorLabel), Runtime->OpenDoorOnEdge(1, 1, EGridEdge::North));
		Test.TestTrue(*FString::Printf(TEXT("%s is animating after the open command"), DoorLabel), Door->IsAnimating());
		Test.TestFalse(*FString::Printf(TEXT("%s is not fully open while animating"), DoorLabel), Door->IsFullyOpen());
		Test.TestTrue(*FString::Printf(TEXT("%s remains passage-blocking while opening"), DoorLabel),
			DoorSystem->IsDoorPassageBlocked(1, 1, EGridEdge::North));
		Test.TestFalse(*FString::Printf(TEXT("%s still rejects CanMove while opening"), DoorLabel), Runtime->CanMove(1, 1, EGridEdge::North));

		Door->Tick(0.5f);
		Test.TestTrue(*FString::Printf(TEXT("%s remains animating at half travel"), DoorLabel), Door->IsAnimating());
		Test.TestTrue(*FString::Printf(TEXT("%s remains blocked at half travel"), DoorLabel),
			DoorSystem->IsDoorPassageBlocked(1, 1, EGridEdge::North));
		Test.TestFalse(*FString::Printf(TEXT("%s still rejects CanMove at half travel"), DoorLabel), Runtime->CanMove(1, 1, EGridEdge::North));

		Door->Tick(0.5f);
		Test.TestTrue(*FString::Printf(TEXT("%s reaches fully open"), DoorLabel), Door->IsFullyOpen());
		Test.TestFalse(*FString::Printf(TEXT("%s passage unblocks only after opening finishes"), DoorLabel),
			DoorSystem->IsDoorPassageBlocked(1, 1, EGridEdge::North));
		Test.TestTrue(*FString::Printf(TEXT("%s allows CanMove only when fully open"), DoorLabel), Runtime->CanMove(1, 1, EGridEdge::North));
		return true;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridDoorPassageBlockingDuringOpeningTest, "Grimrock.Runtime.Doors.PassageBlockedUntilFullyOpen",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridDoorPassageBlockingDuringOpeningTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridDoorPassageTestWorld TestWorld;
	TestNotNull(TEXT("The door-passage transient world is created"), TestWorld.World);
	if (!TestWorld.World)
	{
		return false;
	}

	ExerciseOpeningPassageBlocking(*this, TestWorld.World, AGridDoorActor::StaticClass(), TEXT("Normal door"));
	ExerciseOpeningPassageBlocking(*this, TestWorld.World, AGridSecretDoorActor::StaticClass(), TEXT("Secret door"));
	return true;
}

#endif
