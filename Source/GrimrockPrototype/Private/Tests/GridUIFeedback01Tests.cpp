#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/Image.h"
#include "UI/GridPartyMemberWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIFeedback01PartyWeightAlertTest, "Grimrock.UI.Feedback01.PartyWeightAlert",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIFeedback01PartyWeightAlertTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridPartyMemberWidget* MemberWidget = NewObject<UGridPartyMemberWidget>();
	UImage* WeightAlert = NewObject<UImage>(MemberWidget);
	TestNotNull(TEXT("Party member widget exists"), MemberWidget);
	TestNotNull(TEXT("Weight alert image exists"), WeightAlert);
	if (!MemberWidget || !WeightAlert)
	{
		return false;
	}

	MemberWidget->Image_WeightAlert = WeightAlert;

	FGridInventoryCharacterSummary Summary;
	Summary.CharacterIndex = 0;

	Summary.WeightState = EGridInventoryWeightState::Normal;
	MemberWidget->SetCharacterSummary(Summary);
	TestEqual(TEXT("Normal weight hides alert"), WeightAlert->GetVisibility(), ESlateVisibility::Collapsed);

	Summary.WeightState = EGridInventoryWeightState::Heavy;
	MemberWidget->SetCharacterSummary(Summary);
	TestEqual(TEXT("Heavy weight still hides overload alert"), WeightAlert->GetVisibility(), ESlateVisibility::Collapsed);

	Summary.WeightState = EGridInventoryWeightState::Overloaded;
	MemberWidget->SetCharacterSummary(Summary);
	TestEqual(TEXT("Overloaded weight shows alert"), WeightAlert->GetVisibility(), ESlateVisibility::HitTestInvisible);

	Summary.WeightState = EGridInventoryWeightState::Normal;
	MemberWidget->SetCharacterSummary(Summary);
	TestEqual(TEXT("Returning to normal hides alert again"), WeightAlert->GetVisibility(), ESlateVisibility::Collapsed);

	return true;
}

#endif
