#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "RPG/RPGPartyRecruitmentService.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Save/GrimrockPartySaveGame.h"

namespace GridTD07332Tests
{
	FGridCharacterInventoryState MakeCurrentCharacter(const FGuid& CharacterId, const TCHAR* DisplayName, int32 Strength = 10)
	{
		FGridCharacterInventoryState Character;
		Character.CharacterId = CharacterId;
		Character.DisplayName = FText::FromString(DisplayName);
		Character.ClassId = TEXT("Warrior");
		Character.ClassDisplayName = FText::FromString(TEXT("Guerrier"));
		Character.RaceId = TEXT("Human");
		Character.RaceDisplayName = FText::FromString(TEXT("Humain"));
		Character.Level = 1;
		Character.Experience = 0;
		Character.Attributes = FRPGAttributes(Strength, 10, 10, 10, 10, 10);
		Character.DerivedStats.MaxHealth = 10;
		Character.DerivedStats.CurrentHealth = 10;
		Character.InventorySlots.SetNum(4);
		Character.CombatHotbarSlots.SetNum(FGridCombatHotbarBinding::SlotCount);
		for (int32 SlotIndex = 0; SlotIndex < Character.CombatHotbarSlots.Num(); ++SlotIndex)
		{
			Character.CombatHotbarSlots[SlotIndex].Reset(SlotIndex);
		}
		return Character;
	}

	UGridPartyInventoryComponent* MakeCurrentParty(int32 MaxActiveCharacters = 2)
	{
		UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
		Inventory->PartyInventoryState = FGridPartyInventoryState();
		Inventory->PartyInventoryState.MaxActiveCharacters = MaxActiveCharacters;
		Inventory->PartyInventoryState.bInitialCharacterCreationCompleted = true;
		Inventory->PartyInventoryState.SelectedCharacterIndex = 0;
		Inventory->PartyInventoryState.ActiveCharacters.Add(MakeCurrentCharacter(FGuid(7, 3, 3, 21), TEXT("MainHero")));
		Inventory->PartyInventoryState.ActiveEquipment.SetNum(1);
		return Inventory;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07332CurrentAttributeAuthorityTest,
	"Grimrock.TechnicalDebt.TD07_3_3_2.CurrentAttributeAuthority",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07332CurrentAttributeAuthorityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD07332Tests;

	UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
	Inventory->PartyInventoryState = FGridPartyInventoryState();
	Inventory->PartyInventoryState.ActiveCharacters.Add(MakeCurrentCharacter(FGuid(7, 3, 3, 22), TEXT("AttributeHero"), 13));

	Inventory->InitializeDefaultPartyIfNeeded();

	const FGridCharacterInventoryState& Character = Inventory->PartyInventoryState.ActiveCharacters[0];
	TestEqual(TEXT("Attributes.Strength remains the current authority"), Character.Attributes.Strength, 13);
	TestTrue(TEXT("Carry capacity is calculated directly from current Attributes"), FMath::IsNearlyEqual(Character.MaxCarryWeight, 65.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07332RecruitmentUsesCurrentAttributesTest,
	"Grimrock.TechnicalDebt.TD07_3_3_2.RecruitmentUsesCurrentAttributes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07332RecruitmentUsesCurrentAttributesTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD07332Tests;

	UGridPartyInventoryComponent* Inventory = MakeCurrentParty();
	const FGuid RecruitId(7, 3, 3, 23);
	Inventory->PartyInventoryState.CharacterPool.Add(MakeCurrentCharacter(RecruitId, TEXT("CurrentRecruit"), 14));

	FRPGPartyRecruitmentResult Result;
	TestTrue(TEXT("Current-schema candidate recruits without a legacy initialization marker"),
		FRPGPartyRecruitmentService::TryRecruitFromPool(Inventory, RecruitId, Result));
	TestTrue(TEXT("Recruitment commits"), Result.bCommitted);
	TestEqual(TEXT("Current Attributes survive recruitment"), Inventory->PartyInventoryState.ActiveCharacters[1].Attributes.Strength, 14);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07332SaveSchemaVersionTest,
	"Grimrock.TechnicalDebt.TD07_3_3_2.SaveSchemaVersion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07332SaveSchemaVersionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	TestEqual(TEXT("Removing the legacy character attribute bridge opens SaveGame v11"),
		UGrimrockPartySaveGame::CurrentSaveVersion, 11);

	UGrimrockPartySaveGame* Current = NewObject<UGrimrockPartySaveGame>();
	TestEqual(TEXT("New saves start on v11"), Current->SaveVersion, 11);
	TestTrue(TEXT("Current v11 is compatible"), Current->IsCompatible());

	UGrimrockPartySaveGame* Previous = NewObject<UGrimrockPartySaveGame>();
	Previous->SaveVersion = 10;
	FText Error;
	TestFalse(TEXT("Previous v10 is rejected without migration"), Previous->ValidateCurrentState(Error));
	TestFalse(TEXT("Previous v10 is incompatible"), Previous->IsCompatible());
	TestEqual(TEXT("Validation never rewrites v10"), Previous->SaveVersion, 10);
	TestTrue(TEXT("Rejected v10 reports an error"), !Error.IsEmpty());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
