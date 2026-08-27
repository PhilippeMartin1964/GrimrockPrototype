#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Save/GrimrockPartySaveGame.h"
#include "UObject/UnrealType.h"

namespace GridTD07334Normalization
{
	UGridPartyInventoryComponent* MakeInventory()
	{
		UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
		Inventory->InitializeDefaultPartyIfNeeded();
		return Inventory;
	}

	FGridItemInstance MakeItem(const TCHAR* DefinitionId, float Weight, int32 Quantity = 1)
	{
		FGridItemInstance Item;
		Item.RuntimeObjectId = FGuid::NewGuid();
		Item.ItemDefinitionId = FName(DefinitionId);
		Item.DisplayName = FText::FromString(DefinitionId);
		Item.Quantity = Quantity;
		Item.Weight = Weight;
		Item.OwnerType = EGridItemOwnerType::World;
		return Item;
	}

	UGridItemDefinitionAsset* MakeCarryBeltDefinition(UObject* Outer)
	{
		UGridItemDefinitionAsset* Definition = NewObject<UGridItemDefinitionAsset>(Outer);
		Definition->ItemDefinitionId = TEXT("TD07334_NormalizedCarryBelt");
		Definition->DisplayName = FText::FromString(TEXT("TD07.3.3.4 Normalized Carry Belt"));
		Definition->ItemType = EGridItemType::Jewelry;
		Definition->Weight = 2.0f;
		Definition->CompatibleEquipmentSlots.Add(EGridEquipmentSlot::Belt);
		Definition->EquipmentStatBonus.StrengthBonus = 4;
		Definition->EquipmentStatBonus.CarryWeightBonus = 7.0f;
		return Definition;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07334SchemaSeparationTest,
	"Grimrock.TechnicalDebt.TD07_3_3_4.Normalization.SchemaSeparation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07334SchemaSeparationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UScriptStruct* CharacterStruct = FGridCharacterInventoryState::StaticStruct();
	TestNotNull(TEXT("Character state reflected"), CharacterStruct);
	TestNull(TEXT("CurrentWeight cache removed from durable character state"), FindFProperty<FProperty>(CharacterStruct, TEXT("CurrentWeight")));
	TestNull(TEXT("MaxCarryWeight cache removed from durable character state"), FindFProperty<FProperty>(CharacterStruct, TEXT("MaxCarryWeight")));

	UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
	TestNull(TEXT("Legacy RecalculateCharacterWeight Blueprint API removed"), Inventory->FindFunction(FName(TEXT("RecalculateCharacterWeight"))));
	TestNull(TEXT("Legacy RecalculateAllWeights Blueprint API removed"), Inventory->FindFunction(FName(TEXT("RecalculateAllWeights"))));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07334LiveProjectionTest,
	"Grimrock.TechnicalDebt.TD07_3_3_4.Normalization.LiveProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07334LiveProjectionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD07334Normalization;

	UGridPartyInventoryComponent* Inventory = MakeInventory();
	FGridCharacterInventoryState& Character = Inventory->PartyInventoryState.ActiveCharacters[0];
	Character.Attributes.Strength = 10;

	TestTrue(TEXT("Two-item stack enters inventory"),
		Inventory->AddItemToCharacterInventory(0, MakeItem(TEXT("TD07334_NormalizedPack"), 3.0f, 2)));

	FGridInventoryCharacterSummary Summary;
	TestTrue(TEXT("Summary resolves"), Inventory->GetCharacterSummary(0, Summary));
	TestTrue(TEXT("CurrentWeight is calculated from quantity and item weight"), FMath::IsNearlyEqual(Summary.CurrentWeight, 6.0f));
	TestTrue(TEXT("Base capacity is calculated from Strength"), FMath::IsNearlyEqual(Summary.BaseMaxWeight, 50.0f));
	TestTrue(TEXT("Final capacity equals base capacity without equipment bonus"), FMath::IsNearlyEqual(Summary.MaxWeight, 50.0f));
	TestFalse(TEXT("Six weight is not overloaded at capacity fifty"), Summary.bOverloaded);

	Character.Attributes.Strength = 12;
	TestTrue(TEXT("Summary resolves after live Strength mutation"), Inventory->GetCharacterSummary(0, Summary));
	TestTrue(TEXT("Capacity changes immediately without recalculation API"), FMath::IsNearlyEqual(Summary.BaseMaxWeight, 60.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07334EquipmentPolicyTest,
	"Grimrock.TechnicalDebt.TD07_3_3_4.Normalization.EquipmentPolicy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07334EquipmentPolicyTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD07334Normalization;

	UGridPartyInventoryComponent* Inventory = MakeInventory();
	FGridCharacterInventoryState& Character = Inventory->PartyInventoryState.ActiveCharacters[0];
	Character.Attributes.Strength = 10;

	UGridItemDefinitionAsset* BeltDefinition = MakeCarryBeltDefinition(Inventory);
	TestTrue(TEXT("Carry belt registers"), Inventory->RegisterItemDefinition(BeltDefinition));
	TestTrue(TEXT("Fifty-four weight enters inventory"),
		Inventory->AddItemToCharacterInventory(0, MakeItem(TEXT("TD07334_NormalizedHeavyPack"), 54.0f)));

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
	TestTrue(TEXT("Summary resolves with carry belt"), Inventory->GetCharacterSummary(0, Summary));
	TestEqual(TEXT("StrengthBonus remains visible in projected attributes"), Summary.Attributes.Strength, 14);
	TestTrue(TEXT("Base capacity remains based on base Strength"), FMath::IsNearlyEqual(Summary.BaseMaxWeight, 50.0f));
	TestTrue(TEXT("CarryWeightBonus remains the only equipment capacity bonus"), FMath::IsNearlyEqual(Summary.MaxWeight, 57.0f));
	TestTrue(TEXT("CurrentWeight includes equipped belt mass"), FMath::IsNearlyEqual(Summary.CurrentWeight, 56.0f));
	TestFalse(TEXT("Unified overload projection uses final capacity"), Summary.bOverloaded);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07334SaveSchemaVersionTest,
	"Grimrock.TechnicalDebt.TD07_3_3_4.Normalization.SaveSchemaVersion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07334SaveSchemaVersionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	TestEqual(TEXT("TD07.3.3.4 weight-cache removal opens SaveGame v13"), UGrimrockPartySaveGame::CurrentSaveVersion, 13);

	UGrimrockPartySaveGame* Current = NewObject<UGrimrockPartySaveGame>();
	TestEqual(TEXT("New SaveGame starts on v13"), Current->SaveVersion, 13);
	TestTrue(TEXT("Current v13 is compatible"), Current->IsCompatible());

	UGrimrockPartySaveGame* Previous = NewObject<UGrimrockPartySaveGame>();
	Previous->SaveVersion = 12;
	FText Error;
	TestFalse(TEXT("Previous v12 is rejected without migration"), Previous->ValidateCurrentState(Error));
	TestFalse(TEXT("Previous v12 is incompatible"), Previous->IsCompatible());
	TestEqual(TEXT("Validation does not rewrite v12"), Previous->SaveVersion, 12);
	TestTrue(TEXT("Rejected v12 reports an error"), !Error.IsEmpty());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
