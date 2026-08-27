#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "RPG/RPGPartyRecruitmentService.h"
#include "RPG/RPGSkillAsset.h"
#include "RPG/RPGSkillPersistence.h"
#include "RPG/RPGSkillService.h"
#include "Runtime/GridPartyInventoryComponent.h"

namespace RPGMON2093Tests
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
		Character.Resources.CurrentHealth = 10;
		Character.InventorySlots.SetNum(4);
		Character.CombatHotbarSlots.SetNum(FGridCombatHotbarBinding::SlotCount);
		for (int32 SlotIndex = 0; SlotIndex < FGridCombatHotbarBinding::SlotCount; ++SlotIndex)
		{
			Character.CombatHotbarSlots[SlotIndex].Reset(SlotIndex);
		}
		return Character;
	}

	UGridPartyInventoryComponent* MakeParty(int32 MaxActiveCharacters = 3)
	{
		UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
		Inventory->PartyInventoryState = FGridPartyInventoryState();
		Inventory->PartyInventoryState.MaxActiveCharacters = MaxActiveCharacters;
		Inventory->PartyInventoryState.bInitialCharacterCreationCompleted = true;
		Inventory->PartyInventoryState.SelectedCharacterIndex = 0;
		Inventory->PartyInventoryState.ActiveCharacters.Add(MakeCharacter(FGuid(20, 9, 3, 1), TEXT("MainHero")));
		Inventory->PartyInventoryState.ActiveEquipment.SetNum(1);
		return Inventory;
	}

	URPGSkillAsset* MakeSkill(FName SkillId, int32 MaxRank = 5)
	{
		URPGSkillAsset* Skill = NewObject<URPGSkillAsset>();
		Skill->SkillId = SkillId;
		Skill->DisplayName = FText::FromName(SkillId);
		Skill->MaxRank = MaxRank;
		return Skill;
	}

	void AddRank(FGridCharacterInventoryState& Character, FName SkillId, int32 Rank)
	{
		FRPGSkillRank Entry;
		Entry.SkillId = SkillId;
		Entry.Rank = Rank;
		Character.SkillRanks.Add(Entry);
	}

	const FGridCharacterInventoryState* FindCharacter(const FGridPartyInventoryState& State, const FGuid& CharacterId)
	{
		if (const FGridCharacterInventoryState* Active = State.ActiveCharacters.FindByPredicate(
				[&CharacterId](const FGridCharacterInventoryState& Character) { return Character.CharacterId == CharacterId; }))
		{
			return Active;
		}
		return State.CharacterPool.FindByPredicate(
			[&CharacterId](const FGridCharacterInventoryState& Character) { return Character.CharacterId == CharacterId; });
	}
}

using namespace RPGMON2093Tests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2093RecruitmentPreservesPoolSkillTest, "Grimrock.MON20.9.ActivePoolPersistence.RecruitmentPreservesPoolSkill",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2093RecruitmentPreservesPoolSkillTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UGridPartyInventoryComponent* Inventory = MakeParty(2);
	const FGuid RecruitId(20, 9, 3, 2);
	FGridCharacterInventoryState Recruit = MakeCharacter(RecruitId, TEXT("SkilledRecruit"));
	AddRank(Recruit, TEXT("Skill_Lockpicking"), 3);
	Inventory->PartyInventoryState.CharacterPool.Add(Recruit);
	FRPGPartyRecruitmentResult Result;
	TestTrue(TEXT("Skilled pool candidate recruits"), FRPGPartyRecruitmentService::TryRecruitFromPool(Inventory, RecruitId, Result));
	TestEqual(TEXT("Runtime Skill rank survives recruitment copy"),
		FRPGSkillService::GetSkillRank(Inventory->PartyInventoryState.ActiveCharacters[1], TEXT("Skill_Lockpicking")), 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2093PoolSnapshotRestoresAfterRecruitmentTest,
	"Grimrock.MON20.9.ActivePoolPersistence.PoolSnapshotRestoresAfterRecruitment", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2093PoolSnapshotRestoresAfterRecruitmentTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UGridPartyInventoryComponent* Inventory = MakeParty(2);
	const FGuid RecruitId(20, 9, 3, 3);
	FGridCharacterInventoryState Recruit = MakeCharacter(RecruitId, TEXT("PoolToActive"));
	AddRank(Recruit, TEXT("Skill_Lockpicking"), 4);
	Inventory->PartyInventoryState.CharacterPool.Add(Recruit);
	const FGridPartyInventoryState SavedState = Inventory->PartyInventoryState;

	UGridPartyInventoryComponent* Restored = NewObject<UGridPartyInventoryComponent>();
	Restored->PartyInventoryState = SavedState;
	FRPGPartyRecruitmentResult Recruitment;
	TestTrue(TEXT("Restored pooled character recruits"), FRPGPartyRecruitmentService::TryRecruitFromPool(Restored, RecruitId, Recruitment));
	TestEqual(TEXT("Active recruit keeps durable rank from whole-party snapshot"),
		FRPGSkillService::GetSkillRank(Restored->PartyInventoryState.ActiveCharacters[1], TEXT("Skill_Lockpicking")), 4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2093ActiveSnapshotRestoresAfterReserveMoveTest,
	"Grimrock.MON20.9.ActivePoolPersistence.ActiveSnapshotRestoresAfterReserveMove", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2093ActiveSnapshotRestoresAfterReserveMoveTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UGridPartyInventoryComponent* Inventory = MakeParty(2);
	const FGuid RecruitId(20, 9, 3, 4);
	FGridCharacterInventoryState Recruit = MakeCharacter(RecruitId, TEXT("ActiveToPool"));
	AddRank(Recruit, TEXT("Skill_Athletics"), 2);
	Inventory->PartyInventoryState.CharacterPool.Add(Recruit);
	FRPGPartyRecruitmentResult Recruitment;
	TestTrue(TEXT("Candidate recruits"), FRPGPartyRecruitmentService::TryRecruitFromPool(Inventory, RecruitId, Recruitment));

	FGridPartyInventoryState SavedState = Inventory->PartyInventoryState;
	FGridCharacterInventoryState Moved = SavedState.ActiveCharacters[1];
	SavedState.ActiveCharacters.RemoveAt(1);
	SavedState.ActiveEquipment.RemoveAt(1);
	SavedState.CharacterPool.Add(Moved);
	const FGridCharacterInventoryState* Found = FindCharacter(SavedState, RecruitId);
	TestNotNull(TEXT("Moved identity remains resolvable"), Found);
	if (Found)
	{
		TestEqual(TEXT("Durable Skill moves with character into pool"), FRPGSkillService::GetSkillRank(*Found, TEXT("Skill_Athletics")), 2);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2093MixedActivePoolIsolationTest, "Grimrock.MON20.9.ActivePoolPersistence.MixedActivePoolIsolation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2093MixedActivePoolIsolationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UGridPartyInventoryComponent* Inventory = MakeParty(3);
	AddRank(Inventory->PartyInventoryState.ActiveCharacters[0], TEXT("Skill_Perception"), 1);
	FGridCharacterInventoryState Pool = MakeCharacter(FGuid(20, 9, 3, 5), TEXT("Reserve"));
	AddRank(Pool, TEXT("Skill_Perception"), 4);
	Inventory->PartyInventoryState.CharacterPool.Add(Pool);
	const FGridPartyInventoryState SavedState = Inventory->PartyInventoryState;
	TestEqual(TEXT("Active durable rank remains isolated"), FRPGSkillService::GetSkillRank(SavedState.ActiveCharacters[0], TEXT("Skill_Perception")), 1);
	TestEqual(TEXT("Pool durable rank remains isolated"), FRPGSkillService::GetSkillRank(SavedState.CharacterPool[0], TEXT("Skill_Perception")), 4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2093SelectedCharacterIndependentTest, "Grimrock.MON20.9.ActivePoolPersistence.SelectedCharacterIndependent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2093SelectedCharacterIndependentTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UGridPartyInventoryComponent* Inventory = MakeParty(2);
	const FGuid RecruitId(20, 9, 3, 6);
	AddRank(Inventory->PartyInventoryState.ActiveCharacters[0], TEXT("Skill_Survival"), 1);
	FGridCharacterInventoryState Recruit = MakeCharacter(RecruitId, TEXT("SelectedRecruit"));
	AddRank(Recruit, TEXT("Skill_Survival"), 3);
	Inventory->PartyInventoryState.CharacterPool.Add(Recruit);
	FRPGPartyRecruitmentResult Recruitment;
	TestTrue(TEXT("Second character recruits"), FRPGPartyRecruitmentService::TryRecruitFromPool(Inventory, RecruitId, Recruitment));
	Inventory->PartyInventoryState.SelectedCharacterIndex = 1;
	const FGridPartyInventoryState SavedState = Inventory->PartyInventoryState;
	TestEqual(TEXT("Selection is preserved independently"), SavedState.SelectedCharacterIndex, 1);
	TestEqual(TEXT("First rank is unaffected by selection"), FRPGSkillService::GetSkillRank(SavedState.ActiveCharacters[0], TEXT("Skill_Survival")), 1);
	TestEqual(TEXT("Second rank is unaffected by selection"), FRPGSkillService::GetSkillRank(SavedState.ActiveCharacters[1], TEXT("Skill_Survival")), 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2093RecruitmentRejectPreservesPoolSkillTest,
	"Grimrock.MON20.9.ActivePoolPersistence.RecruitmentRejectPreservesPoolSkill", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2093RecruitmentRejectPreservesPoolSkillTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UGridPartyInventoryComponent* Inventory = MakeParty(1);
	const FGuid RecruitId(20, 9, 3, 7);
	FGridCharacterInventoryState Recruit = MakeCharacter(RecruitId, TEXT("WaitingRecruit"));
	AddRank(Recruit, TEXT("Skill_Lockpicking"), 3);
	Inventory->PartyInventoryState.CharacterPool.Add(Recruit);
	FRPGPartyRecruitmentResult Result;
	TestFalse(TEXT("Full party rejects recruitment"), FRPGPartyRecruitmentService::TryRecruitFromPool(Inventory, RecruitId, Result));
	TestEqual(TEXT("Rejected transaction preserves pool Skill rank"),
		FRPGSkillService::GetSkillRank(Inventory->PartyInventoryState.CharacterPool[0], TEXT("Skill_Lockpicking")), 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2093PoolReorderRestoresByIdentityTest, "Grimrock.MON20.9.ActivePoolPersistence.PoolReorderRestoresByIdentity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2093PoolReorderRestoresByIdentityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UGridPartyInventoryComponent* Inventory = MakeParty(3);
	const FGuid FirstId(20, 9, 3, 8);
	const FGuid SecondId(20, 9, 3, 9);
	FGridCharacterInventoryState First = MakeCharacter(FirstId, TEXT("FirstReserve"));
	FGridCharacterInventoryState Second = MakeCharacter(SecondId, TEXT("SecondReserve"));
	AddRank(First, TEXT("Skill_Perception"), 1);
	AddRank(Second, TEXT("Skill_Perception"), 4);
	Inventory->PartyInventoryState.CharacterPool.Add(First);
	Inventory->PartyInventoryState.CharacterPool.Add(Second);
	FGridPartyInventoryState SavedState = Inventory->PartyInventoryState;
	SavedState.CharacterPool.Swap(0, 1);
	const FGridCharacterInventoryState* FirstFound = FindCharacter(SavedState, FirstId);
	const FGridCharacterInventoryState* SecondFound = FindCharacter(SavedState, SecondId);
	TestNotNull(TEXT("First identity resolves after reorder"), FirstFound);
	TestNotNull(TEXT("Second identity resolves after reorder"), SecondFound);
	if (FirstFound && SecondFound)
	{
		TestEqual(TEXT("First rank follows its character"), FRPGSkillService::GetSkillRank(*FirstFound, TEXT("Skill_Perception")), 1);
		TestEqual(TEXT("Second rank follows its character"), FRPGSkillService::GetSkillRank(*SecondFound, TEXT("Skill_Perception")), 4);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2093SnapshotValidAfterRecruitmentMoveTest, "Grimrock.MON20.9.ActivePoolPersistence.SnapshotValidAfterRecruitmentMove",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2093SnapshotValidAfterRecruitmentMoveTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UGridPartyInventoryComponent* Inventory = MakeParty(2);
	const FGuid RecruitId(20, 9, 3, 10);
	FGridCharacterInventoryState Recruit = MakeCharacter(RecruitId, TEXT("MovingIdentity"));
	AddRank(Recruit, TEXT("Skill_Lockpicking"), 2);
	Inventory->PartyInventoryState.CharacterPool.Add(Recruit);
	URPGSkillAsset* Skill = MakeSkill(TEXT("Skill_Lockpicking"), 5);
	const auto Resolver = [Skill](FName SkillId) -> const URPGSkillAsset* { return SkillId == Skill->SkillId ? Skill : nullptr; };
	FString Error;
	TestTrue(TEXT("Pooled durable state validates"), FRPGSkillPersistence::ValidatePartySkills(Inventory->PartyInventoryState, Resolver, Error));
	FRPGPartyRecruitmentResult Recruitment;
	TestTrue(TEXT("Identity moves from pool to active"), FRPGPartyRecruitmentService::TryRecruitFromPool(Inventory, RecruitId, Recruitment));
	TestTrue(TEXT("Durable Skill state remains valid after move"), FRPGSkillPersistence::ValidatePartySkills(Inventory->PartyInventoryState, Resolver, Error));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
