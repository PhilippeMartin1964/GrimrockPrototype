#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"

namespace GridTD07334Characterization
{
	UGridPartyInventoryComponent* MakeInventory()
	{
		UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
		Inventory->InitializeDefaultPartyIfNeeded();
		return Inventory;
	}

	FGridItemInstance MakeItem(const TCHAR* DefinitionId, float Weight)
	{
		FGridItemInstance Item;
		Item.RuntimeObjectId = FGuid::NewGuid();
		Item.ItemDefinitionId = FName(DefinitionId);
		Item.DisplayName = FText::FromString(DefinitionId);
		Item.Quantity = 1;
		Item.Weight = Weight;
		Item.OwnerType = EGridItemOwnerType::World;
		return Item;
	}

	UGridItemDefinitionAsset* MakeCarryBeltDefinition(UObject* Outer)
	{
		UGridItemDefinitionAsset* Definition = NewObject<UGridItemDefinitionAsset>(Outer);
		Definition->ItemDefinitionId = TEXT("TD07334_CarryBelt");
		Definition->DisplayName = FText::FromString(TEXT("TD07.3.3.4 Carry Belt"));
		Definition->ItemType = EGridItemType::Jewelry;
		Definition->Weight = 0.0f;
		Definition->CompatibleEquipmentSlots.Add(EGridEquipmentSlot::Belt);
		Definition->EquipmentStatBonus.StrengthBonus = 4;
		Definition->EquipmentStatBonus.CarryWeightBonus = 7.0f;
		return Definition;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07334CachedCapacityContractTest, "Grimrock.TechnicalDebt.TD07_3_3_4.Characterization.CachedCapacityContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07334CachedCapacityContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD07334Characterization;

	UGridPartyInventoryComponent* Inventory = MakeInventory();
	FGridCharacterInventoryState& Character = Inventory->PartyInventoryState.ActiveCharacters[0];

	Character.Attributes.Strength = 10;
	FGridInventoryCharacterSummary Summary;
	TestTrue(TEXT("Summary resolves at Strength 10"), Inventory->GetCharacterSummary(0, Summary));
	TestTrue(TEXT("Strength 10 projects capacity 50"), FMath::IsNearlyEqual(Summary.BaseMaxWeight, 50.0f));

	Character.Attributes.Strength = 12;
	TestTrue(TEXT("Summary resolves immediately after Strength change"), Inventory->GetCharacterSummary(0, Summary));
	TestTrue(TEXT("Capacity is now live and immediately reflects Strength 12"), FMath::IsNearlyEqual(Summary.BaseMaxWeight, 60.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07334EquipmentCapacityProjectionContractTest,
	"Grimrock.TechnicalDebt.TD07_3_3_4.Characterization.EquipmentCapacityProjectionContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07334EquipmentCapacityProjectionContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD07334Characterization;

	UGridPartyInventoryComponent* Inventory = MakeInventory();
	FGridCharacterInventoryState& Character = Inventory->PartyInventoryState.ActiveCharacters[0];
	Character.Attributes.Strength = 10;

	UGridItemDefinitionAsset* BeltDefinition = MakeCarryBeltDefinition(Inventory);
	TestTrue(TEXT("Carry belt definition registers"), Inventory->RegisterItemDefinition(BeltDefinition));
	TestTrue(TEXT("Heavy item can be stored even above base capacity"), Inventory->AddItemToCharacterInventory(0, MakeItem(TEXT("TD07334_HeavyPack"), 54.0f)));

	FGridItemInstance Belt;
	Belt.RuntimeObjectId = FGuid::NewGuid();
	Belt.ItemDefinitionId = BeltDefinition->ItemDefinitionId;
	Belt.DisplayName = BeltDefinition->DisplayName;
	Belt.Quantity = 1;
	Belt.Weight = BeltDefinition->Weight;
	Belt.OwnerType = EGridItemOwnerType::EquipmentSlot;
	Belt.OwnerGuid = Character.CharacterId;
	Belt.OwnerCharacterIndex = 0;
	Belt.EquipmentSlot = EGridEquipmentSlot::Belt;
	Inventory->PartyInventoryState.ActiveEquipment[0].Belt = Belt;

	FGridInventoryCharacterSummary Summary;
	TestTrue(TEXT("Summary resolves with carry belt equipped"), Inventory->GetCharacterSummary(0, Summary));
	TestEqual(TEXT("Equipment StrengthBonus changes projected Strength"), Summary.Attributes.Strength, 14);
	TestTrue(TEXT("Base capacity still uses base Strength only"), FMath::IsNearlyEqual(Summary.BaseMaxWeight, 50.0f));
	TestTrue(TEXT("CarryWeightBonus changes projected max capacity"), FMath::IsNearlyEqual(Summary.MaxWeight, 57.0f));
	TestTrue(TEXT("Projected current weight includes inventory and equipment"), FMath::IsNearlyEqual(Summary.CurrentWeight, 54.0f));
	TestFalse(TEXT("Unified overload projection uses CarryWeightBonus"), Summary.bOverloaded);
	TestTrue(TEXT("Equipment StrengthBonus still does not add Strength-derived carry capacity"), !FMath::IsNearlyEqual(Summary.MaxWeight, 77.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07334CursorWeightBoundaryTest, "Grimrock.TechnicalDebt.TD07_3_3_4.Characterization.CursorWeightBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07334CursorWeightBoundaryTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD07334Characterization;

	UGridPartyInventoryComponent* Inventory = MakeInventory();
	TestTrue(TEXT("Five-weight item enters inventory"), Inventory->AddItemToCharacterInventory(0, MakeItem(TEXT("TD07334_CursorItem"), 5.0f)));

	FGridInventoryCharacterSummary Summary;
	TestTrue(TEXT("Summary resolves before cursor transfer"), Inventory->GetCharacterSummary(0, Summary));
	TestTrue(TEXT("Projected weight includes the inventory item"), FMath::IsNearlyEqual(Summary.CurrentWeight, 5.0f));

	TestTrue(TEXT("Item can move from inventory to global cursor"), Inventory->TryTakeInventorySlotToCursor(0, 0));
	TestTrue(TEXT("Cursor now owns the item"), Inventory->HasCursorItem());
	TestTrue(TEXT("Summary resolves after cursor transfer"), Inventory->GetCharacterSummary(0, Summary));
	TestTrue(TEXT("Projected character weight excludes an item held by the global cursor"), FMath::IsNearlyEqual(Summary.CurrentWeight, 0.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07334RestoreRecalculatesCachesTest, "Grimrock.TechnicalDebt.TD07_3_3_4.Characterization.RestoreRecalculatesCaches",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07334RestoreRecalculatesCachesTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD07334Characterization;

	UGridPartyInventoryComponent* Source = MakeInventory();
	Source->PartyInventoryState.bInitialCharacterCreationCompleted = true;
	FGridCharacterInventoryState& SourceCharacter = Source->PartyInventoryState.ActiveCharacters[0];
	SourceCharacter.Attributes.Strength = 10;
	TestTrue(TEXT("Three-weight item enters source inventory"), Source->AddItemToCharacterInventory(0, MakeItem(TEXT("TD07334_SaveItem"), 3.0f)));

	const FGridPartyInventoryState SavedState = Source->PartyInventoryState;
	UGridPartyInventoryComponent* Restored = NewObject<UGridPartyInventoryComponent>();
	FText Error;
	TestTrue(TEXT("Current-schema party state restores"), Restored->RestorePartyInventoryState(SavedState, Error));
	TestTrue(TEXT("Restore reports no error"), Error.IsEmpty());

	FGridInventoryCharacterSummary Summary;
	TestTrue(TEXT("Restored summary resolves"), Restored->GetCharacterSummary(0, Summary));
	TestTrue(TEXT("Restored weight is reconstructed from contents"), FMath::IsNearlyEqual(Summary.CurrentWeight, 3.0f));
	TestTrue(TEXT("Restored base capacity is reconstructed from Strength"), FMath::IsNearlyEqual(Summary.BaseMaxWeight, 50.0f));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
