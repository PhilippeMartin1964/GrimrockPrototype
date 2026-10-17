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
			*FString::Printf(TEXT("%s split summary hook exists"), *PropertyName.ToString()),
			Property);
		if (Property)
		{
			TestTrue(
				*FString::Printf(TEXT("%s is a mandatory BindWidget"), *PropertyName.ToString()),
				Property->HasMetaData(TEXT("BindWidget")));
			TestFalse(
				*FString::Printf(TEXT("%s is no longer optional"), *PropertyName.ToString()),
				Property->HasMetaData(TEXT("BindWidgetOptional")));
		}
	}

	TestNull(
		TEXT("Legacy combined Skill summary native property is retired"),
		FindFProperty<FProperty>(SkillsClass, TEXT("Text_SkillPoints")));

	return true;
}

#endif
