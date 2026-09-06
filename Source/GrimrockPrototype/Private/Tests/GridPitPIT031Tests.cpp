#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Components/StaticMeshComponent.h"

#include "Core/GridDungeonAsset.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridObjectArchetypeAsset.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Runtime/GridActivationComponent.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPitTrapdoorActor.h"

namespace GridPIT031
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
				FName(*FString::Printf(TEXT("PIT031TestWorld_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true,
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

	UGridLevelAsset* MakeFloor(UObject* Outer)
	{
		UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Outer);
		Level->Width = 5;
		Level->Height = 5;
		Level->CellSize = 200.0f;
		Level->EnsureCellCount();
		for (FGridLevelCellData& Cell : Level->Cells)
		{
			Cell.CellType = EGridCellType::Floor;
			Cell.bBlocksOccupancy = false;
		}
		return Level;
	}

	FGridLevelObjectData MakeAnimatedPit(int32 X, int32 Y, FName TargetLevelId)
	{
		FGridLevelObjectData Pit;
		Pit.ObjectId = FGuid::NewGuid();
		Pit.Type = EGridLevelObjectType::Pit;
		Pit.ArchetypeId = TEXT("Pit_Animated_Test");
		Pit.CellX = X;
		Pit.CellY = Y;
		Pit.Edge = EGridEdge::None;
		Pit.bInitiallyEnabled = true;
		Pit.Behavior.Pit.bInitiallyOpen = false;
		Pit.Behavior.Pit.bUseSameCellCoordinates = true;
		Pit.Behavior.PitAnimation.LeftHingeLocation = FVector(-85.0f, 0.0f, -5.0f);
		Pit.Behavior.PitAnimation.RightHingeLocation = FVector(85.0f, 0.0f, -5.0f);
		Pit.Behavior.PitAnimation.OpenAngleDegrees = 80.0f;
		Pit.Behavior.PitAnimation.MoveDuration = 1.0f;
		Pit.Behavior.Transition.bIsTransition = true;
		Pit.Behavior.Transition.TargetLevelId = TargetLevelId;
		Pit.Behavior.Transition.TargetFacing = EGridEdge::North;
		Pit.Behavior.Transition.bRequireUseAction = false;
		return Pit;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridPIT031AnimationRuntimeTest, "Grimrock.Pit.PIT03_2.AnimationRuntimeContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridPIT031AnimationRuntimeTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridPIT031;

	FTestWorld TestWorld;
	if (!TestNotNull(TEXT("PIT03.2 world exists"), TestWorld.World))
	{
		return false;
	}

	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	if (!TestNotNull(TEXT("Runtime exists"), Runtime))
	{
		return false;
	}

	UGridDungeonAsset* Dungeon = NewObject<UGridDungeonAsset>(Runtime);
	UGridLevelAsset* Upper = MakeFloor(Dungeon);
	UGridLevelAsset* Lower = MakeFloor(Dungeon);
	const FName UpperId(TEXT("PIT031_Upper"));
	const FName LowerId(TEXT("PIT031_Lower"));

	FGridDungeonLevelEntry UpperEntry;
	UpperEntry.LevelId = UpperId;
	UpperEntry.DisplayName = FText::FromString(TEXT("Upper"));
	UpperEntry.LevelAsset = Upper;
	UpperEntry.LogicalPosition = FIntVector(0, 0, 1);
	UpperEntry.bEnabled = true;

	FGridDungeonLevelEntry LowerEntry;
	LowerEntry.LevelId = LowerId;
	LowerEntry.DisplayName = FText::FromString(TEXT("Lower"));
	LowerEntry.LevelAsset = Lower;
	LowerEntry.LogicalPosition = FIntVector(0, 0, 0);
	LowerEntry.bEnabled = true;

	Dungeon->DefaultLevelId = UpperId;
	Dungeon->Levels = { UpperEntry, LowerEntry };

	FGridLevelObjectData PitA = MakeAnimatedPit(2, 2, LowerId);
	FGridLevelObjectData PitB = MakeAnimatedPit(3, 2, LowerId);
	const FGuid PitAId = PitA.ObjectId;
	const FGuid PitBId = PitB.ObjectId;
	Upper->Objects = { PitA, PitB };

	FGridObjectLink OpenSecondPit;
	OpenSecondPit.SourceObjectId = PitAId;
	OpenSecondPit.TargetObjectId = PitBId;
	OpenSecondPit.SourceEvent = EGridObjectEvent::Opened;
	OpenSecondPit.Command = EGridObjectCommand::Open;
	Upper->Links = { OpenSecondPit };

	UStaticMesh* PitMesh = NewObject<UStaticMesh>(Runtime);
	UStaticMesh* LeftLeafMesh = NewObject<UStaticMesh>(Runtime);
	UStaticMesh* RightLeafMesh = NewObject<UStaticMesh>(Runtime);
	UGridObjectArchetypeAsset* PitArchetype = NewObject<UGridObjectArchetypeAsset>(Runtime);
	PitArchetype->ArchetypeId = TEXT("Pit_Animated_Test");
	PitArchetype->SupportedType = EGridLevelObjectType::Pit;
	PitArchetype->PlacementKind = EGridObjectPlacementKind::Floor;
	PitArchetype->StaticPart.Mesh = PitMesh;
	PitArchetype->MovingParts.Part0.Mesh = LeftLeafMesh;
	PitArchetype->MovingParts.Part0.Motion.Type = EGridWorldObjectMotionType::Rotation;
	PitArchetype->MovingParts.Part0.Motion.Axis = EGridWorldObjectMotionAxis::Y;
	PitArchetype->MovingParts.Part0.Motion.Pivot = PitA.Behavior.PitAnimation.LeftHingeLocation;
	PitArchetype->MovingParts.Part0.Motion.Amount = -PitA.Behavior.PitAnimation.OpenAngleDegrees;
	PitArchetype->MovingParts.Part0.Motion.Duration = PitA.Behavior.PitAnimation.MoveDuration;
	PitArchetype->MovingParts.Part1.Mesh = RightLeafMesh;
	PitArchetype->MovingParts.Part1.Motion.Type = EGridWorldObjectMotionType::Rotation;
	PitArchetype->MovingParts.Part1.Motion.Axis = EGridWorldObjectMotionAxis::Y;
	PitArchetype->MovingParts.Part1.Motion.Pivot = PitA.Behavior.PitAnimation.RightHingeLocation;
	PitArchetype->MovingParts.Part1.Motion.Amount = PitA.Behavior.PitAnimation.OpenAngleDegrees;
	PitArchetype->MovingParts.Part1.Motion.Duration = PitA.Behavior.PitAnimation.MoveDuration;
	PitArchetype->RuntimeActorClass = AGridPitTrapdoorActor::StaticClass();

	Runtime->DungeonAsset = Dungeon;
	Runtime->CurrentDungeonLevelId = UpperId;
	Runtime->LevelAsset = Upper;
	Runtime->ObjectArchetypes.Add(PitArchetype);
	Runtime->RebuildRuntimeObjects();

	UGridActivationComponent* Activation = Runtime->FindComponentByClass<UGridActivationComponent>();
	if (!TestNotNull(TEXT("Activation component exists"), Activation))
	{
		return false;
	}
	Activation->Initialize(Runtime);
	Activation->RebuildIndexes();

	AGridPitTrapdoorActor* PitAActor = Runtime->FindRuntimeObjectActor<AGridPitTrapdoorActor>(PitAId);
	AGridPitTrapdoorActor* PitBActor = Runtime->FindRuntimeObjectActor<AGridPitTrapdoorActor>(PitBId);
	if (!TestNotNull(TEXT("Pit A presentation actor exists"), PitAActor) ||
		!TestNotNull(TEXT("Pit B presentation actor exists"), PitBActor))
	{
		return false;
	}

	TestTrue(TEXT("Pit A has a complete dual-leaf cover"), PitAActor->HasCompleteTrapdoorCover());
	TestEqual(TEXT("Left hinge uses requested local X"), PitAActor->GetLeftHingeLocation().X, -85.0);
	TestEqual(TEXT("Left hinge uses requested local Y"), PitAActor->GetLeftHingeLocation().Y, 0.0);
	TestEqual(TEXT("Left hinge uses requested local Z"), PitAActor->GetLeftHingeLocation().Z, -5.0);
	TestEqual(TEXT("Right hinge uses requested local X"), PitAActor->GetRightHingeLocation().X, 85.0);
	TestEqual(TEXT("Right hinge uses requested local Y"), PitAActor->GetRightHingeLocation().Y, 0.0);
	TestEqual(TEXT("Right hinge uses requested local Z"), PitAActor->GetRightHingeLocation().Z, -5.0);

	TestTrue(TEXT("Closed left leaf uses authored MovingPart local transform"),
		PitAActor->LeftLeafMeshComponent->GetRelativeTransform().Equals(PitArchetype->MovingParts.Part0.LocalTransform, KINDA_SMALL_NUMBER));
	TestTrue(TEXT("Closed right leaf uses authored MovingPart local transform"),
		PitAActor->RightLeafMeshComponent->GetRelativeTransform().Equals(PitArchetype->MovingParts.Part1.LocalTransform, KINDA_SMALL_NUMBER));

	TestEqual(TEXT("Closed left leaf pitch is zero"), PitAActor->GetLeftLeafPitch(), 0.0f);
	TestEqual(TEXT("Closed right leaf pitch is zero"), PitAActor->GetRightLeafPitch(), 0.0f);

	TestFalse(TEXT("Pit A starts closed for gameplay"), Runtime->IsPitOpen(PitAId));
	TestFalse(TEXT("Pit B starts closed for gameplay"), Runtime->IsPitOpen(PitBId));
	TestEqual(TEXT("Pit A starts at alpha 0"), PitAActor->GetCurrentOpenAlpha(), 0.0f);

	UGridItemDefinitionAsset* StoneDefinition = NewObject<UGridItemDefinitionAsset>(Runtime);
	StoneDefinition->ItemDefinitionId = TEXT("Item_PIT031_Stone");
	StoneDefinition->Weight = 1.5f;

	FGridItemInstance Stone;
	Stone.RuntimeObjectId = FGuid::NewGuid();
	Stone.ItemDefinitionId = StoneDefinition->ItemDefinitionId;
	Stone.Quantity = 1;

	TestTrue(TEXT("Stone can rest on closed animated trapdoor"),
		Runtime->TryDropItemInstanceAtCell(Stone, StoneDefinition, 2, 2, EGridEdge::None, FVector::ZeroVector));
	TestEqual(TEXT("Closed trapdoor carries stone weight"), Runtime->GetWorldItemWeightAtCell(2, 2, false), StoneDefinition->Weight);

	TestTrue(TEXT("Open command starts animated Pit A"), Runtime->SetPitOpen(PitAId, true, true));
	TestTrue(TEXT("Pit A animation is running"), PitAActor->IsAnimating());
	TestTrue(TEXT("Pit A target is Open"), PitAActor->IsTargetOpen());
	TestTrue(TEXT("Pit A becomes gameplay-open immediately when Open is commanded"), Runtime->IsPitOpen(PitAId));

	FGridObjectTransitionParams DuringOpeningTransition;
	TestTrue(TEXT("Party movement detector resolves Pit A as open while leaves are still opening"),
		Runtime->FindOpenPitAtCell(2, 2, DuringOpeningTransition));

	TestTrue(TEXT("Pit A Opened event immediately starts Pit B opening"), PitBActor->IsTargetOpen());
	TestTrue(TEXT("Pit B animation immediately starts from chained Opened"), PitBActor->IsAnimating());
	TestTrue(TEXT("Pit B is gameplay-open immediately while its leaves are opening"), Runtime->IsPitOpen(PitBId));
	TestEqual(TEXT("Stone falls immediately when Open command is received"), Runtime->GetWorldItemWeightAtCell(2, 2, false), 0.0f);

	const FGridLevelRuntimeState* LowerState = Runtime->DungeonRuntimeState.LevelStates.Find(LowerId);
	if (!TestNotNull(TEXT("Lower state exists after stone fall"), LowerState))
	{
		return false;
	}
	TestTrue(TEXT("Stone entered PIT02 pending queue immediately on Open command"), LowerState->PendingInboundItems.Contains(Stone.RuntimeObjectId));

	PitAActor->Tick(0.40f);
	TestTrue(TEXT("Pit A is partially open visually"), PitAActor->GetCurrentOpenAlpha() > 0.35f && PitAActor->GetCurrentOpenAlpha() < 0.45f);
	TestTrue(TEXT("Left leaf rotates downward around Y during opening"), FMath::IsNearlyEqual(PitAActor->GetLeftLeafPitch(), -32.0f, 0.5f));
	TestTrue(TEXT("Right leaf counter-rotates downward around Y during opening"), FMath::IsNearlyEqual(PitAActor->GetRightLeafPitch(), 32.0f, 0.5f));
	TestTrue(TEXT("Partially opened Pit A remains gameplay-open"), Runtime->IsPitOpen(PitAId));

	PitAActor->Tick(0.60f);
	TestFalse(TEXT("Pit A visual animation completed"), PitAActor->IsAnimating());
	TestTrue(TEXT("Pit A remains gameplay-open at visual endpoint"), Runtime->IsPitOpen(PitAId));
	TestEqual(TEXT("Pit A reaches alpha 1"), PitAActor->GetCurrentOpenAlpha(), 1.0f);
	TestTrue(TEXT("Left leaf reaches -80 degrees"), FMath::IsNearlyEqual(PitAActor->GetLeftLeafPitch(), -80.0f, 0.1f));
	TestTrue(TEXT("Right leaf reaches +80 degrees"), FMath::IsNearlyEqual(PitAActor->GetRightLeafPitch(), 80.0f, 0.1f));

	PitBActor->Tick(1.0f);
	TestTrue(TEXT("Pit B becomes gameplay-open at its endpoint"), Runtime->IsPitOpen(PitBId));

	TestTrue(TEXT("Closing Pit A starts reverse endpoint request"), Runtime->SetPitOpen(PitAId, false, false));
	TestTrue(TEXT("Pit A remains gameplay-open while closing"), Runtime->IsPitOpen(PitAId));
	PitAActor->Tick(0.35f);
	const float AlphaBeforeReverse = PitAActor->GetCurrentOpenAlpha();
	TestTrue(TEXT("Pit A moved part-way toward Closed"), AlphaBeforeReverse > 0.60f && AlphaBeforeReverse < 0.70f);

	TestTrue(TEXT("Toggle during closing reverses toward Open"), Runtime->TogglePit(PitAId, false));
	TestTrue(TEXT("Reversal target is Open"), PitAActor->IsTargetOpen());
	TestTrue(TEXT("Reversal keeps current angle instead of snapping"), FMath::IsNearlyEqual(PitAActor->GetCurrentOpenAlpha(), AlphaBeforeReverse, 0.01f));

	PitAActor->Tick(0.35f);
	TestFalse(TEXT("Reversal completes"), PitAActor->IsAnimating());
	TestTrue(TEXT("Pit A remains gameplay-open after reversing before Closed endpoint"), Runtime->IsPitOpen(PitAId));
	TestEqual(TEXT("Pit A returns to alpha 1"), PitAActor->GetCurrentOpenAlpha(), 1.0f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridPIT031NoCoverFallbackTest, "Grimrock.Pit.PIT03_2.NoCoverImmediateFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridPIT031NoCoverFallbackTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridPIT031;

	FTestWorld TestWorld;
	if (!TestNotNull(TEXT("Fallback world exists"), TestWorld.World))
	{
		return false;
	}

	AGridPitTrapdoorActor* Actor = TestWorld.World->SpawnActor<AGridPitTrapdoorActor>();
	if (!TestNotNull(TEXT("Fallback pit actor exists"), Actor))
	{
		return false;
	}

	FGridLevelObjectData Pit = MakeAnimatedPit(1, 1, TEXT("Lower"));
	Pit.Behavior.Pit.bInitiallyOpen = false;
	Actor->InitializeMechanismVisuals(Pit, nullptr, FTransform::Identity);
	Actor->InitializeGridObject(Pit, nullptr, FTransform::Identity);

	TestFalse(TEXT("No-cover Pit has no dual-leaf cover"), Actor->HasCompleteTrapdoorCover());
	TestTrue(TEXT("No-cover Pit is forced Open immediately"), Actor->IsPitOpenVisualState());

	int32 CompletionCount = 0;
	Actor->OnPitAnimationFinished.AddLambda(
		[&CompletionCount](FGuid ObjectId, bool bWasOpen, bool bIsOpen)
		{
			(void)ObjectId;
			if (!bWasOpen && bIsOpen)
			{
				++CompletionCount;
			}
		});

	Actor->SetPitOpenVisualState(false, false);
	TestFalse(TEXT("No-cover static Pit does not animate"), Actor->IsAnimating());
	TestTrue(TEXT("No-cover static Pit ignores Close and remains Open"), Actor->IsPitOpenVisualState());
	TestEqual(TEXT("No-cover Close does not fabricate an Open completion"), CompletionCount, 0);
	return true;
}

#endif
