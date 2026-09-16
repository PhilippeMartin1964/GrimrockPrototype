#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Core/GridObjectPaletteAsset.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "EditorTools/GridLuaAuthoringCompiler.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	constexpr const TCHAR* ProductionLevelPath =
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_00.DA_GridLevel_00");
	constexpr const TCHAR* ProductionPalettePath =
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/DA_ObjectPalette_Default.DA_ObjectPalette_Default");
	const FName ProductionScriptId(TEXT("puzzle1_lvl1"));
	const FName GuardianDoorLogicId(TEXT("GuardianDoor"));

	struct FPUZZLE01Lua02EditorWorld
	{
		UWorld* World = nullptr;
		AGridLevelEditorActor* Editor = nullptr;

		FPUZZLE01Lua02EditorWorld()
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
				FName(*FString::Printf(TEXT("PUZZLE01_LUA02_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
				nullptr, true, ERHIFeatureLevel::Num, &Values);
			if (!World)
			{
				return;
			}
			if (GEngine)
			{
				GEngine->CreateNewWorldContext(EWorldType::EditorPreview).SetCurrentWorld(World);
			}
			Editor = World->SpawnActor<AGridLevelEditorActor>();
		}

		~FPUZZLE01Lua02EditorWorld()
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

	bool HasGuardianDoorUnknownDiagnostic(const FGridLuaCompileResult& Result)
	{
		return Result.Diagnostics.ContainsByPredicate(
			[](const FGridLuaCompileDiagnostic& Diagnostic)
			{
				return Diagnostic.Code == TEXT("E201") && Diagnostic.ScriptId == ProductionScriptId &&
					Diagnostic.Message.Contains(GuardianDoorLogicId.ToString());
			});
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridPUZZLE01Lua02ProductionAuthoringAuditTest,
	"Grimrock.PUZZLE01.LUA02.ProductionAuthoringAudit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridPUZZLE01Lua02ProductionAuthoringAuditTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FPUZZLE01Lua02EditorWorld TestWorld;
	if (!TestNotNull(TEXT("PUZZLE01-LUA02 editor fixture exists"), TestWorld.Editor))
	{
		return false;
	}

	UGridLevelAsset* ProductionLevel = LoadObject<UGridLevelAsset>(nullptr, ProductionLevelPath);
	UGridObjectPaletteAsset* ProductionPalette = LoadObject<UGridObjectPaletteAsset>(nullptr, ProductionPalettePath);
	if (!TestNotNull(TEXT("Production DA_GridLevel_00 loads"), ProductionLevel) ||
		!TestNotNull(TEXT("Production DA_ObjectPalette_Default loads"), ProductionPalette))
	{
		return false;
	}

	const FGridLuaScriptSource* ProductionScript = ProductionLevel->LuaScripts.FindByPredicate(
		[](const FGridLuaScriptSource& Candidate)
		{
			return Candidate.ScriptId == ProductionScriptId;
		});
	if (!TestNotNull(TEXT("Production puzzle1_lvl1 script exists"), ProductionScript))
	{
		return false;
	}
	TestTrue(TEXT("Production puzzle1_lvl1 script is enabled"), ProductionScript->bEnabled);

	const FGridObjectLink* GuardianBinding = ProductionLevel->Links.FindByPredicate(
		[](const FGridObjectLink& Link)
		{
			return Link.Command == EGridObjectCommand::LuaCallback && Link.SourceEvent == EGridObjectEvent::ItemInserted &&
				Link.LuaScriptId == ProductionScriptId && !Link.LuaCallbackName.IsNone();
		});
	if (!TestNotNull(TEXT("Production Guardian ItemInserted -> puzzle1_lvl1 binding exists"), GuardianBinding))
	{
		return false;
	}

	const FGridWorldObjectInstance* GuardianPlacement = ProductionLevel->WorldObjectInstances.FindByPredicate(
		[GuardianBinding](const FGridWorldObjectInstance& Placement)
		{
			return Placement.InstanceId == GuardianBinding->SourceObjectId;
		});
	if (!TestNotNull(TEXT("Production Guardian binding source exists"), GuardianPlacement))
	{
		return false;
	}
	TestFalse(TEXT("Production Guardian exposes a LogicId"), GuardianPlacement->LogicId.IsNone());

	TArray<FGuid> GuardianDoorIds;
	const int32 GuardianDoorCount = ProductionLevel->FindTypedPlacementIdsByLogicId(GuardianDoorLogicId, GuardianDoorIds);
	TestEqual(TEXT("Production level contains exactly one GuardianDoor LogicId"), GuardianDoorCount, 1);
	if (GuardianDoorCount != 1 || GuardianDoorIds.Num() != 1)
	{
		return false;
	}

	const FGuid GuardianDoorId = GuardianDoorIds[0];
	TestTrue(TEXT("GuardianDoor resolves to a Door placement"),
		ProductionLevel->GetTypedPlacementType(GuardianDoorId) == EGridLevelObjectType::Door);

	// Compile only the production Guardian script/bindings against the complete production
	// placement data. This isolates PUZZLE01 from unrelated Lua scripts without rewriting
	// a single byte of the real LevelAsset.
	UGridLevelAsset* GuardianOnlyLevel = DuplicateObject<UGridLevelAsset>(ProductionLevel, TestWorld.Editor);
	if (!TestNotNull(TEXT("Transient production audit copy exists"), GuardianOnlyLevel))
	{
		return false;
	}
	GuardianOnlyLevel->LuaScripts.RemoveAll(
		[](const FGridLuaScriptSource& Script)
		{
			return Script.ScriptId != ProductionScriptId;
		});
	GuardianOnlyLevel->Links.RemoveAll(
		[](const FGridObjectLink& Link)
		{
			return Link.Command == EGridObjectCommand::LuaCallback && Link.LuaScriptId != ProductionScriptId;
		});

	TestWorld.Editor->LevelAsset = GuardianOnlyLevel;
	TestWorld.Editor->ObjectPalette = ProductionPalette;

	FGridLuaCompileResult ProductionCompile;
	const bool bProductionCompiles = FGridLuaAuthoringCompiler::CompileLevel(*TestWorld.Editor, ProductionCompile);
	if (!bProductionCompiles)
	{
		AddError(FString::Printf(TEXT("Production puzzle1_lvl1 does not compile against current level data:\n%s"),
			*ProductionCompile.GetSummaryText(16)));
		return false;
	}
	TestTrue(TEXT("Production puzzle1_lvl1 compiles with zero authoring diagnostics"), ProductionCompile.Diagnostics.IsEmpty());

	// Prove semantically that puzzle1_lvl1 really references GuardianDoor: remove only
	// that LogicId from a second transient copy. The authoring compiler must now reject
	// the unchanged production script with E201 for GuardianDoor. No source-string
	// matching and no production Lua mutation is involved.
	UGridLevelAsset* MissingDoorIdentityLevel = DuplicateObject<UGridLevelAsset>(GuardianOnlyLevel, TestWorld.Editor);
	if (!TestNotNull(TEXT("Transient missing-door audit copy exists"), MissingDoorIdentityLevel))
	{
		return false;
	}
	FGridWorldObjectInstance* DoorPlacement = MissingDoorIdentityLevel->WorldObjectInstances.FindByPredicate(
		[GuardianDoorId](const FGridWorldObjectInstance& Placement)
		{
			return Placement.InstanceId == GuardianDoorId;
		});
	if (!TestNotNull(TEXT("GuardianDoor placement exists in transient audit copy"), DoorPlacement))
	{
		return false;
	}
	DoorPlacement->LogicId = NAME_None;
	TestWorld.Editor->LevelAsset = MissingDoorIdentityLevel;

	FGridLuaCompileResult MissingDoorCompile;
	const bool bMissingDoorCompiles = FGridLuaAuthoringCompiler::CompileLevel(*TestWorld.Editor, MissingDoorCompile);
	TestFalse(TEXT("Removing GuardianDoor identity invalidates the production Guardian script"), bMissingDoorCompiles);
	TestTrue(TEXT("Compiler proves puzzle1_lvl1 references GuardianDoor through E201"),
		HasGuardianDoorUnknownDiagnostic(MissingDoorCompile));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
