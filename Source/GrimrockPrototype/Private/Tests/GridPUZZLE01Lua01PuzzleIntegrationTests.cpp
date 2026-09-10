#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Core/GridLevelVariableTypes.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GridLuaScriptTypes.h"
#include "Materials/Material.h"
#include "Runtime/GridActivationComponent.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridLevelVariableStore.h"
#include "Runtime/GridReceptacleActor.h"

namespace
{
	struct FPUZZLE01Lua01IntegrationWorld
	{
		UWorld* World = nullptr;

		FPUZZLE01Lua01IntegrationWorld()
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
				FName(*FString::Printf(TEXT("PUZZLE01_LUA01_Integration_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
				nullptr, true, ERHIFeatureLevel::Num, &Values);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FPUZZLE01Lua01IntegrationWorld()
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

	FGridLevelVariableDefinition MakeIntVariable(FName Id, int32 DefaultValue)
	{
		FGridLevelVariableDefinition Variable;
		Variable.VariableId = Id;
		Variable.Type = EGridLevelVariableType::Int32;
		Variable.DefaultInt32Value = DefaultValue;
		return Variable;
	}

	FGridLevelVariableDefinition MakeBoolVariable(FName Id, bool bDefaultValue)
	{
		FGridLevelVariableDefinition Variable;
		Variable.VariableId = Id;
		Variable.Type = EGridLevelVariableType::Bool;
		Variable.bDefaultBoolValue = bDefaultValue;
		return Variable;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridPUZZLE01Lua01GuardianPuzzleIntegrationTest,
	"Grimrock.PUZZLE01.LUA01.GuardianPuzzleIntegration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridPUZZLE01Lua01GuardianPuzzleIntegrationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FPUZZLE01Lua01IntegrationWorld TestWorld;
	if (!TestWorld.World)
	{
		AddError(TEXT("Unable to create integration world."));
		return false;
	}

	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	if (!Runtime)
	{
		AddError(TEXT("Unable to spawn level runtime actor."));
		return false;
	}

	UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Runtime);
	Level->Width = 1;
	Level->Height = 1;
	Level->EnsureCellCount();
	Level->Cells[0].CellType = EGridCellType::Floor;
	Level->LevelVariables.Add(MakeIntVariable(TEXT("GuardianGemCount"), 0));
	Level->LevelVariables.Add(MakeBoolVariable(TEXT("GuardianComplete"), false));
	Runtime->LevelAsset = Level;
	Runtime->CurrentDungeonLevelId = TEXT("PUZZLE01_LUA01");

	UMaterial* EmptyLeft = NewObject<UMaterial>(GetTransientPackage());
	UMaterial* EmptyRight = NewObject<UMaterial>(GetTransientPackage());
	UMaterial* BlueMaterial = NewObject<UMaterial>(GetTransientPackage());
	UStaticMesh* GuardianMesh = NewObject<UStaticMesh>(GetTransientPackage());
	GuardianMesh->GetStaticMaterials().Add(FStaticMaterial(EmptyLeft, FName(TEXT("EyesLeft"))));
	GuardianMesh->GetStaticMaterials().Add(FStaticMaterial(EmptyRight, FName(TEXT("EyesRight"))));

	UGridItemDefinitionAsset* BlueGem = NewObject<UGridItemDefinitionAsset>(Runtime);
	BlueGem->ItemDefinitionId = TEXT("Gem_Blue");
	BlueGem->DisplayName = FText::FromString(TEXT("Blue Gem"));

	UGridWorldObjectDefinitionAsset* GuardianDefinition = NewObject<UGridWorldObjectDefinitionAsset>(Runtime);
	GuardianDefinition->DefinitionId = TEXT("Guardian");
	GuardianDefinition->SupportedType = EGridLevelObjectType::Receptacle;
	GuardianDefinition->PlacementSurface = EGridObjectPlacementKind::Floor;
	GuardianDefinition->StaticPart.Mesh = GuardianMesh;
	GuardianDefinition->RuntimeActorClass = AGridReceptacleActor::StaticClass();
	GuardianDefinition->bIsInteractable = true;
	GuardianDefinition->RuntimeMaterialAliases.Add(TEXT("BlueGem"), BlueMaterial);
	GuardianDefinition->DefaultBehavior.Receptacle.bAcceptAnyItem = false;
	GuardianDefinition->DefaultBehavior.Receptacle.MaxContainedItems = 1;
	FGridReceptacleAcceptedItemConfig AcceptedGem;
	AcceptedGem.ItemDefinition = BlueGem;
	GuardianDefinition->DefaultBehavior.Receptacle.AcceptedItems.Add(AcceptedGem);
	Runtime->WorldObjectDefinitions.Add(GuardianDefinition);

	const FGuid GuardianId(1, 2, 3, 4);
	FGridWorldObjectInstance GuardianPlacement;
	GuardianPlacement.InstanceId = GuardianId;
	GuardianPlacement.LogicId = TEXT("Guardian");
	GuardianPlacement.Type = EGridLevelObjectType::Receptacle;
	GuardianPlacement.WorldObjectDefinitionId = GuardianDefinition->DefinitionId;
	GuardianPlacement.CellX = 0;
	GuardianPlacement.CellY = 0;
	GuardianPlacement.bInitiallyEnabled = true;
	Level->WorldObjectInstances.Add(GuardianPlacement);

	FGridLuaScriptSource Script;
	Script.ScriptId = TEXT("GuardianGemDoor");
	Script.bEnabled = true;
	Script.Source =
		TEXT("persistent = { GuardianGemCount = 0, GuardianComplete = false }\n")
		TEXT("local function must(ok, err) assert(ok, err) end\n")
		TEXT("function on_gem_inserted(event)\n")
		TEXT("  must(grid.command('Guardian', 'ReceptacleConsumeItem'))\n")
		TEXT("  must(grid.command('Guardian', 'ReceptacleDisableRemoval'))\n")
		TEXT("  if persistent.GuardianGemCount >= 2 then return end\n")
		TEXT("  local next_count = persistent.GuardianGemCount + 1\n")
		TEXT("  if next_count == 1 then\n")
		TEXT("    must(grid.visual.set_material('Guardian', 'EyesLeft', 'BlueGem'))\n")
		TEXT("  else\n")
		TEXT("    must(grid.visual.set_material('Guardian', 'EyesRight', 'BlueGem'))\n")
		TEXT("    persistent.GuardianComplete = true\n")
		TEXT("  end\n")
		TEXT("  persistent.GuardianGemCount = next_count\n")
		TEXT("end\n");
	Level->LuaScripts.Add(Script);

	FGridObjectLink Binding;
	Binding.SourceObjectId = GuardianId;
	Binding.SourceEvent = EGridObjectEvent::ItemInserted;
	Binding.Command = EGridObjectCommand::LuaCallback;
	Binding.LuaScriptId = Script.ScriptId;
	Binding.LuaCallbackName = TEXT("on_gem_inserted");
	Level->Links.Add(Binding);

	UGridActivationComponent* Activation = Runtime->FindComponentByClass<UGridActivationComponent>();
	if (!Activation)
	{
		AddError(TEXT("Activation component is missing."));
		return false;
	}
	Activation->Initialize(Runtime);
	Activation->RebuildIndexes();
	FString Error;
	if (!Activation->ReloadLuaRuntime(&Error))
	{
		AddError(FString::Printf(TEXT("Lua runtime failed to load: %s"), *Error));
		return false;
	}

	Runtime->AddRuntimeObjectActor(GuardianPlacement);
	AGridReceptacleActor* Guardian = Runtime->FindRuntimeObjectActor<AGridReceptacleActor>(GuardianId);
	if (!Guardian)
	{
		AddError(TEXT("Guardian receptacle failed to spawn/register."));
		return false;
	}

	TestTrue(TEXT("First blue gem is inserted through the ordinary receptacle"), Guardian->TryInsertItem(BlueGem->ItemDefinitionId, BlueGem, nullptr));
	TestEqual(TEXT("Lua consumes the first inserted gem"), Guardian->GetContainedItemCount(), 0);
	TestTrue(TEXT("Lua lights EyesLeft after the first gem"), Guardian->MeshComponent->GetMaterial(0) == BlueMaterial);
	TestTrue(TEXT("EyesRight remains dark after the first gem"), Guardian->MeshComponent->GetMaterial(1) == EmptyRight);

	FGridLevelRuntimeState* State = Runtime->GetOrCreateRuntimeStateForCurrentLevel();
	int32 GemCount = 0;
	bool bComplete = false;
	TestTrue(TEXT("GuardianGemCount is readable after first insertion"),
		State && GridLevelVariableStore::TryGetInt32(*Level, *State, TEXT("GuardianGemCount"), GemCount, Error));
	TestEqual(TEXT("First insertion commits persistent count 1"), GemCount, 1);
	TestTrue(TEXT("GuardianComplete is readable after first insertion"),
		State && GridLevelVariableStore::TryGetBool(*Level, *State, TEXT("GuardianComplete"), bComplete, Error));
	TestFalse(TEXT("Guardian is not complete after one gem"), bComplete);

	TestTrue(TEXT("Second blue gem is inserted through the same ordinary receptacle"), Guardian->TryInsertItem(BlueGem->ItemDefinitionId, BlueGem, nullptr));
	TestEqual(TEXT("Lua consumes the second inserted gem"), Guardian->GetContainedItemCount(), 0);
	TestTrue(TEXT("EyesLeft remains lit"), Guardian->MeshComponent->GetMaterial(0) == BlueMaterial);
	TestTrue(TEXT("Lua lights EyesRight after the second gem"), Guardian->MeshComponent->GetMaterial(1) == BlueMaterial);

	TestTrue(TEXT("GuardianGemCount is readable after second insertion"),
		GridLevelVariableStore::TryGetInt32(*Level, *State, TEXT("GuardianGemCount"), GemCount, Error));
	TestEqual(TEXT("Second insertion commits persistent count 2"), GemCount, 2);
	TestTrue(TEXT("GuardianComplete is readable after second insertion"),
		GridLevelVariableStore::TryGetBool(*Level, *State, TEXT("GuardianComplete"), bComplete, Error));
	TestTrue(TEXT("Guardian is complete after two gems"), bComplete);

	const FGridRuntimeObjectVisualState* VisualState = State ? State->ObjectVisuals.Find(GuardianId) : nullptr;
	TestNotNull(TEXT("Lua visual changes are persisted generically"), VisualState);
	if (VisualState)
	{
		TestEqual(TEXT("Both eye material overrides are persisted"), VisualState->MaterialAliasesBySlot.Num(), 2);
	}
	return true;
}

#endif