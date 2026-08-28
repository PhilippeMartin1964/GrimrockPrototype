#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridDirectionUtils.h"
#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GrimrockPartyPawn.h"

namespace
{
	struct FGridTD029TestWorld
	{
		UWorld* World = nullptr;

		FGridTD029TestWorld()
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
				FName(*FString::Printf(TEXT("TD029PartyMovementWorld_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true,
				ERHIFeatureLevel::Num, &InitializationValues);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FGridTD029TestWorld()
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

	bool TD029YawMatches(float ActualYaw, float ExpectedYaw)
	{
		return FMath::Abs(FMath::FindDeltaAngleDegrees(ActualYaw, ExpectedYaw)) <= KINDA_SMALL_NUMBER;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD029PartyMovementFacadeGridStartContractTest, "Grimrock.TechnicalDebt.TD02_9.PartyMovementFacade.GridStartContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD029PartyMovementFacadeGridStartContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridTD029TestWorld TestWorld;
	TestNotNull(TEXT("The transient world is created"), TestWorld.World);
	if (!TestWorld.World)
	{
		return false;
	}

	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	AGrimrockPartyPawn* Party = TestWorld.World->SpawnActor<AGrimrockPartyPawn>();
	TestNotNull(TEXT("The level runtime is spawned"), Runtime);
	TestNotNull(TEXT("The party pawn is spawned"), Party);
	if (!Runtime || !Party)
	{
		return false;
	}

	UGridLevelAsset* LevelAsset = NewObject<UGridLevelAsset>(Runtime);
	LevelAsset->Width = 3;
	LevelAsset->Height = 3;
	LevelAsset->EnsureCellCount();
	for (FGridLevelCellData& Cell : LevelAsset->Cells)
	{
		Cell.CellType = EGridCellType::Floor;
		Cell.bBlocksOccupancy = false;
	}
	Runtime->LevelAsset = LevelAsset;

	Party->SetGridStart(Runtime, 1, 1, EGridEdge::North);
	TestTrue(TEXT("SetGridStart keeps the supplied runtime authority"), Party->LevelRuntimeActor == Runtime);
	TestEqual(TEXT("SetGridStart stores the X cell"), Party->CurrentCellX, 1);
	TestEqual(TEXT("SetGridStart stores the Y cell"), Party->CurrentCellY, 1);
	TestEqual(TEXT("SetGridStart stores the facing"), Party->Facing, EGridEdge::North);
	TestTrue(TEXT("SetGridStart snaps the actor to the cell center"),
		Party->GetActorLocation().Equals(Runtime->GetCellCenterWorld(1, 1, Party->EyeHeight), KINDA_SMALL_NUMBER));
	TestTrue(
		TEXT("SetGridStart snaps actor yaw to the grid facing"), TD029YawMatches(Party->GetActorRotation().Yaw, GridDirectionUtils::ToYaw(EGridEdge::North)));

	Party->CurrentCellX = 2;
	Party->CurrentCellY = 0;
	Party->Facing = EGridEdge::West;
	Party->SnapToCurrentCell();
	TestTrue(TEXT("SnapToCurrentCell follows the current logical cell"),
		Party->GetActorLocation().Equals(Runtime->GetCellCenterWorld(2, 0, Party->EyeHeight), KINDA_SMALL_NUMBER));
	TestTrue(TEXT("SnapToCurrentCell follows the current logical facing"),
		TD029YawMatches(Party->GetActorRotation().Yaw, GridDirectionUtils::ToYaw(EGridEdge::West)));

	const FVector LocationBeforeMissingRuntime = Party->GetActorLocation();
	const FRotator RotationBeforeMissingRuntime = Party->GetActorRotation();
	Party->LevelRuntimeActor = nullptr;
	Party->CurrentCellX = 0;
	Party->CurrentCellY = 2;
	Party->Facing = EGridEdge::South;
	Party->SnapToCurrentCell();
	TestTrue(TEXT("SnapToCurrentCell is non-destructive without a level runtime"),
		Party->GetActorLocation().Equals(LocationBeforeMissingRuntime, KINDA_SMALL_NUMBER));
	TestTrue(TEXT("Missing runtime leaves actor rotation unchanged"), Party->GetActorRotation().Equals(RotationBeforeMissingRuntime, KINDA_SMALL_NUMBER));

	return true;
}

#endif
