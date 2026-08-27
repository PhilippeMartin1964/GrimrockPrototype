#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Engine/Texture2D.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGCustomRecruitService.h"
#include "RPG/RPGRaceAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"

namespace RPGCustomRecruitMON205Tests
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
		Inventory->DefaultInventorySlotCountPerCharacter = 6;
		Inventory->PartyInventoryState = FGridPartyInventoryState();
		Inventory->PartyInventoryState.MaxActiveCharacters = MaxActiveCharacters;
		Inventory->PartyInventoryState.bInitialCharacterCreationCompleted = true;
		Inventory->PartyInventoryState.SelectedCharacterIndex = 0;
		Inventory->PartyInventoryState.ActiveCharacters.Add(MakeCharacter(FGuid(20, 5, 1, 1), TEXT("MainHero")));
		Inventory->PartyInventoryState.ActiveEquipment.SetNum(1);
		return Inventory;
	}

	URPGRaceAsset* MakeRace()
	{
		URPGRaceAsset* Race = NewObject<URPGRaceAsset>();
		Race->RaceId = TEXT("Human");
		Race->DisplayName = FText::FromString(TEXT("Humain"));
		Race->AttributeBonuses = FRPGAttributes(1, 0, 0, 0, 0, 0);
		return Race;
	}

	URPGClassAsset* MakeClass()
	{
		URPGClassAsset* CharacterClass = NewObject<URPGClassAsset>();
		CharacterClass->ClassId = TEXT("Warrior");
		CharacterClass->DisplayName = FText::FromString(TEXT("Guerrier"));
		CharacterClass->BaseAttributes = FRPGAttributes(10, 10, 10, 10, 10, 10);
		CharacterClass->HealthAtLevelOne = 12;
		CharacterClass->ManaAtLevelOne = 2;
		return CharacterClass;
	}

	FRPGCharacterCreationRequest MakeRequest(URPGRaceAsset* Race, URPGClassAsset* CharacterClass, const TCHAR* Name = TEXT("Mercenaire"))
	{
		FRPGCharacterCreationRequest Request;
		Request.DisplayName = FText::FromString(Name);
		Request.RaceDefinition = Race;
		Request.ClassDefinition = CharacterClass;
		Request.CombatActionSourceClassDefinition = CharacterClass;
		Request.PortraitGender = ERPGCharacterPortraitGender::Male;
		Request.PortraitVariantId = TEXT("Portrait_A");
		return Request;
	}

	void PutMalformedOwnedItem(FGridCharacterInventoryState& Character)
	{
		check(Character.InventorySlots.IsValidIndex(0));
		FGridInventorySlot& Slot = Character.InventorySlots[0];
		Slot.bOccupied = true;
		Slot.Item.RuntimeObjectId = FGuid(20, 5, 7, 50);
		Slot.Item.ItemDefinitionId = TEXT("Item_InvalidOwnership");
		Slot.Item.DisplayName = FText::FromString(TEXT("Ownership invalide"));
		Slot.Item.Quantity = 1;
		Slot.Item.Weight = 1.0f;
		Slot.Item.OwnerType = EGridItemOwnerType::CharacterInventory;
		Slot.Item.OwnerGuid = FGuid(20, 5, 7, 99);
		Slot.Item.OwnerCharacterIndex = 99;
		Slot.Item.EquipmentSlot = EGridEquipmentSlot::None;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGMON205ContextContractTest, "Grimrock.MON20.5.CustomRecruit.ContextContract", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON205ContextContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UEnum* ContextEnum = StaticEnum<ERPGCharacterCreationContext>();
	TestNotNull(TEXT("Character creation context enum is reflected"), ContextEnum);
	if (ContextEnum)
	{
		TestEqual(TEXT("New Game context keeps the first value"), static_cast<int64>(ERPGCharacterCreationContext::NewGameMainHero), static_cast<int64>(0));
		TestEqual(TEXT("Custom Recruit context is available"), static_cast<int64>(ERPGCharacterCreationContext::CustomRecruit), static_cast<int64>(1));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON205ValidCreateAndRecruitTest, "Grimrock.MON20.5.CustomRecruit.ValidCreateAndRecruit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON205ValidCreateAndRecruitTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPGCustomRecruitMON205Tests;

	UGridPartyInventoryComponent* Inventory = MakeParty(3);
	URPGRaceAsset* Race = MakeRace();
	URPGClassAsset* CharacterClass = MakeClass();
	const FRPGCharacterCreationRequest Request = MakeRequest(Race, CharacterClass);

	FRPGCustomRecruitResult Result;
	TestTrue(TEXT("Custom recruit transaction commits"), FRPGCustomRecruitService::TryCreateAndRecruit(Inventory, Request, Result));
	TestTrue(TEXT("Result is committed"), Result.bCommitted);
	TestEqual(TEXT("Active count before"), Result.ActiveCountBefore, 1);
	TestEqual(TEXT("Active count after"), Result.ActiveCountAfter, 2);
	TestEqual(TEXT("Recruit active index"), Result.CharacterIndex, 1);
	TestTrue(TEXT("Recruit CharacterId is valid"), Result.CharacterId.IsValid());
	TestEqual(TEXT("Temporary pool candidate is consumed"), Inventory->PartyInventoryState.CharacterPool.Num(), 0);
	TestEqual(TEXT("Active equipment stays aligned"), Inventory->PartyInventoryState.ActiveEquipment.Num(), 2);

	const FGridCharacterInventoryState& Recruit = Inventory->PartyInventoryState.ActiveCharacters[1];
	TestTrue(TEXT("Committed identity matches result"), Recruit.CharacterId == Result.CharacterId);
	TestEqual(TEXT("Display name is preserved"), Recruit.DisplayName.ToString(), FString(TEXT("Mercenaire")));
	TestEqual(TEXT("Recruit starts at level one"), Recruit.Level, 1);
	TestEqual(TEXT("Recruit starts without XP"), Recruit.Experience, 0);
	TestEqual(TEXT("Recruit inventory size uses component default"), Recruit.InventorySlots.Num(), 6);
	TestEqual(TEXT("Recruit hotbar is structurally initialized"), Recruit.CombatHotbarSlots.Num(), FGridCombatHotbarBinding::SlotCount);

	FString OwnershipError;
	TestTrue(TEXT("Committed party ownership remains valid"), Inventory->ValidateInventoryOwnership(OwnershipError));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON205PartyFullAtomicRejectTest, "Grimrock.MON20.5.CustomRecruit.PartyFullAtomicReject",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON205PartyFullAtomicRejectTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPGCustomRecruitMON205Tests;

	UGridPartyInventoryComponent* Inventory = MakeParty(1);
	URPGRaceAsset* Race = MakeRace();
	URPGClassAsset* CharacterClass = MakeClass();
	const FRPGCharacterCreationRequest Request = MakeRequest(Race, CharacterClass);

	const FGuid HeroId = Inventory->PartyInventoryState.ActiveCharacters[0].CharacterId;

	FRPGCustomRecruitResult Result;
	TestFalse(TEXT("Full party rejects custom recruit"), FRPGCustomRecruitService::TryCreateAndRecruit(Inventory, Request, Result));
	TestTrue(TEXT("Reject reason is PartyFull"), Result.RejectReason == ERPGCustomRecruitRejectReason::PartyFull);
	TestEqual(TEXT("Active party is unchanged"), Inventory->PartyInventoryState.ActiveCharacters.Num(), 1);
	TestEqual(TEXT("No hidden reserve candidate is created"), Inventory->PartyInventoryState.CharacterPool.Num(), 0);
	TestEqual(TEXT("Equipment array is unchanged"), Inventory->PartyInventoryState.ActiveEquipment.Num(), 1);
	TestTrue(TEXT("Hero identity is unchanged"), Inventory->PartyInventoryState.ActiveCharacters[0].CharacterId == HeroId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON205InvalidRequestAtomicRejectTest, "Grimrock.MON20.5.CustomRecruit.InvalidRequestAtomicReject",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON205InvalidRequestAtomicRejectTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPGCustomRecruitMON205Tests;

	UGridPartyInventoryComponent* Inventory = MakeParty(3);
	URPGRaceAsset* Race = MakeRace();
	URPGClassAsset* CharacterClass = MakeClass();
	FRPGCharacterCreationRequest Request = MakeRequest(Race, CharacterClass);
	Request.DisplayName = FText::GetEmpty();

	FRPGCustomRecruitResult Result;
	TestFalse(TEXT("Empty name rejects custom recruit"), FRPGCustomRecruitService::TryCreateAndRecruit(Inventory, Request, Result));
	TestTrue(TEXT("Reject reason is InvalidName"), Result.RejectReason == ERPGCustomRecruitRejectReason::InvalidName);
	TestEqual(TEXT("Active party is unchanged"), Inventory->PartyInventoryState.ActiveCharacters.Num(), 1);
	TestEqual(TEXT("Pool is unchanged"), Inventory->PartyInventoryState.CharacterPool.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON205UniqueCharacterIdentityTest, "Grimrock.MON20.5.CustomRecruit.UniqueCharacterIdentity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON205UniqueCharacterIdentityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPGCustomRecruitMON205Tests;

	UGridPartyInventoryComponent* Inventory = MakeParty(3);
	const FGuid HeroId = Inventory->PartyInventoryState.ActiveCharacters[0].CharacterId;
	const FGuid ExistingPoolId(20, 5, 4, 40);
	Inventory->PartyInventoryState.CharacterPool.Add(MakeCharacter(ExistingPoolId, TEXT("ReserveExistante")));

	URPGRaceAsset* Race = MakeRace();
	URPGClassAsset* CharacterClass = MakeClass();
	const FRPGCharacterCreationRequest Request = MakeRequest(Race, CharacterClass);

	FRPGCustomRecruitResult Result;
	TestTrue(TEXT("Custom recruit commits with an existing pool entry"), FRPGCustomRecruitService::TryCreateAndRecruit(Inventory, Request, Result));
	TestTrue(TEXT("Generated identity is valid"), Result.CharacterId.IsValid());
	TestTrue(TEXT("Generated identity differs from hero"), Result.CharacterId != HeroId);
	TestTrue(TEXT("Generated identity differs from existing pool candidate"), Result.CharacterId != ExistingPoolId);
	TestEqual(TEXT("Unrelated pool candidate remains"), Inventory->PartyInventoryState.CharacterPool.Num(), 1);
	TestTrue(TEXT("Unrelated pool identity is preserved"), Inventory->PartyInventoryState.CharacterPool[0].CharacterId == ExistingPoolId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON205AllocatedAttributesPreservedTest, "Grimrock.MON20.5.CustomRecruit.AllocatedAttributesPreserved",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON205AllocatedAttributesPreservedTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPGCustomRecruitMON205Tests;

	UGridPartyInventoryComponent* Inventory = MakeParty(3);
	URPGRaceAsset* Race = MakeRace();
	Race->AttributeBonuses = FRPGAttributes(1, 0, 1, 0, 0, 0);
	URPGClassAsset* CanonicalClass = MakeClass();
	URPGClassAsset* AllocatedClass = DuplicateObject<URPGClassAsset>(CanonicalClass, GetTransientPackage());
	AllocatedClass->BaseAttributes = FRPGAttributes(12, 9, 11, 8, 10, 10);

	FRPGCharacterCreationRequest Request = MakeRequest(Race, AllocatedClass);
	Request.CombatActionSourceClassDefinition = CanonicalClass;

	FRPGCustomRecruitResult Result;
	TestTrue(TEXT("Allocated custom recruit commits"), FRPGCustomRecruitService::TryCreateAndRecruit(Inventory, Request, Result));

	const FGridCharacterInventoryState& Recruit = Inventory->PartyInventoryState.ActiveCharacters[Result.CharacterIndex];
	TestEqual(TEXT("Allocated Strength plus race bonus"), Recruit.Attributes.Strength, 13);
	TestEqual(TEXT("Allocated Dexterity"), Recruit.Attributes.Dexterity, 9);
	TestEqual(TEXT("Allocated Constitution plus race bonus"), Recruit.Attributes.Constitution, 12);
	TestEqual(TEXT("Allocated Intelligence"), Recruit.Attributes.Intelligence, 8);
	TestTrue(TEXT("Persistent class source remains canonical"), Recruit.ClassDefinition.Get() == CanonicalClass);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON205VisualSelectionPreservedTest, "Grimrock.MON20.5.CustomRecruit.VisualSelectionPreserved",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON205VisualSelectionPreservedTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPGCustomRecruitMON205Tests;

	UGridPartyInventoryComponent* Inventory = MakeParty(3);
	URPGRaceAsset* Race = MakeRace();
	URPGClassAsset* CharacterClass = MakeClass();
	FRPGCharacterCreationRequest Request = MakeRequest(Race, CharacterClass);
	Request.PortraitGender = ERPGCharacterPortraitGender::Female;
	Request.PortraitVariantId = TEXT("Portrait_F_02");
	Request.Portrait = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/Tests/T_MON205_Portrait.T_MON205_Portrait")));
	Request.ClassIcon = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/Tests/T_MON205_ClassIcon.T_MON205_ClassIcon")));

	FRPGCustomRecruitResult Result;
	TestTrue(TEXT("Visual custom recruit commits"), FRPGCustomRecruitService::TryCreateAndRecruit(Inventory, Request, Result));

	const FGridCharacterInventoryState& Recruit = Inventory->PartyInventoryState.ActiveCharacters[Result.CharacterIndex];
	TestTrue(TEXT("Portrait gender is preserved"), Recruit.PortraitGender == ERPGCharacterPortraitGender::Female);
	TestEqual(TEXT("Portrait variant is preserved"), Recruit.PortraitVariantId, FName(TEXT("Portrait_F_02")));
	TestTrue(TEXT("Portrait soft path is preserved"), Recruit.Portrait.ToSoftObjectPath() == Request.Portrait.ToSoftObjectPath());
	TestTrue(TEXT("Class icon soft path is preserved"), Recruit.ClassIcon.ToSoftObjectPath() == Request.ClassIcon.ToSoftObjectPath());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON205RecruitmentRollbackLeavesNoPoolCandidateTest,
	"Grimrock.MON20.5.CustomRecruit.RecruitmentRollbackLeavesNoPoolCandidate", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON205RecruitmentRollbackLeavesNoPoolCandidateTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPGCustomRecruitMON205Tests;

	UGridPartyInventoryComponent* Inventory = MakeParty(3);
	PutMalformedOwnedItem(Inventory->PartyInventoryState.ActiveCharacters[0]);

	URPGRaceAsset* Race = MakeRace();
	URPGClassAsset* CharacterClass = MakeClass();
	const FRPGCharacterCreationRequest Request = MakeRequest(Race, CharacterClass);

	const FGuid HeroId = Inventory->PartyInventoryState.ActiveCharacters[0].CharacterId;

	FRPGCustomRecruitResult Result;
	TestFalse(TEXT("Downstream ownership failure rejects custom recruit"), FRPGCustomRecruitService::TryCreateAndRecruit(Inventory, Request, Result));
	TestTrue(TEXT("Reject reason is RecruitmentFailed"), Result.RejectReason == ERPGCustomRecruitRejectReason::RecruitmentFailed);
	TestEqual(TEXT("Outer rollback restores active count"), Inventory->PartyInventoryState.ActiveCharacters.Num(), 1);
	TestEqual(TEXT("Outer rollback removes temporary pool candidate"), Inventory->PartyInventoryState.CharacterPool.Num(), 0);
	TestEqual(TEXT("Outer rollback restores equipment count"), Inventory->PartyInventoryState.ActiveEquipment.Num(), 1);
	TestTrue(TEXT("Outer rollback preserves hero identity"), Inventory->PartyInventoryState.ActiveCharacters[0].CharacterId == HeroId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON205InitialHeroStatePreservedTest, "Grimrock.MON20.5.CustomRecruit.InitialHeroStatePreserved",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON205InitialHeroStatePreservedTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPGCustomRecruitMON205Tests;

	UGridPartyInventoryComponent* Inventory = MakeParty(3);
	const FGuid HeroId = Inventory->PartyInventoryState.ActiveCharacters[0].CharacterId;
	const FString HeroName = Inventory->PartyInventoryState.ActiveCharacters[0].DisplayName.ToString();
	const int32 SelectedBefore = Inventory->PartyInventoryState.SelectedCharacterIndex;

	URPGRaceAsset* Race = MakeRace();
	URPGClassAsset* CharacterClass = MakeClass();
	const FRPGCharacterCreationRequest Request = MakeRequest(Race, CharacterClass, TEXT("DeuxiemeMembre"));

	FRPGCustomRecruitResult Result;
	TestTrue(TEXT("Custom recruit commits"), FRPGCustomRecruitService::TryCreateAndRecruit(Inventory, Request, Result));

	TestTrue(TEXT("Initial hero CharacterId is preserved"), Inventory->PartyInventoryState.ActiveCharacters[0].CharacterId == HeroId);
	TestEqual(TEXT("Initial hero display name is preserved"), Inventory->PartyInventoryState.ActiveCharacters[0].DisplayName.ToString(), HeroName);
	TestEqual(TEXT("Selected hero index is preserved"), Inventory->PartyInventoryState.SelectedCharacterIndex, SelectedBefore);
	TestTrue(TEXT("Initial creation remains completed"), Inventory->PartyInventoryState.bInitialCharacterCreationCompleted);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
