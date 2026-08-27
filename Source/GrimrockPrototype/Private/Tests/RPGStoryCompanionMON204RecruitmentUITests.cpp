#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "RPG/RPGClassAsset.h"
#include "RPG/RPGPartyRecruitmentService.h"
#include "RPG/RPGRaceAsset.h"
#include "RPG/RPGStoryCompanionAsset.h"
#include "RPG/RPGStoryCompanionService.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "UI/RPGStoryCompanionRecruitmentWidget.h"

namespace GridMON204RecruitmentUITests
{
	URPGRaceAsset* MakeRace()
	{
		URPGRaceAsset* Race = NewObject<URPGRaceAsset>();
		Race->RaceId = TEXT("Human");
		Race->DisplayName = FText::FromString(TEXT("Humain"));
		Race->Description = FText::FromString(TEXT("Polyvalent et adaptable."));
		return Race;
	}

	URPGClassAsset* MakeClass()
	{
		URPGClassAsset* ClassDefinition = NewObject<URPGClassAsset>();
		ClassDefinition->ClassId = TEXT("Rogue");
		ClassDefinition->DisplayName = FText::FromString(TEXT("Voleur"));
		ClassDefinition->Description = FText::FromString(TEXT("Éclaireur agile et discret."));
		ClassDefinition->BaseAttributes = FRPGAttributes(10, 12, 10, 10, 10, 10);
		ClassDefinition->HealthAtLevelOne = 8;
		ClassDefinition->HealthPerLevel = 4;
		ClassDefinition->ManaAtLevelOne = 0;
		ClassDefinition->ManaPerLevel = 0;
		return ClassDefinition;
	}

	URPGStoryCompanionAsset* MakeCompanion()
	{
		URPGStoryCompanionAsset* Companion = NewObject<URPGStoryCompanionAsset>();
		Companion->CompanionId = TEXT("Companion_Scout");
		Companion->CharacterId = FGuid(20, 4, 2, 1);
		Companion->DisplayName = FText::FromString(TEXT("Serana de Valombre"));
		Companion->ShortDescription = FText::FromString(TEXT("Ancienne éclaireuse."));
		Companion->RaceDefinition = MakeRace();
		Companion->ClassDefinition = MakeClass();
		Companion->Level = 3;
		Companion->PortraitVariantId = TEXT("Default");
		Companion->RecruitmentConditionText = FText::FromString(TEXT("Avoir ouvert la porte nord."));
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
		Character.Resources.CurrentHealth = 10;
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
		Inventory->PartyInventoryState.ActiveCharacters.Add(MakeCharacter(FGuid(20, 4, 2, 0), TEXT("Human"), TEXT("Warrior"), TEXT("MainHero")));
		Inventory->PartyInventoryState.ActiveEquipment.SetNum(1);
		return Inventory;
	}

	URPGStoryCompanionRecruitmentWidget* MakeWidget(UGridPartyInventoryComponent* Inventory, URPGStoryCompanionAsset* Companion)
	{
		URPGStoryCompanionRecruitmentWidget* Widget = NewObject<URPGStoryCompanionRecruitmentWidget>();
		Widget->InitializeRecruitmentWidget(Inventory, Companion);
		return Widget;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON204RecruitmentViewProjectionTest, "Grimrock.MON20.4.RecruitmentUI.ViewProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON204RecruitmentViewProjectionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMON204RecruitmentUITests;

	UGridPartyInventoryComponent* Inventory = MakeParty(2);
	URPGStoryCompanionAsset* Companion = MakeCompanion();
	URPGStoryCompanionRecruitmentWidget* Widget = NewObject<URPGStoryCompanionRecruitmentWidget>();

	TestTrue(TEXT("Valid recruitment widget initializes"), Widget->InitializeRecruitmentWidget(Inventory, Companion));
	TestEqual(TEXT("Companion id is projected"), Widget->View.CompanionId, Companion->CompanionId);
	TestTrue(TEXT("Character id is projected"), Widget->View.CharacterId == Companion->CharacterId);
	TestEqual(TEXT("Display name is projected"), Widget->View.DisplayName.ToString(), Companion->DisplayName.ToString());
	TestEqual(TEXT("Race name is projected"), Widget->View.RaceName.ToString(), FString(TEXT("Humain")));
	TestEqual(TEXT("Class name is projected"), Widget->View.ClassName.ToString(), FString(TEXT("Voleur")));
	TestEqual(TEXT("Level is projected"), Widget->View.Level, 3);
	TestEqual(TEXT("Initial state is Ready"), static_cast<int32>(Widget->View.State), static_cast<int32>(ERPGStoryCompanionRecruitmentState::Ready));
	TestTrue(TEXT("Recruit action is initially available"), Widget->View.bCanRecruit);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON204NominalRecruitmentTest, "Grimrock.MON20.4.RecruitmentUI.NominalRecruitment",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON204NominalRecruitmentTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMON204RecruitmentUITests;

	UGridPartyInventoryComponent* Inventory = MakeParty(2);
	URPGStoryCompanionAsset* Companion = MakeCompanion();
	URPGStoryCompanionRecruitmentWidget* Widget = MakeWidget(Inventory, Companion);

	bool bAccepted = false;
	bool bClosed = false;
	Widget->OnAccepted().AddLambda(
		[&bAccepted](URPGStoryCompanionRecruitmentWidget*)
		{
			bAccepted = true;
		});
	Widget->OnClosed().AddLambda(
		[&bClosed](URPGStoryCompanionRecruitmentWidget*)
		{
			bClosed = true;
		});

	TestTrue(TEXT("Recruitment commits through MON20.3 then MON20.2"), Widget->TryRecruit());
	TestEqual(TEXT("Active party gains exactly one character"), Inventory->PartyInventoryState.ActiveCharacters.Num(), 2);
	TestEqual(TEXT("Pool is empty after activation"), Inventory->PartyInventoryState.CharacterPool.Num(), 0);
	TestEqual(
		TEXT("Widget records recruited state"), static_cast<int32>(Widget->View.State), static_cast<int32>(ERPGStoryCompanionRecruitmentState::Recruited));
	TestTrue(TEXT("Accepted delegate fires"), bAccepted);
	TestTrue(TEXT("Closed delegate fires"), bClosed);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMON204AlreadyInPoolTest, "Grimrock.MON20.4.RecruitmentUI.AlreadyInPool", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON204AlreadyInPoolTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMON204RecruitmentUITests;

	UGridPartyInventoryComponent* Inventory = MakeParty(2);
	URPGStoryCompanionAsset* Companion = MakeCompanion();
	FRPGStoryCompanionRegistrationResult Registration;
	TestTrue(TEXT("Fixture candidate registers in pool"), FRPGStoryCompanionService::EnsureCandidateRegistered(Inventory, Companion, Registration));

	URPGStoryCompanionRecruitmentWidget* Widget = MakeWidget(Inventory, Companion);
	TestTrue(TEXT("View recognizes existing pool candidate"), Widget->View.bCandidateAlreadyRegistered);
	TestTrue(TEXT("Existing pool candidate recruits"), Widget->TryRecruit());
	TestEqual(TEXT("No duplicate pool candidate remains"), Inventory->PartyInventoryState.CharacterPool.Num(), 0);
	TestEqual(TEXT("Active party contains two characters"), Inventory->PartyInventoryState.ActiveCharacters.Num(), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON204AlreadyActiveNoDoubleRecruitmentTest, "Grimrock.MON20.4.RecruitmentUI.AlreadyActiveNoDoubleRecruitment",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON204AlreadyActiveNoDoubleRecruitmentTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMON204RecruitmentUITests;

	UGridPartyInventoryComponent* Inventory = MakeParty(2);
	URPGStoryCompanionAsset* Companion = MakeCompanion();
	FRPGStoryCompanionRegistrationResult Registration;
	FRPGPartyRecruitmentResult Recruitment;
	TestTrue(TEXT("Fixture registration succeeds"), FRPGStoryCompanionService::EnsureCandidateRegistered(Inventory, Companion, Registration));
	TestTrue(TEXT("Fixture recruitment succeeds"), FRPGPartyRecruitmentService::TryRecruitFromPool(Inventory, Companion->CharacterId, Recruitment));

	URPGStoryCompanionRecruitmentWidget* Widget = MakeWidget(Inventory, Companion);
	TestEqual(TEXT("Already-active state is projected"), static_cast<int32>(Widget->View.State),
		static_cast<int32>(ERPGStoryCompanionRecruitmentState::AlreadyActive));
	TestFalse(TEXT("Recruit button is disabled"), Widget->View.bCanRecruit);
	TestFalse(TEXT("Second recruit call does not commit"), Widget->TryRecruit());
	TestEqual(TEXT("Active count remains unchanged"), Inventory->PartyInventoryState.ActiveCharacters.Num(), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMON204PartyFullAtomicTest, "Grimrock.MON20.4.RecruitmentUI.PartyFull", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON204PartyFullAtomicTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMON204RecruitmentUITests;

	UGridPartyInventoryComponent* Inventory = MakeParty(1);
	URPGStoryCompanionAsset* Companion = MakeCompanion();
	URPGStoryCompanionRecruitmentWidget* Widget = MakeWidget(Inventory, Companion);

	TestFalse(TEXT("Full party rejects active recruitment"), Widget->TryRecruit());
	TestEqual(TEXT("Active party is unchanged"), Inventory->PartyInventoryState.ActiveCharacters.Num(), 1);
	TestEqual(TEXT("Registered candidate remains in reserve"), Inventory->PartyInventoryState.CharacterPool.Num(), 1);
	TestTrue(TEXT("Reserve candidate keeps companion identity"), Inventory->PartyInventoryState.CharacterPool[0].CharacterId == Companion->CharacterId);
	TestEqual(
		TEXT("Widget exposes PartyFull state"), static_cast<int32>(Widget->View.State), static_cast<int32>(ERPGStoryCompanionRecruitmentState::PartyFull));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON204IdentityCollisionAtomicTest, "Grimrock.MON20.4.RecruitmentUI.IdentityCollision",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON204IdentityCollisionAtomicTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMON204RecruitmentUITests;

	UGridPartyInventoryComponent* Inventory = MakeParty(2);
	URPGStoryCompanionAsset* Companion = MakeCompanion();
	Inventory->PartyInventoryState.ActiveCharacters[0].CharacterId = Companion->CharacterId;

	URPGStoryCompanionRecruitmentWidget* Widget = MakeWidget(Inventory, Companion);
	TestFalse(TEXT("Identity collision rejects recruitment"), Widget->TryRecruit());
	TestEqual(TEXT("Active party is unchanged"), Inventory->PartyInventoryState.ActiveCharacters.Num(), 1);
	TestEqual(TEXT("Collision does not create pool candidate"), Inventory->PartyInventoryState.CharacterPool.Num(), 0);
	TestEqual(TEXT("Widget exposes failure state"), static_cast<int32>(Widget->View.State), static_cast<int32>(ERPGStoryCompanionRecruitmentState::Failed));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON204InvalidDefinitionTest, "Grimrock.MON20.4.RecruitmentUI.InvalidDefinition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON204InvalidDefinitionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMON204RecruitmentUITests;

	UGridPartyInventoryComponent* Inventory = MakeParty(2);
	URPGStoryCompanionAsset* Companion = MakeCompanion();
	Companion->CompanionId = NAME_None;

	URPGStoryCompanionRecruitmentWidget* Widget = NewObject<URPGStoryCompanionRecruitmentWidget>();
	TestFalse(TEXT("Invalid companion cannot initialize recruitment"), Widget->InitializeRecruitmentWidget(Inventory, Companion));
	TestEqual(TEXT("Invalid state is projected"), static_cast<int32>(Widget->View.State), static_cast<int32>(ERPGStoryCompanionRecruitmentState::Invalid));
	TestFalse(TEXT("Invalid view cannot recruit"), Widget->View.bCanRecruit);
	TestFalse(TEXT("Invalid request does not recruit"), Widget->TryRecruit());
	TestEqual(TEXT("No mutation reaches pool"), Inventory->PartyInventoryState.CharacterPool.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON204DeclineNoMutationTest, "Grimrock.MON20.4.RecruitmentUI.DeclineNoMutation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON204DeclineNoMutationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMON204RecruitmentUITests;

	UGridPartyInventoryComponent* Inventory = MakeParty(2);
	URPGStoryCompanionAsset* Companion = MakeCompanion();
	URPGStoryCompanionRecruitmentWidget* Widget = MakeWidget(Inventory, Companion);

	bool bDeclined = false;
	bool bClosed = false;
	Widget->OnDeclined().AddLambda(
		[&bDeclined](URPGStoryCompanionRecruitmentWidget*)
		{
			bDeclined = true;
		});
	Widget->OnClosed().AddLambda(
		[&bClosed](URPGStoryCompanionRecruitmentWidget*)
		{
			bClosed = true;
		});

	Widget->DeclineRecruitment();
	TestTrue(TEXT("Declined delegate fires"), bDeclined);
	TestTrue(TEXT("Closed delegate fires"), bClosed);
	TestEqual(TEXT("Decline leaves active party unchanged"), Inventory->PartyInventoryState.ActiveCharacters.Num(), 1);
	TestEqual(TEXT("Decline does not register candidate"), Inventory->PartyInventoryState.CharacterPool.Num(), 0);
	TestEqual(TEXT("Widget records declined state"), static_cast<int32>(Widget->View.State), static_cast<int32>(ERPGStoryCompanionRecruitmentState::Declined));
	return true;
}

#endif
