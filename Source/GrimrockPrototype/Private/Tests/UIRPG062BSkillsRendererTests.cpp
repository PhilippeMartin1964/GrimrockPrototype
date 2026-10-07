#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "UI/GridSkillEntryWidget.h"
#include "UI/GridSkillsWidget.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPG062BSkillEntryPresentationTest,
	"Grimrock.UI.RPG06.Skills.Renderer.EntryPresentation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPG062BSkillEntryPresentationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridSkillEntryView Lockpicking;
	Lockpicking.SkillId = TEXT("Skill_Lockpicking");
	Lockpicking.DisplayName = FText::FromString(TEXT("Crochetage"));
	Lockpicking.GoverningAttribute = ERPGSkillGoverningAttribute::Dexterity;
	Lockpicking.Rank = 0;
	Lockpicking.MaxRank = 5;
	Lockpicking.bAllowUntrainedChecks = false;
	Lockpicking.bTrained = false;

	UGridSkillEntryWidget* Widget = NewObject<UGridSkillEntryWidget>();
	TestTrue(TEXT("Canonical untrained Skill row initializes"), Widget->InitializeSkillEntry(Lockpicking));
	TestTrue(TEXT("Display name is projected"), UGridSkillEntryWidget::ResolveDisplayName(Lockpicking).EqualTo(FText::FromString(TEXT("Crochetage"))));
	TestTrue(TEXT("Governing attribute uses enum presentation"), UGridSkillEntryWidget::ResolveAttributeLabel(Lockpicking).EqualTo(FText::FromString(TEXT("Dextérité"))));
	TestTrue(TEXT("Rank uses existing rank/max only"), UGridSkillEntryWidget::ResolveRankLabel(Lockpicking).EqualTo(FText::FromString(TEXT("Rang 0 / 5"))));
	TestTrue(TEXT("Trained-only policy is explicit"), UGridSkillEntryWidget::ResolveTrainingLabel(Lockpicking).EqualTo(FText::FromString(TEXT("Entraînement requis"))));
	TestFalse(TEXT("Empty description remains absent"), UGridSkillEntryWidget::HasDescription(Lockpicking));

	FGridSkillEntryView Perception = Lockpicking;
	Perception.SkillId = TEXT("Skill_Perception");
	Perception.DisplayName = FText::FromString(TEXT("Perception"));
	Perception.GoverningAttribute = ERPGSkillGoverningAttribute::Wisdom;
	Perception.Rank = 2;
	Perception.bAllowUntrainedChecks = true;
	Perception.bTrained = true;
	Perception.Description = FText::FromString(TEXT("Description réelle."));

	TestTrue(TEXT("Trained Skill row initializes"), Widget->InitializeSkillEntry(Perception));
	TestTrue(TEXT("Trained state is explicit"), UGridSkillEntryWidget::ResolveTrainingLabel(Perception).EqualTo(FText::FromString(TEXT("Entraînée"))));
	TestTrue(TEXT("Existing description is exposed"), UGridSkillEntryWidget::HasDescription(Perception));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPG062BSkillEntryRejectsInvalidProjectionTest,
	"Grimrock.UI.RPG06.Skills.Renderer.RejectsInvalidProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPG062BSkillEntryRejectsInvalidProjectionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridSkillEntryWidget* Widget = NewObject<UGridSkillEntryWidget>();
	FGridSkillEntryView Invalid;
	Invalid.SkillId = TEXT("Skill_Invalid");
	Invalid.Rank = 6;
	Invalid.MaxRank = 5;
	Invalid.bTrained = true;
	TestFalse(TEXT("Rank above MaxRank is rejected"), Widget->InitializeSkillEntry(Invalid));
	TestFalse(TEXT("Rejected row is reset"), Widget->bInitialized);

	Invalid.Rank = 1;
	Invalid.MaxRank = 5;
	Invalid.bTrained = false;
	TestFalse(TEXT("Inconsistent trained flag is rejected"), Widget->InitializeSkillEntry(Invalid));
	TestFalse(TEXT("Rejected inconsistent row is reset"), Widget->bInitialized);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPG062BSkillsRendererContractTest,
	"Grimrock.UI.RPG06.Skills.Renderer.WidgetContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPG062BSkillsRendererContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UClass* SkillsClass = UGridSkillsWidget::StaticClass();
	TestNotNull(TEXT("SkillEntryWidgetClass property exists"),
		FindFProperty<FProperty>(SkillsClass, GET_MEMBER_NAME_CHECKED(UGridSkillsWidget, SkillEntryWidgetClass)));
	TestNotNull(TEXT("Panel_SkillEntries property exists"),
		FindFProperty<FProperty>(SkillsClass, GET_MEMBER_NAME_CHECKED(UGridSkillsWidget, Panel_SkillEntries)));
	TestNotNull(TEXT("Text_EmptySkills property exists"),
		FindFProperty<FProperty>(SkillsClass, GET_MEMBER_NAME_CHECKED(UGridSkillsWidget, Text_EmptySkills)));
	TestNotNull(TEXT("RebuildSkillEntryWidgets function exists"),
		SkillsClass->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UGridSkillsWidget, RebuildSkillEntryWidgets)));
	TestNotNull(TEXT("Read-only GetSkillEntry remains available"),
		SkillsClass->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UGridSkillsWidget, GetSkillEntry)));
	return true;
}

#endif
