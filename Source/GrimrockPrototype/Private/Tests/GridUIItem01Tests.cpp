#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Runtime/GridItemContextActionLibrary.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "UI/GridInventorySlotWidget.h"
#include "UI/GridInventoryWidget.h"

namespace
{
	UGridPartyInventoryComponent* MakeInventory()
	{
		UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();

		FGridCharacterInventoryState Character;
		Character.CharacterId = FGuid::NewGuid();
		Character.DisplayName = FText::FromString(TEXT("Tooltip Hero"));
		Character.InventorySlots.SetNum(4);

		Inventory->PartyInventoryState.ActiveCharacters.Add(Character);
		Inventory->PartyInventoryState.ActiveEquipment.SetNum(1);
		Inventory->PartyInventoryState.SelectedCharacterIndex = 0;
		Inventory->PartyInventoryState.MaxActiveCharacters = 6;
		return Inventory;
	}

	UGridItemDefinitionAsset* MakeBeltDefinition(FName Id, const TCHAR* Name)
	{
		UGridItemDefinitionAsset* Definition = NewObject<UGridItemDefinitionAsset>();
		Definition->ItemDefinitionId = Id;
		Definition->DisplayName = FText::FromString(Name);
		Definition->Description = FText::FromString(TEXT("Un objet de test pour le tooltip."));
		Definition->ItemType = EGridItemType::Jewelry;
		Definition->Weight = 1.5f;
		Definition->CompatibleEquipmentSlots.Add(EGridEquipmentSlot::Belt);
		return Definition;
	}

	const FGridItemTooltipStatLine* FindLine(const TArray<FGridItemTooltipStatLine>& Lines, const FString& Label)
	{
		return Lines.FindByPredicate(
			[&Label](const FGridItemTooltipStatLine& Line)
			{
				return Line.Label.ToString() == Label;
			});
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIItem01TooltipProjectionTest, "Grimrock.UI.Item01.TooltipProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIItem01TooltipProjectionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridPartyInventoryComponent* Inventory = MakeInventory();
	UGridItemDefinitionAsset* CandidateDefinition = MakeBeltDefinition(TEXT("Item_TooltipBelt"), TEXT("Ceinture du voyageur"));
	CandidateDefinition->EquipmentStatBonus.StrengthBonus = 3;
	CandidateDefinition->EquipmentStatBonus.CarryWeightBonus = 5.0f;
	CandidateDefinition->EquipmentResistanceBonus.FireResistance = 8;
	TestTrue(TEXT("Candidate definition registers"), Inventory->RegisterItemDefinition(CandidateDefinition));

	UGridInventoryWidget* OwnerWidget = NewObject<UGridInventoryWidget>();
	OwnerWidget->InventoryComponent = Inventory;

	UGridInventorySlotWidget* SlotWidget = NewObject<UGridInventorySlotWidget>();
	SlotWidget->SetOwnerInventoryWidget(OwnerWidget);
	SlotWidget->InitializeInventorySlot(EGridInventoryUiSlotType::Inventory, 0);

	FGridItemInstance Candidate;
	Candidate.RuntimeObjectId = FGuid::NewGuid();
	Candidate.ItemDefinitionId = CandidateDefinition->ItemDefinitionId;
	Candidate.DisplayName = CandidateDefinition->DisplayName;
	Candidate.Quantity = 2;
	Candidate.Weight = CandidateDefinition->Weight;
	Candidate.OwnerType = EGridItemOwnerType::CharacterInventory;
	Candidate.OwnerCharacterIndex = 0;
	SlotWidget->SetItem(Candidate);

	const FGridItemTooltipView View = SlotWidget->GetTooltipView();
	TestTrue(TEXT("Tooltip view is valid"), View.bValid);
	TestEqual(TEXT("Tooltip name comes from item definition"), View.DisplayName.ToString(), FString(TEXT("Ceinture du voyageur")));
	TestEqual(TEXT("Tooltip type covers jewelry"), View.ItemType.ToString(), FString(TEXT("Bijou")));
	TestEqual(TEXT("Tooltip quantity is projected"), View.Quantity, 2);
	TestTrue(TEXT("Tooltip unit weight is projected"), FMath::IsNearlyEqual(View.UnitWeight, 1.5f));
	TestTrue(TEXT("Tooltip total weight multiplies stack quantity"), FMath::IsNearlyEqual(View.TotalWeight, 3.0f));
	TestTrue(TEXT("Tooltip reports equippable state"), View.bEquippable);
	TestTrue(TEXT("Tooltip exposes the Belt compatibility"), View.CompatibleSlotsText.ToString().Contains(TEXT("Ceinture")));
	TestEqual(TEXT("Empty compatible equipment slots do not create comparisons"), View.EquipmentComparisons.Num(), 0);
	TestTrue(TEXT("Fallback tooltip contains the authored description"),
		SlotWidget->GetTooltipText().ToString().Contains(TEXT("Un objet de test pour le tooltip.")));

	const FGridItemTooltipStatLine* Strength = FindLine(View.StatLines, TEXT("Force"));
	TestNotNull(TEXT("Strength stat line exists"), Strength);
	if (Strength)
	{
		TestTrue(TEXT("Strength value is three"), FMath::IsNearlyEqual(Strength->ItemValue, 3.0f));
	}

	UGridItemDefinitionAsset* BookDefinition = NewObject<UGridItemDefinitionAsset>();
	BookDefinition->ItemDefinitionId = TEXT("Item_ReadableBook");
	BookDefinition->DisplayName = FText::FromString(TEXT("Livre lisible"));
	BookDefinition->ItemType = EGridItemType::Book;
	BookDefinition->ReadText = FText::FromString(TEXT("Texte du livre"));
	TestTrue(TEXT("Book definition registers"), Inventory->RegisterItemDefinition(BookDefinition));

	FGridItemInstance Book;
	Book.RuntimeObjectId = FGuid::NewGuid();
	Book.ItemDefinitionId = BookDefinition->ItemDefinitionId;
	Book.Quantity = 1;
	Book.OwnerType = EGridItemOwnerType::CharacterInventory;
	Book.OwnerCharacterIndex = 0;
	TestTrue(TEXT("Shared readable capability recognizes the book"), UGridItemContextActionLibrary::IsItemReadable(Book, BookDefinition));

	SlotWidget->SetItem(Book);
	const FGridItemTooltipView BookView = SlotWidget->GetTooltipView();
	TestTrue(TEXT("Readable state reaches tooltip view"), BookView.bReadable);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIItem01NoEmptyEquipmentComparisonTest, "Grimrock.UI.Item01.NoEmptyEquipmentComparison",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIItem01NoEmptyEquipmentComparisonTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridPartyInventoryComponent* Inventory = MakeInventory();

	UGridItemDefinitionAsset* CandidateDefinition = MakeBeltDefinition(TEXT("Item_EmptyCompareBelt"), TEXT("Ceinture sans référence"));
	CandidateDefinition->EquipmentStatBonus.StrengthBonus = 2;
	TestTrue(TEXT("Candidate definition registers"), Inventory->RegisterItemDefinition(CandidateDefinition));

	UGridInventoryWidget* OwnerWidget = NewObject<UGridInventoryWidget>();
	OwnerWidget->InventoryComponent = Inventory;

	UGridInventorySlotWidget* SlotWidget = NewObject<UGridInventorySlotWidget>();
	SlotWidget->SetOwnerInventoryWidget(OwnerWidget);
	SlotWidget->InitializeInventorySlot(EGridInventoryUiSlotType::Inventory, 0);

	FGridItemInstance Candidate;
	Candidate.RuntimeObjectId = FGuid::NewGuid();
	Candidate.ItemDefinitionId = CandidateDefinition->ItemDefinitionId;
	Candidate.DisplayName = CandidateDefinition->DisplayName;
	Candidate.Quantity = 1;
	Candidate.Weight = CandidateDefinition->Weight;
	Candidate.OwnerType = EGridItemOwnerType::CharacterInventory;
	Candidate.OwnerCharacterIndex = 0;
	SlotWidget->SetItem(Candidate);

	const FGridItemTooltipView View = SlotWidget->GetTooltipView();
	TestTrue(TEXT("Tooltip remains valid"), View.bValid);
	TestTrue(TEXT("Candidate remains equippable"), View.bEquippable);
	TestEqual(TEXT("No comparison is created when the compatible slot is empty"), View.EquipmentComparisons.Num(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIItem01EquipmentComparisonTest, "Grimrock.UI.Item01.EquipmentComparison",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIItem01EquipmentComparisonTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridPartyInventoryComponent* Inventory = MakeInventory();

	UGridItemDefinitionAsset* EquippedDefinition = MakeBeltDefinition(TEXT("Item_OldBelt"), TEXT("Ancienne ceinture"));
	EquippedDefinition->EquipmentStatBonus.StrengthBonus = 1;
	EquippedDefinition->EquipmentStatBonus.ArmorBonus = 2;
	EquippedDefinition->EquipmentResistanceBonus.FireResistance = 5;
	TestTrue(TEXT("Equipped definition registers"), Inventory->RegisterItemDefinition(EquippedDefinition));

	UGridItemDefinitionAsset* CandidateDefinition = MakeBeltDefinition(TEXT("Item_NewBelt"), TEXT("Nouvelle ceinture"));
	CandidateDefinition->EquipmentStatBonus.StrengthBonus = 3;
	CandidateDefinition->EquipmentStatBonus.ArmorBonus = 1;
	CandidateDefinition->EquipmentResistanceBonus.FireResistance = 8;
	TestTrue(TEXT("Candidate definition registers"), Inventory->RegisterItemDefinition(CandidateDefinition));

	FGridItemInstance Equipped;
	Equipped.RuntimeObjectId = FGuid::NewGuid();
	Equipped.ItemDefinitionId = EquippedDefinition->ItemDefinitionId;
	Equipped.DisplayName = EquippedDefinition->DisplayName;
	Equipped.Quantity = 1;
	Equipped.Weight = EquippedDefinition->Weight;
	Equipped.OwnerType = EGridItemOwnerType::EquipmentSlot;
	Equipped.OwnerCharacterIndex = 0;
	Equipped.EquipmentSlot = EGridEquipmentSlot::Belt;
	Inventory->PartyInventoryState.ActiveEquipment[0].Belt = Equipped;

	UGridInventoryWidget* OwnerWidget = NewObject<UGridInventoryWidget>();
	OwnerWidget->InventoryComponent = Inventory;

	UGridInventorySlotWidget* SlotWidget = NewObject<UGridInventorySlotWidget>();
	SlotWidget->SetOwnerInventoryWidget(OwnerWidget);
	SlotWidget->InitializeInventorySlot(EGridInventoryUiSlotType::Inventory, 0);

	FGridItemInstance Candidate;
	Candidate.RuntimeObjectId = FGuid::NewGuid();
	Candidate.ItemDefinitionId = CandidateDefinition->ItemDefinitionId;
	Candidate.DisplayName = CandidateDefinition->DisplayName;
	Candidate.Quantity = 1;
	Candidate.Weight = CandidateDefinition->Weight;
	Candidate.OwnerType = EGridItemOwnerType::CharacterInventory;
	Candidate.OwnerCharacterIndex = 0;
	SlotWidget->SetItem(Candidate);

	const FGridItemTooltipView View = SlotWidget->GetTooltipView();
	TestEqual(TEXT("Exactly one Belt comparison is projected"), View.EquipmentComparisons.Num(), 1);
	if (View.EquipmentComparisons.Num() != 1)
	{
		return false;
	}

	const FGridItemTooltipEquipmentComparison& Comparison = View.EquipmentComparisons[0];
	TestTrue(TEXT("Compared slot is occupied"), Comparison.bHasEquippedItem);
	TestEqual(TEXT("Compared item name is projected"), Comparison.EquippedItemName.ToString(), FString(TEXT("Ancienne ceinture")));

	const FGridItemTooltipStatLine* Strength = FindLine(Comparison.StatLines, TEXT("Force"));
	const FGridItemTooltipStatLine* Armor = FindLine(Comparison.StatLines, TEXT("Armure physique"));
	const FGridItemTooltipStatLine* Fire = FindLine(Comparison.StatLines, TEXT("Résistance feu"));

	TestNotNull(TEXT("Strength comparison exists"), Strength);
	TestNotNull(TEXT("Armor comparison exists"), Armor);
	TestNotNull(TEXT("Fire comparison exists"), Fire);

	if (Strength)
	{
		TestTrue(TEXT("Strength delta is +2"), FMath::IsNearlyEqual(Strength->Delta, 2.0f));
		TestTrue(TEXT("Strength delta is positive"), Strength->DeltaState == EGridItemTooltipDeltaState::Positive);
	}
	if (Armor)
	{
		TestTrue(TEXT("Armor delta is -1"), FMath::IsNearlyEqual(Armor->Delta, -1.0f));
		TestTrue(TEXT("Armor delta is negative"), Armor->DeltaState == EGridItemTooltipDeltaState::Negative);
	}
	if (Fire)
	{
		TestTrue(TEXT("Fire resistance delta is +3"), FMath::IsNearlyEqual(Fire->Delta, 3.0f));
		TestTrue(TEXT("Fire resistance delta is positive"), Fire->DeltaState == EGridItemTooltipDeltaState::Positive);
	}


	return true;
}

#endif
