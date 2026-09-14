#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Core/GridDungeonAsset.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridRelocationUtils.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "Runtime/GridActivationComponent.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GrimrockPartyPawn.h"

namespace
{
	struct FRelocationTestWorld
	{
		UWorld* World = nullptr;

		FRelocationTestWorld()
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
				FName(*FString::Printf(TEXT("RELOC01TestWorld_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true, ERHIFeatureLevel::Num,
				&Values);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FRelocationTestWorld()
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

	UGridLevelAsset* MakeFloor(UObject* Outer, int32 Width = 4, int32 Height = 4)
	{
		UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Outer);
		Level->Width = Width;
		Level->Height = Height;
		Level->EnsureCellCount();
		for (FGridLevelCellData& Cell : Level->Cells)
		{
			Cell.CellType = EGridCellType::Floor;
			Cell.bBlocksOccupancy = false;
		}
		return Level;
	}

}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridRELOC01RuntimeTest, "Grimrock.Relocation.RELOC01.Runtime", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridRELOC01RuntimeTest::RunTest(const FString& Parameters)
{
	FRelocationTestWorld Fixture;
	if (!TestNotNull(TEXT("World"), Fixture.World))
		return false;
	auto* Runtime = Fixture.World->SpawnActor<AGridLevelRuntimeActor>();
	auto* Party = Fixture.World->SpawnActor<AGrimrockPartyPawn>();
	auto* Dungeon = NewObject<UGridDungeonAsset>(Runtime);
	auto* Upper = MakeFloor(Dungeon);
	auto* Lower = MakeFloor(Dungeon);
	FGridDungeonLevelEntry A, B;
	A.LevelId = TEXT("Upper");
	A.LevelAsset = Upper;
	A.LogicalPosition.Z = 1;
	B.LevelId = TEXT("Lower");
	B.LevelAsset = Lower;
	Dungeon->Levels = { A, B };
	Runtime->DungeonAsset = Dungeon;
	Runtime->LevelAsset = Upper;
	Runtime->CurrentDungeonLevelId = A.LevelId;
	Party->SetGridStart(Runtime, 0, 1, EGridEdge::East);
	Party->bNativeMovementAudioPlaybackEnabled = false;
	FGridWorldObjectInstance Teleporter;
	Teleporter.InstanceId = FGuid::NewGuid();
	Teleporter.Type = EGridLevelObjectType::Teleporter;
	Teleporter.CellX = 1;
	Teleporter.CellY = 1;
	Teleporter.InstanceConfig.Teleporter.TargetCellX = 2;
	Teleporter.InstanceConfig.Teleporter.TargetCellY = 3;
	Teleporter.InstanceConfig.bTeleporterInitiallyEnabled = false;
	Upper->WorldObjectInstances = { Teleporter };
	Runtime->FindComponentByClass<UGridActivationComponent>()->RegisterInitialObjectState(Teleporter);
	FGridObjectTransitionParams Resolved = GridRelocation::Resolve(Teleporter);
	TestTrue(TEXT("Legacy None means current level"), Resolved.TargetLevelId.IsNone());
	TestEqual(TEXT("Legacy X"), Resolved.TargetCellX, 2);
	TestEqual(TEXT("Legacy Y"), Resolved.TargetCellY, 3);
	TestEqual(TEXT("Legacy facing preserves incoming facing"), Resolved.TargetFacing, EGridEdge::None);
	TestFalse(TEXT("Reading does not migrate"), Teleporter.InstanceConfig.Transition.bIsTransition);
	TestTrue(TEXT("Enter disabled teleporter"), Party->TryStartMove(EGridEdge::East));
	Party->UpdateMove(10.f);
	TestEqual(TEXT("Inactive teleporter does not execute on entry"), Party->CurrentCellX, 1);
	TestFalse(TEXT("Inactive lookup"), Runtime->FindTransitionAtCell(1, 1, false, Resolved));

	Teleporter.InstanceConfig.bTeleporterInitiallyEnabled = true;
	Runtime->FindComponentByClass<UGridActivationComponent>()->RegisterInitialObjectState(Teleporter);
	TestTrue(TEXT("Active lookup"), Runtime->FindTransitionAtCell(1, 1, false, Resolved));
	Runtime->FindComponentByClass<UGridActivationComponent>()->SetActiveObjectIds({});
	TestFalse(TEXT("Runtime disabled overrides authored enabled state"), Runtime->TryExecuteTransitionAtCell(1, 1, Party, false));
	Runtime->FindComponentByClass<UGridActivationComponent>()->RegisterInitialObjectState(Teleporter);
	FGridWorldObjectInstance Return = Teleporter;
	Return.InstanceId = FGuid::NewGuid();
	Return.CellX = 2;
	Return.CellY = 3;
	Return.InstanceConfig.Teleporter.TargetCellX = 1;
	Return.InstanceConfig.Teleporter.TargetCellY = 1;
	Upper->WorldObjectInstances.Add(Return);
	Runtime->FindComponentByClass<UGridActivationComponent>()->RegisterInitialObjectState(Return);
	const int32 Generation = Runtime->GetRuntimeObjectRebuildGeneration();
	const int32 StateCount = Runtime->DungeonRuntimeState.LevelStates.Num();
	Party->SetGridStart(Runtime, 0, 1, EGridEdge::East);
	TestTrue(TEXT("Enter enabled teleporter"), Party->TryStartMove(EGridEdge::East));
	Party->BufferMoveCommand(EGridEdge::North);
	Party->UpdateMove(10.f);
	TestEqual(TEXT("One hop X"), Party->CurrentCellX, 2);
	TestEqual(TEXT("One hop Y"), Party->CurrentCellY, 3);
	TestEqual(TEXT("Incoming facing preserved"), Party->Facing, EGridEdge::East);
	TestTrue(TEXT("Same asset remains active"), Runtime->LevelAsset == Upper);
	TestEqual(TEXT("No rebuild"), Runtime->GetRuntimeObjectRebuildGeneration(), Generation);
	TestEqual(TEXT("No state capture"), Runtime->DungeonRuntimeState.LevelStates.Num(), StateCount);
	TestFalse(TEXT("Buffered movement cleared"), Party->BufferedCommandType != AGrimrockPartyPawn::EBufferedCommandType::None);

	Resolved.TargetLevelId = A.LevelId;
	Resolved.TargetCellX = 3;
	Resolved.TargetCellY = 2;
	Resolved.TargetFacing = EGridEdge::South;
	GridRelocation::ApplyAuthoringEdit(Upper->WorldObjectInstances[0].InstanceConfig, Teleporter.Type, Resolved);
	auto& Modern = Upper->WorldObjectInstances[0].InstanceConfig;
	TestTrue(TEXT("Edit promotes canonical payload"), Modern.Transition.bIsTransition);
	TestFalse(TEXT("Edit clears Use compatibility flag"), Modern.Transition.bRequireUseAction);
	TestEqual(TEXT("Edit mirrors legacy coordinates"), Modern.Teleporter.TargetCellX, 3);
	Modern.Transition.bRequireUseAction = true;
	TestTrue(TEXT("Modern lookup ignores old Use flag"), Runtime->FindTransitionAtCell(1, 1, false, Resolved));
	TestFalse(TEXT("Normalized Use flag"), Resolved.bRequireUseAction);
	TestTrue(TEXT("Explicit current id executes locally"), Runtime->TryExecuteTransitionAtCell(1, 1, Party, false));
	TestEqual(TEXT("Modern X"), Party->CurrentCellX, 3);
	TestEqual(TEXT("Modern Y"), Party->CurrentCellY, 2);
	TestEqual(TEXT("Modern facing"), Party->Facing, EGridEdge::South);
	TestEqual(TEXT("Explicit current id does not rebuild"), Runtime->GetRuntimeObjectRebuildGeneration(), Generation);
	Upper->GetCellMutable(3, 2).bBlocksOccupancy = true;
	TestFalse(TEXT("Blocked destination rejected"), Runtime->TryExecuteTransitionAtCell(1, 1, Party, false));
	Upper->GetCellMutable(3, 2).bBlocksOccupancy = false;
	Upper->GetCellMutable(3, 2).CellType = EGridCellType::Empty;
	TestFalse(TEXT("Empty destination rejected"), Runtime->TryExecuteTransitionAtCell(1, 1, Party, false));
	Upper->GetCellMutable(3, 2).CellType = EGridCellType::Floor;
	Modern.Transition.TargetCellX = 99;
	TestFalse(TEXT("Out of bounds rejected"), Runtime->TryExecuteTransitionAtCell(1, 1, Party, false));
	Modern.Transition.TargetCellX = 3;
	Upper->WorldObjectInstances[0].Type = EGridLevelObjectType::Decoration;
	TestTrue(TEXT("Stairs or Passage transition candidate executes"), Runtime->TryExecuteTransitionAtCell(1, 1, Party, false));
	TestFalse(TEXT("Null pawn rejected"), Runtime->TryExecuteTransitionAtCell(1, 1, nullptr, false));
	Modern.Transition.TargetLevelId = B.LevelId;
	Modern.Transition.TargetFacing = EGridEdge::None;
	TestTrue(TEXT("Cross-level travel"), Runtime->TryExecuteTransitionAtCell(1, 1, Party, false));
	TestEqual(TEXT("Cross-level id"), Runtime->CurrentDungeonLevelId, B.LevelId);
	TestTrue(TEXT("Cross-level asset"), Runtime->LevelAsset == Lower);
	TestTrue(TEXT("Cross-level rebuild"), Runtime->GetRuntimeObjectRebuildGeneration() > Generation);
	TestTrue(TEXT("Source state captured"), Runtime->DungeonRuntimeState.LevelStates.Contains(A.LevelId));
	TestEqual(TEXT("Cross-level preserves facing"), Party->Facing, EGridEdge::South);

	FGridWorldObjectInstance Pit;
	Pit.InstanceId = FGuid::NewGuid();
	Pit.Type = EGridLevelObjectType::Pit;
	Pit.CellX = 1;
	Pit.CellY = 2;
	Pit.InstanceConfig.Pit.bUseSameCellCoordinates = true;
	Upper->WorldObjectInstances = { Pit };
	Runtime->LevelAsset = Upper;
	Runtime->CurrentDungeonLevelId = A.LevelId;
	TestFalse(TEXT("Pit excluded from normal resolver"), GridRelocation::IsCandidate(Pit));
	TestFalse(TEXT("Pit excluded from generic lookup"), Runtime->FindTransitionAtCell(1, 2, false, Resolved));
	TestTrue(TEXT("Pit None resolves"), Runtime->FindOpenPitAtCell(1, 2, Resolved));
	TestEqual(TEXT("Pit None means lower level"), Resolved.TargetLevelId, B.LevelId);
	TestEqual(TEXT("Pit same-cell X"), Resolved.TargetCellX, 1);
	TestEqual(TEXT("Pit same-cell Y"), Resolved.TargetCellY, 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridRELOC01ValidationTest, "Grimrock.Relocation.RELOC01.Validation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridRELOC01ValidationTest::RunTest(const FString& Parameters)
{
	FRelocationTestWorld Fixture;
	if (!TestNotNull(TEXT("World"), Fixture.World))
		return false;
	auto* Editor = Fixture.World->SpawnActor<AGridLevelEditorActor>();
	auto* Dungeon = NewObject<UGridDungeonAsset>(Editor);
	auto* Level = MakeFloor(Dungeon);
	FGridDungeonLevelEntry Entry;
	Entry.LevelId = TEXT("OnlyLevel");
	Entry.LevelAsset = Level;
	Dungeon->Levels = { Entry };
	Dungeon->DefaultLevelId = Entry.LevelId;
	Editor->LevelAsset = Level;
	Editor->DungeonAsset = Dungeon;
	FGridWorldObjectInstance Object;
	Object.InstanceId = FGuid::NewGuid();
	Object.Type = EGridLevelObjectType::Teleporter;
	Object.CellX = 1;
	Object.CellY = 1;
	Object.InstanceConfig.bTeleporterInitiallyEnabled = false;
	Object.InstanceConfig.Teleporter.TargetCellX = 2;
	Object.InstanceConfig.Teleporter.TargetCellY = 3;
	Level->WorldObjectInstances = { Object };
	TestFalse(TEXT("Normal None diagnostic is valid"), Dungeon->GetTransitionDiagnostics().Contains(TEXT("Status=ERROR")));
	TestTrue(TEXT("Legacy teleporter counted"), Dungeon->GetTransitionDiagnostics().Contains(TEXT("RelocationObjects=1")));
	TestTrue(TEXT("Legacy coordinates diagnosed"), Dungeon->GetTransitionDiagnostics().Contains(TEXT("TargetCell=(2,3)")));
	auto Messages = Editor->ValidateCurrentLevel();
	TestFalse(TEXT("Editor accepts None level and facing"),
		Messages.ContainsByPredicate(
			[](const FGridLevelValidationMessage& Message)
			{
				return Message.Message.Contains(TEXT("TargetLevelId")) || Message.Message.Contains(TEXT("TargetFacing"));
			}));
	FGridObjectTransitionParams Modern = GridRelocation::Resolve(Object);
	Modern.TargetLevelId = Entry.LevelId;
	GridRelocation::ApplyAuthoringEdit(Level->WorldObjectInstances[0].InstanceConfig, Object.Type, Modern);
	TestFalse(TEXT("Explicit source id validates"), Dungeon->GetTransitionDiagnostics().Contains(TEXT("Status=ERROR")));
	Level->WorldObjectInstances[0].InstanceConfig.Transition.TargetLevelId = TEXT("MissingLevel");
	TestTrue(TEXT("Missing explicit level rejected"), Dungeon->GetTransitionDiagnostics().Contains(TEXT("TargetLevelId does not exist")));
	Level->WorldObjectInstances[0].InstanceConfig.Transition.TargetLevelId = NAME_None;
	Editor->DungeonAsset = nullptr;
	Messages = Editor->ValidateCurrentLevel();
	TestFalse(TEXT("Standalone current level needs no dungeon"),
		Messages.ContainsByPredicate(
			[](const FGridLevelValidationMessage& Message)
			{
				return Message.Message.Contains(TEXT("Relocation cannot validate"));
			}));
	Editor->DungeonAsset = Dungeon;
	Level->WorldObjectInstances[0].Type = EGridLevelObjectType::Pit;
	TestTrue(TEXT("Pit no lower level diagnostic error"), Dungeon->GetTransitionDiagnostics().Contains(TEXT("Pit has no enabled lower dungeon level")));
	Messages = Editor->ValidateCurrentLevel();
	TestTrue(TEXT("Editor Pit no lower level error"),
		Messages.ContainsByPredicate(
			[](const FGridLevelValidationMessage& Message)
			{
				return Message.Severity == EGridLevelValidationSeverity::Error && Message.Message.Contains(TEXT("Pit has no enabled dungeon level below"));
			}));
	return true;
}
#endif
