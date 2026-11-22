#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "GridCanonicalWeightTestWorld.h"

#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"

namespace GridTD07334Characterization
{
	UGridPartyInventoryComponent* MakeInventory(FGridCanonicalWeightTestWorld& Fixture)
	{
		UGridPartyInventoryComponent* Inventory = Fixture.MakeInventory();
		if (!Inventory) return nullptr;
		Inventory->InitializeDefaultPartyIfNeeded();
		return Inventory;
	}

	FGridItemInstance MakeItem(const TCHAR* DefinitionId)
	{
		FGridItemInstance Item;
		Item.RuntimeObjectId = FGuid::NewGuid();
		Item.ItemDefinitionId = FName(DefinitionId);
		Item.Quantity = 1;
		Item.OwnerType = EGridItemOwnerType::World;
		return Item;
	}

	void AddCarryBelt(UGridItemSystemAsset& Root)
	{
		auto* Definition = NewObject<UGridOtherEquipmentDefinitionAsset>(&Root);
		Definition->ItemId = TEXT("TD07334_CarryBelt");
		Definition->DisplayName = FText::FromString(TEXT("TD07.3.3.4 Carry Belt"));
		Definition->WeightKg = 0.0f;
		Definition->EquipmentSlots.Add(EGridEquipmentSlot::Belt);
		Definition->Bonuses.StrengthBonus = 4;
		Definition->Bonuses.CarryWeightBonus = 7.0f;
		Root.ItemDefinitions.Add(Definition);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07334CachedCapacityContractTest, "Grimrock.TechnicalDebt.TD07_3_3_4.Characterization.CachedCapacityContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07334CachedCapacityContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD07334Characterization;
	FGridCanonicalWeightTestWorld Fixture(TMap<FName, float>{});
	UGridPartyInventoryComponent* Inventory = MakeInventory(Fixture);
	if (!TestNotNull(TEXT("Canonical capacity fixture"), Inventory)) return false;
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

	FGridCanonicalWeightTestWorld Fixture({{TEXT("TD07334_HeavyPack"), 54.0f}}, AddCarryBelt);
	UGridPartyInventoryComponent* Inventory = MakeInventory(Fixture);
	if (!TestNotNull(TEXT("Canonical equipment capacity fixture"), Inventory)) return false;
	FGridCharacterInventoryState& Character = Inventory->PartyInventoryState.ActiveCharacters[0];
	Character.Attributes.Strength = 10;
	TestTrue(TEXT("Carry belt definition registers"), Fixture.Scope.Registry->ContainsItem(TEXT("TD07334_CarryBelt")));
	TestTrue(TEXT("Heavy item can be stored even above base capacity"), Inventory->AddItemToCharacterInventory(0, MakeItem(TEXT("TD07334_HeavyPack"))));

	FGridItemInstance Belt;
	Belt.RuntimeObjectId = FGuid::NewGuid();
	Belt.ItemDefinitionId = TEXT("TD07334_CarryBelt");
	Belt.Quantity = 1;
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

	FGridCanonicalWeightTestWorld Fixture({{TEXT("TD07334_CursorItem"), 5.f}});
	UGridPartyInventoryComponent* Inventory = MakeInventory(Fixture);
	if (!TestNotNull(TEXT("Canonical cursor fixture"), Inventory)) return false;
	TestTrue(TEXT("Five-weight item enters inventory"), Inventory->AddItemToCharacterInventory(0, MakeItem(TEXT("TD07334_CursorItem"))));

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

	FGridCanonicalWeightTestWorld Fixture({{TEXT("TD07334_SaveItem"), 3.f}});
	UGridPartyInventoryComponent* Source = MakeInventory(Fixture);
	if (!TestNotNull(TEXT("Canonical restore source"), Source)) return false;
	Source->PartyInventoryState.bInitialCharacterCreationCompleted = true;
	FGridCharacterInventoryState& SourceCharacter = Source->PartyInventoryState.ActiveCharacters[0];
	SourceCharacter.Attributes.Strength = 10;
	TestTrue(TEXT("Three-weight item enters source inventory"), Source->AddItemToCharacterInventory(0, MakeItem(TEXT("TD07334_SaveItem"))));

	const FGridPartyInventoryState SavedState = Source->PartyInventoryState;
	UGridPartyInventoryComponent* Restored = Fixture.MakeInventory();
	if (!TestNotNull(TEXT("Canonical restore destination"), Restored)) return false;
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
