#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Components/Border.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "UI/GridPartyMemberWidget.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUICharacter01SelectionAuthorityTest, "Grimrock.UI.Character01.SelectionAuthority",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUICharacter01SelectionAuthorityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
	TestNotNull(TEXT("Party inventory component exists"), Inventory);
	if (!Inventory)
	{
		return false;
	}

	Inventory->PartyInventoryState.ActiveCharacters.SetNum(2);
	Inventory->PartyInventoryState.ActiveCharacters[0].DisplayName = FText::FromString(TEXT("Hero A"));
	Inventory->PartyInventoryState.ActiveCharacters[1].DisplayName = FText::FromString(TEXT("Hero B"));
	Inventory->PartyInventoryState.SelectedCharacterIndex = 0;

	FGridInventoryCharacterSummary Summary0;
	FGridInventoryCharacterSummary Summary1;
	TestTrue(TEXT("Character 0 summary is available"), Inventory->GetCharacterSummary(0, Summary0));
	TestTrue(TEXT("Character 1 summary is available"), Inventory->GetCharacterSummary(1, Summary1));
	TestTrue(TEXT("Character 0 is initially selected"), Summary0.bIsSelected);
	TestFalse(TEXT("Character 1 is initially not selected"), Summary1.bIsSelected);

	TestTrue(TEXT("Selection moves through the authoritative component"), Inventory->SetSelectedCharacterIndex(1));
	TestEqual(TEXT("SelectedCharacterIndex is updated"), Inventory->GetSelectedCharacterIndex(), 1);
	TestTrue(TEXT("Character 0 summary refreshes"), Inventory->GetCharacterSummary(0, Summary0));
	TestTrue(TEXT("Character 1 summary refreshes"), Inventory->GetCharacterSummary(1, Summary1));
	TestFalse(TEXT("Character 0 is no longer selected"), Summary0.bIsSelected);
	TestTrue(TEXT("Character 1 is now selected"), Summary1.bIsSelected);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUICharacter01SelectionVisualTest, "Grimrock.UI.Character01.SelectionVisual",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUICharacter01SelectionVisualTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UGridPartyMemberWidget* MemberWidget = NewObject<UGridPartyMemberWidget>();
	UBorder* SelectionBorder = NewObject<UBorder>(MemberWidget);
	TestNotNull(TEXT("Party member widget exists"), MemberWidget);
	TestNotNull(TEXT("Selection border exists"), SelectionBorder);
	if (!MemberWidget || !SelectionBorder)
	{
		return false;
	}

	MemberWidget->Border_Selected = SelectionBorder;
	FGridInventoryCharacterSummary Summary;
	Summary.CharacterIndex = 2;
	Summary.DisplayName = FText::FromString(TEXT("Selected Hero"));
	Summary.bIsSelected = true;
	MemberWidget->SetCharacterSummary(Summary);

	TestTrue(TEXT("Party member exposes selected state"), MemberWidget->IsSelected());
	TestEqual(TEXT("Selection overlay is visible"), SelectionBorder->GetVisibility(), ESlateVisibility::HitTestInvisible);

	Summary.bIsSelected = false;
	MemberWidget->SetCharacterSummary(Summary);
	TestFalse(TEXT("Party member exposes unselected state"), MemberWidget->IsSelected());
	TestEqual(TEXT("Selection overlay collapses"), SelectionBorder->GetVisibility(), ESlateVisibility::Collapsed);
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUICharacter01PartyMemberCompactContractTest, "Grimrock.UI.Character01.PartyMemberCompactContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUICharacter01PartyMemberCompactContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UClass* PartyMemberClass = UGridPartyMemberWidget::StaticClass();
	if (!TestNotNull(TEXT("Party member widget class exists"), PartyMemberClass))
	{
		return false;
	}

	TestNull(TEXT("Party selector no longer binds character name text"), FindFProperty<FProperty>(PartyMemberClass, TEXT("Text_Name")));
	TestNull(TEXT("Party selector no longer binds class/level text"), FindFProperty<FProperty>(PartyMemberClass, TEXT("Text_ClassLevel")));
	TestNull(TEXT("Party selector no longer binds weight text"), FindFProperty<FProperty>(PartyMemberClass, TEXT("Text_Weight")));
	TestNull(TEXT("Party selector display-name helper is removed"), PartyMemberClass->FindFunctionByName(TEXT("GetDisplayNameText")));
	TestNull(TEXT("Party selector class-level helper is removed"), PartyMemberClass->FindFunctionByName(TEXT("GetClassLevelText")));
	TestNull(TEXT("Party selector weight helper is removed"), PartyMemberClass->FindFunctionByName(TEXT("GetWeightText")));

	TestNotNull(TEXT("Portrait binding remains"), FindFProperty<FProperty>(PartyMemberClass, TEXT("Image_Portrait")));
	TestNull(TEXT("Redundant class icon binding is removed"), FindFProperty<FProperty>(PartyMemberClass, TEXT("Image_ClassIcon")));
	TestNull(TEXT("Redundant class accent binding is removed"), FindFProperty<FProperty>(PartyMemberClass, TEXT("Border_ClassAccent")));
	TestNull(TEXT("Party-specific class visual setter is removed"), PartyMemberClass->FindFunctionByName(TEXT("SetAvailableClassVisuals")));
	TestNotNull(TEXT("Selection border binding remains"), FindFProperty<FProperty>(PartyMemberClass, TEXT("Border_Selected")));
	TestNotNull(TEXT("Overload warning binding remains"), FindFProperty<FProperty>(PartyMemberClass, TEXT("Image_WeightAlert")));
	TestNotNull(TEXT("Status-effect row binding remains"), FindFProperty<FProperty>(PartyMemberClass, TEXT("HorizontalBox_StatusEffects")));
	return true;
}

#endif
