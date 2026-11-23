#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "GridCanonicalWeightTestWorld.h"

#include "Runtime/GridItemContextActionLibrary.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "UI/GridInventorySlotWidget.h"
#include "UI/GridInventoryWidget.h"

namespace
{
	UGridPartyInventoryComponent* MakeInventory(const FGridCanonicalWeightTestWorld* World = nullptr)
	{
		UGridPartyInventoryComponent* Inventory = World ? World->MakeInventory() : NewObject<UGridPartyInventoryComponent>();
		if (!Inventory) return nullptr;

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

	UGridOtherEquipmentDefinitionAsset* DefineBelt(UGridItemSystemAsset& Root, FName Id, const TCHAR* Name)
	{
		auto* Belt = NewObject<UGridOtherEquipmentDefinitionAsset>(&Root);
		Belt->ItemId = Id;
		Belt->DisplayName = FText::FromString(Name);
		Belt->WeightKg = 1.5f;
		Belt->EquipmentSlots.Add(EGridEquipmentSlot::Belt);
		Root.ItemDefinitions.Add(Belt);
		return Belt;
	}

	const FGridItemTooltipStatLine* FindLine(const TArray<FGridItemTooltipStatLine>& Lines, FName StatId)
	{
		return Lines.FindByPredicate(
			[StatId](const FGridItemTooltipStatLine& Line)
			{
				return Line.StatId == StatId;
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
	CandidateDefinition->LightEmitter.bUsePointLight = true;
	TestTrue(TEXT("Candidate definition registers"), Inventory->RegisterItemDefinition(CandidateDefinition));

	UGridInventoryWidget* OwnerWidget = NewObject<UGridInventoryWidget>();
	OwnerWidget->InventoryComponent = Inventory;

	UGridInventorySlotWidget* SlotWidget = NewObject<UGridInventorySlotWidget>();
	SlotWidget->SetOwnerInventoryWidget(OwnerWidget);
	SlotWidget->InitializeInventorySlot(EGridInventoryUiSlotType::Inventory, 0);

	FGridItemInstance Candidate;
	Candidate.RuntimeObjectId = FGuid::NewGuid();
	Candidate.ItemDefinitionId = CandidateDefinition->ItemDefinitionId;
	Candidate.Quantity = 2;
	Candidate.bLightsEnabled = true;
	Candidate.OwnerType = EGridItemOwnerType::CharacterInventory;
	Candidate.OwnerCharacterIndex = 0;
	SlotWidget->SetItem(Candidate);

	const FGridItemTooltipView View = SlotWidget->GetTooltipView();
	TestTrue(TEXT("Tooltip view is valid"), View.bValid);
	TestEqual(TEXT("Tooltip name comes from item definition"), View.DisplayName.ToString(), FString(TEXT("Ceinture du voyageur")));
	// INDUS04.4: the native read model already exposes authored narrative
	// Description, without mixing it into weapon stat lines or comparison.
	TestEqual(TEXT("Tooltip narrative comes from authorable ItemDefinition.Description"),
		View.Description.ToString(), FString(TEXT("Un objet de test pour le tooltip.")));
	TestEqual(TEXT("Tooltip type covers jewelry"), View.ItemType.ToString(), FString(TEXT("Bijou")));
	TestEqual(TEXT("Tooltip quantity is projected"), View.Quantity, 2);
	TestEqual(TEXT("Tooltip quantity presentation is projected"), View.QuantityText.ToString(), FString(TEXT("(2x)")));
	TestTrue(TEXT("Tooltip unit weight is projected"), FMath::IsNearlyEqual(View.UnitWeight, 1.5f));
	TestTrue(TEXT("Tooltip total weight multiplies stack quantity"), FMath::IsNearlyEqual(View.TotalWeight, 3.0f));
	TestTrue(TEXT("Tooltip reports equippable state"), View.bEquippable);
	TestTrue(TEXT("Tooltip reports light capability"), View.bProvidesLight);
	TestTrue(TEXT("Tooltip reports enabled light state"), View.bLightEnabled);
	TestEqual(TEXT("Tooltip light presentation is projected"), View.LightStateText.ToString(), FString(TEXT("Lumière : allumée")));
	TestTrue(TEXT("Tooltip exposes the Belt compatibility"), View.CompatibleSlotsText.ToString().Contains(TEXT("Ceinture")));
	TestTrue(TEXT("Fallback tooltip contains the authored description"),
		SlotWidget->GetTooltipText().ToString().Contains(TEXT("Un objet de test pour le tooltip.")));

	const FGridItemTooltipStatLine* Strength = FindLine(View.StatLines, TEXT("Strength"));
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIItem01EmptyEquipmentComparisonTest, "Grimrock.UI.Item01.EmptyEquipmentComparison",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIItem01EmptyEquipmentComparisonTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const FName CandidateDefinitionId(TEXT("Item_EmptyCompareBelt"));
	FGridCanonicalWeightTestWorld World({}, [&](UGridItemSystemAsset& Root)
	{
		auto* CandidateDefinition = DefineBelt(Root, CandidateDefinitionId, TEXT("Ceinture sans référence"));
		CandidateDefinition->Bonuses.StrengthBonus = 2;
	});
	UGridPartyInventoryComponent* Inventory = MakeInventory(&World);
	if (!TestNotNull(TEXT("Inventory with registered belts"), Inventory)) return false;

	UGridInventoryWidget* OwnerWidget = NewObject<UGridInventoryWidget>();
	OwnerWidget->InventoryComponent = Inventory;

	UGridInventorySlotWidget* SlotWidget = NewObject<UGridInventorySlotWidget>();
	SlotWidget->SetOwnerInventoryWidget(OwnerWidget);
	SlotWidget->InitializeInventorySlot(EGridInventoryUiSlotType::Inventory, 0);

	FGridItemInstance Candidate;
	Candidate.RuntimeObjectId = FGuid::NewGuid();
	Candidate.ItemDefinitionId = CandidateDefinitionId;
	Candidate.Quantity = 1;
	Candidate.OwnerType = EGridItemOwnerType::CharacterInventory;
	Candidate.OwnerCharacterIndex = 0;
	SlotWidget->SetItem(Candidate);

	const FGridItemTooltipView View = SlotWidget->GetTooltipView();
	TestTrue(TEXT("Tooltip remains valid"), View.bValid);
	TestTrue(TEXT("Candidate remains equippable"), View.bEquippable);
	TestEqual(TEXT("One comparison is created for the empty compatible Belt slot"), View.EquipmentComparisons.Num(), 1);
	if (View.EquipmentComparisons.Num() != 1)
	{
		return false;
	}

	const FGridItemTooltipEquipmentComparison& Comparison = View.EquipmentComparisons[0];
	TestEqual(TEXT("Empty-slot presentation is projected"), Comparison.EquippedItemDisplayText.ToString(), FString(TEXT("vide")));
	TestEqual(TEXT("Empty-slot comparison identifies the Belt slot"), Comparison.EquipmentSlot, EGridEquipmentSlot::Belt);

	const FGridItemTooltipStatLine* Strength = FindLine(Comparison.StatLines, TEXT("Strength"));
	TestNotNull(TEXT("Candidate Strength is compared against zero in an empty slot"), Strength);
	if (Strength)
	{
		TestTrue(TEXT("Empty-slot Strength delta is +2"), FMath::IsNearlyEqual(Strength->Delta, 2.0f));
		TestTrue(TEXT("Empty-slot Strength delta is positive"), Strength->DeltaState == EGridItemTooltipDeltaState::Positive);
		TestEqual(TEXT("Empty-slot Strength delta presentation is projected"), Strength->DeltaDisplayText.ToString(), FString(TEXT("(Δ +2)")));
	}

	TestTrue(TEXT("Fallback tooltip labels the empty equipment reference"), SlotWidget->GetTooltipText().ToString().Contains(TEXT("vide")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIItem01EquipmentComparisonTest, "Grimrock.UI.Item01.EquipmentComparison",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIItem01EquipmentComparisonTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const FName EquippedDefinitionId(TEXT("Item_OldBelt"));
	const FName CandidateDefinitionId(TEXT("Item_NewBelt"));
	FGridCanonicalWeightTestWorld World({}, [&](UGridItemSystemAsset& Root)
	{
		auto* EquippedDefinition = DefineBelt(Root, EquippedDefinitionId, TEXT("Ancienne ceinture"));
		auto* CandidateDefinition = DefineBelt(Root, CandidateDefinitionId, TEXT("Nouvelle ceinture"));
		EquippedDefinition->Bonuses.StrengthBonus = 1;
		EquippedDefinition->Bonuses.ArmorBonus = 2;
		EquippedDefinition->Resistances.FireResistance = 5;
		CandidateDefinition->Bonuses.StrengthBonus = 3;
		CandidateDefinition->Bonuses.ArmorBonus = 1;
		CandidateDefinition->Resistances.FireResistance = 8;
	});
	UGridPartyInventoryComponent* Inventory = MakeInventory(&World);
	if (!TestNotNull(TEXT("Inventory with registered belts"), Inventory)) return false;

	FGridItemInstance Equipped;
	Equipped.RuntimeObjectId = FGuid::NewGuid();
	Equipped.ItemDefinitionId = EquippedDefinitionId;
	Equipped.Quantity = 1;
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
	Candidate.ItemDefinitionId = CandidateDefinitionId;
	Candidate.Quantity = 1;
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
	TestEqual(TEXT("Compared item display text reuses the item name"), Comparison.EquippedItemDisplayText.ToString(), FString(TEXT("Ancienne ceinture")));

	const FGridItemTooltipStatLine* Strength = FindLine(Comparison.StatLines, TEXT("Strength"));
	const FGridItemTooltipStatLine* Armor = FindLine(Comparison.StatLines, TEXT("Armor"));
	const FGridItemTooltipStatLine* Fire = FindLine(Comparison.StatLines, TEXT("FireResistance"));

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIItem01Icon01StatIdentifiersTest, "Grimrock.UI.Item01.Icon01.StatIdentifiers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIItem01Icon01StatIdentifiersTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const FName DefinitionId(TEXT("Item_AllTooltipStats"));
	const FName PlainDefinitionId(TEXT("Item_PlainTooltipBelt"));
	FGridCanonicalWeightTestWorld World({}, [&](UGridItemSystemAsset& Root)
	{
		auto* Definition = DefineBelt(Root, DefinitionId, TEXT("All stats belt"));
		DefineBelt(Root, PlainDefinitionId, TEXT("Plain belt"));
		Definition->Bonuses.StrengthBonus = 1;
		Definition->Bonuses.DexterityBonus = 2;
		Definition->Bonuses.ConstitutionBonus = 3;
		Definition->Bonuses.IntelligenceBonus = 4;
		Definition->Bonuses.WisdomBonus = 5;
		Definition->Bonuses.CharismaBonus = 6;
		Definition->Bonuses.MaxHealthBonus = 7;
		Definition->Bonuses.MaxManaBonus = 8;
		Definition->Bonuses.CarryWeightBonus = 9.5f;
		Definition->Bonuses.ArmorBonus = 10;
		Definition->Resistances.PhysicalResistance = 11;
		Definition->Resistances.FireResistance = 12;
		Definition->Resistances.IceResistance = 13;
		Definition->Resistances.LightningResistance = 14;
		Definition->Resistances.PoisonResistance = 15;
		Definition->Resistances.HolyResistance = 16;
		Definition->Resistances.NecroticResistance = 17;
		Definition->Resistances.ArcaneResistance = 18;
	});
	UGridPartyInventoryComponent* Inventory = MakeInventory(&World);
	if (!TestNotNull(TEXT("Inventory with registered belts"), Inventory)) return false;

	UGridInventoryWidget* OwnerWidget = NewObject<UGridInventoryWidget>();
	OwnerWidget->InventoryComponent = Inventory;
	UGridInventorySlotWidget* SlotWidget = NewObject<UGridInventorySlotWidget>();
	SlotWidget->SetOwnerInventoryWidget(OwnerWidget);
	SlotWidget->InitializeInventorySlot(EGridInventoryUiSlotType::Inventory, 0);

	FGridItemInstance Item;
	Item.RuntimeObjectId = FGuid::NewGuid();
	Item.ItemDefinitionId = DefinitionId;
	Item.Quantity = 1;
	Item.OwnerType = EGridItemOwnerType::CharacterInventory;
	Item.OwnerCharacterIndex = 0;
	SlotWidget->SetItem(Item);

	const FName ExpectedIds[] = {
		TEXT("Strength"), TEXT("Dexterity"), TEXT("Constitution"), TEXT("Intelligence"), TEXT("Wisdom"), TEXT("Charisma"),
		TEXT("MaxHealth"), TEXT("MaxMana"), TEXT("CarryWeight"), TEXT("Armor"), TEXT("PhysicalResistance"), TEXT("FireResistance"),
		TEXT("IceResistance"), TEXT("LightningResistance"), TEXT("PoisonResistance"), TEXT("HolyResistance"), TEXT("NecroticResistance"),
		TEXT("ArcaneResistance")
	};

	const FGridItemTooltipView View = SlotWidget->GetTooltipView();
	TestEqual(TEXT("All 18 item stats are projected"), View.StatLines.Num(), 18);
	TestEqual(TEXT("Empty compatible slot has one comparison"), View.EquipmentComparisons.Num(), 1);
	if (View.EquipmentComparisons.Num() != 1)
	{
		return false;
	}
	TestEqual(TEXT("All 18 stats also appear in the empty-slot comparison"), View.EquipmentComparisons[0].StatLines.Num(), 18);
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(ExpectedIds); ++Index)
	{
		const FName StatId = ExpectedIds[Index];
		const float ExpectedValue = StatId == FName(TEXT("CarryWeight")) ? 9.5f : static_cast<float>(Index + 1);
		const FGridItemTooltipStatLine* Stat = FindLine(View.StatLines, StatId);
		const FGridItemTooltipStatLine* ComparedStat = FindLine(View.EquipmentComparisons[0].StatLines, StatId);
		if (TestNotNull(*FString::Printf(TEXT("Item stat %s is addressable by stable ID"), *StatId.ToString()), Stat))
		{
			TestEqual(TEXT("ID selects the correct item value"), Stat->ItemValue, ExpectedValue);
		}
		if (TestNotNull(*FString::Printf(TEXT("Empty-slot comparison retains %s"), *StatId.ToString()), ComparedStat))
		{
			TestEqual(TEXT("Empty-slot ID selects the correct delta"), ComparedStat->Delta, ExpectedValue);
		}
	}

	// An unmodified candidate still needs icons for stats lost from the equipped item.
	Item.OwnerType = EGridItemOwnerType::EquipmentSlot;
	Item.EquipmentSlot = EGridEquipmentSlot::Belt;
	Inventory->PartyInventoryState.ActiveEquipment[0].Belt = Item;
	FGridItemInstance PlainItem = Item;
	PlainItem.RuntimeObjectId = FGuid::NewGuid();
	PlainItem.ItemDefinitionId = PlainDefinitionId;
	PlainItem.OwnerType = EGridItemOwnerType::CharacterInventory;
	PlainItem.EquipmentSlot = EGridEquipmentSlot::None;
	SlotWidget->SetItem(PlainItem);

	const FGridItemTooltipView PlainView = SlotWidget->GetTooltipView();
	TestEqual(TEXT("Zero candidate stats remain omitted"), PlainView.StatLines.Num(), 0);
	TestEqual(TEXT("Occupied compatible slot has one comparison"), PlainView.EquipmentComparisons.Num(), 1);
	if (PlainView.EquipmentComparisons.Num() != 1)
	{
		return false;
	}
	TestFalse(TEXT("Equipped reference is displayed rather than an empty slot"),
		PlainView.EquipmentComparisons[0].EquippedItemDisplayText.IsEmpty());
	TestTrue(TEXT("Equipped reference is not the empty slot label"),
		PlainView.EquipmentComparisons[0].EquippedItemDisplayText.ToString() != TEXT("vide"));
	TestEqual(TEXT("All equipped-only stats retain a comparison row"), PlainView.EquipmentComparisons[0].StatLines.Num(), 18);
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(ExpectedIds); ++Index)
	{
		const FName StatId = ExpectedIds[Index];
		const float ExpectedValue = StatId == FName(TEXT("CarryWeight")) ? 9.5f : static_cast<float>(Index + 1);
		const FGridItemTooltipStatLine* Stat = FindLine(PlainView.EquipmentComparisons[0].StatLines, StatId);
		if (TestNotNull(*FString::Printf(TEXT("Equipped-only stat retains %s for icon lookup"), *StatId.ToString()), Stat))
		{
			TestEqual(TEXT("Equipped-only ID selects the correct loss"), Stat->Delta, -ExpectedValue);
			TestTrue(TEXT("Equipped-only delta remains negative"), Stat->DeltaState == EGridItemTooltipDeltaState::Negative);
		}
	}
	return true;
}

#endif
