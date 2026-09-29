#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "InputActionValue.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "UI/GridMapWidget.h"

namespace
{
	struct FGridMapInput01TestWorld
	{
		UWorld* World = nullptr;

		FGridMapInput01TestWorld()
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
				EWorldType::Game,
				false,
				FName(*FString::Printf(TEXT("MapInput01World_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
				nullptr,
				true,
				ERHIFeatureLevel::Num,
				&Values);

			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FGridMapInput01TestWorld()
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

	UGridLevelAsset* MakeOpenLevel(AGridLevelRuntimeActor* Runtime)
	{
		UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Runtime);
		Level->Width = 32;
		Level->Height = 32;
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
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMapInput01MovementLockTest, "Grimrock.UI.MapInput01.KeyboardMovementLock",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMapInput01MovementLockTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridMapInput01TestWorld TestWorld;
	if (!TestNotNull(TEXT("Transient world exists"), TestWorld.World))
	{
		return false;
	}

	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	AGrimrockPartyPawn* Party = TestWorld.World->SpawnActor<AGrimrockPartyPawn>();
	if (!TestNotNull(TEXT("Runtime actor exists"), Runtime) || !TestNotNull(TEXT("Party pawn exists"), Party))
	{
		return false;
	}

	MakeOpenLevel(Runtime);
	Party->SetGridStart(Runtime, 10, 10, EGridEdge::North);

	UGridMapWidget* Map = NewObject<UGridMapWidget>(Party);
	if (!TestNotNull(TEXT("Standalone map state exists"), Map))
	{
		return false;
	}

	Party->MapWidgetInstance = Map;
	Map->SetVisibility(ESlateVisibility::Visible);
	TestTrue(TEXT("Map is visible for the keyboard movement lock"), Party->IsMapWidgetVisible());

	const FInputActionValue EmptyInput;
	Party->HandleMoveForward(EmptyInput);
	Party->HandleMoveBackward(EmptyInput);
	Party->HandleStrafeLeft(EmptyInput);
	Party->HandleStrafeRight(EmptyInput);
	Party->HandleTurnLeft(EmptyInput);
	Party->HandleTurnRight(EmptyInput);

	TestEqual(TEXT("Visible map blocks keyboard translation X"), Party->CurrentCellX, 10);
	TestEqual(TEXT("Visible map blocks keyboard translation Y"), Party->CurrentCellY, 10);
	TestEqual(TEXT("Visible map blocks keyboard rotation"), Party->Facing, EGridEdge::North);
	TestFalse(TEXT("Visible map starts no translation interpolation"), Party->bIsMoving);
	TestFalse(TEXT("Visible map starts no rotation interpolation"), Party->bIsTurning);
	TestTrue(TEXT("Visible map queues no keyboard movement"),
		Party->BufferedCommandType == AGrimrockPartyPawn::EBufferedCommandType::None);

	Map->SetVisibility(ESlateVisibility::Collapsed);
	TestFalse(TEXT("Map is no longer visible"), Party->IsMapWidgetVisible());

	Party->HandleMoveForward(EmptyInput);
	TestTrue(TEXT("Closing map immediately restores keyboard translation"), Party->bIsMoving);
	TestEqual(TEXT("Restored forward input targets the north cell"), Party->CurrentCellY, 11);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
