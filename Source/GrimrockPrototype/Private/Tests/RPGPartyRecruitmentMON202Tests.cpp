#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "RPG/RPGPartyRecruitmentService.h"
#include "Runtime/GridPartyInventoryComponent.h"

namespace GridMON202RecruitmentTests
{
	FGridCharacterInventoryState MakeCharacter(const FGuid& CharacterId, const TCHAR* Name)
	{
		FGridCharacterInventoryState Character;
		Character.CharacterId = CharacterId;
		Character.DisplayName = FText::FromString(Name);
		Character.ClassId = TEXT("Warrior");
		Character.ClassDisplayName = FText::FromString(TEXT("Guerrier"));
		Character.RaceId = TEXT("Human");
		Character.RaceDisplayName = FText::FromString(TEXT("Humain"));
		Character.Level = 1;
		Character.Experience = 0;
		Character.Attributes = FRPGAttributes(10, 10, 10, 10, 10, 10);
		Character.DerivedStats.MaxHealth = 10;
		Character.DerivedStats.CurrentHealth = 10;
		Character.bRPGAttributesInitialized = true;
		Character.Strength = 10.0f;
		Character.InventorySlots.SetNum(4);
		Character.CombatHotbarSlots.SetNum(FGridCombatHotbarBinding::SlotCount);
		for (int32 SlotIndex = 0; SlotIndex < FGridCombatHotbarBinding::SlotCount; ++SlotIndex)
		{
			Character.CombatHotbarSlots[SlotIndex].Reset(SlotIndex);
		}
		return Character;
	}

	UGridPartyInventoryComponent* MakeParty(int32 MaxActiveCharacters = 2)
	{
		UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
		Inventory->PartyInventoryState = FGridPartyInventoryState();
		Inventory->PartyInventoryState.MaxActiveCharacters = MaxActiveCharacters;
		Inventory->PartyInventoryState.bInitialCharacterCreationCompleted = true;
		Inventory->PartyInventoryState.SelectedCharacterIndex = 0;
		Inventory->PartyInventoryState.ActiveCharacters.Add(MakeCharacter(FGuid(20, 2, 1, 1), TEXT("MainHero")));
		Inventory->PartyInventoryState.ActiveEquipment.SetNum(1);
		return Inventory;
	}

	void PutInventoryItem(
		FGridCharacterInventoryState& Character, int32 SlotIndex, const FGuid& RuntimeObjectId, float Weight, int32 Quantity, int32 OwnerCharacterIndex)
	{
		check(Character.InventorySlots.IsValidIndex(SlotIndex));
		FGridInventorySlot& Slot = Character.InventorySlots[SlotIndex];
		Slot.bOccupied = true;
		Slot.Item.RuntimeObjectId = RuntimeObjectId;
		Slot.Item.ItemDefinitionId = TEXT("Item_TestRecruitment");
		Slot.Item.DisplayName = FText::FromString(TEXT("Objet de recrutement"));
		Slot.Item.Weight = Weight;
		Slot.Item.Quantity = Quantity;
		Slot.Item.OwnerType = EGridItemOwnerType::CharacterInventory;
		Slot.Item.OwnerGuid = Character.CharacterId;
		Slot.Item.OwnerCharacterIndex = OwnerCharacterIndex;
		Slot.Item.EquipmentSlot = EGridEquipmentSlot::None;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON202ValidPoolRecruitmentTest, "Grimrock.MON20.2.Recruitment.ValidPoolRecruitment",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON202ValidPoolRecruitmentTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMON202RecruitmentTests;

	UGridPartyInventoryComponent* Inventory = MakeParty(2);
	const FGuid RecruitId(20, 2, 1, 2);
	Inventory->PartyInventoryState.CharacterPool.Add(MakeCharacter(RecruitId, TEXT("Recruit")));

	FRPGPartyRecruitmentResult Result;
	TestTrue(TEXT("Pool recruit commits"), FRPGPartyRecruitmentService::TryRecruitFromPool(Inventory, RecruitId, Result));
	TestTrue(TEXT("Result is committed"), Result.bCommitted);
	TestEqual(TEXT("Active count before"), Result.ActiveCountBefore, 1);
	TestEqual(TEXT("Active count after"), Result.ActiveCountAfter, 2);
	TestEqual(TEXT("New active index"), Result.CharacterIndex, 1);
	TestEqual(TEXT("CharacterPool candidate is consumed"), Inventory->PartyInventoryState.CharacterPool.Num(), 0);
	TestEqual(TEXT("Active equipment stays aligned"), Inventory->PartyInventoryState.ActiveEquipment.Num(), 2);
	TestTrue(TEXT("Recruited identity is preserved"), Inventory->PartyInventoryState.ActiveCharacters[1].CharacterId == RecruitId);

	FString OwnershipError;
	TestTrue(TEXT("Committed party ownership is valid"), Inventory->ValidateInventoryOwnership(OwnershipError));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON202FullPartyAtomicRejectTest, "Grimrock.MON20.2.Recruitment.FullPartyAtomicReject",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON202FullPartyAtomicRejectTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMON202RecruitmentTests;

	UGridPartyInventoryComponent* Inventory = MakeParty(1);
	const FGuid RecruitId(20, 2, 2, 2);
	Inventory->PartyInventoryState.CharacterPool.Add(MakeCharacter(RecruitId, TEXT("WaitingRecruit")));

	FRPGPartyRecruitmentResult Result;
	TestFalse(TEXT("Full party rejects recruitment"), FRPGPartyRecruitmentService::TryRecruitFromPool(Inventory, RecruitId, Result));
	TestTrue(TEXT("Reject reason is PartyFull"), Result.RejectReason == ERPGPartyRecruitmentRejectReason::PartyFull);
	TestEqual(TEXT("Active party is unchanged"), Inventory->PartyInventoryState.ActiveCharacters.Num(), 1);
	TestEqual(TEXT("Candidate remains in pool"), Inventory->PartyInventoryState.CharacterPool.Num(), 1);
	TestEqual(TEXT("Equipment array is unchanged"), Inventory->PartyInventoryState.ActiveEquipment.Num(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON202MissingCandidateAtomicRejectTest, "Grimrock.MON20.2.Recruitment.MissingCandidateAtomicReject",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON202MissingCandidateAtomicRejectTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMON202RecruitmentTests;

	UGridPartyInventoryComponent* Inventory = MakeParty(2);
	const FGuid ExistingPoolId(20, 2, 3, 2);
	Inventory->PartyInventoryState.CharacterPool.Add(MakeCharacter(ExistingPoolId, TEXT("OtherRecruit")));

	FRPGPartyRecruitmentResult Result;
	TestFalse(TEXT("Unknown candidate is rejected"), FRPGPartyRecruitmentService::TryRecruitFromPool(Inventory, FGuid(20, 2, 3, 99), Result));
	TestTrue(TEXT("Reject reason is CandidateNotFound"), Result.RejectReason == ERPGPartyRecruitmentRejectReason::CandidateNotFound);
	TestEqual(TEXT("Active party remains unchanged"), Inventory->PartyInventoryState.ActiveCharacters.Num(), 1);
	TestEqual(TEXT("Existing pool is untouched"), Inventory->PartyInventoryState.CharacterPool.Num(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON202DuplicateIdentityRejectTest, "Grimrock.MON20.2.Recruitment.DuplicateIdentityReject",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON202DuplicateIdentityRejectTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMON202RecruitmentTests;

	UGridPartyInventoryComponent* Inventory = MakeParty(2);
	const FGuid MainHeroId = Inventory->PartyInventoryState.ActiveCharacters[0].CharacterId;
	Inventory->PartyInventoryState.CharacterPool.Add(MakeCharacter(MainHeroId, TEXT("Duplicate")));

	FRPGPartyRecruitmentResult Result;
	TestFalse(TEXT("Already-active CharacterId is rejected"), FRPGPartyRecruitmentService::TryRecruitFromPool(Inventory, MainHeroId, Result));
	TestTrue(TEXT("Reject reason is DuplicateActiveCharacter"), Result.RejectReason == ERPGPartyRecruitmentRejectReason::DuplicateActiveCharacter);
	TestEqual(TEXT("Active count is unchanged"), Inventory->PartyInventoryState.ActiveCharacters.Num(), 1);
	TestEqual(TEXT("Duplicate candidate remains in pool after rejection"), Inventory->PartyInventoryState.CharacterPool.Num(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON202InventoryOwnershipNormalizationTest, "Grimrock.MON20.2.Recruitment.InventoryOwnershipNormalization",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON202InventoryOwnershipNormalizationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMON202RecruitmentTests;

	UGridPartyInventoryComponent* Inventory = MakeParty(2);
	const FGuid RecruitId(20, 2, 5, 2);
	FGridCharacterInventoryState Candidate = MakeCharacter(RecruitId, TEXT("Carrier"));
	PutInventoryItem(Candidate, 0, FGuid(20, 2, 5, 10), 1.25f, 2, INDEX_NONE);
	Candidate.InventorySlots[0].Item.OwnerType = EGridItemOwnerType::None;
	Candidate.InventorySlots[0].Item.OwnerGuid = FGuid();
	Inventory->PartyInventoryState.CharacterPool.Add(Candidate);

	FRPGPartyRecruitmentResult Result;
	TestTrue(TEXT("Candidate with pool inventory recruits"), FRPGPartyRecruitmentService::TryRecruitFromPool(Inventory, RecruitId, Result));

	const FGridCharacterInventoryState& Recruited = Inventory->PartyInventoryState.ActiveCharacters[1];
	const FGridItemInstance& Item = Recruited.InventorySlots[0].Item;
	TestTrue(TEXT("Inventory owner type is normalized"), Item.OwnerType == EGridItemOwnerType::CharacterInventory);
	TestEqual(TEXT("Inventory owner index is normalized"), Item.OwnerCharacterIndex, 1);
	TestTrue(TEXT("Inventory owner guid is normalized"), Item.OwnerGuid == RecruitId);
	TestTrue(TEXT("Inventory item is not equipment"), Item.EquipmentSlot == EGridEquipmentSlot::None);
	TestEqual(TEXT("Inventory weight is recomputed"), Recruited.CurrentWeight, 2.5f);

	FString OwnershipError;
	TestTrue(TEXT("Normalized ownership validates"), Inventory->ValidateInventoryOwnership(OwnershipError));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON202OwnershipCollisionRollbackTest, "Grimrock.MON20.2.Recruitment.OwnershipCollisionRollback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON202OwnershipCollisionRollbackTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMON202RecruitmentTests;

	UGridPartyInventoryComponent* Inventory = MakeParty(2);
	const FGuid SharedItemRuntimeId(20, 2, 6, 10);
	PutInventoryItem(Inventory->PartyInventoryState.ActiveCharacters[0], 0, SharedItemRuntimeId, 1.0f, 1, 0);

	const FGuid RecruitId(20, 2, 6, 2);
	FGridCharacterInventoryState Candidate = MakeCharacter(RecruitId, TEXT("CollisionRecruit"));
	PutInventoryItem(Candidate, 0, SharedItemRuntimeId, 1.0f, 1, INDEX_NONE);
	Inventory->PartyInventoryState.CharacterPool.Add(Candidate);

	FString PreOwnershipError;
	TestTrue(TEXT("Pre-transaction active ownership is valid"), Inventory->ValidateInventoryOwnership(PreOwnershipError));

	FRPGPartyRecruitmentResult Result;
	TestFalse(TEXT("Runtime item identity collision rejects commit"), FRPGPartyRecruitmentService::TryRecruitFromPool(Inventory, RecruitId, Result));
	TestTrue(TEXT("Reject reason is OwnershipValidationFailed"), Result.RejectReason == ERPGPartyRecruitmentRejectReason::OwnershipValidationFailed);
	TestEqual(TEXT("Rollback restores active count"), Inventory->PartyInventoryState.ActiveCharacters.Num(), 1);
	TestEqual(TEXT("Rollback restores pool candidate"), Inventory->PartyInventoryState.CharacterPool.Num(), 1);
	TestEqual(TEXT("Rollback restores equipment count"), Inventory->PartyInventoryState.ActiveEquipment.Num(), 1);

	FString PostOwnershipError;
	TestTrue(TEXT("Rollback leaves active ownership valid"), Inventory->ValidateInventoryOwnership(PostOwnershipError));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
