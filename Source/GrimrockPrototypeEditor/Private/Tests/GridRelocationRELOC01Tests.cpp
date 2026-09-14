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
#include "UObject/UnrealType.h"

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
	FGridWorldObjectInstance RelocationObject;
	RelocationObject.InstanceId = FGuid::NewGuid();
	RelocationObject.Type = EGridLevelObjectType::Relocation;
	RelocationObject.CellX = 1;
	RelocationObject.CellY = 1;
	RelocationObject.InstanceConfig.Relocation.TargetCellX = 2;
	RelocationObject.InstanceConfig.Relocation.TargetCellY = 3;
	RelocationObject.InstanceConfig.bRelocationInitiallyEnabled = false;
	Upper->WorldObjectInstances = { RelocationObject };
	Runtime->FindComponentByClass<UGridActivationComponent>()->RegisterInitialObjectState(RelocationObject);
	FGridRelocationBehaviorParams Resolved;
	TestTrue(TEXT("Enter disabled relocation"), Party->TryStartMove(EGridEdge::East));
	Party->UpdateMove(10.f);
	TestEqual(TEXT("Inactive relocation does not execute on entry"), Party->CurrentCellX, 1);
	TestFalse(TEXT("Inactive lookup"), Runtime->FindRelocationAtCell(1, 1, Resolved));

	RelocationObject.InstanceConfig.bRelocationInitiallyEnabled = true;
	Runtime->FindComponentByClass<UGridActivationComponent>()->RegisterInitialObjectState(RelocationObject);
	TestTrue(TEXT("Active lookup"), Runtime->FindRelocationAtCell(1, 1, Resolved));
	Runtime->FindComponentByClass<UGridActivationComponent>()->SetActiveObjectIds({});
	TestFalse(TEXT("Runtime disabled overrides authored enabled state"), Runtime->TryExecuteRelocationAtCell(1, 1, Party));
	Runtime->FindComponentByClass<UGridActivationComponent>()->RegisterInitialObjectState(RelocationObject);
	FGridWorldObjectInstance Return = RelocationObject;
	Return.InstanceId = FGuid::NewGuid();
	Return.CellX = 2;
	Return.CellY = 3;
	Return.InstanceConfig.Relocation.TargetCellX = 1;
	Return.InstanceConfig.Relocation.TargetCellY = 1;
	Upper->WorldObjectInstances.Add(Return);
	Runtime->FindComponentByClass<UGridActivationComponent>()->RegisterInitialObjectState(Return);
	const int32 Generation = Runtime->GetRuntimeObjectRebuildGeneration();
	const int32 StateCount = Runtime->DungeonRuntimeState.LevelStates.Num();
	Party->SetGridStart(Runtime, 0, 1, EGridEdge::East);
	TestTrue(TEXT("Enter enabled relocation"), Party->TryStartMove(EGridEdge::East));
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
	Upper->WorldObjectInstances[0].InstanceConfig.Relocation = Resolved;
	auto& Modern = Upper->WorldObjectInstances[0].InstanceConfig;
	TestTrue(TEXT("Canonical lookup"), Runtime->FindRelocationAtCell(1, 1, Resolved));
	TestTrue(TEXT("Explicit current id executes locally"), Runtime->TryExecuteRelocationAtCell(1, 1, Party));
	TestEqual(TEXT("Modern X"), Party->CurrentCellX, 3);
	TestEqual(TEXT("Modern Y"), Party->CurrentCellY, 2);
	TestEqual(TEXT("Modern facing"), Party->Facing, EGridEdge::South);
	TestEqual(TEXT("Explicit current id does not rebuild"), Runtime->GetRuntimeObjectRebuildGeneration(), Generation);
	Upper->GetCellMutable(3, 2).bBlocksOccupancy = true;
	TestFalse(TEXT("Blocked destination rejected"), Runtime->TryExecuteRelocationAtCell(1, 1, Party));
	Upper->GetCellMutable(3, 2).bBlocksOccupancy = false;
	Upper->GetCellMutable(3, 2).CellType = EGridCellType::Empty;
	TestFalse(TEXT("Empty destination rejected"), Runtime->TryExecuteRelocationAtCell(1, 1, Party));
	Upper->GetCellMutable(3, 2).CellType = EGridCellType::Floor;
	Modern.Relocation.TargetCellX = 99;
	TestFalse(TEXT("Out of bounds rejected"), Runtime->TryExecuteRelocationAtCell(1, 1, Party));
	Modern.Relocation.TargetCellX = 3;
	Upper->WorldObjectInstances[0].Type = EGridLevelObjectType::Decoration;
	TestFalse(TEXT("Decoration with relocation data does not execute"), Runtime->TryExecuteRelocationAtCell(1, 1, Party));
	Upper->WorldObjectInstances[0].Type = EGridLevelObjectType::Relocation;
	TestFalse(TEXT("Null pawn rejected"), Runtime->TryExecuteRelocationAtCell(1, 1, nullptr));
	Modern.Relocation.TargetLevelId = B.LevelId;
	Modern.Relocation.TargetFacing = EGridEdge::None;
	TestTrue(TEXT("Cross-level travel"), Runtime->TryExecuteRelocationAtCell(1, 1, Party));
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
	TestFalse(TEXT("Pit excluded from generic lookup"), Runtime->FindRelocationAtCell(1, 2, Resolved));
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
	Object.Type = EGridLevelObjectType::Relocation;
	Object.CellX = 1;
	Object.CellY = 1;
	Object.InstanceConfig.bRelocationInitiallyEnabled = false;
	Object.InstanceConfig.Relocation.TargetCellX = 2;
	Object.InstanceConfig.Relocation.TargetCellY = 3;
	Level->WorldObjectInstances = { Object };
	TestFalse(TEXT("Normal None diagnostic is valid"), Dungeon->GetRelocationDiagnostics().Contains(TEXT("Status=ERROR")));
	TestTrue(TEXT("Canonical relocation counted"), Dungeon->GetRelocationDiagnostics().Contains(TEXT("RelocationObjects=1")));
	TestTrue(TEXT("Canonical coordinates diagnosed"), Dungeon->GetRelocationDiagnostics().Contains(TEXT("TargetCell=(2,3)")));
	auto Messages = Editor->ValidateCurrentLevel();
	TestFalse(TEXT("Editor accepts None level and facing"),
		Messages.ContainsByPredicate(
			[](const FGridLevelValidationMessage& Message)
			{
				return Message.Message.Contains(TEXT("TargetLevelId")) || Message.Message.Contains(TEXT("TargetFacing"));
			}));
	FGridRelocationBehaviorParams Modern = Object.InstanceConfig.Relocation;
	Modern.TargetLevelId = Entry.LevelId;
	Level->WorldObjectInstances[0].InstanceConfig.Relocation = Modern;
	TestFalse(TEXT("Explicit source id validates"), Dungeon->GetRelocationDiagnostics().Contains(TEXT("Status=ERROR")));
	Level->WorldObjectInstances[0].InstanceConfig.Relocation.TargetLevelId = TEXT("MissingLevel");
	TestTrue(TEXT("Missing explicit level rejected"), Dungeon->GetRelocationDiagnostics().Contains(TEXT("TargetLevelId does not exist")));
	Level->WorldObjectInstances[0].InstanceConfig.Relocation.TargetLevelId = NAME_None;
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
	TestTrue(TEXT("Pit no lower level diagnostic error"), Dungeon->GetRelocationDiagnostics().Contains(TEXT("Pit has no enabled lower dungeon level")));
	Messages = Editor->ValidateCurrentLevel();
	TestTrue(TEXT("Editor Pit no lower level error"),
		Messages.ContainsByPredicate(
			[](const FGridLevelValidationMessage& Message)
			{
				return Message.Severity == EGridLevelValidationSeverity::Error && Message.Message.Contains(TEXT("Pit has no enabled dungeon level below"));
			}));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridRELOC012SchemaTest, "Grimrock.Relocation.RELOC01.2.Schema", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridRELOC012SchemaTest::RunTest(const FString& Parameters)
{
	for (UScriptStruct* Owner : { FGridObjectBehaviorParams::StaticStruct(), FGridWorldObjectInstanceConfig::StaticStruct() })
	{
		const FStructProperty* Destination = FindFProperty<FStructProperty>(Owner, TEXT("Relocation"));
		TestTrue(TEXT("Canonical destination is reflected and editable"), Destination && Destination->HasAnyPropertyFlags(CPF_Edit));
		TestTrue(
			TEXT("Canonical destination has the single relocation type"), Destination && Destination->Struct == FGridRelocationBehaviorParams::StaticStruct());
		TestNull(TEXT("No obsolete relocation payload"),
			FindFProperty<FProperty>(Owner,
				TEXT("Tele"
					 "porter")));
		TestNull(TEXT("No obsolete secondary payload"),
			FindFProperty<FProperty>(Owner,
				TEXT("Tran"
					 "sition")));
	}
	UScriptStruct* Schema = FGridRelocationBehaviorParams::StaticStruct();
	for (const TCHAR* Name : { TEXT("TargetLevelId"), TEXT("TargetCellX"), TEXT("TargetCellY"), TEXT("TargetFacing") })
		TestNotNull(Name, FindFProperty<FProperty>(Schema, Name));
	// Deliberate negative reflection checks; no removed C++ member is referenced.
	TestNull(TEXT("No obsolete mode flag"),
		FindFProperty<FProperty>(Schema,
			TEXT("bIs"
				 "Tran"
				 "sition")));
	TestNull(TEXT("No Use mode flag"),
		FindFProperty<FProperty>(Schema,
			TEXT("bRequire"
				 "UseAction")));
	int32 PropertyCount = 0;
	for (TFieldIterator<FProperty> It(Schema); It; ++It)
		++PropertyCount;
	TestEqual(TEXT("Exactly four destination properties"), PropertyCount, 4);
	FGridWorldObjectInstance Object;
	TestEqual(TEXT("Default X is unset"), Object.InstanceConfig.Relocation.TargetCellX, INDEX_NONE);
	TestEqual(TEXT("Default Y is unset"), Object.InstanceConfig.Relocation.TargetCellY, INDEX_NONE);
	TestEqual(TEXT("Default facing preserves incoming facing"), Object.InstanceConfig.Relocation.TargetFacing, EGridEdge::None);
	Object.Type = EGridLevelObjectType::Decoration;
	TestFalse(TEXT("Ordinary decoration has no relocation"), GridRelocation::IsCandidate(Object));
	Object.InstanceConfig.Relocation.TargetCellX = 0;
	TestFalse(TEXT("Partial destination is unconfigured"), GridRelocation::IsCandidate(Object));
	Object.InstanceConfig.Relocation.TargetCellY = 0;
	TestFalse(TEXT("Configured decoration is not a relocation candidate"), GridRelocation::IsCandidate(Object));
	Object.Type = EGridLevelObjectType::Relocation;
	TestTrue(TEXT("Configured Relocation is a candidate"), GridRelocation::IsCandidate(Object));
	Object.Type = EGridLevelObjectType::Pit;
	TestFalse(TEXT("Configured Pit never takes the generic path"), GridRelocation::IsCandidate(Object));
	Object.Type = EGridLevelObjectType::Relocation;
	Object.InstanceConfig.Relocation = FGridRelocationBehaviorParams();
	TestFalse(TEXT("Unset Relocation is not a runtime candidate"), GridRelocation::IsCandidate(Object));
	return true;
}
#endif
