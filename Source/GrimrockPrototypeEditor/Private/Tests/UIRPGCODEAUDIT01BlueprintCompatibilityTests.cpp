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

	const TCHAR* BlueprintPaths[] = {
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Skills/WBP_GridSkills.WBP_GridSkills"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Skills/RPG/WBP_RPGTalentDetail.WBP_RPGTalentDetail"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Skills/RPG/WBP_RPGTalentDetailLine.WBP_RPGTalentDetailLine"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Skills/RPG/WBP_RPGTalentVariantFlyout.WBP_RPGTalentVariantFlyout"),
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Skills/RPG/WBP_RPGTalentVariantIcon.WBP_RPGTalentVariantIcon")
	};

	for (const TCHAR* BlueprintPath : BlueprintPaths)
	{
		UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, BlueprintPath);
		if (!TestNotNull(*FString::Printf(TEXT("%s loads"), BlueprintPath), Blueprint))
		{
			continue;
		}

		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		TestTrue(
			*FString::Printf(TEXT("%s recompiles after Talent presentation cleanup"), BlueprintPath),
			Blueprint->Status != BS_Error);
	}
	return true;
}

#endif
