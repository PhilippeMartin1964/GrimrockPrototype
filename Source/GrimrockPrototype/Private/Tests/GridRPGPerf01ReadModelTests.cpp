#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "RPGMON155TestHelpers.h"
#include "Runtime/GridPartyInventoryComponent.h"

namespace GridRPGPerf01Tests
{
	bool AssignTestWeight(UGridPartyInventoryComponent* Inventory, int32 CharacterIndex, float Weight)
	{
		if (!Inventory || !Inventory->PartyInventoryState.ActiveCharacters.IsValidIndex(CharacterIndex))
		{
			return false;
		}
		FGridCharacterInventoryState& Character = Inventory->PartyInventoryState.ActiveCharacters[CharacterIndex];
		Character.InventorySlots.SetNum(1);
		FGridItemInstance Item;
		Item.RuntimeObjectId = FGuid::NewGuid();
		Item.ItemDefinitionId = TEXT("RPG_PERF01_CarryLoad");
		Item.Quantity = 1;
		Item.Weight = Weight;
		Item.OwnerType = EGridItemOwnerType::CharacterInventory;
		Item.OwnerGuid = Character.CharacterId;
		Item.OwnerCharacterIndex = CharacterIndex;
		Item.EquipmentSlot = EGridEquipmentSlot::None;
		Character.InventorySlots[0].bOccupied = true;
		Character.InventorySlots[0].Item = Item;
		return true;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridRPGPerf01ProgressionReadTest,
	"Grimrock.RPG.PERF01.ProgressionLightweightRead",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridRPGPerf01ProgressionReadTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FMON155RuntimeStateGuard Guard;
	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* Inventory = MakeMON155Inventory(1, 0, ClassDefinition);
	if (!TestNotNull(TEXT("Inventory for lightweight progression reads"), Inventory))
	{
		return false;
	}

	TestFalse(TEXT("Negative character index has no pending progression"),
		Inventory->HasUnspentCharacterProgressionPoints(-1));
	TestFalse(TEXT("Out-of-range character index has no pending progression"),
		Inventory->HasUnspentCharacterProgressionPoints(99));
	TestTrue(TEXT("Invalid character index has no display name"),
		Inventory->GetActiveCharacterDisplayName(99).IsEmpty());

	FGridInventoryCharacterSummary Summary;
	if (!TestTrue(TEXT("Level-one canonical summary exists"), Inventory->GetCharacterSummary(0, Summary)))
	{
		return false;
	}
	TestTrue(TEXT("Level-one unspent Skill Points are visible via lightweight query"),
		Inventory->HasUnspentCharacterProgressionPoints(0));
	TestEqual(TEXT("Lightweight progression flag equals existing summary"),
		Inventory->HasUnspentCharacterProgressionPoints(0), Summary.bHasUnspentProgressionPoints);
	TestTrue(TEXT("Lightweight name equals existing summary fallback/name"),
		Inventory->GetActiveCharacterDisplayName(0).EqualTo(Summary.DisplayName));

	FGridCharacterInventoryState& Character = Inventory->PartyInventoryState.ActiveCharacters[0];
	FRPGSkillRank FirstRank;
	FirstRank.SkillId = TEXT("Skill_RPG_PERF01_A");
	FirstRank.Rank = 2;
	FRPGSkillRank SecondRank;
	SecondRank.SkillId = TEXT("Skill_RPG_PERF01_B");
	SecondRank.Rank = 2;
	Character.SkillRanks = { FirstRank, SecondRank };
	if (TestTrue(TEXT("Summary remains available after spending level-one Skill Points"),
		Inventory->GetCharacterSummary(0, Summary)))
	{
		TestFalse(TEXT("Fully allocated level-one character has no pending progression"),
			Inventory->HasUnspentCharacterProgressionPoints(0));
		TestEqual(TEXT("Spent-point lightweight flag still equals the summary"),
			Inventory->HasUnspentCharacterProgressionPoints(0), Summary.bHasUnspentProgressionPoints);
	}

	Character.Level = 2;
	if (TestTrue(TEXT("Level-two summary remains available after progression"),
		Inventory->GetCharacterSummary(0, Summary)))
	{
		TestTrue(TEXT("Level-up unlocks pending points in lightweight view"),
			Inventory->HasUnspentCharacterProgressionPoints(0));
		TestEqual(TEXT("Level-two point flag equals the canonical summary"),
			Inventory->HasUnspentCharacterProgressionPoints(0), Summary.bHasUnspentProgressionPoints);
	}

	Character.DisplayName = FText::GetEmpty();
	if (TestTrue(TEXT("Unnamed character summary remains available"),
		Inventory->GetCharacterSummary(0, Summary)))
	{
		TestTrue(TEXT("Fallback name is identical to previous summary formatting"),
			Inventory->GetActiveCharacterDisplayName(0).EqualTo(Summary.DisplayName));
		TestEqual(TEXT("First character fallback name remains Hero_01"),
			Inventory->GetActiveCharacterDisplayName(0).ToString(), FString(TEXT("Hero_01")));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridRPGPerf01OverloadReadTest,
	"Grimrock.RPG.PERF01.OverloadSinglePass",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridRPGPerf01OverloadReadTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FMON155RuntimeStateGuard Guard;
	using namespace GridRPGPerf01Tests;
	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* Inventory = MakeMON155Inventory(1, 0, ClassDefinition);
	if (!TestNotNull(TEXT("Inventory for overload read"), Inventory) ||
		!TestNotNull(TEXT("Class definition exists"), ClassDefinition))
	{
		return false;
	}

	FGridCharacterInventoryState Another = MakeMON155Character(ClassDefinition, 1, 0, TEXT("Second"));
	Another.DisplayName = FText::GetEmpty();
	Inventory->PartyInventoryState.ActiveCharacters.Add(Another);
	Inventory->PartyInventoryState.ActiveEquipment.SetNum(2);

	TArray<int32> Overloaded;
	Inventory->GetOverloadedActiveCharacterIndices(Overloaded);
	TestEqual(TEXT("Empty inventory has no overload indices"), Overloaded.Num(), 0);
	TestFalse(TEXT("Empty inventory does not block movement"), Inventory->IsAnyActiveCharacterOverloaded());

	FGridInventoryCharacterSummary FirstSummary;
	FGridInventoryCharacterSummary SecondSummary;
	if (!TestTrue(TEXT("First character summary available"), Inventory->GetCharacterSummary(0, FirstSummary)) ||
		!TestTrue(TEXT("Second character summary available"), Inventory->GetCharacterSummary(1, SecondSummary)))
	{
		return false;
	}
	TestTrue(TEXT("First character has positive carrying capacity"), FirstSummary.MaxWeight > 0.0f);
	TestTrue(TEXT("Second character has positive carrying capacity"), SecondSummary.MaxWeight > 0.0f);
	TestEqual(TEXT("Unnamed second character fallback is Hero_02"),
		Inventory->GetActiveCharacterDisplayName(1).ToString(), FString(TEXT("Hero_02")));
	TestTrue(TEXT("Second fallback matches the full summary"),
		Inventory->GetActiveCharacterDisplayName(1).EqualTo(SecondSummary.DisplayName));

	TestTrue(TEXT("Can assign exactly the first character capacity"),
		AssignTestWeight(Inventory, 0, FirstSummary.MaxWeight));
	Inventory->GetOverloadedActiveCharacterIndices(Overloaded);
	TestEqual(TEXT("Exactly at maximum capacity is not overloaded"), Overloaded.Num(), 0);
	TestFalse(TEXT("Exactly at capacity does not block movement"),
		Inventory->IsAnyActiveCharacterOverloaded());

	TestTrue(TEXT("Can assign one unit beyond the first capacity"),
		AssignTestWeight(Inventory, 0, FirstSummary.MaxWeight + 1.0f));
	Inventory->GetOverloadedActiveCharacterIndices(Overloaded);
	if (TestEqual(TEXT("Only the first character is overloaded"), Overloaded.Num(), 1))
	{
		TestEqual(TEXT("Overloaded index is first party member"), Overloaded[0], 0);
	}
	TestTrue(TEXT("Any-overloaded query agrees with indices"), Inventory->IsAnyActiveCharacterOverloaded());

	TestTrue(TEXT("Can also overload the second character"),
		AssignTestWeight(Inventory, 1, SecondSummary.MaxWeight + 1.0f));
	Inventory->GetOverloadedActiveCharacterIndices(Overloaded);
	if (TestEqual(TEXT("Two overloaded characters detected"), Overloaded.Num(), 2))
	{
		TestEqual(TEXT("First overload keeps active-character order"), Overloaded[0], 0);
		TestEqual(TEXT("Second overload keeps active-character order"), Overloaded[1], 1);
	}
	for (int32 Index = 0; Index < 2; ++Index)
	{
		FGridInventoryCharacterSummary Summary;
		if (TestTrue(TEXT("Full summary available for equivalence"), Inventory->GetCharacterSummary(Index, Summary)))
		{
			TestEqual(TEXT("Lightweight overload index agrees with full summary"),
				Overloaded.Contains(Index), Summary.bOverloaded);
			TestTrue(TEXT("Lightweight display name agrees with full summary"),
				Inventory->GetActiveCharacterDisplayName(Index).EqualTo(Summary.DisplayName));
		}
	}

	TestTrue(TEXT("Can clear first character's weight"),
		AssignTestWeight(Inventory, 0, 0.0f));
	Inventory->GetOverloadedActiveCharacterIndices(Overloaded);
	if (TestEqual(TEXT("Only second overloaded character remains"), Overloaded.Num(), 1))
	{
		TestEqual(TEXT("Second character index is preserved"), Overloaded[0], 1);
	}
	TestTrue(TEXT("Remaining second overload still blocks movement"), Inventory->IsAnyActiveCharacterOverloaded());

	TestTrue(TEXT("Can clear second character's weight"),
		AssignTestWeight(Inventory, 1, 0.0f));
	Inventory->GetOverloadedActiveCharacterIndices(Overloaded);
	TestEqual(TEXT("No overloaded characters after unloading"), Overloaded.Num(), 0);
	TestFalse(TEXT("Movement lock releases after unloading"), Inventory->IsAnyActiveCharacterOverloaded());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
