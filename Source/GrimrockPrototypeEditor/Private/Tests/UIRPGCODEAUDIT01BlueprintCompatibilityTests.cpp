#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Engine/Blueprint.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "UI/GridSkillsUiTypes.h"
#include "UI/GridSkillsWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGCODEAUDIT01BlueprintCompatibilityTest,
	"Grimrock.UI.RPG.CODEAUDIT01.BlueprintCompatibility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGCODEAUDIT01BlueprintCompatibilityTest::RunTest(const FString&)
{
	TestNull(
		TEXT("Legacy GetTalentEntry function is absent"),
		UGridSkillsWidget::StaticClass()->FindFunctionByName(TEXT("GetTalentEntry")));
	TestNull(
		TEXT("Legacy GetTalentEntryCount function is absent"),
		UGridSkillsWidget::StaticClass()->FindFunctionByName(TEXT("GetTalentEntryCount")));
	TestNull(
		TEXT("Legacy flat Talents property is absent from FGridSkillsPageView"),
		FGridSkillsPageView::StaticStruct()->FindPropertyByName(TEXT("Talents")));

	UBlueprint* SkillsBlueprint = LoadObject<UBlueprint>(
		nullptr,
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/InGameMenu/WBP_GridSkills.WBP_GridSkills"));
	if (!TestNotNull(TEXT("Production WBP_GridSkills loads"), SkillsBlueprint))
	{
		return false;
	}

	FKismetEditorUtilities::CompileBlueprint(SkillsBlueprint);
	TestTrue(
		TEXT("Production WBP_GridSkills recompiles without references to removed compatibility symbols"),
		SkillsBlueprint->Status != BS_Error);
	return true;
}

#endif
