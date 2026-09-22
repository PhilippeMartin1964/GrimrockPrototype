#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "UI/GridInventoryWidget.h"
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIFeedback012StatusIndicatorRenderingTest, "Grimrock.UI.Feedback01.PartyStatusIndicators",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIFeedback012StatusIndicatorRenderingTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridPartyMemberWidget* MemberWidget = NewObject<UGridPartyMemberWidget>();
	UHorizontalBox* StatusPanel = NewObject<UHorizontalBox>(MemberWidget);
	if (!TestNotNull(TEXT("Party member widget exists"), MemberWidget) || !TestNotNull(TEXT("Status panel exists"), StatusPanel))
	{
		return false;
	}

	MemberWidget->HorizontalBox_StatusEffects = StatusPanel;
	MemberWidget->MaxStatusEffectIndicators = 4;

	TArray<FGridStatusEffectPresentationView> Views;
	FGridStatusEffectPresentationView Poison;
	Poison.EffectId = TEXT("Poison");
	Poison.DisplayName = FText::FromString(TEXT("Poison"));
	Poison.ToolTipText = FText::FromString(TEXT("Poison actif"));
	Views.Add(Poison);

	FGridStatusEffectPresentationView Haste;
	Haste.EffectId = TEXT("Haste");
	Haste.DisplayName = FText::FromString(TEXT("Hâte"));
	Haste.ToolTipText = FText::FromString(TEXT("Hâte active"));
	Views.Add(Haste);

	MemberWidget->SetStatusEffects(Views);
	TestEqual(TEXT("Two status views are cached"), MemberWidget->CachedStatusEffects.Num(), 2);
	TestEqual(TEXT("Two indicators are rendered"), StatusPanel->GetChildrenCount(), 2);
	TestEqual(TEXT("Status row is visible without intercepting its own hit test"), StatusPanel->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);

	MemberWidget->SetStatusEffects(TArray<FGridStatusEffectPresentationView>());
	TestEqual(TEXT("Clearing status views removes indicators"), StatusPanel->GetChildrenCount(), 0);
	TestEqual(TEXT("Empty status row is collapsed"), StatusPanel->GetVisibility(), ESlateVisibility::Collapsed);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIFeedback012StatusProjectionIntegrationTest, "Grimrock.UI.Feedback01.PartyStatusProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIFeedback012StatusProjectionIntegrationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
	UGridInventoryWidget* InventoryWidget = NewObject<UGridInventoryWidget>();
	UGridPartyMemberWidget* MemberWidget = NewObject<UGridPartyMemberWidget>();
	UHorizontalBox* StatusPanel = NewObject<UHorizontalBox>(MemberWidget);
	UGridStatusEffectDefinitionAsset* Definition = NewObject<UGridStatusEffectDefinitionAsset>(Inventory);
	if (!TestNotNull(TEXT("Inventory exists"), Inventory) || !TestNotNull(TEXT("Inventory widget exists"), InventoryWidget) ||
		!TestNotNull(TEXT("Member widget exists"), MemberWidget) || !TestNotNull(TEXT("Status definition exists"), Definition))
	{
		return false;
	}

	Inventory->PartyInventoryState.ActiveCharacters.SetNum(1);
	Inventory->PartyInventoryState.SelectedCharacterIndex = 0;
	FGridCharacterInventoryState& Character = Inventory->PartyInventoryState.ActiveCharacters[0];
	Character.DisplayName = FText::FromString(TEXT("Status Hero"));

	Definition->EffectId = TEXT("Feedback012_Poison");
	Definition->DisplayName = FText::FromString(TEXT("Poison"));

	FGridStatusEffectRuntimeState State;
	State.EffectId = Definition->EffectId;
	State.SourceId = FGuid::NewGuid();
	State.StackCount = 1;
	State.DurationUnit = EGridStatusEffectDurationUnit::Rounds;
	State.RemainingDuration = 2;
	State.Potency = 0;
	State.DefinitionAsset = Definition;
	Character.StatusEffects.ActiveEffects.Add(State);

	InventoryWidget->InventoryComponent = Inventory;
	MemberWidget->HorizontalBox_StatusEffects = StatusPanel;
	InventoryWidget->RegisterPartyMemberWidget(MemberWidget, 0);

	TestEqual(TEXT("Authoritative character status projects to portrait"), MemberWidget->CachedStatusEffects.Num(), 1);
	if (MemberWidget->CachedStatusEffects.Num() == 1)
	{
		TestEqual(TEXT("Projected effect id is preserved"), MemberWidget->CachedStatusEffects[0].EffectId, Definition->EffectId);
	}
	TestEqual(TEXT("One indicator is rendered from authoritative status"), StatusPanel->GetChildrenCount(), 1);
	return true;
}

#endif
