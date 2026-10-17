#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "UI/GridSkillsWidget.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUISKILLSSUMMARY01WidgetContractTest,
	"Grimrock.UI.Skills02.Summary.WidgetContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUISKILLSSUMMARY01WidgetContractTest::RunTest(const FString&)
{
	UClass* SkillsClass = UGridSkillsWidget::StaticClass();

	for (const FName PropertyName : {
		FName(TEXT("Text_AvailableSkillPoints")),
		FName(TEXT("Text_CurrentSkillRankCap"))
	})
	{
		FProperty* Property = FindFProperty<FProperty>(SkillsClass, PropertyName);
		TestNotNull(
			*FString::Printf(TEXT("%s future split summary hook exists"), *PropertyName.ToString()),
			Property);
		if (Property)
		{
			TestTrue(
				*FString::Printf(TEXT("%s remains optional until WBP materialization"), *PropertyName.ToString()),
				Property->HasMetaData(TEXT("BindWidgetOptional")));
		}
	}

	FProperty* LegacyProperty = FindFProperty<FProperty>(SkillsClass, TEXT("Text_SkillPoints"));
	TestNotNull(TEXT("Legacy combined summary remains only for migration"), LegacyProperty);
	if (LegacyProperty)
	{
		TestTrue(TEXT("Legacy combined summary remains optional during migration"),
			LegacyProperty->HasMetaData(TEXT("BindWidgetOptional")));
	}

	return true;
}

#endif
