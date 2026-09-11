#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "EditorTools/GridEditorLinkPolicy.h"
#include "EditorTools/GridEditorLuaService.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridEditorLUAUX03LegacyAuthoringPurgeTest,
	"Grimrock.LUAUX03.LegacyAuthoringPurge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorLUAUX03LegacyAuthoringPurgeTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	TestNull(TEXT("World-object Tag is no longer reflected/serialized"), FGridWorldObjectInstance::StaticStruct()->FindPropertyByName(TEXT("Tag")));
	TestNull(TEXT("Loose-item Tag is no longer reflected/serialized"), FGridLooseItemInstance::StaticStruct()->FindPropertyByName(TEXT("Tag")));
	TestNull(TEXT("Monster-spawn Tag is no longer reflected/serialized"), FGridMonsterSpawnInstance::StaticStruct()->FindPropertyByName(TEXT("Tag")));
	TestNull(TEXT("Item-spawn Tag is no longer reflected/serialized"), FGridItemSpawnInstance::StaticStruct()->FindPropertyByName(TEXT("Tag")));
	TestNull(TEXT("Logic-object Tag is no longer reflected/serialized"), FGridLogicObjectInstance::StaticStruct()->FindPropertyByName(TEXT("Tag")));
	TestNull(TEXT("Definition DefaultTag is removed"), UGridWorldObjectDefinitionAsset::StaticClass()->FindPropertyByName(TEXT("DefaultTag")));

	const UScriptStruct* LinkStruct = FGridObjectLink::StaticStruct();
	TestNull(TEXT("ConditionVariableId is no longer serialized"), LinkStruct->FindPropertyByName(TEXT("ConditionVariableId")));
	TestNull(TEXT("ConditionBoolValue is no longer serialized"), LinkStruct->FindPropertyByName(TEXT("ConditionBoolValue")));
	TestNull(TEXT("ConditionIntComparison is no longer serialized"), LinkStruct->FindPropertyByName(TEXT("ConditionIntComparison")));
	TestNull(TEXT("ConditionIntValue is no longer serialized"), LinkStruct->FindPropertyByName(TEXT("ConditionIntValue")));

	const TArray<EGridObjectCondition> DoorConditions = GridEditorLinkPolicy::GetSupportedConditionsForTarget(EGridLevelObjectType::Door);
	TestEqual(TEXT("Non-receptacle connector exposes exactly one condition"), DoorConditions.Num(), 1);
	TestTrue(TEXT("Non-receptacle connector is unconditional"), DoorConditions.Contains(EGridObjectCondition::None));
	TestFalse(TEXT("Legacy Bool link condition is not authorable"), DoorConditions.Contains(EGridObjectCondition::LevelVariableBoolEquals));
	TestFalse(TEXT("Legacy Int link condition is not authorable"), DoorConditions.Contains(EGridObjectCondition::LevelVariableIntCompare));

	const TArray<EGridObjectCondition> ReceptacleConditions = GridEditorLinkPolicy::GetSupportedConditionsForTarget(EGridLevelObjectType::Receptacle);
	TestFalse(TEXT("Receptacle no longer exposes legacy Bool link condition"), ReceptacleConditions.Contains(EGridObjectCondition::LevelVariableBoolEquals));
	TestFalse(TEXT("Receptacle no longer exposes legacy Int link condition"), ReceptacleConditions.Contains(EGridObjectCondition::LevelVariableIntCompare));
	TestTrue(TEXT("Native receptacle conditions remain available"), ReceptacleConditions.Contains(EGridObjectCondition::ReceptacleHasAnyItem));

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

	FGridObjectLink LegacyLuaLink;
	LegacyLuaLink.SourceObjectId = Source.InstanceId;
	LegacyLuaLink.SourceEvent = EGridObjectEvent::Activated;
	LegacyLuaLink.Command = EGridObjectCommand::LuaCallback;
	LegacyLuaLink.LuaScriptId = Script.ScriptId;
	LegacyLuaLink.LuaCallbackName = TEXT("on_used");
	LegacyLuaLink.Condition = EGridObjectCondition::LevelVariableBoolEquals;
	LegacyLuaLink.ConditionVariableId = TEXT("LegacyGate");
	LegacyLuaLink.ConditionBoolValue = true;

	FString Error;
	TestFalse(TEXT("Lua binding rejects legacy connector-side puzzle conditions"), GridEditorLuaService::IsLuaLinkSupported(*Level, LegacyLuaLink, Error));
	TestTrue(TEXT("Lua rejection directs puzzle conditions into Lua"), Error.Contains(TEXT("inside Lua")));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
