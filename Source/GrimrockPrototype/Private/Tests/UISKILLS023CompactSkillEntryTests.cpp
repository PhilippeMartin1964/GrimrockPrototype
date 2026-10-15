#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Engine/Texture2D.h"
#include "UI/GridSkillEntryWidget.h"
#include "UI/GridSkillsWidget.h"
#include "UObject/UnrealType.h"

namespace UISKILLS023Tests
{
	FGridSkillEntryView MakeEntry(
		FName SkillId,
		int32 Rank,
		bool bAllowUntrainedChecks)
	{
		FGridSkillEntryView Entry;
		Entry.SkillId = SkillId;
		Entry.DisplayName = FText::FromString(TEXT("Compétence test"));
		Entry.Description = FText::FromString(TEXT("Description longue."));
		Entry.GoverningAttribute = ERPGSkillGoverningAttribute::Dexterity;
		Entry.Rank = Rank;
		Entry.MaxRank = 5;
		Entry.bAllowUntrainedChecks = bAllowUntrainedChecks;
		Entry.bTrained = Rank > 0;
		Entry.CurrentRankCap = 2;
		Entry.bCanIncreaseRank = Rank < 2;
		Entry.bCanDecreaseRank = false;
		return Entry;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUISKILLS023CompactSemanticsTest,
	"Grimrock.UI.Skills02.CompactEntry.Semantics",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUISKILLS023CompactSemanticsTest::RunTest(const FString&)
{
	using namespace UISKILLS023Tests;

	const FGridSkillEntryView Allowed =
		MakeEntry(TEXT("Skill_Acrobatics"), 0, true);
	const FGridSkillEntryView Required =
		MakeEntry(TEXT("Skill_Lockpicking"), 0, false);
	const FGridSkillEntryView Trained =
		MakeEntry(TEXT("Skill_Athletics"), 1, true);

	TestEqual(
		TEXT("Rank zero allowed maps to UntrainedAllowed"),
		UGridSkillEntryWidget::ResolveTrainingState(Allowed),
		EGridSkillTrainingState::UntrainedAllowed);
	TestTrue(
		TEXT("Untrained-allowed label is player-facing"),
		UGridSkillEntryWidget::ResolveTrainingLabel(Allowed).EqualTo(
			FText::FromString(TEXT("Utilisable sans entraînement"))));

	TestEqual(
		TEXT("Rank zero trained-only maps to TrainingRequired"),
		UGridSkillEntryWidget::ResolveTrainingState(Required),
		EGridSkillTrainingState::TrainingRequired);
	TestTrue(
		TEXT("Training-required label remains explicit"),
		UGridSkillEntryWidget::ResolveTrainingLabel(Required).EqualTo(
			FText::FromString(TEXT("Entraînement requis"))));

	TestEqual(
		TEXT("Positive rank maps to Trained"),
		UGridSkillEntryWidget::ResolveTrainingState(Trained),
		EGridSkillTrainingState::Trained);
	TestTrue(
		TEXT("Trained label remains compact"),
		UGridSkillEntryWidget::ResolveTrainingLabel(Trained).EqualTo(
			FText::FromString(TEXT("Entraînée"))));

	TestEqual(
		TEXT("Normal interaction state resolves"),
		UGridSkillEntryWidget::ResolveVisualState(false, false),
		EGridSkillEntryVisualState::Normal);
	TestEqual(
		TEXT("Hover resolves when not selected"),
		UGridSkillEntryWidget::ResolveVisualState(false, true),
		EGridSkillEntryVisualState::Hovered);
	TestEqual(
		TEXT("Selection takes precedence over hover"),
		UGridSkillEntryWidget::ResolveVisualState(true, true),
		EGridSkillEntryVisualState::Selected);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUISKILLS023EntryStateTest,
	"Grimrock.UI.Skills02.CompactEntry.StateAndIcon",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUISKILLS023EntryStateTest::RunTest(const FString&)
{
	using namespace UISKILLS023Tests;

	UGridSkillEntryWidget* Widget = NewObject<UGridSkillEntryWidget>();
	const FGridSkillEntryView Entry =
		MakeEntry(TEXT("Skill_Acrobatics"), 0, true);

	TestFalse(TEXT("Uninitialized row cannot request selection"), Widget->RequestSkillSelection());
	TestTrue(TEXT("Valid compact row initializes"), Widget->InitializeSkillEntry(Entry));
	TestEqual(TEXT("Initialized row starts normal"),
		Widget->VisualState, EGridSkillEntryVisualState::Normal);

	Widget->SetSelected(true);
	TestTrue(TEXT("Selection flag is stored"), Widget->bSelected);
	TestEqual(TEXT("Selected visual state is projected"),
		Widget->VisualState, EGridSkillEntryVisualState::Selected);
	TestTrue(TEXT("Initialized row can request selection"), Widget->RequestSkillSelection());

	UTexture2D* Icon = NewObject<UTexture2D>();
	Widget->SetSkillIcon(Icon);
	TestEqual(TEXT("Presentation icon is stored"), Widget->SkillIcon.Get(), Icon);

	Widget->ClearSkillEntry();
	TestFalse(TEXT("Clear resets initialization"), Widget->bInitialized);
	TestFalse(TEXT("Clear resets selection"), Widget->bSelected);
	TestNull(TEXT("Clear resets icon"), Widget->SkillIcon.Get());
	TestEqual(TEXT("Clear restores normal visual state"),
		Widget->VisualState, EGridSkillEntryVisualState::Normal);
	TestFalse(TEXT("Cleared row cannot request selection"), Widget->RequestSkillSelection());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUISKILLS023WidgetContractTest,
	"Grimrock.UI.Skills02.CompactEntry.WidgetContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUISKILLS023WidgetContractTest::RunTest(const FString&)
{
	UClass* EntryClass = UGridSkillEntryWidget::StaticClass();

	TestNotNull(
		TEXT("Selection request event exists"),
		FindFProperty<FProperty>(EntryClass, GET_MEMBER_NAME_CHECKED(UGridSkillEntryWidget, OnSelectSkillRequested)));
	TestNotNull(
		TEXT("SetSelected function exists"),
		EntryClass->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UGridSkillEntryWidget, SetSelected)));
	TestNotNull(
		TEXT("SetSkillIcon function exists"),
		EntryClass->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UGridSkillEntryWidget, SetSkillIcon)));
	TestNotNull(
		TEXT("RequestSkillSelection function exists"),
		EntryClass->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UGridSkillEntryWidget, RequestSkillSelection)));

	const FName OptionalBindings[] =
	{
		TEXT("Image_SkillIcon"),
		TEXT("Border_SkillSelection"),
		TEXT("Border_SkillHover")
	};
	for (const FName PropertyName : OptionalBindings)
	{
		FProperty* Property = FindFProperty<FProperty>(EntryClass, PropertyName);
		TestNotNull(
			*FString::Printf(TEXT("%s property exists"), *PropertyName.ToString()),
			Property);
		if (Property)
		{
			TestTrue(
				*FString::Printf(TEXT("%s remains optional before Designer materialization"), *PropertyName.ToString()),
				Property->HasMetaData(TEXT("BindWidgetOptional")));
			TestFalse(
				*FString::Printf(TEXT("%s is not mandatory yet"), *PropertyName.ToString()),
				Property->HasMetaData(TEXT("BindWidget")));
		}
	}

	UClass* SkillsClass = UGridSkillsWidget::StaticClass();
	TestNotNull(
		TEXT("Parent selection handler exists"),
		SkillsClass->FindFunctionByName(TEXT("HandleSkillSelectionRequested")));
	return true;
}

#endif
