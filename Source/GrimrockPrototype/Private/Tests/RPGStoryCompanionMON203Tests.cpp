#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "RPG/RPGClassAsset.h"
#include "RPG/RPGPartyRecruitmentService.h"
#include "RPG/RPGRaceAsset.h"
#include "RPG/RPGStoryCompanionAsset.h"
#include "RPG/RPGStoryCompanionService.h"
#include "Runtime/GridPartyInventoryComponent.h"

namespace GridMON203StoryCompanionTests
{
	URPGRaceAsset* MakeRace()
	{
		URPGRaceAsset* Race = NewObject<URPGRaceAsset>();
		Race->RaceId = TEXT("Human");
		Race->DisplayName = FText::FromString(TEXT("Humain"));
		return Race;
	}

	URPGClassAsset* MakeClass()
	{
		URPGClassAsset* ClassDefinition = NewObject<URPGClassAsset>();
		ClassDefinition->ClassId = TEXT("Rogue");
		ClassDefinition->DisplayName = FText::FromString(TEXT("Voleur"));
		ClassDefinition->BaseAttributes = FRPGAttributes(10, 12, 10, 10, 10, 10);
		ClassDefinition->HealthAtLevelOne = 8;
		ClassDefinition->HealthPerLevel = 4;
		ClassDefinition->ManaAtLevelOne = 0;
		ClassDefinition->ManaPerLevel = 0;
		return ClassDefinition;
	}

	URPGStoryCompanionAsset* MakeCompanion(URPGRaceAsset* Race, URPGClassAsset* ClassDefinition)
	{
		URPGStoryCompanionAsset* Companion = NewObject<URPGStoryCompanionAsset>();
		Companion->CompanionId = TEXT("Companion_Scout");
		Companion->CharacterId = FGuid(20, 3, 1, 2);
		Companion->DisplayName = FText::FromString(TEXT("Serana de Valombre"));
		Companion->ShortDescription = FText::FromString(TEXT("Ancienne éclaireuse."));
		Companion->RaceDefinition = Race;
		Companion->ClassDefinition = ClassDefinition;
		Companion->Level = 3;
		Companion->PortraitVariantId = TEXT("Default");
		return Companion;
	}

	FGridCharacterInventoryState MakeCharacter(const FGuid& CharacterId, FName RaceId, FName ClassId, const TCHAR* Name)
	{
		FGridCharacterInventoryState Character;
		Character.CharacterId = CharacterId;
		Character.DisplayName = FText::FromString(Name);
		Character.RaceId = RaceId;
		Character.RaceDisplayName = FText::FromName(RaceId);
		Character.ClassId = ClassId;
		Character.ClassDisplayName = FText::FromName(ClassId);
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
		Inventory->PartyInventoryState.ActiveCharacters.Add(MakeCharacter(FGuid(20, 3, 1, 1), TEXT("Human"), TEXT("Warrior"), TEXT("MainHero")));
		Inventory->PartyInventoryState.ActiveEquipment.SetNum(1);
		return Inventory;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON203DefinitionValidationTest, "Grimrock.MON20.3.StoryCompanion.DefinitionValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON203DefinitionValidationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMON203StoryCompanionTests;

	URPGRaceAsset* Race = MakeRace();
	URPGClassAsset* ClassDefinition = MakeClass();
	URPGStoryCompanionAsset* Companion = MakeCompanion(Race, ClassDefinition);

	TestTrue(TEXT("Complete story companion definition is valid"), Companion->IsValidDefinition());

	Companion->CharacterId = FGuid();
	TestFalse(TEXT("Missing stable CharacterId is rejected"), Companion->IsValidDefinition());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON203RegisterCandidateTest, "Grimrock.MON20.3.StoryCompanion.RegisterCandidate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON203RegisterCandidateTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMON203StoryCompanionTests;

	UGridPartyInventoryComponent* Inventory = MakeParty();
	URPGRaceAsset* Race = MakeRace();
	URPGClassAsset* ClassDefinition = MakeClass();
	URPGStoryCompanionAsset* Companion = MakeCompanion(Race, ClassDefinition);

	FRPGStoryCompanionRegistrationResult Result;
	TestTrue(TEXT("Story companion registers"), FRPGStoryCompanionService::EnsureCandidateRegistered(Inventory, Companion, Result));
	TestTrue(TEXT("Registration reports success"), Result.bSucceeded);
	TestEqual(
		TEXT("Registration status is AddedToPool"), static_cast<int32>(Result.Status), static_cast<int32>(ERPGStoryCompanionRegistrationStatus::AddedToPool));
	TestEqual(TEXT("Exactly one pool candidate exists"), Inventory->PartyInventoryState.CharacterPool.Num(), 1);

	const FGridCharacterInventoryState& Candidate = Inventory->PartyInventoryState.CharacterPool[0];
	TestTrue(TEXT("Stable CharacterId is copied"), Candidate.CharacterId == Companion->CharacterId);
	TestEqual(TEXT("Race identity is copied"), Candidate.RaceId, Race->RaceId);
	TestEqual(TEXT("Class identity is copied"), Candidate.ClassId, ClassDefinition->ClassId);
	TestEqual(TEXT("Level is copied"), Candidate.Level, 3);
	TestEqual(TEXT("Level-three XP floor is derived"), Candidate.Experience, 3000);
	TestEqual(TEXT("Inventory slots use party default"), Candidate.InventorySlots.Num(), Inventory->DefaultInventorySlotCountPerCharacter);
	TestEqual(TEXT("Hotbar contains ten empty bindings"), Candidate.CombatHotbarSlots.Num(), FGridCombatHotbarBinding::SlotCount);
	TestTrue(TEXT("Candidate RPG state is initialized"), Candidate.bRPGAttributesInitialized);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON203IdempotentRegistrationTest, "Grimrock.MON20.3.StoryCompanion.IdempotentRegistration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON203IdempotentRegistrationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMON203StoryCompanionTests;

	UGridPartyInventoryComponent* Inventory = MakeParty();
	URPGStoryCompanionAsset* Companion = MakeCompanion(MakeRace(), MakeClass());

	FRPGStoryCompanionRegistrationResult FirstResult;
	TestTrue(TEXT("First registration succeeds"), FRPGStoryCompanionService::EnsureCandidateRegistered(Inventory, Companion, FirstResult));

	FRPGStoryCompanionRegistrationResult SecondResult;
	TestTrue(TEXT("Second registration resolves idempotently"), FRPGStoryCompanionService::EnsureCandidateRegistered(Inventory, Companion, SecondResult));
	TestEqual(TEXT("Second status is AlreadyInPool"), static_cast<int32>(SecondResult.Status),
		static_cast<int32>(ERPGStoryCompanionRegistrationStatus::AlreadyInPool));
	TestEqual(TEXT("Pool still contains one candidate"), Inventory->PartyInventoryState.CharacterPool.Num(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON203AlreadyActiveRecognitionTest, "Grimrock.MON20.3.StoryCompanion.AlreadyActiveRecognition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON203AlreadyActiveRecognitionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMON203StoryCompanionTests;

	UGridPartyInventoryComponent* Inventory = MakeParty(2);
	URPGStoryCompanionAsset* Companion = MakeCompanion(MakeRace(), MakeClass());

	FRPGStoryCompanionRegistrationResult Registration;
	TestTrue(TEXT("Companion enters pool"), FRPGStoryCompanionService::EnsureCandidateRegistered(Inventory, Companion, Registration));

	FRPGPartyRecruitmentResult Recruitment;
	TestTrue(
		TEXT("MON20.2 recruits the registered companion"), FRPGPartyRecruitmentService::TryRecruitFromPool(Inventory, Companion->CharacterId, Recruitment));

	FRPGStoryCompanionRegistrationResult Recheck;
	TestTrue(TEXT("Recheck recognizes the active companion"), FRPGStoryCompanionService::EnsureCandidateRegistered(Inventory, Companion, Recheck));
	TestEqual(
		TEXT("Recheck status is AlreadyActive"), static_cast<int32>(Recheck.Status), static_cast<int32>(ERPGStoryCompanionRegistrationStatus::AlreadyActive));
	TestEqual(TEXT("Active party contains two characters"), Inventory->PartyInventoryState.ActiveCharacters.Num(), 2);
	TestEqual(TEXT("Pool is empty after recruitment"), Inventory->PartyInventoryState.CharacterPool.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON203IdentityCollisionRejectTest, "Grimrock.MON20.3.StoryCompanion.IdentityCollisionReject",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON203IdentityCollisionRejectTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMON203StoryCompanionTests;

	UGridPartyInventoryComponent* Inventory = MakeParty(2);
	URPGStoryCompanionAsset* Companion = MakeCompanion(MakeRace(), MakeClass());

	Inventory->PartyInventoryState.ActiveCharacters[0].CharacterId = Companion->CharacterId;

	FRPGStoryCompanionRegistrationResult Result;
	TestFalse(TEXT("Colliding identity is rejected"), FRPGStoryCompanionService::EnsureCandidateRegistered(Inventory, Companion, Result));
	TestEqual(
		TEXT("Collision status is explicit"), static_cast<int32>(Result.Status), static_cast<int32>(ERPGStoryCompanionRegistrationStatus::IdentityCollision));
	TestEqual(TEXT("Rejected collision does not mutate pool"), Inventory->PartyInventoryState.CharacterPool.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON203InvalidDefinitionAtomicRejectTest, "Grimrock.MON20.3.StoryCompanion.InvalidDefinitionAtomicReject",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON203InvalidDefinitionAtomicRejectTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMON203StoryCompanionTests;

	UGridPartyInventoryComponent* Inventory = MakeParty(2);
	URPGStoryCompanionAsset* Companion = MakeCompanion(MakeRace(), MakeClass());
	Companion->CompanionId = NAME_None;

	FRPGStoryCompanionRegistrationResult Result;
	TestFalse(TEXT("Invalid definition is rejected"), FRPGStoryCompanionService::EnsureCandidateRegistered(Inventory, Companion, Result));
	TestEqual(TEXT("Invalid definition status is explicit"), static_cast<int32>(Result.Status),
		static_cast<int32>(ERPGStoryCompanionRegistrationStatus::InvalidDefinition));
	TestEqual(TEXT("Rejected definition does not mutate pool"), Inventory->PartyInventoryState.CharacterPool.Num(), 0);
	return true;
}

#endif
