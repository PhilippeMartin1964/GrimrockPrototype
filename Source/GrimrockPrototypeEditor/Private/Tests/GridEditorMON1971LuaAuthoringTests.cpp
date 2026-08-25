#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Core/GridLevelVariableTypes.h"
#include "EditorTools/GridEditorLuaService.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

namespace
{
	struct FMON1971EditorWorld
	{
		UWorld* World = nullptr;

		FMON1971EditorWorld()
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
				FName(*FString::Printf(TEXT("MON1971Editor_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true, ERHIFeatureLevel::Num,
				&Values);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::EditorPreview);
				Context.SetCurrentWorld(World);
			}
		}

		~FMON1971EditorWorld()
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

	UGridLevelAsset* MakeMON1971Level(UObject* Outer)
	{
		UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Outer);
		Level->Width = 2;
		Level->Height = 2;
		Level->EnsureCellCount();
		for (FGridLevelCellData& Cell : Level->Cells)
		{
			Cell.CellType = EGridCellType::Floor;
			Cell.bBlocksOccupancy = false;
		}
		return Level;
	}

	const FGridLevelVariableDefinition* FindMON1971Variable(const UGridLevelAsset& Level, FName VariableId)
	{
		return Level.LevelVariables.FindByPredicate(
			[VariableId](const FGridLevelVariableDefinition& Definition)
			{
				return Definition.VariableId == VariableId;
			});
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON1971PersistentEditorSyncTest, "Grimrock.MON19.7.1.Editor.PersistentSynchronization",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON1971PersistentEditorSyncTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridLevelAsset* Level = MakeMON1971Level(GetTransientPackage());
	FString Error;
	TestTrue(TEXT("Adding a script synchronizes persistent declarations"),
		GridEditorLuaService::AddScript(*Level, TEXT("Puzzle"),
			TEXT("persistent = { GateOpen = false, RuneCount = 3 }\n") TEXT("function on_trigger(event) persistent.GateOpen = true end\n"), Error));

	const FGridLevelVariableDefinition* Gate = FindMON1971Variable(*Level, TEXT("GateOpen"));
	const FGridLevelVariableDefinition* RuneCount = FindMON1971Variable(*Level, TEXT("RuneCount"));
	TestNotNull(TEXT("GateOpen LevelVariable is created"), Gate);
	TestNotNull(TEXT("RuneCount LevelVariable is created"), RuneCount);
	if (Gate)
	{
		TestEqual(TEXT("GateOpen is Bool"), Gate->Type, EGridLevelVariableType::Bool);
		TestFalse(TEXT("GateOpen default is false"), Gate->bDefaultBoolValue);
	}
	if (RuneCount)
	{
		TestEqual(TEXT("RuneCount is Int32"), RuneCount->Type, EGridLevelVariableType::Int32);
		TestEqual(TEXT("RuneCount default is 3"), RuneCount->DefaultInt32Value, 3);
	}

	FGridEditorLuaAnalysis Analysis;
	TestTrue(TEXT("Synchronized level validates"), GridEditorLuaService::AnalyzeLevel(*Level, Analysis));

	TestTrue(TEXT("Removing declarations from source does not delete existing persistent data definitions"),
		GridEditorLuaService::SetScriptSource(*Level, TEXT("Puzzle"), TEXT("function on_trigger(event) end\n"), Error));
	TestNotNull(TEXT("GateOpen definition is retained after declaration removal"), FindMON1971Variable(*Level, TEXT("GateOpen")));
	TestNotNull(TEXT("RuneCount definition is retained after declaration removal"), FindMON1971Variable(*Level, TEXT("RuneCount")));

	const FString SourceBeforeConflict = Level->LuaScripts[0].Source;
	TestFalse(TEXT("Conflicting existing LevelVariable default is rejected"),
		GridEditorLuaService::SetScriptSource(*Level, TEXT("Puzzle"), TEXT("persistent = { GateOpen = true }\nfunction on_trigger(event) end\n"), Error));
	TestEqual(TEXT("Rejected synchronization leaves script source unchanged"), Level->LuaScripts[0].Source, SourceBeforeConflict);
	TestTrue(TEXT("Conflict is diagnosed"), Error.Contains(TEXT("conflicts")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMON1971LogicIdEditorTest, "Grimrock.MON19.7.1.Editor.LogicIdAuthoring", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON1971LogicIdEditorTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FMON1971EditorWorld TestWorld;
	TestNotNull(TEXT("Editor world exists"), TestWorld.World);
	if (!TestWorld.World)
	{
		return false;
	}

	AGridLevelEditorActor* Editor = TestWorld.World->SpawnActor<AGridLevelEditorActor>();
	TestNotNull(TEXT("Grid editor actor spawns"), Editor);
	if (!Editor)
	{
		return false;
	}

	UGridLevelAsset* Level = MakeMON1971Level(Editor);
	Editor->LevelAsset = Level;

	FGridLevelObjectData DoorA;
	DoorA.ObjectId = FGuid(19, 7, 1, 10);
	DoorA.Type = EGridLevelObjectType::Door;
	DoorA.CellX = 0;
	DoorA.CellY = 0;
	Level->Objects.Add(DoorA);

	FGridLevelObjectData DoorB;
	DoorB.ObjectId = FGuid(19, 7, 1, 11);
	DoorB.Type = EGridLevelObjectType::Door;
	DoorB.CellX = 1;
	DoorB.CellY = 0;
	Level->Objects.Add(DoorB);

	FString Error;
	Editor->LastSelectedObjectId = DoorA.ObjectId;
	TestTrue(TEXT("Readable LogicId can be assigned"), GridEditorLuaService::SetSelectedObjectLogicId(*Editor, TEXT("SecretDoor"), Error));
	TestEqual(TEXT("LogicId is stored on selected object"), Level->Objects[0].LogicId, FName(TEXT("SecretDoor")));

	Editor->LastSelectedObjectId = DoorB.ObjectId;
	TestFalse(TEXT("Duplicate LogicId is rejected"), GridEditorLuaService::SetSelectedObjectLogicId(*Editor, TEXT("SecretDoor"), Error));
	TestTrue(TEXT("Duplicate LogicId is diagnosed"), Error.Contains(TEXT("already used")));

	TestFalse(TEXT("Unreadable LogicId syntax is rejected"), GridEditorLuaService::SetSelectedObjectLogicId(*Editor, FName(TEXT("Door @ 28")), Error));
	TestTrue(TEXT("LogicId syntax error is diagnosed"), Error.Contains(TEXT("A-Za-z")));

	TestTrue(TEXT("Second object accepts a different LogicId"), GridEditorLuaService::SetSelectedObjectLogicId(*Editor, TEXT("ExitDoor"), Error));

	const TArray<FGridLevelValidationMessage> Messages = GridEditorLuaService::ValidateCurrentLevelWithLua(*Editor);
	TestFalse(TEXT("Valid unique LogicIds produce no LogicId validation error"),
		Messages.ContainsByPredicate(
			[](const FGridLevelValidationMessage& Message)
			{
				return Message.Severity == EGridLevelValidationSeverity::Error && Message.Category == TEXT("LogicId");
			}));
	return true;
}

#endif
