#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GridLuaScriptTypes.h"
#include "Runtime/GridActivationComponent.h"
#include "Runtime/GridLevelRuntimeActor.h"

namespace
{
	struct FMON1971LogicIdWorld
	{
		UWorld* World = nullptr;

		FMON1971LogicIdWorld()
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
				FName(*FString::Printf(TEXT("MON1971LogicId_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true, ERHIFeatureLevel::Num,
				&Values);

			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FMON1971LogicIdWorld()
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMON1971LogicIdCommandTest, "Grimrock.MON19.7.1.LuaAuthoring.LogicIdCommand", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON1971LogicIdCommandTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FMON1971LogicIdWorld TestWorld;
	if (!TestWorld.World)
	{
		AddError(TEXT("Unable to create MON19.7.1 test world."));
		return false;
	}

	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	if (!Runtime)
	{
		AddError(TEXT("Unable to spawn MON19.7.1 runtime actor."));
		return false;
	}

	UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Runtime);
	Level->Width = 1;
	Level->Height = 1;
	Level->EnsureCellCount();
	Level->Cells[0].CellType = EGridCellType::Floor;

	const FGuid SourceId(19, 7, 1, 1);
	FGridLevelObjectData Source;
	Source.ObjectId = SourceId;
	Source.Type = EGridLevelObjectType::Trigger;
	Level->Objects.Add(Source);

	const FGuid TargetId(19, 7, 1, 2);
	FGridLevelObjectData Target;
	Target.ObjectId = TargetId;
	Target.LogicId = TEXT("SecretLever");
	Target.Type = EGridLevelObjectType::Lever;
	Level->Objects.Add(Target);

	FGridLuaScriptSource Script;
	Script.ScriptId = TEXT("Puzzle");
	Script.bEnabled = true;
	Script.Source =
		TEXT("function on_trigger(event)\n") TEXT("  local ok, err = grid.command('SecretLever', 'Activate')\n") TEXT("  assert(ok, err)\n") TEXT("end\n");
	Level->LuaScripts.Add(Script);

	FGridObjectLink Link;
	Link.SourceObjectId = SourceId;
	Link.SourceEvent = EGridObjectEvent::Activated;
	Link.Command = EGridObjectCommand::LuaCallback;
	Link.LuaScriptId = TEXT("Puzzle");
	Link.LuaCallbackName = TEXT("on_trigger");
	Level->Links.Add(Link);

	Runtime->LevelAsset = Level;
	Runtime->CurrentDungeonLevelId = TEXT("MON1971");
	UGridActivationComponent* Activation = Runtime->FindComponentByClass<UGridActivationComponent>();
	if (!Activation)
	{
		AddError(TEXT("MON19.7.1 activation component is missing."));
		return false;
	}

	Activation->Initialize(Runtime);
	Activation->RebuildIndexes();
	FString Error;
	if (!Activation->ReloadLuaRuntime(&Error))
	{
		AddError(FString::Printf(TEXT("MON19.7.1 Lua runtime failed to load: %s"), *Error));
		return false;
	}

	TestTrue(TEXT("Lua callback resolves LogicId and applies command"), Activation->ExecuteLinksFromObjectForEvent(SourceId, EGridObjectEvent::Activated));
	TestTrue(TEXT("LogicId target becomes active"), Activation->GetActiveObjectIds().Contains(TargetId));
	return true;
}

#endif
