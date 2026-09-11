#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Core/GridObjectPaletteAsset.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "EditorTools/GridLuaAuthoringCompiler.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/Material.h"

namespace
{
	struct FLUACOMP01World
	{
		UWorld* World = nullptr;
		AGridLevelEditorActor* Editor = nullptr;
		UGridLevelAsset* Level = nullptr;
		UGridWorldObjectDefinitionAsset* GuardianDefinition = nullptr;
		UGridWorldObjectDefinitionAsset* DoorDefinition = nullptr;
		FGuid GuardianId;
		FGuid DoorId;

		FLUACOMP01World()
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
				FName(*FString::Printf(TEXT("LUACOMP01_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true,
				ERHIFeatureLevel::Num, &Values);
			if (!World)
			{
				return;
			}
			if (GEngine)
			{
				GEngine->CreateNewWorldContext(EWorldType::EditorPreview).SetCurrentWorld(World);
			}

			Editor = World->SpawnActor<AGridLevelEditorActor>();
			if (!Editor)
			{
				return;
			}

			Level = NewObject<UGridLevelAsset>(Editor);
			Level->Width = 2;
			Level->Height = 2;
			Level->EnsureCellCount();
			for (FGridLevelCellData& Cell : Level->Cells)
			{
				Cell.CellType = EGridCellType::Floor;
				Cell.bBlocksOccupancy = false;
			}
			Editor->LevelAsset = Level;

			UGridObjectPaletteAsset* Palette = NewObject<UGridObjectPaletteAsset>(Editor);
			Editor->ObjectPalette = Palette;

			GuardianDefinition = NewObject<UGridWorldObjectDefinitionAsset>(Editor);
			GuardianDefinition->DefinitionId = TEXT("GuardianDefinition");
			GuardianDefinition->SupportedType = EGridLevelObjectType::Receptacle;
			GuardianDefinition->StaticPart.Mesh = NewObject<UStaticMesh>(GuardianDefinition);
			FStaticMaterial& LeftSlot = GuardianDefinition->StaticPart.Mesh->GetStaticMaterials().AddDefaulted_GetRef();
			LeftSlot.MaterialSlotName = TEXT("EyesLeft");
			LeftSlot.ImportedMaterialSlotName = TEXT("EyesLeft");
			FStaticMaterial& RightSlot = GuardianDefinition->StaticPart.Mesh->GetStaticMaterials().AddDefaulted_GetRef();
			RightSlot.MaterialSlotName = TEXT("EyesRight");
			RightSlot.ImportedMaterialSlotName = TEXT("EyesRight");
			GuardianDefinition->RuntimeMaterialAliases.Add(TEXT("BlueGem"), NewObject<UMaterial>(GuardianDefinition));

			DoorDefinition = NewObject<UGridWorldObjectDefinitionAsset>(Editor);
			DoorDefinition->DefinitionId = TEXT("GuardianDoorDefinition");
			DoorDefinition->SupportedType = EGridLevelObjectType::Door;

			FGridObjectPaletteEntry& GuardianEntry = Palette->Entries.AddDefaulted_GetRef();
			GuardianEntry.EntryId = TEXT("GuardianDefinition");
			GuardianEntry.DefaultWorldObjectDefinition = GuardianDefinition;
			FGridObjectPaletteEntry& DoorEntry = Palette->Entries.AddDefaulted_GetRef();
			DoorEntry.EntryId = TEXT("GuardianDoorDefinition");
			DoorEntry.DefaultWorldObjectDefinition = DoorDefinition;

			FGridWorldObjectInstance& Guardian = Level->WorldObjectInstances.AddDefaulted_GetRef();
			GuardianId = FGuid::NewGuid();
			Guardian.InstanceId = GuardianId;
			Guardian.Type = EGridLevelObjectType::Receptacle;
			Guardian.WorldObjectDefinitionId = GuardianDefinition->DefinitionId;
			Guardian.LogicId = TEXT("Guardian");
			Guardian.CellX = 0;
			Guardian.CellY = 0;

			FGridWorldObjectInstance& Door = Level->WorldObjectInstances.AddDefaulted_GetRef();
			DoorId = FGuid::NewGuid();
			Door.InstanceId = DoorId;
			Door.Type = EGridLevelObjectType::Door;
			Door.WorldObjectDefinitionId = DoorDefinition->DefinitionId;
			Door.LogicId = TEXT("GuardianDoor");
			Door.CellX = 1;
			Door.CellY = 0;
		}

		~FLUACOMP01World()
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

		void SetScript(const FString& Source, FName Callback = TEXT("on_gem_inserted"))
		{
			Level->LuaScripts.Reset();
			Level->Links.Reset();

			FGridLuaScriptSource& Script = Level->LuaScripts.AddDefaulted_GetRef();
			Script.ScriptId = TEXT("GuardianGemDoor");
			Script.bEnabled = true;
			Script.Source = Source;

			FGridObjectLink& Link = Level->Links.AddDefaulted_GetRef();
			Link.SourceObjectId = GuardianId;
			Link.SourceEvent = EGridObjectEvent::ItemInserted;
			Link.Command = EGridObjectCommand::LuaCallback;
			Link.LuaScriptId = Script.ScriptId;
			Link.LuaCallbackName = Callback;
			Link.Condition = EGridObjectCondition::None;
		}
	};

	bool HasCode(const FGridLuaCompileResult& Result, const TCHAR* Code)
	{
		return Result.Diagnostics.ContainsByPredicate(
			[Code](const FGridLuaCompileDiagnostic& Diagnostic)
			{
				return Diagnostic.Code == Code;
			});
	}

	const TCHAR* ValidGuardianScript =
		TEXT("persistent = { GuardianGemCount = 0 }\n")
		TEXT("function on_gem_inserted()\n")
		TEXT("    if persistent.GuardianGemCount >= 2 then return end\n")
		TEXT("    persistent.GuardianGemCount = persistent.GuardianGemCount + 1\n")
		TEXT("    if persistent.GuardianGemCount == 1 then\n")
		TEXT("        grid.visual.set_material(\"Guardian\", \"EyesLeft\", \"BlueGem\")\n")
		TEXT("    elseif persistent.GuardianGemCount == 2 then\n")
		TEXT("        grid.visual.set_material(\"Guardian\", \"EyesRight\", \"BlueGem\")\n")
		TEXT("        grid.command(\"GuardianDoor\", \"Open\")\n")
		TEXT("    end\n")
		TEXT("end\n");
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridLUACOMP01CanonicalScriptTest, "Grimrock.LUACOMP01.CanonicalGuardianScript",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridLUACOMP01CanonicalScriptTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FLUACOMP01World Fixture;
	TestNotNull(TEXT("Compiler test editor exists"), Fixture.Editor);
	if (!Fixture.Editor)
	{
		return false;
	}

	Fixture.SetScript(ValidGuardianScript);
	FGridLuaCompileResult Result;
	TestTrue(TEXT("Canonical Guardian script compiles"), FGridLuaAuthoringCompiler::CompileLevel(*Fixture.Editor, Result));
	TestTrue(TEXT("Canonical compile has no diagnostics"), Result.Diagnostics.IsEmpty());
	TestTrue(TEXT("Compile summary reports success"), Result.GetSummaryText().Contains(TEXT("Lua compile OK")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridLUACOMP01SyntaxPersistentTest, "Grimrock.LUACOMP01.SyntaxAndPersistent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridLUACOMP01SyntaxPersistentTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FLUACOMP01World Fixture;
	if (!TestNotNull(TEXT("Compiler test editor exists"), Fixture.Editor))
	{
		return false;
	}

	FGridLuaCompileResult Result;
	Fixture.SetScript(
		TEXT("persistent = { GuardianGemCount = 0 }\n")
		TEXT("function on_gem_inserted()\n")
		TEXT("    persistent.GuardianGemCount++\n")
		TEXT("end\n"));
	TestFalse(TEXT("C-style increment is rejected by Lua pass"), FGridLuaAuthoringCompiler::CompileLevel(*Fixture.Editor, Result));
	TestTrue(TEXT("Lua syntax failure is E001"), HasCode(Result, TEXT("E001")));

	Fixture.SetScript(
		TEXT("persistent = { GuardianGemCount = 0 }\n")
		TEXT("function on_gem_inserted()\n")
		TEXT("    persistent.Unknown = 1\n")
		TEXT("end\n"));
	TestFalse(TEXT("Undeclared persistent member is rejected"), FGridLuaAuthoringCompiler::CompileLevel(*Fixture.Editor, Result));
	TestTrue(TEXT("Undeclared persistent member is E101"), HasCode(Result, TEXT("E101")));

	Fixture.SetScript(
		TEXT("persistent = { GuardianGemCount = 0 }\n")
		TEXT("function on_gem_inserted()\n")
		TEXT("    persistent.GuardianGemCount = \"two\"\n")
		TEXT("end\n"));
	TestFalse(TEXT("Obvious persistent type change is rejected"), FGridLuaAuthoringCompiler::CompileLevel(*Fixture.Editor, Result));
	TestTrue(TEXT("Persistent type mismatch is E102"), HasCode(Result, TEXT("E102")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridLUACOMP01CommandSemanticsTest, "Grimrock.LUACOMP01.CommandSemantics",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridLUACOMP01CommandSemanticsTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FLUACOMP01World Fixture;
	if (!TestNotNull(TEXT("Compiler test editor exists"), Fixture.Editor))
	{
		return false;
	}

	FGridLuaCompileResult Result;
	Fixture.SetScript(TEXT("function on_gem_inserted() grid.command(\"MissingDoor\", \"Open\") end\n"));
	TestFalse(TEXT("Unknown LogicId is rejected"), FGridLuaAuthoringCompiler::CompileLevel(*Fixture.Editor, Result));
	TestTrue(TEXT("Unknown LogicId is E201"), HasCode(Result, TEXT("E201")));

	Fixture.SetScript(TEXT("function on_gem_inserted() grid.command(\"GuardianDoor\", \"Opeen\") end\n"));
	TestFalse(TEXT("Unknown command is rejected"), FGridLuaAuthoringCompiler::CompileLevel(*Fixture.Editor, Result));
	TestTrue(TEXT("Unknown command is E203"), HasCode(Result, TEXT("E203")));

	Fixture.SetScript(TEXT("function on_gem_inserted() grid.command(\"Guardian\", \"Open\") end\n"));
	TestFalse(TEXT("Unsupported target/command pair is rejected"), FGridLuaAuthoringCompiler::CompileLevel(*Fixture.Editor, Result));
	TestTrue(TEXT("Unsupported target/command pair is E204"), HasCode(Result, TEXT("E204")));

	Fixture.SetScript(
		TEXT("function on_gem_inserted()\n")
		TEXT("    local target = \"GuardianDoor\"\n")
		TEXT("    grid.command(target, \"Open\")\n")
		TEXT("end\n"));
	TestFalse(TEXT("Dynamic gameplay target is rejected"), FGridLuaAuthoringCompiler::CompileLevel(*Fixture.Editor, Result));
	TestTrue(TEXT("Dynamic grid.command is E200"), HasCode(Result, TEXT("E200")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridLUACOMP01VisualBindingDraftTest, "Grimrock.LUACOMP01.VisualBindingAndDraft",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridLUACOMP01VisualBindingDraftTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FLUACOMP01World Fixture;
	if (!TestNotNull(TEXT("Compiler test editor exists"), Fixture.Editor))
	{
		return false;
	}

	FGridLuaCompileResult Result;
	Fixture.SetScript(TEXT("function on_gem_inserted() grid.visual.set_material(\"Guardian\", \"EyeLeft\", \"BlueGem\") end\n"));
	TestFalse(TEXT("Unknown material slot is rejected"), FGridLuaAuthoringCompiler::CompileLevel(*Fixture.Editor, Result));
	TestTrue(TEXT("Unknown material slot is E302"), HasCode(Result, TEXT("E302")));

	Fixture.SetScript(TEXT("function on_gem_inserted() grid.visual.set_material(\"Guardian\", \"EyesLeft\", \"BlueGemm\") end\n"));
	TestFalse(TEXT("Unknown material alias is rejected"), FGridLuaAuthoringCompiler::CompileLevel(*Fixture.Editor, Result));
	TestTrue(TEXT("Unknown material alias is E303"), HasCode(Result, TEXT("E303")));

	Fixture.SetScript(TEXT("function on_gem_inserted() end\n"), TEXT("missing_callback"));
	TestFalse(TEXT("Missing bound callback is rejected"), FGridLuaAuthoringCompiler::CompileLevel(*Fixture.Editor, Result));
	TestTrue(TEXT("Missing bound callback is E502"), HasCode(Result, TEXT("E502")));

	Fixture.SetScript(ValidGuardianScript);
	const FString StoredSource = Fixture.Level->LuaScripts[0].Source;
	TestFalse(TEXT("Invalid draft does not compile"),
		FGridLuaAuthoringCompiler::CompileScriptDraft(*Fixture.Editor, TEXT("GuardianGemDoor"), TEXT("GuardianGemDoor"),
			TEXT("function on_gem_inserted() grid.command(\"MissingDoor\", \"Open\") end\n"), Result));
	TestEqual(TEXT("Draft compilation never mutates stored source"), Fixture.Level->LuaScripts[0].Source, StoredSource);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
