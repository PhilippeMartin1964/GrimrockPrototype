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
	FGridLuaScriptSource MakeScript(FName ScriptId, const TCHAR* Source, bool bEnabled = true)
	{
		FGridLuaScriptSource Script;
		Script.ScriptId = ScriptId;
		Script.bEnabled = bEnabled;
		Script.Source = Source;
		return Script;
	}

	FGridLevelVariableDefinition MakeBoolVariable(FName Id)
	{
		FGridLevelVariableDefinition Variable;
		Variable.VariableId = Id;
		Variable.Type = EGridLevelVariableType::Bool;
		Variable.bDefaultBoolValue = false;
		return Variable;
	}

	FGridLevelObjectData MakeSourceButton(FGuid Id)
	{
		FGridLevelObjectData Source;
		Source.ObjectId = Id;
		Source.Type = EGridLevelObjectType::Button;
		Source.CellX = 0;
		Source.CellY = 0;
		Source.bInitiallyEnabled = true;
		return Source;
	}

	struct FGridMON196World
	{
		UWorld* World = nullptr;

		FGridMON196World()
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
				FName(*FString::Printf(TEXT("MON196_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true, ERHIFeatureLevel::Num, &Values);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::EditorPreview);
				Context.SetCurrentWorld(World);
			}
		}

		~FGridMON196World()
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

	UGridLevelAsset* MakeLevel(UObject* Outer)
	{
		UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Outer);
		Level->Width = 4;
		Level->Height = 4;
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
		return Level;
	}

	bool HasMessageContaining(const TArray<FGridLevelValidationMessage>& Messages, const FString& Needle, FName Category = NAME_None)
	{
		return Messages.ContainsByPredicate(
			[&Needle, Category](const FGridLevelValidationMessage& Message)
			{
				return (Category.IsNone() || Message.Category == Category) && Message.Message.Contains(Needle);
			});
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMON196LuaScriptAnalysisTest, "Grimrock.MON19.6.Editor.LuaScriptAnalysis", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON196LuaScriptAnalysisTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridLevelAsset* Level = MakeLevel(GetTransientPackage());
	Level->LuaScripts.Add(MakeScript(TEXT("Puzzle"), TEXT("function on_trigger(event)\nend\nhelper = function(event)\nend\nvalue = 3\n")));

	FGridEditorLuaAnalysis Analysis;
	TestTrue(TEXT("Valid level Lua analysis succeeds"), GridEditorLuaService::AnalyzeLevel(*Level, Analysis));
	TestTrue(TEXT("Script definitions are valid"), Analysis.bDefinitionsValid);
	TestTrue(TEXT("Combined VM builds"), Analysis.bFullVmValid);
	TestEqual(TEXT("One script analyzed"), Analysis.Scripts.Num(), 1);
	if (Analysis.Scripts.Num() == 1)
	{
		TestTrue(TEXT("on_trigger callback detected"), Analysis.Scripts[0].CallbackNames.Contains(TEXT("on_trigger")));
		TestTrue(TEXT("global function assignment detected"), Analysis.Scripts[0].CallbackNames.Contains(TEXT("helper")));
		TestFalse(TEXT("non-function global excluded"), Analysis.Scripts[0].CallbackNames.Contains(TEXT("value")));
	}

	Level->LuaScripts[0].Source = TEXT("function broken(\n");
	TestFalse(TEXT("Syntax error is rejected"), GridEditorLuaService::AnalyzeLevel(*Level, Analysis));
	TestFalse(TEXT("Invalid enabled script is reported"), Analysis.Scripts.IsEmpty() || Analysis.Scripts[0].bValid);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON196LuaBindingMutationTest, "Grimrock.MON19.6.Editor.LuaBindingAndScriptMutations",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON196LuaBindingMutationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridMON196World TestWorld;
	TestNotNull(TEXT("Editor test world exists"), TestWorld.World);
	if (!TestWorld.World)
	{
		return false;
	}

	AGridLevelEditorActor* Editor = TestWorld.World->SpawnActor<AGridLevelEditorActor>();
	TestNotNull(TEXT("Editor actor spawns"), Editor);
	if (!Editor)
	{
		return false;
	}

	UGridLevelAsset* Level = MakeLevel(Editor);
	Editor->LevelAsset = Level;
	const FGuid SourceId(19, 6, 1, 1);
	Level->Objects.Add(MakeSourceButton(SourceId));
	Level->LevelVariables.Add(MakeBoolVariable(TEXT("Gate")));
	Level->LuaScripts.Add(MakeScript(TEXT("Puzzle"), TEXT("function on_trigger(event)\nend\n")));

	FGridObjectLink Link;
	Link.SourceObjectId = SourceId;
	Link.SourceEvent = EGridObjectEvent::Activated;
	Link.Command = EGridObjectCommand::LuaCallback;
	Link.LuaScriptId = TEXT("Puzzle");
	Link.LuaCallbackName = TEXT("on_trigger");
	Link.Condition = EGridObjectCondition::LevelVariableBoolEquals;
	Link.ConditionVariableId = TEXT("Gate");
	Link.ConditionBoolValue = true;

	FString Error;
	TestTrue(TEXT("Targetless typed Lua binding is supported"), GridEditorLuaService::IsLuaLinkSupported(*Level, Link, Error));
	TestTrue(TEXT("Lua binding is added"), GridEditorLuaService::AddLuaLink(*Editor, Link, Error));
	TestEqual(TEXT("One link stored"), Level->Links.Num(), 1);
	if (!Level->Links.IsEmpty())
	{
		TestFalse(TEXT("Lua binding is canonical targetless"), Level->Links[0].TargetObjectId.IsValid());
	}

	FGridObjectLink InvalidCondition = Link;
	InvalidCondition.Condition = EGridObjectCondition::ReceptacleHasAnyItem;
	TestFalse(TEXT("Receptacle condition is rejected for Lua"), GridEditorLuaService::IsLuaLinkSupported(*Level, InvalidCondition, Error));

	TestTrue(TEXT("Rename updates script and binding atomically"), GridEditorLuaService::RenameScript(*Level, TEXT("Puzzle"), TEXT("CryptPuzzle"), Error));
	TestEqual(TEXT("Binding follows renamed ScriptId"), Level->Links[0].LuaScriptId, FName(TEXT("CryptPuzzle")));

	TestFalse(TEXT("Referenced script cannot be disabled"), GridEditorLuaService::SetScriptEnabled(*Level, TEXT("CryptPuzzle"), false, Error));
	TestFalse(TEXT("Referenced script cannot be removed"), GridEditorLuaService::RemoveScript(*Level, TEXT("CryptPuzzle"), Error));

	TestTrue(TEXT("Exact Lua binding can be removed"), GridEditorLuaService::RemoveLuaLink(*Editor, Level->Links[0]));
	TestTrue(TEXT("Unreferenced script can be removed"), GridEditorLuaService::RemoveScript(*Level, TEXT("CryptPuzzle"), Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMON196ValidationAlignmentTest, "Grimrock.MON19.6.Editor.ValidationAlignment", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON196ValidationAlignmentTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridMON196World TestWorld;
	if (!TestWorld.World)
	{
		AddError(TEXT("Editor test world could not be created."));
		return false;
	}
	AGridLevelEditorActor* Editor = TestWorld.World->SpawnActor<AGridLevelEditorActor>();
	if (!Editor)
	{
		AddError(TEXT("Editor actor could not be spawned."));
		return false;
	}

	UGridLevelAsset* Level = MakeLevel(Editor);
	Editor->LevelAsset = Level;

	const FGuid SourceId(19, 6, 2, 1);
	Level->Objects.Add(MakeSourceButton(SourceId));

	FGridLevelObjectData Logic;
	Logic.ObjectId = FGuid(19, 6, 2, 2);
	Logic.Type = EGridLevelObjectType::Logic;
	Logic.CellX = 1;
	Logic.CellY = 1;
	Logic.Logic.NodeType = EGridLogicNodeType::Relay;
	Level->Objects.Add(Logic);

	Level->LuaScripts.Add(MakeScript(TEXT("Puzzle"), TEXT("function on_trigger(event)\nend\nfunction helper(event)\nend\n")));

	FGridObjectLink First;
	First.SourceObjectId = SourceId;
	First.SourceEvent = EGridObjectEvent::Activated;
	First.Command = EGridObjectCommand::LuaCallback;
	First.LuaScriptId = TEXT("Puzzle");
	First.LuaCallbackName = TEXT("on_trigger");
	Level->Links.Add(First);

	FGridObjectLink Second = First;
	Second.LuaCallbackName = TEXT("helper");
	Level->Links.Add(Second);

	const TArray<FGridLevelValidationMessage> Messages = GridEditorLuaService::ValidateCurrentLevelWithLua(*Editor);

	TestFalse(TEXT("Targetless Lua link is not a false legacy error"), HasMessageContaining(Messages, TEXT("invalid TargetObjectId")));
	TestFalse(TEXT("Different Lua callbacks are not false duplicates"), HasMessageContaining(Messages, TEXT("Link 1 duplicates an identical link")));
	TestFalse(TEXT("Data-only Relay has no Logic validation error"), HasMessageContaining(Messages, TEXT("Logic object is invalid"), TEXT("Logic")));
	TestFalse(TEXT("Data-only Logic does not require an archetype"),
		Messages.ContainsByPredicate(
			[&Logic](const FGridLevelValidationMessage& Message)
			{
				return Message.OptionalObjectId == Logic.ObjectId && Message.Message.Contains(TEXT("no ArchetypeId"));
			}));
	TestFalse(TEXT("Valid Lua bindings have no Lua validation error"),
		Messages.ContainsByPredicate(
			[](const FGridLevelValidationMessage& Message)
			{
				return Message.Category == TEXT("Lua") && Message.Severity == EGridLevelValidationSeverity::Error;
			}));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
