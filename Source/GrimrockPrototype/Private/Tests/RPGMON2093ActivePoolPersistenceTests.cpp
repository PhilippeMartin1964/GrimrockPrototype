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

	URPGSkillAsset* MakeMON2093Skill(FName SkillId, int32 MaxRank = 5)
	{
		URPGSkillAsset* Skill = NewObject<URPGSkillAsset>();
		Skill->SkillId = SkillId;
		Skill->DisplayName = FText::FromName(SkillId);
		Skill->MaxRank = MaxRank;
		return Skill;
	}

	void AddMON2093Rank(FGridCharacterInventoryState& Character, FName SkillId, int32 Rank)
	{
		FRPGSkillRank SkillRank;
		SkillRank.SkillId = SkillId;
		SkillRank.Rank = Rank;
		Character.SkillRanks.Add(SkillRank);
	}

	const FGridCharacterInventoryState* FindCharacter(const FGridPartyInventoryState& State, const FGuid& CharacterId)
	{
		if (const FGridCharacterInventoryState* Active = State.ActiveCharacters.FindByPredicate(
				[&CharacterId](const FGridCharacterInventoryState& Character)
				{
					return Character.CharacterId == CharacterId;
				}))
		{
			return Active;
		}
		return State.CharacterPool.FindByPredicate(
			[&CharacterId](const FGridCharacterInventoryState& Character)
			{
				return Character.CharacterId == CharacterId;
			});
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2093RecruitmentPreservesPoolSkillTest, "Grimrock.MON20.9.ActivePoolPersistence.RecruitmentPreservesPoolSkill",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2093RecruitmentPreservesPoolSkillTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPGMON2093Tests;

	UGridPartyInventoryComponent* Inventory = MakeParty(2);
	const FGuid RecruitId(20, 9, 3, 2);
	FGridCharacterInventoryState Recruit = MakeCharacter(RecruitId, TEXT("SkilledRecruit"));
	AddMON2093Rank(Recruit, TEXT("Skill_Lockpicking"), 3);
	Inventory->PartyInventoryState.CharacterPool.Add(Recruit);

	FRPGPartyRecruitmentResult Result;
	TestTrue(TEXT("Skilled pool candidate recruits"), FRPGPartyRecruitmentService::TryRecruitFromPool(Inventory, RecruitId, Result));
	TestEqual(TEXT("Candidate leaves the pool"), Inventory->PartyInventoryState.CharacterPool.Num(), 0);
	TestTrue(TEXT("Candidate becomes active"),
		Inventory->PartyInventoryState.ActiveCharacters.IsValidIndex(1) && Inventory->PartyInventoryState.ActiveCharacters[1].CharacterId == RecruitId);
	TestEqual(TEXT("Runtime Skill rank survives recruitment copy"),
		FRPGSkillService::GetSkillRank(Inventory->PartyInventoryState.ActiveCharacters[1], TEXT("Skill_Lockpicking")), 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2093PoolSnapshotRestoresAfterRecruitmentTest,
	"Grimrock.MON20.9.ActivePoolPersistence.PoolSnapshotRestoresAfterRecruitment", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2093PoolSnapshotRestoresAfterRecruitmentTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPGMON2093Tests;

	UGridPartyInventoryComponent* Inventory = MakeParty(2);
	const FGuid RecruitId(20, 9, 3, 3);
	FGridCharacterInventoryState Recruit = MakeCharacter(RecruitId, TEXT("PoolToActive"));
	AddMON2093Rank(Recruit, TEXT("Skill_Lockpicking"), 4);
	Inventory->PartyInventoryState.CharacterPool.Add(Recruit);

	URPGSkillAsset* Skill = MakeMON2093Skill(TEXT("Skill_Lockpicking"), 5);
	const auto Resolver = [Skill](FName SkillId) -> const URPGSkillAsset*
	{
		return SkillId == Skill->SkillId ? Skill : nullptr;
	};

	TArray<FRPGCharacterSkillSaveState> SavedStates;
	FString Error;
	TestTrue(TEXT("Pool Skill snapshot captures"), FRPGSkillPersistence::CapturePartySkills(Inventory->PartyInventoryState, Resolver, SavedStates, Error));

	FRPGPartyRecruitmentResult Recruitment;
	TestTrue(TEXT("Pool character recruits after capture"), FRPGPartyRecruitmentService::TryRecruitFromPool(Inventory, RecruitId, Recruitment));
	Inventory->PartyInventoryState.ActiveCharacters[1].SkillRanks.Reset();

	TestTrue(TEXT("Pre-recruitment snapshot restores to active identity"),
		FRPGSkillPersistence::RestorePartySkills(Inventory->PartyInventoryState, SavedStates, Resolver, Error));
	TestEqual(TEXT("Active recruit receives captured rank"),
		FRPGSkillService::GetSkillRank(Inventory->PartyInventoryState.ActiveCharacters[1], TEXT("Skill_Lockpicking")), 4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2093ActiveSnapshotRestoresAfterReserveMoveTest,
	"Grimrock.MON20.9.ActivePoolPersistence.ActiveSnapshotRestoresAfterReserveMove", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2093ActiveSnapshotRestoresAfterReserveMoveTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPGMON2093Tests;

	UGridPartyInventoryComponent* Inventory = MakeParty(2);
	const FGuid RecruitId(20, 9, 3, 4);
	FGridCharacterInventoryState Recruit = MakeCharacter(RecruitId, TEXT("ActiveToPool"));
	AddMON2093Rank(Recruit, TEXT("Skill_Athletics"), 2);
	Inventory->PartyInventoryState.CharacterPool.Add(Recruit);

	FRPGPartyRecruitmentResult Recruitment;
	TestTrue(TEXT("Candidate recruits before capture"), FRPGPartyRecruitmentService::TryRecruitFromPool(Inventory, RecruitId, Recruitment));

	URPGSkillAsset* Skill = MakeMON2093Skill(TEXT("Skill_Athletics"), 5);
	const auto Resolver = [Skill](FName SkillId) -> const URPGSkillAsset*
	{
		return SkillId == Skill->SkillId ? Skill : nullptr;
	};

	TArray<FRPGCharacterSkillSaveState> SavedStates;
	FString Error;
	TestTrue(TEXT("Active Skill snapshot captures"), FRPGSkillPersistence::CapturePartySkills(Inventory->PartyInventoryState, Resolver, SavedStates, Error));

	FGridPartyInventoryState TargetState = Inventory->PartyInventoryState;
	FGridCharacterInventoryState MovedCharacter = TargetState.ActiveCharacters[1];
	MovedCharacter.SkillRanks.Reset();
	TargetState.ActiveCharacters.RemoveAt(1);
	TargetState.ActiveEquipment.RemoveAt(1);
	TargetState.CharacterPool.Add(MovedCharacter);

	TestTrue(TEXT("Active snapshot restores to pooled identity"), FRPGSkillPersistence::RestorePartySkills(TargetState, SavedStates, Resolver, Error));
	const FGridCharacterInventoryState* Restored = FindCharacter(TargetState, RecruitId);
	TestNotNull(TEXT("Moved identity remains resolvable"), Restored);
	if (!Restored)
	{
		return false;
	}
	TestEqual(TEXT("Pooled identity receives captured rank"), FRPGSkillService::GetSkillRank(*Restored, TEXT("Skill_Athletics")), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2093MixedActivePoolIsolationTest, "Grimrock.MON20.9.ActivePoolPersistence.MixedActivePoolIsolation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2093MixedActivePoolIsolationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPGMON2093Tests;

	UGridPartyInventoryComponent* Inventory = MakeParty(3);
	const FGuid PoolId(20, 9, 3, 5);
	AddMON2093Rank(Inventory->PartyInventoryState.ActiveCharacters[0], TEXT("Skill_Perception"), 1);
	FGridCharacterInventoryState PoolCharacter = MakeCharacter(PoolId, TEXT("Reserve"));
	AddMON2093Rank(PoolCharacter, TEXT("Skill_Perception"), 4);
	Inventory->PartyInventoryState.CharacterPool.Add(PoolCharacter);

	URPGSkillAsset* Skill = MakeMON2093Skill(TEXT("Skill_Perception"), 5);
	const auto Resolver = [Skill](FName SkillId) -> const URPGSkillAsset*
	{
		return SkillId == Skill->SkillId ? Skill : nullptr;
	};

	TArray<FRPGCharacterSkillSaveState> SavedStates;
	FString Error;
	TestTrue(
		TEXT("Mixed active/pool snapshot captures"), FRPGSkillPersistence::CapturePartySkills(Inventory->PartyInventoryState, Resolver, SavedStates, Error));

	Inventory->PartyInventoryState.ActiveCharacters[0].SkillRanks.Reset();
	Inventory->PartyInventoryState.CharacterPool[0].SkillRanks.Reset();
	TestTrue(TEXT("Mixed snapshot restores"), FRPGSkillPersistence::RestorePartySkills(Inventory->PartyInventoryState, SavedStates, Resolver, Error));

	TestEqual(TEXT("Active rank remains attached to active CharacterId"),
		FRPGSkillService::GetSkillRank(Inventory->PartyInventoryState.ActiveCharacters[0], TEXT("Skill_Perception")), 1);
	TestEqual(TEXT("Pool rank remains attached to pool CharacterId"),
		FRPGSkillService::GetSkillRank(Inventory->PartyInventoryState.CharacterPool[0], TEXT("Skill_Perception")), 4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2093SelectedCharacterIndependentTest, "Grimrock.MON20.9.ActivePoolPersistence.SelectedCharacterIndependent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2093SelectedCharacterIndependentTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPGMON2093Tests;

	UGridPartyInventoryComponent* Inventory = MakeParty(2);
	const FGuid RecruitId(20, 9, 3, 6);
	AddMON2093Rank(Inventory->PartyInventoryState.ActiveCharacters[0], TEXT("Skill_Survival"), 1);
	FGridCharacterInventoryState Recruit = MakeCharacter(RecruitId, TEXT("SelectedRecruit"));
	AddMON2093Rank(Recruit, TEXT("Skill_Survival"), 3);
	Inventory->PartyInventoryState.CharacterPool.Add(Recruit);

	FRPGPartyRecruitmentResult Recruitment;
	TestTrue(TEXT("Second character recruits"), FRPGPartyRecruitmentService::TryRecruitFromPool(Inventory, RecruitId, Recruitment));
	Inventory->PartyInventoryState.SelectedCharacterIndex = 1;

	URPGSkillAsset* Skill = MakeMON2093Skill(TEXT("Skill_Survival"), 5);
	const auto Resolver = [Skill](FName SkillId) -> const URPGSkillAsset*
	{
		return SkillId == Skill->SkillId ? Skill : nullptr;
	};

	TArray<FRPGCharacterSkillSaveState> SavedStates;
	FString Error;
	TestTrue(TEXT("Snapshot captures independently of selection"),
		FRPGSkillPersistence::CapturePartySkills(Inventory->PartyInventoryState, Resolver, SavedStates, Error));

	Inventory->PartyInventoryState.SelectedCharacterIndex = 0;
	for (FGridCharacterInventoryState& Character : Inventory->PartyInventoryState.ActiveCharacters)
	{
		Character.SkillRanks.Reset();
	}

	TestTrue(TEXT("Snapshot restores after selection changes"),
		FRPGSkillPersistence::RestorePartySkills(Inventory->PartyInventoryState, SavedStates, Resolver, Error));
	TestEqual(TEXT("Restore does not alter SelectedCharacterIndex"), Inventory->PartyInventoryState.SelectedCharacterIndex, 0);
	TestEqual(
		TEXT("First active rank restores"), FRPGSkillService::GetSkillRank(Inventory->PartyInventoryState.ActiveCharacters[0], TEXT("Skill_Survival")), 1);
	TestEqual(
		TEXT("Second active rank restores"), FRPGSkillService::GetSkillRank(Inventory->PartyInventoryState.ActiveCharacters[1], TEXT("Skill_Survival")), 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2093RecruitmentRejectPreservesPoolSkillTest,
	"Grimrock.MON20.9.ActivePoolPersistence.RecruitmentRejectPreservesPoolSkill", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2093RecruitmentRejectPreservesPoolSkillTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPGMON2093Tests;

	UGridPartyInventoryComponent* Inventory = MakeParty(1);
	const FGuid RecruitId(20, 9, 3, 7);
	FGridCharacterInventoryState Recruit = MakeCharacter(RecruitId, TEXT("WaitingRecruit"));
	AddMON2093Rank(Recruit, TEXT("Skill_Lockpicking"), 3);
	Inventory->PartyInventoryState.CharacterPool.Add(Recruit);

	FRPGPartyRecruitmentResult Result;
	TestFalse(TEXT("Full party rejects recruitment"), FRPGPartyRecruitmentService::TryRecruitFromPool(Inventory, RecruitId, Result));
	TestTrue(TEXT("Reject reason is PartyFull"), Result.RejectReason == ERPGPartyRecruitmentRejectReason::PartyFull);
	TestEqual(TEXT("Candidate remains in pool"), Inventory->PartyInventoryState.CharacterPool.Num(), 1);
	TestEqual(TEXT("Rejected transaction preserves pool Skill rank"),
		FRPGSkillService::GetSkillRank(Inventory->PartyInventoryState.CharacterPool[0], TEXT("Skill_Lockpicking")), 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2093PoolReorderRestoresByIdentityTest, "Grimrock.MON20.9.ActivePoolPersistence.PoolReorderRestoresByIdentity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2093PoolReorderRestoresByIdentityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPGMON2093Tests;

	UGridPartyInventoryComponent* Inventory = MakeParty(3);
	const FGuid FirstId(20, 9, 3, 8);
	const FGuid SecondId(20, 9, 3, 9);
	FGridCharacterInventoryState First = MakeCharacter(FirstId, TEXT("FirstReserve"));
	FGridCharacterInventoryState Second = MakeCharacter(SecondId, TEXT("SecondReserve"));
	AddMON2093Rank(First, TEXT("Skill_Perception"), 1);
	AddMON2093Rank(Second, TEXT("Skill_Perception"), 4);
	Inventory->PartyInventoryState.CharacterPool.Add(First);
	Inventory->PartyInventoryState.CharacterPool.Add(Second);

	URPGSkillAsset* Skill = MakeMON2093Skill(TEXT("Skill_Perception"), 5);
	const auto Resolver = [Skill](FName SkillId) -> const URPGSkillAsset*
	{
		return SkillId == Skill->SkillId ? Skill : nullptr;
	};

	TArray<FRPGCharacterSkillSaveState> SavedStates;
	FString Error;
	TestTrue(
		TEXT("Pool snapshot captures before reorder"), FRPGSkillPersistence::CapturePartySkills(Inventory->PartyInventoryState, Resolver, SavedStates, Error));

	Inventory->PartyInventoryState.CharacterPool.Swap(0, 1);
	for (FGridCharacterInventoryState& Character : Inventory->PartyInventoryState.CharacterPool)
	{
		Character.SkillRanks.Reset();
	}

	TestTrue(
		TEXT("Pool snapshot restores after reorder"), FRPGSkillPersistence::RestorePartySkills(Inventory->PartyInventoryState, SavedStates, Resolver, Error));
	const FGridCharacterInventoryState* FirstRestored = FindCharacter(Inventory->PartyInventoryState, FirstId);
	const FGridCharacterInventoryState* SecondRestored = FindCharacter(Inventory->PartyInventoryState, SecondId);
	TestNotNull(TEXT("First reserve identity resolves"), FirstRestored);
	TestNotNull(TEXT("Second reserve identity resolves"), SecondRestored);
	if (!FirstRestored || !SecondRestored)
	{
		return false;
	}
	TestEqual(TEXT("First rank follows CharacterId"), FRPGSkillService::GetSkillRank(*FirstRestored, TEXT("Skill_Perception")), 1);
	TestEqual(TEXT("Second rank follows CharacterId"), FRPGSkillService::GetSkillRank(*SecondRestored, TEXT("Skill_Perception")), 4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2093SnapshotValidAfterRecruitmentMoveTest, "Grimrock.MON20.9.ActivePoolPersistence.SnapshotValidAfterRecruitmentMove",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2093SnapshotValidAfterRecruitmentMoveTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPGMON2093Tests;

	UGridPartyInventoryComponent* Inventory = MakeParty(2);
	const FGuid RecruitId(20, 9, 3, 10);
	FGridCharacterInventoryState Recruit = MakeCharacter(RecruitId, TEXT("MovingIdentity"));
	AddMON2093Rank(Recruit, TEXT("Skill_Lockpicking"), 2);
	Inventory->PartyInventoryState.CharacterPool.Add(Recruit);

	URPGSkillAsset* Skill = MakeMON2093Skill(TEXT("Skill_Lockpicking"), 5);
	const auto Resolver = [Skill](FName SkillId) -> const URPGSkillAsset*
	{
		return SkillId == Skill->SkillId ? Skill : nullptr;
	};

	TArray<FRPGCharacterSkillSaveState> SavedStates;
	FString Error;
	TestTrue(TEXT("Snapshot captures while identity is pooled"),
		FRPGSkillPersistence::CapturePartySkills(Inventory->PartyInventoryState, Resolver, SavedStates, Error));

	FRPGPartyRecruitmentResult Recruitment;
	TestTrue(TEXT("Identity moves from pool to active"), FRPGPartyRecruitmentService::TryRecruitFromPool(Inventory, RecruitId, Recruitment));

	TestTrue(TEXT("Existing snapshot remains valid after location change"),
		FRPGSkillPersistence::ValidateSavedPartySkills(Inventory->PartyInventoryState, SavedStates, Resolver, Error));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
