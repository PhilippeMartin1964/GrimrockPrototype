#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "EditorTools/GridEditorLinkPolicy.h"
#include "EditorTools/GridEditorLuaService.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridEditorLUAUX03HardPurgeTest,
	"Grimrock.LUAUX03.HardPurge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorLUAUX03HardPurgeTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	// LUA-UX03 owns one simple connector contract: generic targets are
	// unconditional; puzzle branching lives in Lua.
	const TArray<EGridObjectCondition> DoorConditions = GridEditorLinkPolicy::GetSupportedConditionsForTarget(EGridLevelObjectType::Door);
	TestEqual(TEXT("Non-receptacle connector exposes exactly one condition"), DoorConditions.Num(), 1);
	TestTrue(TEXT("Non-receptacle connector is unconditional"), DoorConditions.Num() == 1 && DoorConditions[0] == EGridObjectCondition::None);

	// Receptacle predicates are native object-state queries, not a second
	// scripting language. They remain available for simple direct wiring.
	const TArray<EGridObjectCondition> ReceptacleConditions = GridEditorLinkPolicy::GetSupportedConditionsForTarget(EGridLevelObjectType::Receptacle);
	const TArray<EGridObjectCondition> ExpectedReceptacleConditions = {
		EGridObjectCondition::None,
		EGridObjectCondition::ReceptacleIsEmpty,
		EGridObjectCondition::ReceptacleHasAnyItem,
		EGridObjectCondition::ReceptacleContainsItemDefinition,
		EGridObjectCondition::ReceptacleContainsItemTag,
		EGridObjectCondition::ReceptacleContainsItemType,
		EGridObjectCondition::ReceptacleItemCountAtLeast,
		EGridObjectCondition::ReceptacleWeightAtLeast
	};
	TestEqual(TEXT("Receptacle connector condition count is exact"), ReceptacleConditions.Num(), ExpectedReceptacleConditions.Num());
	for (const EGridObjectCondition Expected : ExpectedReceptacleConditions)
	{
		TestTrue(TEXT("Expected native receptacle condition remains available"), ReceptacleConditions.Contains(Expected));
	}

	// Lua bindings themselves stay unconditional. Any condition/counter/branch
	// belongs to the Lua callback body.
	UGridLevelAsset* Level = NewObject<UGridLevelAsset>(GetTransientPackage());
	Level->Width = 1;
	Level->Height = 1;
	Level->EnsureCellCount();
	Level->Cells[0].CellType = EGridCellType::Floor;

	FGridWorldObjectInstance Source;
	Source.InstanceId = FGuid::NewGuid();
	Source.Type = EGridLevelObjectType::Button;
	Source.CellX = 0;
	Source.CellY = 0;
	Source.bInitiallyEnabled = true;
	Level->WorldObjectInstances.Add(Source);

	FGridLuaScriptSource Script;
	Script.ScriptId = TEXT("LUAUX03");
	Script.bEnabled = true;
	Script.Source = TEXT("function on_used(event)\nend\n");
	Level->LuaScripts.Add(Script);

	FGridObjectLink LuaLink;
	LuaLink.SourceObjectId = Source.InstanceId;
	LuaLink.SourceEvent = EGridObjectEvent::Activated;
	LuaLink.Command = EGridObjectCommand::LuaCallback;
	LuaLink.LuaScriptId = Script.ScriptId;
	LuaLink.LuaCallbackName = TEXT("on_used");
	LuaLink.Condition = EGridObjectCondition::ReceptacleHasAnyItem;

	FString Error;
	TestFalse(TEXT("Lua binding rejects connector-side conditions"), GridEditorLuaService::IsLuaLinkSupported(*Level, LuaLink, Error));
	TestTrue(TEXT("Lua rejection directs puzzle conditions into Lua"), Error.Contains(TEXT("inside Lua")));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
