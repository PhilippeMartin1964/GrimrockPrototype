#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GrimrockPartyPawn.h"

namespace
{
	struct FGridBlockedMovementTestWorld
	{
		UWorld* World = nullptr;

		FGridBlockedMovementTestWorld()
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
				FName(*FString::Printf(TEXT("PartyBlockedMovementWorld_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true,
				ERHIFeatureLevel::Num, &InitializationValues);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FGridBlockedMovementTestWorld()
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridPartyBlockedMovementFeedbackTest, "Grimrock.Runtime.PartyMovement.BlockedFeedback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridPartyBlockedMovementFeedbackTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridBlockedMovementTestWorld TestWorld;
	TestNotNull(TEXT("The blocked-movement transient world is created"), TestWorld.World);
	if (!TestWorld.World)
	{
		return false;
	}

	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	AGrimrockPartyPawn* Party = TestWorld.World->SpawnActor<AGrimrockPartyPawn>();
	TestNotNull(TEXT("The blocked-movement runtime is spawned"), Runtime);
	TestNotNull(TEXT("The blocked-movement party is spawned"), Party);
	if (!Runtime || !Party)
	{
		return false;
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
	LevelAsset->GetCellMutable(1, 1).NorthWall = EGridWallType::Solid;
	Runtime->LevelAsset = LevelAsset;

	Party->BlockedMoveDistance = 15.f;
	Party->BlockedMoveForwardDuration = 0.08f;
	Party->BlockedMoveReturnDuration = 0.10f;
	Party->bEnableBlockedMoveFeedback = true;
	Party->SetGridStart(Runtime, 1, 1, EGridEdge::North);

	const FVector LogicalOrigin = Runtime->GetCellCenterWorld(1, 1, Party->EyeHeight);
	TestFalse(TEXT("A wall still rejects the logical translation"), Party->TryStartMove(EGridEdge::North));
	TestTrue(TEXT("A spatial rejection starts blocked-movement feedback"), Party->bIsBlockedMoveFeedbackActive);
	TestFalse(TEXT("Blocked feedback never enters the real movement state"), Party->bIsMoving);
	TestEqual(TEXT("Blocked feedback preserves logical X"), Party->CurrentCellX, 1);
	TestEqual(TEXT("Blocked feedback preserves logical Y"), Party->CurrentCellY, 1);

	Party->BufferMoveCommand(EGridEdge::East);
	TestTrue(TEXT("Inputs pressed during blocked feedback are not buffered"),
		Party->BufferedCommandType == AGrimrockPartyPawn::EBufferedCommandType::None);

	Party->UpdateBlockedMoveFeedback(Party->BlockedMoveForwardDuration);
	const FVector ExpectedImpact = LogicalOrigin + FVector(0.f, Party->BlockedMoveDistance, 0.f);
	TestTrue(TEXT("The feedback reaches the configured visual impact distance"),
		Party->GetActorLocation().Equals(ExpectedImpact, 0.01f));
	TestEqual(TEXT("The logical X is unchanged at visual impact"), Party->CurrentCellX, 1);
	TestEqual(TEXT("The logical Y is unchanged at visual impact"), Party->CurrentCellY, 1);

	Party->UpdateBlockedMoveFeedback(Party->BlockedMoveReturnDuration);
	TestFalse(TEXT("The blocked feedback ends after the return phase"), Party->bIsBlockedMoveFeedbackActive);
	TestTrue(TEXT("The party returns exactly to its logical cell center"),
		Party->GetActorLocation().Equals(LogicalOrigin, KINDA_SMALL_NUMBER));
	TestEqual(TEXT("The logical X is unchanged after feedback"), Party->CurrentCellX, 1);
	TestEqual(TEXT("The logical Y is unchanged after feedback"), Party->CurrentCellY, 1);

	Party->bEnableBlockedMoveFeedback = false;
	TestFalse(TEXT("A blocked translation remains rejected when feedback is disabled"), Party->TryStartMove(EGridEdge::North));
	TestFalse(TEXT("Disabling feedback suppresses only the visual nudge"), Party->bIsBlockedMoveFeedbackActive);
	TestTrue(TEXT("Disabling feedback leaves the pawn on its logical cell"),
		Party->GetActorLocation().Equals(LogicalOrigin, KINDA_SMALL_NUMBER));

	Party->bEnableBlockedMoveFeedback = true;
	LevelAsset->GetCellMutable(1, 1).NorthWall = EGridWallType::None;
	TestTrue(TEXT("An open passage still starts the normal grid move"), Party->TryStartMove(EGridEdge::North));
	TestTrue(TEXT("An open passage uses the real movement state"), Party->bIsMoving);
	TestFalse(TEXT("A real move never starts blocked feedback"), Party->bIsBlockedMoveFeedbackActive);
	TestEqual(TEXT("The accepted move updates logical X normally"), Party->CurrentCellX, 1);
	TestEqual(TEXT("The accepted move updates logical Y normally"), Party->CurrentCellY, 2);

	return true;
}

#endif
