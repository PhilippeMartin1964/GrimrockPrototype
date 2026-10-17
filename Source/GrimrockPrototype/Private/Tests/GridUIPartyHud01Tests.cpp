#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/VerticalBox.h"
#include "RPG/RPGClassAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "UI/GridPartyPortraitHudWidget.h"
#include "UI/GridPartyPortraitDragDropOperation.h"
#include "UI/GridInventoryDragDropOperation.h"
#include "UI/GridPersistentHudWidget.h"
#include "UObject/UnrealType.h"

namespace GridPartyHud01Tests
{
	FGuid MakeId(uint32 Value)
	{
		return FGuid(0x50485544, 0x30310000, 0, Value);
	}

	FGridCharacterInventoryState MakeCharacter(
		const FGuid& CharacterId,
		URPGClassAsset* ClassDefinition,
		int32 CurrentHealth,
		int32 MaximumHealth,
		int32 CurrentMana,
		int32 MaximumMana,
		int32 CurrentPhysicalArmor,
		int32 CurrentMagicalArmor)
	{
		FGridCharacterInventoryState Character;
		Character.CharacterId = CharacterId;
		Character.DisplayName = FText::FromString(FString::Printf(TEXT("Portrait_%u"), CharacterId.D));
		Character.ClassId = ClassDefinition ? ClassDefinition->ClassId : NAME_None;
		Character.ClassDefinition = ClassDefinition;
		Character.DerivedStats.MaxHealth = MaximumHealth;
		Character.DerivedStats.MaxMana = MaximumMana;
		Character.Resources.CurrentHealth = CurrentHealth;
		Character.Resources.CurrentMana = CurrentMana;
		Character.Resources.CurrentPhysicalArmor = CurrentPhysicalArmor;
		Character.Resources.CurrentMagicalArmor = CurrentMagicalArmor;
		return Character;
	}

	URPGClassAsset* MakeClass(UObject* Outer, FName ClassId, int32 PhysicalArmor, int32 MagicalArmor)
	{
		URPGClassAsset* ClassDefinition = NewObject<URPGClassAsset>(Outer);
		ClassDefinition->ClassId = ClassId;
		ClassDefinition->DisplayName = FText::FromName(ClassId);
		ClassDefinition->HealthAtLevelOne = 1;
		ClassDefinition->BasePhysicalArmor = PhysicalArmor;
		ClassDefinition->BaseMagicalArmor = MagicalArmor;
		return ClassDefinition;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridPartyHud01ViewProjectionTest, "Grimrock.UI.PartyHud01.ViewProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridPartyHud01ViewProjectionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridPartyHud01Tests;

	UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
	URPGClassAsset* MageClass = MakeClass(Inventory, TEXT("PartyHudMage"), 10, 4);
	URPGClassAsset* WarriorClass = MakeClass(Inventory, TEXT("PartyHudWarrior"), 6, 0);

	Inventory->PartyInventoryState.ActiveCharacters.Add(MakeCharacter(MakeId(1), MageClass, 75, 100, 25, 50, 8, 2));
	Inventory->PartyInventoryState.ActiveCharacters.Add(MakeCharacter(MakeId(2), WarriorClass, 40, 80, 0, 0, 3, 0));
	Inventory->PartyInventoryState.ActiveEquipment.SetNum(2);
	Inventory->PartyInventoryState.SelectedCharacterIndex = 0;

	TArray<FGridPartyPortraitView> Views;
	TestTrue(TEXT("Portrait projection is complete"), Inventory->GetPartyPortraitViews(Views));
	TestEqual(TEXT("Two portraits are projected"), Views.Num(), 2);
	if (Views.Num() != 2)
	{
		return false;
	}

	TestEqual(TEXT("Mage CharacterId remains stable"), Views[0].CharacterId, MakeId(1));
	TestEqual(TEXT("Mage runtime index is resolved"), Views[0].CharacterIndex, 0);
	TestTrue(TEXT("Mage is selected"), Views[0].bIsSelected);
	TestEqual(TEXT("Health current"), Views[0].CurrentHealth, 75);
	TestEqual(TEXT("Health maximum"), Views[0].MaximumHealth, 100);
	TestTrue(TEXT("Health ratio"), FMath::IsNearlyEqual(Views[0].HealthRatio, 0.75f));
	TestTrue(TEXT("Mage exposes mana"), Views[0].bHasMana);
	TestTrue(TEXT("Mana ratio"), FMath::IsNearlyEqual(Views[0].ManaRatio, 0.5f));
	TestEqual(TEXT("Physical armor reference uses class authority"), Views[0].ReferencePhysicalArmor, 10);
	TestTrue(TEXT("Physical armor ratio"), FMath::IsNearlyEqual(Views[0].PhysicalArmorRatio, 0.8f));
	TestEqual(TEXT("Magical armor reference uses class authority"), Views[0].ReferenceMagicalArmor, 4);
	TestTrue(TEXT("Magical armor ratio"), FMath::IsNearlyEqual(Views[0].MagicalArmorRatio, 0.5f));

	TestFalse(TEXT("Zero-mana warrior hides mana bar"), Views[1].bHasMana);
	TestEqual(TEXT("Zero-mana warrior ratio is zero"), Views[1].ManaRatio, 0.0f);
	TestEqual(TEXT("Zero magical armor reference remains zero"), Views[1].ReferenceMagicalArmor, 0);
	TestEqual(TEXT("Zero magical armor ratio remains zero"), Views[1].MagicalArmorRatio, 0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridPartyHud01OrderSanitizationTest, "Grimrock.UI.PartyHud01.OrderSanitization",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridPartyHud01OrderSanitizationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridPartyHud01Tests;

	UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
	URPGClassAsset* ClassDefinition = MakeClass(Inventory, TEXT("PartyHudOrder"), 0, 0);
	const FGuid A = MakeId(10);
	const FGuid B = MakeId(11);
	const FGuid C = MakeId(12);
	Inventory->PartyInventoryState.ActiveCharacters.Add(MakeCharacter(A, ClassDefinition, 1, 1, 0, 0, 0, 0));
	Inventory->PartyInventoryState.ActiveCharacters.Add(MakeCharacter(B, ClassDefinition, 1, 1, 0, 0, 0, 0));
	Inventory->PartyInventoryState.ActiveCharacters.Add(MakeCharacter(C, ClassDefinition, 1, 1, 0, 0, 0, 0));
	Inventory->PartyInventoryState.ActiveEquipment.SetNum(3);

	Inventory->PartyPortraitOrder = { C, FGuid(), C, A, MakeId(99) };
	Inventory->SanitizePartyPortraitOrder();

	TestEqual(TEXT("Sanitized order contains every active character once"), Inventory->PartyPortraitOrder.Num(), 3);
	TestEqual(TEXT("Existing valid first entry is preserved"), Inventory->PartyPortraitOrder[0], C);
	TestEqual(TEXT("Existing valid second entry is preserved"), Inventory->PartyPortraitOrder[1], A);
	TestEqual(TEXT("Missing active character is appended"), Inventory->PartyPortraitOrder[2], B);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridPartyHud01ReorderPreservesRuntimeTest, "Grimrock.UI.PartyHud01.ReorderPreservesRuntime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridPartyHud01ReorderPreservesRuntimeTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridPartyHud01Tests;

	UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
	URPGClassAsset* ClassDefinition = MakeClass(Inventory, TEXT("PartyHudReorder"), 0, 0);
	const FGuid A = MakeId(20);
	const FGuid B = MakeId(21);
	const FGuid C = MakeId(22);
	Inventory->PartyInventoryState.ActiveCharacters.Add(MakeCharacter(A, ClassDefinition, 1, 1, 0, 0, 0, 0));
	Inventory->PartyInventoryState.ActiveCharacters.Add(MakeCharacter(B, ClassDefinition, 1, 1, 0, 0, 0, 0));
	Inventory->PartyInventoryState.ActiveCharacters.Add(MakeCharacter(C, ClassDefinition, 1, 1, 0, 0, 0, 0));
	Inventory->PartyInventoryState.ActiveEquipment.SetNum(3);
	Inventory->PartyInventoryState.SelectedCharacterIndex = 1;
	Inventory->SanitizePartyPortraitOrder();

	TestTrue(TEXT("C can be inserted before A"), Inventory->ReorderPartyPortrait(C, A, false));
	TestEqual(TEXT("Visual order C"), Inventory->PartyPortraitOrder[0], C);
	TestEqual(TEXT("Visual order A"), Inventory->PartyPortraitOrder[1], A);
	TestEqual(TEXT("Visual order B"), Inventory->PartyPortraitOrder[2], B);

	TestEqual(TEXT("Runtime A index is unchanged"), Inventory->PartyInventoryState.ActiveCharacters[0].CharacterId, A);
	TestEqual(TEXT("Runtime B index is unchanged"), Inventory->PartyInventoryState.ActiveCharacters[1].CharacterId, B);
	TestEqual(TEXT("Runtime C index is unchanged"), Inventory->PartyInventoryState.ActiveCharacters[2].CharacterId, C);
	TestEqual(TEXT("SelectedCharacterIndex is unchanged"), Inventory->PartyInventoryState.SelectedCharacterIndex, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridPartyHud01RosterChangeTest, "Grimrock.UI.PartyHud01.RosterChangeSanitization",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridPartyHud01RosterChangeTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridPartyHud01Tests;

	UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
	URPGClassAsset* ClassDefinition = MakeClass(Inventory, TEXT("PartyHudRoster"), 0, 0);
	const FGuid A = MakeId(30);
	const FGuid B = MakeId(31);
	const FGuid C = MakeId(32);
	Inventory->PartyInventoryState.ActiveCharacters.Add(MakeCharacter(A, ClassDefinition, 1, 1, 0, 0, 0, 0));
	Inventory->PartyInventoryState.ActiveCharacters.Add(MakeCharacter(B, ClassDefinition, 1, 1, 0, 0, 0, 0));
	Inventory->PartyInventoryState.ActiveEquipment.SetNum(2);
	Inventory->NotifyPartyInventoryChanged(INDEX_NONE);

	TestEqual(TEXT("Initial roster creates two portrait ids"), Inventory->PartyPortraitOrder.Num(), 2);

	Inventory->PartyInventoryState.ActiveCharacters.Add(MakeCharacter(C, ClassDefinition, 1, 1, 0, 0, 0, 0));
	Inventory->PartyInventoryState.ActiveEquipment.AddDefaulted();
	Inventory->NotifyPartyInventoryChanged(2);
	TestEqual(TEXT("New recruit is appended"), Inventory->PartyPortraitOrder.Last(), C);

	Inventory->PartyInventoryState.ActiveCharacters.RemoveAt(1);
	Inventory->PartyInventoryState.ActiveEquipment.RemoveAt(1);
	Inventory->NotifyPartyInventoryChanged(INDEX_NONE);
	TestEqual(TEXT("Removed character disappears from portrait order"), Inventory->PartyPortraitOrder.Num(), 2);
	TestFalse(TEXT("Removed id is absent"), Inventory->PartyPortraitOrder.Contains(B));
	TestTrue(TEXT("Remaining A is present"), Inventory->PartyPortraitOrder.Contains(A));
	TestTrue(TEXT("Remaining C is present"), Inventory->PartyPortraitOrder.Contains(C));
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridPartyHud012PortraitWidgetPresentationTest, "Grimrock.UI.PartyHud01.PersistentPortraitWidgetPresentation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridPartyHud012PortraitWidgetPresentationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridPartyPortraitHudWidget* Widget = NewObject<UGridPartyPortraitHudWidget>();
	if (!TestNotNull(TEXT("Persistent portrait widget exists"), Widget))
	{
		return false;
	}

	Widget->Image_Portrait = NewObject<UImage>(Widget);
	Widget->ProgressBar_Health = NewObject<UProgressBar>(Widget);
	Widget->ProgressBar_Mana = NewObject<UProgressBar>(Widget);
	Widget->ProgressBar_PhysicalArmor = NewObject<UProgressBar>(Widget);
	Widget->ProgressBar_MagicalArmor = NewObject<UProgressBar>(Widget);
	Widget->Border_Selected = NewObject<UBorder>(Widget);

	FGridPartyPortraitView View;
	View.CharacterId = GridPartyHud01Tests::MakeId(40);
	View.CharacterIndex = 2;
	View.bIsSelected = true;
	View.HealthRatio = 0.75f;
	View.bHasMana = true;
	View.ManaRatio = 0.5f;
	View.PhysicalArmorRatio = 0.25f;
	View.MagicalArmorRatio = 1.0f;
	Widget->SetPortraitView(View);

	TestEqual(TEXT("Widget keeps stable CharacterId"), Widget->GetCharacterId(), View.CharacterId);
	TestTrue(TEXT("Widget exposes selected state"), Widget->IsSelected());
	TestTrue(TEXT("Health percentage follows read model"), FMath::IsNearlyEqual(Widget->ProgressBar_Health->GetPercent(), 0.75f));
	TestTrue(TEXT("Mana percentage follows read model"), FMath::IsNearlyEqual(Widget->ProgressBar_Mana->GetPercent(), 0.5f));
	TestTrue(TEXT("Physical armor percentage follows read model"), FMath::IsNearlyEqual(Widget->ProgressBar_PhysicalArmor->GetPercent(), 0.25f));
	TestTrue(TEXT("Magical armor percentage follows read model"), FMath::IsNearlyEqual(Widget->ProgressBar_MagicalArmor->GetPercent(), 1.0f));
	TestEqual(TEXT("Mana bar is visible for a mana user"), Widget->ProgressBar_Mana->GetVisibility(), ESlateVisibility::HitTestInvisible);
	TestEqual(TEXT("Selected frame is visible"), Widget->Border_Selected->GetVisibility(), ESlateVisibility::HitTestInvisible);
	TestEqual(TEXT("Portrait root is hit-testable because the portrait itself is the interaction surface"), Widget->GetVisibility(), ESlateVisibility::Visible);

	View.bHasMana = false;
	View.ManaRatio = 0.9f;
	View.bIsSelected = false;
	Widget->SetPortraitView(View);
	TestEqual(TEXT("Mana bar collapses without a mana pool"), Widget->ProgressBar_Mana->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("Hidden mana bar resets to zero"), Widget->ProgressBar_Mana->GetPercent(), 0.0f);
	TestEqual(TEXT("Selection frame collapses when not selected"), Widget->Border_Selected->GetVisibility(), ESlateVisibility::Collapsed);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridPartyHud012PersistentRailContractTest, "Grimrock.UI.PartyHud01.PersistentRailContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridPartyHud012PersistentRailContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const UClass* PortraitClass = UGridPartyPortraitHudWidget::StaticClass();
	const FName PortraitBindings[] = {
		TEXT("Image_Portrait"),
		TEXT("ProgressBar_Health"),
		TEXT("ProgressBar_Mana"),
		TEXT("ProgressBar_PhysicalArmor"),
		TEXT("ProgressBar_MagicalArmor"),
		TEXT("Border_Selected")
	};
	for (const FName PropertyName : PortraitBindings)
	{
		const FProperty* Property = PortraitClass->FindPropertyByName(PropertyName);
		if (TestNotNull(FString::Printf(TEXT("Portrait presenter exposes optional binding %s"), *PropertyName.ToString()), Property))
		{
			TestTrue(FString::Printf(TEXT("%s is an optional Designer binding"), *PropertyName.ToString()),
				Property->HasMetaData(TEXT("BindWidgetOptional")));
		}
	}

	const UClass* HudClass = UGridPersistentHudWidget::StaticClass();
	const FProperty* PanelProperty = HudClass->FindPropertyByName(TEXT("Panel_PartyPortraits"));
	if (TestNotNull(TEXT("Persistent HUD owns Panel_PartyPortraits"), PanelProperty))
	{
		TestTrue(TEXT("Party rail panel is an optional Designer binding"), PanelProperty->HasMetaData(TEXT("BindWidgetOptional")));
	}

	const FProperty* WidgetClassProperty = HudClass->FindPropertyByName(TEXT("PartyPortraitWidgetClass"));
	if (TestNotNull(TEXT("Persistent HUD owns PartyPortraitWidgetClass"), WidgetClassProperty))
	{
		TestTrue(TEXT("Portrait class is editable on widget defaults"), WidgetClassProperty->HasAnyPropertyFlags(CPF_Edit));
		TestTrue(TEXT("Portrait class is not a per-instance gameplay authority"),
			WidgetClassProperty->HasAnyPropertyFlags(CPF_DisableEditOnInstance));
	}

	TestNotNull(TEXT("Persistent HUD exposes runtime PartyPortraitWidgets"), HudClass->FindPropertyByName(TEXT("PartyPortraitWidgets")));
	TestNotNull(TEXT("Persistent HUD exposes event-driven RefreshPartyPortraits"), HudClass->FindFunctionByName(TEXT("RefreshPartyPortraits")));
	TestNull(TEXT("Persistent portrait presenter does not reuse inventory OwningInventoryWidget semantics"),
		PortraitClass->FindPropertyByName(TEXT("OwningInventoryWidget")));
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridPartyHud013InteractionContractTest, "Grimrock.UI.PartyHud01.PortraitInteractionContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridPartyHud013InteractionContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const UClass* PortraitClass = UGridPartyPortraitHudWidget::StaticClass();
	TestNotNull(TEXT("Portrait exposes stable-id click delegate"),
		PortraitClass->FindPropertyByName(TEXT("OnPortraitClicked")));
	TestNotNull(TEXT("Portrait exposes native selection request"),
		PortraitClass->FindFunctionByName(TEXT("RequestSelection")));
	TestNull(TEXT("Portrait owns no parallel SelectedCharacterIndex"),
		PortraitClass->FindPropertyByName(TEXT("SelectedCharacterIndex")));

	UGridPartyPortraitHudWidget* Widget = NewObject<UGridPartyPortraitHudWidget>();
	FGridPartyPortraitView InvalidView;
	Widget->SetPortraitView(InvalidView);
	TestFalse(TEXT("Invalid CharacterId cannot emit a selection request"), Widget->RequestSelection());

	FGridPartyPortraitView ValidView;
	ValidView.CharacterId = GridPartyHud01Tests::MakeId(50);
	Widget->SetPortraitView(ValidView);
	TestTrue(TEXT("Valid CharacterId can emit a selection request"), Widget->RequestSelection());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridPartyHud013CharacterSelectionTest, "Grimrock.UI.PartyHud01.CharacterSelection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridPartyHud013CharacterSelectionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridPartyHud01Tests;

	UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
	URPGClassAsset* ClassDefinition = MakeClass(Inventory, TEXT("PartyHudSelection"), 0, 0);
	const FGuid A = MakeId(60);
	const FGuid B = MakeId(61);
	Inventory->PartyInventoryState.ActiveCharacters.Add(MakeCharacter(A, ClassDefinition, 10, 10, 0, 0, 0, 0));
	Inventory->PartyInventoryState.ActiveCharacters.Add(MakeCharacter(B, ClassDefinition, 10, 10, 0, 0, 0, 0));
	Inventory->PartyInventoryState.ActiveEquipment.SetNum(2);
	Inventory->PartyInventoryState.SelectedCharacterIndex = 0;
	Inventory->SanitizePartyPortraitOrder();
	const TArray<FGuid> OrderBefore = Inventory->PartyPortraitOrder;

	UGridPersistentHudWidget* Hud = NewObject<UGridPersistentHudWidget>();
	Hud->InventoryComponent = Inventory;

	TestTrue(TEXT("Stable CharacterId selects the second runtime character"), Hud->SelectPartyPortraitCharacter(B));
	TestEqual(TEXT("Existing SelectedCharacterIndex authority changes to runtime index one"),
		Inventory->GetSelectedCharacterIndex(), 1);
	TestTrue(TEXT("Portrait selection never changes presentation order"), Inventory->PartyPortraitOrder == OrderBefore);
	TestEqual(TEXT("Runtime character zero remains A"), Inventory->PartyInventoryState.ActiveCharacters[0].CharacterId, A);
	TestEqual(TEXT("Runtime character one remains B"), Inventory->PartyInventoryState.ActiveCharacters[1].CharacterId, B);

	TestFalse(TEXT("Unknown CharacterId is rejected"), Hud->SelectPartyPortraitCharacter(MakeId(999)));
	TestEqual(TEXT("Rejected selection preserves the selected runtime index"),
		Inventory->GetSelectedCharacterIndex(), 1);
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridPartyHud014DedicatedDragPayloadTest, "Grimrock.UI.PartyHud01.DedicatedPortraitDragPayload",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridPartyHud014DedicatedDragPayloadTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridPartyPortraitHudWidget* Widget = NewObject<UGridPartyPortraitHudWidget>();
	TestNull(TEXT("Portrait without stable identity cannot start a drag"), Widget->CreatePortraitDragDropOperation());

	FGridPartyPortraitView View;
	View.CharacterId = GridPartyHud01Tests::MakeId(70);
	Widget->SetPortraitView(View);

	UGridPartyPortraitDragDropOperation* Operation = Widget->CreatePortraitDragDropOperation();
	if (!TestNotNull(TEXT("Valid portrait creates dedicated drag operation"), Operation))
	{
		return false;
	}

	TestTrue(TEXT("Portrait drag payload carries a valid stable identity"), Operation->HasValidSource());
	TestEqual(TEXT("Portrait drag payload keeps source CharacterId"), Operation->SourceCharacterId, View.CharacterId);
	TestFalse(TEXT("Portrait drag operation is distinct from inventory item drag"),
		Operation->IsA(UGridInventoryDragDropOperation::StaticClass()));
	TestTrue(TEXT("Upper half resolves insertion before"), !UGridPartyPortraitHudWidget::ResolveInsertAfter(24.0f, 100.0f));
	TestTrue(TEXT("Lower half resolves insertion after"), UGridPartyPortraitHudWidget::ResolveInsertAfter(76.0f, 100.0f));
	TestTrue(TEXT("Exact middle belongs to lower/after half"), UGridPartyPortraitHudWidget::ResolveInsertAfter(50.0f, 100.0f));
	TestFalse(TEXT("Zero-height target never fabricates an after insertion"),
		UGridPartyPortraitHudWidget::ResolveInsertAfter(10.0f, 0.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridPartyHud014PortraitDropReorderTest, "Grimrock.UI.PartyHud01.PortraitDropReorder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridPartyHud014PortraitDropReorderTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridPartyHud01Tests;

	UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
	URPGClassAsset* ClassDefinition = MakeClass(Inventory, TEXT("PartyHudDrag"), 0, 0);
	const FGuid A = MakeId(80);
	const FGuid B = MakeId(81);
	const FGuid C = MakeId(82);
	Inventory->PartyInventoryState.ActiveCharacters.Add(MakeCharacter(A, ClassDefinition, 1, 1, 0, 0, 0, 0));
	Inventory->PartyInventoryState.ActiveCharacters.Add(MakeCharacter(B, ClassDefinition, 1, 1, 0, 0, 0, 0));
	Inventory->PartyInventoryState.ActiveCharacters.Add(MakeCharacter(C, ClassDefinition, 1, 1, 0, 0, 0, 0));
	Inventory->PartyInventoryState.ActiveEquipment.SetNum(3);
	Inventory->PartyInventoryState.SelectedCharacterIndex = 1;
	Inventory->SanitizePartyPortraitOrder();

	UGridPersistentHudWidget* Hud = NewObject<UGridPersistentHudWidget>();
	Hud->InventoryComponent = Inventory;

	UGridPartyPortraitDragDropOperation* DragC = NewObject<UGridPartyPortraitDragDropOperation>(Hud);
	DragC->InitializeFromPortrait(C);
	TestTrue(TEXT("Dropping C on upper half of A inserts C before A"),
		Hud->HandlePartyPortraitDrop(DragC, A, false));
	TestEqual(TEXT("Order[0] is C"), Inventory->PartyPortraitOrder[0], C);
	TestEqual(TEXT("Order[1] is A"), Inventory->PartyPortraitOrder[1], A);
	TestEqual(TEXT("Order[2] is B"), Inventory->PartyPortraitOrder[2], B);

	UGridPartyPortraitDragDropOperation* DragCToAfterB = NewObject<UGridPartyPortraitDragDropOperation>(Hud);
	DragCToAfterB->InitializeFromPortrait(C);
	TestTrue(TEXT("Dropping C on lower half of B inserts C after B"),
		Hud->HandlePartyPortraitDrop(DragCToAfterB, B, true));
	TestEqual(TEXT("Order returns A first"), Inventory->PartyPortraitOrder[0], A);
	TestEqual(TEXT("Order keeps B second"), Inventory->PartyPortraitOrder[1], B);
	TestEqual(TEXT("Order moves C after B"), Inventory->PartyPortraitOrder[2], C);

	TestEqual(TEXT("SelectedCharacterIndex remains runtime B"), Inventory->GetSelectedCharacterIndex(), 1);
	TestEqual(TEXT("Runtime roster A index is unchanged"), Inventory->PartyInventoryState.ActiveCharacters[0].CharacterId, A);
	TestEqual(TEXT("Runtime roster B index is unchanged"), Inventory->PartyInventoryState.ActiveCharacters[1].CharacterId, B);
	TestEqual(TEXT("Runtime roster C index is unchanged"), Inventory->PartyInventoryState.ActiveCharacters[2].CharacterId, C);
	TestEqual(TEXT("ActiveEquipment count is unchanged"), Inventory->PartyInventoryState.ActiveEquipment.Num(), 3);

	TestFalse(TEXT("Dropping a portrait onto itself is rejected"),
		Hud->HandlePartyPortraitDrop(DragCToAfterB, C, false));
	TestFalse(TEXT("Null portrait drag operation is rejected"),
		Hud->HandlePartyPortraitDrop(nullptr, A, false));
	return true;
}

#endif
