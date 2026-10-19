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
	UImage* ProgressionAlert = NewObject<UImage>(MemberWidget);
	TestNotNull(TEXT("Party member widget exists"), MemberWidget);
	TestNotNull(TEXT("Weight alert image exists"), WeightAlert);
	TestNotNull(TEXT("Progression alert image exists"), ProgressionAlert);
	if (!MemberWidget || !WeightAlert || !ProgressionAlert)
	{
		return false;
	}

	MemberWidget->Image_WeightAlert = WeightAlert;
	MemberWidget->Image_LevelUpAlert = ProgressionAlert;

	FGridInventoryCharacterSummary Summary;
	Summary.CharacterIndex = 0;

	Summary.WeightState = EGridInventoryWeightState::Normal;
	MemberWidget->SetCharacterSummary(Summary);
	TestEqual(TEXT("Normal weight hides alert"), WeightAlert->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("No progression points hides the '+' badge"), ProgressionAlert->GetVisibility(), ESlateVisibility::Collapsed);

	Summary.WeightState = EGridInventoryWeightState::Heavy;
	MemberWidget->SetCharacterSummary(Summary);
	TestEqual(TEXT("Heavy weight still hides overload alert"), WeightAlert->GetVisibility(), ESlateVisibility::Collapsed);

	Summary.WeightState = EGridInventoryWeightState::Overloaded;
	Summary.bHasUnspentProgressionPoints = true;
	MemberWidget->SetCharacterSummary(Summary);
	TestEqual(TEXT("Overloaded weight shows alert"), WeightAlert->GetVisibility(), ESlateVisibility::HitTestInvisible);
	TestEqual(TEXT("Unspent progression coexists with overload"), ProgressionAlert->GetVisibility(), ESlateVisibility::HitTestInvisible);

	Summary.bHasUnspentProgressionPoints = false;
	MemberWidget->SetCharacterSummary(Summary);
	TestEqual(TEXT("Spending progression hides only '+'"), ProgressionAlert->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("Spending progression leaves overload visible"), WeightAlert->GetVisibility(), ESlateVisibility::HitTestInvisible);

	Summary.WeightState = EGridInventoryWeightState::Normal;
	MemberWidget->SetCharacterSummary(Summary);
	TestEqual(TEXT("Returning to normal hides alert again"), WeightAlert->GetVisibility(), ESlateVisibility::Collapsed);

	Summary.bHasUnspentProgressionPoints = true;
	MemberWidget->SetCharacterSummary(Summary);
	TestEqual(TEXT("Progression badge is independent of weight"), ProgressionAlert->GetVisibility(), ESlateVisibility::HitTestInvisible);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIFeedback01CharacterSheetPortraitAlertsTest,
	"Grimrock.UI.Feedback01.CharacterSheetPortraitAlerts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIFeedback01CharacterSheetPortraitAlertsTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
	UGridInventoryWidget* Sheet = NewObject<UGridInventoryWidget>();
	if (!TestNotNull(TEXT("Inventory component exists"), Inventory) ||
		!TestNotNull(TEXT("Character sheet presenter exists"), Sheet))
	{
		return false;
	}

	Sheet->InventoryComponent = Inventory;
	Sheet->Image_CharacterLevelUpAlert = NewObject<UImage>(Sheet);
	Sheet->Image_CharacterWeightAlert = NewObject<UImage>(Sheet);

	// No selected character: the two overlays must disappear.
	Sheet->RefreshSelectedCharacterDetails();
	TestEqual(TEXT("Missing character hides progression"), Sheet->Image_CharacterLevelUpAlert->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("Missing character hides overload"), Sheet->Image_CharacterWeightAlert->GetVisibility(), ESlateVisibility::Collapsed);

	FGridCharacterInventoryState Character;
	Character.CharacterId = FGuid(1, 2, 3, 4);
	Character.Level = 1; // Four unspent skill points from the canonical RPG balance.
	Character.DisplayName = FText::FromString(TEXT("Sheet Test Hero"));
	Inventory->PartyInventoryState.ActiveCharacters.Add(Character);
	Inventory->PartyInventoryState.ActiveEquipment.SetNum(1);
	Inventory->PartyInventoryState.SelectedCharacterIndex = 0;
	Sheet->RefreshSelectedCharacterDetails();
	TestEqual(TEXT("Available Skill points show the selected portrait '+'"), Sheet->Image_CharacterLevelUpAlert->GetVisibility(), ESlateVisibility::HitTestInvisible);
	TestEqual(TEXT("Normal weight hides the selected portrait warning"), Sheet->Image_CharacterWeightAlert->GetVisibility(), ESlateVisibility::Collapsed);

	FGridInventorySlot HeavySlot;
	HeavySlot.bOccupied = true;
	HeavySlot.Item.RuntimeObjectId = FGuid(1, 2, 3, 5);
	HeavySlot.Item.ItemDefinitionId = TEXT("TestHeavyPortraitItem");
	HeavySlot.Item.Quantity = 1;
	HeavySlot.Item.Weight = 100000.0f;
	Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots.Add(HeavySlot);
	Sheet->RefreshSelectedCharacterDetails();
	TestEqual(TEXT("Overload appears on the selected portrait"), Sheet->Image_CharacterWeightAlert->GetVisibility(), ESlateVisibility::HitTestInvisible);
	TestEqual(TEXT("Overload coexists with the selected portrait '+'"), Sheet->Image_CharacterLevelUpAlert->GetVisibility(), ESlateVisibility::HitTestInvisible);

	Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots.Reset();
	FRPGSkillRank RankA;
	RankA.SkillId = TEXT("SkillA");
	RankA.Rank = 2;
	FRPGSkillRank RankB;
	RankB.SkillId = TEXT("SkillB");
	RankB.Rank = 2;
	Inventory->PartyInventoryState.ActiveCharacters[0].SkillRanks = { RankA, RankB };
	Sheet->RefreshSelectedCharacterDetails();
	TestEqual(TEXT("Spent points hide the selected portrait '+'"), Sheet->Image_CharacterLevelUpAlert->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("Weight relief hides the selected portrait warning"), Sheet->Image_CharacterWeightAlert->GetVisibility(), ESlateVisibility::Collapsed);
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
