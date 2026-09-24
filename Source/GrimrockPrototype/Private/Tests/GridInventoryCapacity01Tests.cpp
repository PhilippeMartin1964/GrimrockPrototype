#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Runtime/GridPartyInventoryComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridInventoryCapacity01UniformPartyTest, "Grimrock.Inventory.Capacity01.UniformParty",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridInventoryCapacity01UniformPartyTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
	if (!TestNotNull(TEXT("Inventory component exists"), Inventory))
	{
		return false;
	}

	Inventory->DefaultInventorySlotCountPerCharacter = 18;
	Inventory->InventoryColumnCount = 6;
	Inventory->InitializeDefaultPartyIfNeeded();

	TestEqual(TEXT("Configured party-wide slot count is exposed"), Inventory->GetInventorySlotCountPerCharacter(), 18);
	TestEqual(TEXT("Configured party-wide column count is exposed"), Inventory->GetInventoryColumnCount(), 6);
	TestEqual(TEXT("Default character receives configured slot count"),
		Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots.Num(), 18);

	FGridCharacterInventoryState ReserveCharacter;
	ReserveCharacter.CharacterId = FGuid::NewGuid();
	ReserveCharacter.DisplayName = FText::FromString(TEXT("Reserve"));
	ReserveCharacter.InventorySlots.SetNum(18);
	Inventory->PartyInventoryState.CharacterPool.Add(ReserveCharacter);

	FString CapacityError;
	TestTrue(TEXT("Active and reserve characters with the same capacity validate"),
		Inventory->ValidateInventorySlotCountConsistency(CapacityError));

	Inventory->PartyInventoryState.CharacterPool[0].InventorySlots.SetNum(12);
	TestFalse(TEXT("A different per-character slot count is rejected"),
		Inventory->ValidateInventorySlotCountConsistency(CapacityError));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridInventoryCapacity01WeightIndependenceTest, "Grimrock.Inventory.Capacity01.WeightIndependence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridInventoryCapacity01WeightIndependenceTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
	if (!TestNotNull(TEXT("Inventory component exists"), Inventory))
	{
		return false;
	}

	Inventory->DefaultInventorySlotCountPerCharacter = 24;
	Inventory->InitializeDefaultPartyIfNeeded();
	FGridCharacterInventoryState& Character = Inventory->PartyInventoryState.ActiveCharacters[0];

	Character.Attributes.Strength = 1;
	FGridInventoryCharacterSummary WeakSummary;
	TestTrue(TEXT("Weak character summary resolves"), Inventory->GetCharacterSummary(0, WeakSummary));

	Character.Attributes.Strength = 20;
	FGridInventoryCharacterSummary StrongSummary;
	TestTrue(TEXT("Strong character summary resolves"), Inventory->GetCharacterSummary(0, StrongSummary));

	TestEqual(TEXT("Weak character keeps configured slot count"), WeakSummary.MaxInventorySlots, 24);
	TestEqual(TEXT("Strong character keeps configured slot count"), StrongSummary.MaxInventorySlots, 24);
	TestEqual(TEXT("Changing carrying strength does not resize inventory storage"), Character.InventorySlots.Num(), 24);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
