#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Core/GridLevelVariableTypes.h"
#include "Core/GridTypes.h"
#include "EditorTools/GridEditorLinkPolicy.h"
#include "EditorTools/GridEditorLinkService.h"

namespace
{
	FGridLevelVariableDefinition MakeEditorBoolVariable1924(FName Id)
	{
		FGridLevelVariableDefinition Definition;
		Definition.VariableId = Id;
		Definition.Type = EGridLevelVariableType::Bool;
		return Definition;
	}

	FGridLevelVariableDefinition MakeEditorIntVariable1924(FName Id)
	{
		FGridLevelVariableDefinition Definition;
		Definition.VariableId = Id;
		Definition.Type = EGridLevelVariableType::Int32;
		return Definition;
	}

	FGridLevelObjectData MakeEditorObject1924(FGuid Id, EGridLevelObjectType Type)
	{
		FGridLevelObjectData Object;
		Object.ObjectId = Id;
		Object.Type = Type;
		return Object;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorMON1924VariableConditionPolicyTest, "Grimrock.MON19.2.Editor.VariableConditions.PolicyAndTyping",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorMON1924VariableConditionPolicyTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridLevelObjectData Door;
	Door.Type = EGridLevelObjectType::Door;
	const TArray<EGridObjectCondition> DoorConditions = GridEditorLinkPolicy::GetSupportedConditionsForTarget(Door);
	TestEqual(TEXT("Non-receptacle exposes None plus two variable conditions"), DoorConditions.Num(), 3);
	TestTrue(TEXT("Door exposes Bool variable condition"), DoorConditions.Contains(EGridObjectCondition::LevelVariableBoolEquals));
	TestTrue(TEXT("Door exposes Int variable condition"), DoorConditions.Contains(EGridObjectCondition::LevelVariableIntCompare));

	FGridLevelObjectData Receptacle;
	Receptacle.Type = EGridLevelObjectType::Receptacle;
	const TArray<EGridObjectCondition> ReceptacleConditions = GridEditorLinkPolicy::GetSupportedConditionsForTarget(Receptacle);
	TestEqual(TEXT("Receptacle keeps eight historical options and adds two variables"), ReceptacleConditions.Num(), 10);
	TestTrue(TEXT("Receptacle still exposes item-tag condition"), ReceptacleConditions.Contains(EGridObjectCondition::ReceptacleContainsItemTag));
	TestTrue(TEXT("Receptacle also exposes Bool variable condition"), ReceptacleConditions.Contains(EGridObjectCondition::LevelVariableBoolEquals));

	UGridLevelAsset* Level = NewObject<UGridLevelAsset>(GetTransientPackage());
	const FGuid SourceId(19, 2, 4, 101);
	const FGuid TargetId(19, 2, 4, 102);
	Level->Objects.Add(MakeEditorObject1924(SourceId, EGridLevelObjectType::Button));
	Level->Objects.Add(MakeEditorObject1924(TargetId, EGridLevelObjectType::Door));
	Level->LevelVariables = { MakeEditorBoolVariable1924(TEXT("Gate")), MakeEditorIntVariable1924(TEXT("Count")) };

	FGridObjectLink BoolLink;
	BoolLink.SourceObjectId = SourceId;
	BoolLink.SourceEvent = EGridObjectEvent::Activated;
	BoolLink.TargetObjectId = TargetId;
	BoolLink.Command = EGridObjectCommand::Open;
	BoolLink.Condition = EGridObjectCondition::LevelVariableBoolEquals;
	BoolLink.ConditionVariableId = TEXT("Gate");
	BoolLink.ConditionBoolValue = true;
	TestTrue(TEXT("Declared Bool variable is accepted for Bool condition"), GridEditorLinkService::IsLinkSupported(*Level, BoolLink));

	BoolLink.ConditionVariableId = TEXT("Count");
	TestFalse(TEXT("Int variable is rejected for Bool condition"), GridEditorLinkService::IsLinkSupported(*Level, BoolLink));
	BoolLink.ConditionVariableId = TEXT("Missing");
	TestFalse(TEXT("Undeclared variable is rejected"), GridEditorLinkService::IsLinkSupported(*Level, BoolLink));

	FGridObjectLink IntLink;
	IntLink.SourceObjectId = SourceId;
	IntLink.SourceEvent = EGridObjectEvent::Activated;
	IntLink.TargetObjectId = TargetId;
	IntLink.Command = EGridObjectCommand::Open;
	IntLink.Condition = EGridObjectCondition::LevelVariableIntCompare;
	IntLink.ConditionVariableId = TEXT("Count");
	IntLink.ConditionIntComparison = EGridLogicIntComparison::GreaterOrEqual;
	IntLink.ConditionIntValue = 3;
	TestTrue(TEXT("Declared Int32 variable is accepted for Int condition"), GridEditorLinkService::IsLinkSupported(*Level, IntLink));

	IntLink.ConditionIntComparison = static_cast<EGridLogicIntComparison>(255);
	TestFalse(TEXT("Invalid Int comparator is rejected"), GridEditorLinkService::IsConditionConfigurationValid(IntLink));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorMON1924VariableConditionIdentityTest, "Grimrock.MON19.2.Editor.VariableConditions.IdentityAndNormalization",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorMON1924VariableConditionIdentityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridObjectLink BoolTrue;
	BoolTrue.SourceObjectId = FGuid(19, 2, 4, 110);
	BoolTrue.TargetObjectId = FGuid(19, 2, 4, 111);
	BoolTrue.SourceEvent = EGridObjectEvent::Activated;
	BoolTrue.Command = EGridObjectCommand::Open;
	BoolTrue.Condition = EGridObjectCondition::LevelVariableBoolEquals;
	BoolTrue.ConditionVariableId = TEXT("Gate");
	BoolTrue.ConditionBoolValue = true;

	FGridObjectLink BoolFalse = BoolTrue;
	BoolFalse.ConditionBoolValue = false;
	TestFalse(TEXT("Bool expected value participates in exact identity"), GridEditorLinkPolicy::AreLinksExactlyEquivalent(BoolTrue, BoolFalse));

	FGridObjectLink OtherVariable = BoolTrue;
	OtherVariable.ConditionVariableId = TEXT("OtherGate");
	TestFalse(TEXT("VariableId participates in exact identity"), GridEditorLinkPolicy::AreLinksExactlyEquivalent(BoolTrue, OtherVariable));

	FGridObjectLink IntA = BoolTrue;
	IntA.Condition = EGridObjectCondition::LevelVariableIntCompare;
	IntA.ConditionVariableId = TEXT("Count");
	IntA.ConditionIntComparison = EGridLogicIntComparison::GreaterOrEqual;
	IntA.ConditionIntValue = 3;
	FGridObjectLink IntB = IntA;
	IntB.ConditionIntComparison = EGridLogicIntComparison::Greater;
	TestFalse(TEXT("Int comparator participates in exact identity"), GridEditorLinkPolicy::AreLinksExactlyEquivalent(IntA, IntB));
	IntB = IntA;
	IntB.ConditionIntValue = 4;
	TestFalse(TEXT("Int threshold participates in exact identity"), GridEditorLinkPolicy::AreLinksExactlyEquivalent(IntA, IntB));

	BoolTrue.ConditionItemDefinitionId = TEXT("StaleItem");
	BoolTrue.ConditionItemTag = TEXT("StaleTag");
	BoolTrue.ConditionCount = 99;
	BoolTrue.ConditionWeight = 42.0f;
	BoolTrue.ConditionIntComparison = EGridLogicIntComparison::Less;
	BoolTrue.ConditionIntValue = 17;
	const FGridObjectLink NormalizedBool = GridEditorLinkService::NormalizeLink(BoolTrue);
	TestTrue(TEXT("Bool normalization keeps VariableId"), NormalizedBool.ConditionVariableId == FName(TEXT("Gate")));
	TestTrue(TEXT("Bool normalization keeps expected Bool"), NormalizedBool.ConditionBoolValue);
	TestTrue(TEXT("Bool normalization clears stale item definition"), NormalizedBool.ConditionItemDefinitionId.IsNone());
	TestTrue(TEXT("Bool normalization clears stale item tag"), NormalizedBool.ConditionItemTag.IsNone());
	TestEqual(TEXT("Bool normalization resets Int threshold"), NormalizedBool.ConditionIntValue, 0);
	TestTrue(TEXT("Bool normalization resets Int comparator"), NormalizedBool.ConditionIntComparison == EGridLogicIntComparison::Equal);

	IntA.ConditionItemDefinitionId = TEXT("StaleItem");
	IntA.ConditionBoolValue = true;
	const FGridObjectLink NormalizedInt = GridEditorLinkService::NormalizeLink(IntA);
	TestTrue(TEXT("Int normalization keeps VariableId"), NormalizedInt.ConditionVariableId == FName(TEXT("Count")));
	TestTrue(TEXT("Int normalization keeps comparator"), NormalizedInt.ConditionIntComparison == EGridLogicIntComparison::GreaterOrEqual);
	TestEqual(TEXT("Int normalization keeps threshold"), NormalizedInt.ConditionIntValue, 3);
	TestFalse(TEXT("Int normalization clears stale Bool value"), NormalizedInt.ConditionBoolValue);
	TestTrue(TEXT("Int normalization clears stale item definition"), NormalizedInt.ConditionItemDefinitionId.IsNone());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
