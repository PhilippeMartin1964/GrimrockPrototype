#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Core/GridTypes.h"
#include "EditorTools/GridEditorLinkPolicy.h"
#include "EditorTools/GridEditorLinkService.h"

namespace
{
	FGridWorldObjectInstance MakeEditorObject1924(FGuid Id, EGridLevelObjectType Type)
	{
		FGridWorldObjectInstance Object;
		Object.InstanceId = Id;
		Object.Type = Type;
		return Object;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridEditorMON1924VariableConditionRetirementTest,
	"Grimrock.MON19.2.Editor.VariableConditions.RetiredByLuaUx03",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorMON1924VariableConditionRetirementTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const TArray<EGridObjectCondition> DoorConditions = GridEditorLinkPolicy::GetSupportedConditionsForTarget(EGridLevelObjectType::Door);
	TestEqual(TEXT("Non-receptacle connector exposes only None"), DoorConditions.Num(), 1);
	TestTrue(TEXT("Door keeps unconditional connector option"), DoorConditions.Contains(EGridObjectCondition::None));
	TestFalse(TEXT("Door no longer exposes Bool LevelVariable condition"), DoorConditions.Contains(EGridObjectCondition::LevelVariableBoolEquals));
	TestFalse(TEXT("Door no longer exposes Int LevelVariable condition"), DoorConditions.Contains(EGridObjectCondition::LevelVariableIntCompare));

	const TArray<EGridObjectCondition> ReceptacleConditions = GridEditorLinkPolicy::GetSupportedConditionsForTarget(EGridLevelObjectType::Receptacle);
	TestEqual(TEXT("Receptacle keeps None plus seven native state conditions"), ReceptacleConditions.Num(), 8);
	TestTrue(TEXT("Receptacle still exposes item-tag condition"), ReceptacleConditions.Contains(EGridObjectCondition::ReceptacleContainsItemTag));
	TestFalse(TEXT("Receptacle no longer exposes Bool LevelVariable condition"), ReceptacleConditions.Contains(EGridObjectCondition::LevelVariableBoolEquals));
	TestFalse(TEXT("Receptacle no longer exposes Int LevelVariable condition"), ReceptacleConditions.Contains(EGridObjectCondition::LevelVariableIntCompare));

	UGridLevelAsset* Level = NewObject<UGridLevelAsset>(GetTransientPackage());
	const FGuid SourceId(19, 2, 4, 101);
	const FGuid TargetId(19, 2, 4, 102);
	Level->WorldObjectInstances.Add(MakeEditorObject1924(SourceId, EGridLevelObjectType::Button));
	Level->WorldObjectInstances.Add(MakeEditorObject1924(TargetId, EGridLevelObjectType::Door));

	FGridObjectLink LegacyBoolLink;
	LegacyBoolLink.SourceObjectId = SourceId;
	LegacyBoolLink.SourceEvent = EGridObjectEvent::Activated;
	LegacyBoolLink.TargetObjectId = TargetId;
	LegacyBoolLink.Command = EGridObjectCommand::Open;
	LegacyBoolLink.Condition = EGridObjectCondition::LevelVariableBoolEquals;
	LegacyBoolLink.ConditionVariableId = TEXT("Gate");
	LegacyBoolLink.ConditionBoolValue = true;
	TestFalse(TEXT("Editor service rejects legacy Bool variable condition"), GridEditorLinkService::IsLinkSupported(*Level, LegacyBoolLink));

	FGridObjectLink LegacyIntLink = LegacyBoolLink;
	LegacyIntLink.Condition = EGridObjectCondition::LevelVariableIntCompare;
	LegacyIntLink.ConditionVariableId = TEXT("Count");
	LegacyIntLink.ConditionIntComparison = EGridLogicIntComparison::GreaterOrEqual;
	LegacyIntLink.ConditionIntValue = 3;
	TestFalse(TEXT("Editor service rejects legacy Int variable condition"), GridEditorLinkService::IsLinkSupported(*Level, LegacyIntLink));

	FGridObjectLink Unconditional = LegacyBoolLink;
	Unconditional.Condition = EGridObjectCondition::None;
	TestTrue(TEXT("Equivalent unconditional connector remains valid"), GridEditorLinkService::IsLinkSupported(*Level, Unconditional));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
