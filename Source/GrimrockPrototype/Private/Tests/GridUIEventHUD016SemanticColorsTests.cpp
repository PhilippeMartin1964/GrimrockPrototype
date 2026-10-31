#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "UI/GridEventFeedWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIEventHUD016SemanticPaletteTest, "Grimrock.UI.EventHUD01.SemanticColors.Palette",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIEventHUD016SemanticPaletteTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridEventFeedSemanticPalette Palette;
	Palette.Default = FLinearColor(0.01f, 0.02f, 0.03f, 1.0f);
	Palette.Information = FLinearColor(0.11f, 0.12f, 0.13f, 1.0f);
	Palette.Interaction = FLinearColor(0.21f, 0.22f, 0.23f, 1.0f);
	Palette.Locked = FLinearColor(0.31f, 0.32f, 0.33f, 1.0f);
	Palette.Mechanism = FLinearColor(0.41f, 0.42f, 0.43f, 1.0f);
	Palette.Secret = FLinearColor(0.51f, 0.52f, 0.53f, 1.0f);
	Palette.Danger = FLinearColor(0.61f, 0.62f, 0.63f, 1.0f);
	Palette.PhysicalDamage = FLinearColor(0.71f, 0.72f, 0.73f, 1.0f);
	Palette.Fire = FLinearColor(0.81f, 0.12f, 0.13f, 1.0f);
	Palette.Ice = FLinearColor(0.11f, 0.82f, 0.83f, 1.0f);
	Palette.Lightning = FLinearColor(0.21f, 0.22f, 0.93f, 1.0f);
	Palette.Poison = FLinearColor(0.31f, 0.92f, 0.33f, 1.0f);
	Palette.Arcane = FLinearColor(0.71f, 0.22f, 0.83f, 1.0f);
	Palette.Healing = FLinearColor(0.21f, 0.92f, 0.53f, 1.0f);
	Palette.Experience = FLinearColor(0.71f, 1.0f, 0.13f, 1.0f);
	Palette.Buff = FLinearColor(0.41f, 0.82f, 0.43f, 1.0f);
	Palette.Debuff = FLinearColor(0.91f, 0.32f, 0.33f, 1.0f);
	Palette.Dialogue = FLinearColor(0.61f, 0.72f, 0.83f, 1.0f);
	Palette.Holy = FLinearColor(0.91f, 0.82f, 0.63f, 1.0f);
	Palette.Necrotic = FLinearColor(0.71f, 0.42f, 0.73f, 1.0f);

	struct FCase
	{
		EGridHudEventSemantic Semantic;
		FLinearColor Expected;
	};

	const TArray<FCase> Cases = {
		{ EGridHudEventSemantic::Default, Palette.Default },
		{ EGridHudEventSemantic::Information, Palette.Information },
		{ EGridHudEventSemantic::Interaction, Palette.Interaction },
		{ EGridHudEventSemantic::Locked, Palette.Locked },
		{ EGridHudEventSemantic::Mechanism, Palette.Mechanism },
		{ EGridHudEventSemantic::Secret, Palette.Secret },
		{ EGridHudEventSemantic::Danger, Palette.Danger },
		{ EGridHudEventSemantic::PhysicalDamage, Palette.PhysicalDamage },
		{ EGridHudEventSemantic::Fire, Palette.Fire },
		{ EGridHudEventSemantic::Ice, Palette.Ice },
		{ EGridHudEventSemantic::Lightning, Palette.Lightning },
		{ EGridHudEventSemantic::Poison, Palette.Poison },
		{ EGridHudEventSemantic::Arcane, Palette.Arcane },
		{ EGridHudEventSemantic::Healing, Palette.Healing },
		{ EGridHudEventSemantic::Experience, Palette.Experience },
		{ EGridHudEventSemantic::Buff, Palette.Buff },
		{ EGridHudEventSemantic::Debuff, Palette.Debuff },
		{ EGridHudEventSemantic::Dialogue, Palette.Dialogue },
		{ EGridHudEventSemantic::Holy, Palette.Holy },
		{ EGridHudEventSemantic::Necrotic, Palette.Necrotic }
	};

	for (const FCase& Case : Cases)
	{
		TestTrue(
			TEXT("Semantic resolves to its dedicated palette colour"),
			UGridEventFeedWidget::ResolveSemanticColor(Case.Semantic, Palette).Equals(Case.Expected));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIEventHUD016SemanticDefaultsTest, "Grimrock.UI.EventHUD01.SemanticColors.Defaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIEventHUD016SemanticDefaultsTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridEventFeedWidget* Widget = NewObject<UGridEventFeedWidget>();
	if (!TestNotNull(TEXT("Event Feed widget exists"), Widget))
	{
		return false;
	}

	TestTrue(TEXT("Information remains neutral"),
		Widget->GetSemanticColor(EGridHudEventSemantic::Information).Equals(Widget->SemanticPalette.Information));
	TestFalse(TEXT("Interaction differs from neutral information"),
		Widget->GetSemanticColor(EGridHudEventSemantic::Interaction).Equals(
			Widget->GetSemanticColor(EGridHudEventSemantic::Information)));
	TestFalse(TEXT("Secret differs from danger"),
		Widget->GetSemanticColor(EGridHudEventSemantic::Secret).Equals(
			Widget->GetSemanticColor(EGridHudEventSemantic::Danger)));
	TestFalse(TEXT("Fire differs from ice"),
		Widget->GetSemanticColor(EGridHudEventSemantic::Fire).Equals(
			Widget->GetSemanticColor(EGridHudEventSemantic::Ice)));
	TestTrue(TEXT("Experience is a distinct vivid green"),
		Widget->GetSemanticColor(EGridHudEventSemantic::Experience).G > 0.95f &&
		Widget->GetSemanticColor(EGridHudEventSemantic::Experience).R > 0.15f &&
		Widget->GetSemanticColor(EGridHudEventSemantic::Experience).B < 0.07f);
	TestFalse(TEXT("XP green is not the healing colour"),
		Widget->GetSemanticColor(EGridHudEventSemantic::Experience).Equals(
			Widget->GetSemanticColor(EGridHudEventSemantic::Healing)));
	TestFalse(TEXT("XP green is not poison colour"),
		Widget->GetSemanticColor(EGridHudEventSemantic::Experience).Equals(
			Widget->GetSemanticColor(EGridHudEventSemantic::Poison)));
	TestFalse(TEXT("Healing differs from debuff"),
		Widget->GetSemanticColor(EGridHudEventSemantic::Healing).Equals(
			Widget->GetSemanticColor(EGridHudEventSemantic::Debuff)));

	return true;
}

#endif
