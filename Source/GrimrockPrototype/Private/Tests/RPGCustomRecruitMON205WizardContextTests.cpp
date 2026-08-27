#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/EditableText.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGRaceAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "UI/RPGCharacterCreationWidget.h"
#include "UI/RPGCharacterCreationWizardWidget.h"

namespace RPGCustomRecruitMON205WizardContextTests
{
	FGridCharacterInventoryState MakeHero()
	{
		FGridCharacterInventoryState Character;
		Character.CharacterId = FGuid(20, 5, 3, 1);
		Character.DisplayName = FText::FromString(TEXT("MainHero"));
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

	UGridPartyInventoryComponent* MakeCompletedParty(int32 MaxActiveCharacters = 2)
	{
		UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
		Inventory->DefaultInventorySlotCountPerCharacter = 6;
		Inventory->PartyInventoryState = FGridPartyInventoryState();
		Inventory->PartyInventoryState.MaxActiveCharacters = MaxActiveCharacters;
		Inventory->PartyInventoryState.bInitialCharacterCreationCompleted = true;
		Inventory->PartyInventoryState.SelectedCharacterIndex = 0;
		Inventory->PartyInventoryState.ActiveCharacters.Add(MakeHero());
		Inventory->PartyInventoryState.ActiveEquipment.SetNum(1);
		return Inventory;
	}

	UGridPartyInventoryComponent* MakeNewGameParty()
	{
		UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
		Inventory->DefaultInventorySlotCountPerCharacter = 6;
		Inventory->DefaultMaxActiveCharacters = 6;
		Inventory->PartyInventoryState = FGridPartyInventoryState();
		Inventory->PartyInventoryState.MaxActiveCharacters = 6;
		return Inventory;
	}

	URPGRaceAsset* MakeRace()
	{
		URPGRaceAsset* Race = NewObject<URPGRaceAsset>();
		Race->RaceId = TEXT("Human");
		Race->DisplayName = FText::FromString(TEXT("Humain"));
		Race->AttributeBonuses = FRPGAttributes(0, 0, 0, 0, 0, 0);
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

	template <typename TWidget>
	TWidget* ConfigureWidget(UGridPartyInventoryComponent* Inventory, URPGRaceAsset* Race, URPGClassAsset* CharacterClass, ERPGCharacterCreationContext Context,
		const TCHAR* Name = TEXT("Mercenaire"))
	{
		TWidget* Widget = NewObject<TWidget>();
		Widget->InventoryComponent = Inventory;
		Widget->RaceDefinition = Race;
		Widget->ClassDefinition = CharacterClass;
		Widget->CreationContext = Context;
		Widget->EditableText_Name = NewObject<UEditableText>(Widget);
		Widget->EditableText_Name->SetText(FText::FromString(Name));
		return Widget;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON205WizardContextDefaultTest, "Grimrock.MON20.5.CustomRecruit.WizardContextDefault",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON205WizardContextDefaultTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	URPGCharacterCreationWidget* Widget = NewObject<URPGCharacterCreationWidget>();
	TestTrue(TEXT("Default context remains New Game"), Widget->GetCreationContext() == ERPGCharacterCreationContext::NewGameMainHero);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON205ContextGateCompletedPartyTest, "Grimrock.MON20.5.CustomRecruit.ContextGateCompletedParty",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON205ContextGateCompletedPartyTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPGCustomRecruitMON205WizardContextTests;

	UGridPartyInventoryComponent* Inventory = MakeCompletedParty(2);
	URPGCharacterCreationWidget* Widget =
		ConfigureWidget<URPGCharacterCreationWidget>(Inventory, MakeRace(), MakeClass(), ERPGCharacterCreationContext::CustomRecruit);

	TestTrue(TEXT("Completed party accepts CustomRecruit submit state"), Widget->CanSubmitCharacterCreation());

	Widget->CreationContext = ERPGCharacterCreationContext::NewGameMainHero;
	TestFalse(TEXT("Same completed party rejects NewGame submit state"), Widget->CanSubmitCharacterCreation());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON205ContextGateIncompletePartyTest, "Grimrock.MON20.5.CustomRecruit.ContextGateIncompleteParty",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON205ContextGateIncompletePartyTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPGCustomRecruitMON205WizardContextTests;

	UGridPartyInventoryComponent* Inventory = MakeNewGameParty();
	URPGCharacterCreationWidget* Widget =
		ConfigureWidget<URPGCharacterCreationWidget>(Inventory, MakeRace(), MakeClass(), ERPGCharacterCreationContext::CustomRecruit);

	TestFalse(TEXT("Custom recruit cannot precede initial hero creation"), Widget->CanSubmitCharacterCreation());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON205ContextGatePartyFullTest, "Grimrock.MON20.5.CustomRecruit.ContextGatePartyFull",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON205ContextGatePartyFullTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPGCustomRecruitMON205WizardContextTests;

	UGridPartyInventoryComponent* Inventory = MakeCompletedParty(1);
	URPGCharacterCreationWidget* Widget =
		ConfigureWidget<URPGCharacterCreationWidget>(Inventory, MakeRace(), MakeClass(), ERPGCharacterCreationContext::CustomRecruit);

	TestFalse(TEXT("Full active party disables CustomRecruit submit"), Widget->CanSubmitCharacterCreation());
	TestEqual(TEXT("Full-party gate does not create a pool candidate"), Inventory->PartyInventoryState.CharacterPool.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON205WizardCustomRecruitSubmitTest, "Grimrock.MON20.5.CustomRecruit.WizardCustomRecruitSubmit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON205WizardCustomRecruitSubmitTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPGCustomRecruitMON205WizardContextTests;

	UGridPartyInventoryComponent* Inventory = MakeCompletedParty(2);
	const FGuid InitialHeroId = Inventory->PartyInventoryState.ActiveCharacters[0].CharacterId;

	URPGCharacterCreationWizardWidget* Wizard =
		ConfigureWidget<URPGCharacterCreationWizardWidget>(Inventory, MakeRace(), MakeClass(), ERPGCharacterCreationContext::CustomRecruit, TEXT("Ariane"));
	Wizard->ResetAttributeAllocationToClassDefinition();

	int32 CommitCallbackCount = 0;
	int32 CommittedCharacterIndex = INDEX_NONE;
	Wizard->OnCustomRecruitCommitted().AddLambda(
		[&CommitCallbackCount, &CommittedCharacterIndex](URPGCharacterCreationWidget* SourceWidget, int32 CharacterIndex)
		{
			if (SourceWidget)
			{
				++CommitCallbackCount;
				CommittedCharacterIndex = CharacterIndex;
			}
		});

	TestTrue(TEXT("Wizard custom submit commits recruitment"), Wizard->SubmitCharacterCreation());
	TestEqual(TEXT("Active party gains exactly one recruit"), Inventory->PartyInventoryState.ActiveCharacters.Num(), 2);
	TestTrue(TEXT("Initial hero identity is preserved"), Inventory->PartyInventoryState.ActiveCharacters[0].CharacterId == InitialHeroId);
	TestEqual(TEXT("Temporary pool staging is consumed"), Inventory->PartyInventoryState.CharacterPool.Num(), 0);
	TestEqual(TEXT("Custom recruit commit delegate fires once"), CommitCallbackCount, 1);
	TestEqual(TEXT("Delegate reports recruited active index"), CommittedCharacterIndex, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON205WizardCustomCancelNoMutationTest, "Grimrock.MON20.5.CustomRecruit.WizardCustomCancelNoMutation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON205WizardCustomCancelNoMutationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPGCustomRecruitMON205WizardContextTests;

	UGridPartyInventoryComponent* Inventory = MakeCompletedParty(2);
	URPGCharacterCreationWizardWidget* Wizard =
		ConfigureWidget<URPGCharacterCreationWizardWidget>(Inventory, MakeRace(), MakeClass(), ERPGCharacterCreationContext::CustomRecruit);

	int32 CancelCallbackCount = 0;
	Wizard->OnCustomRecruitCancelled().AddLambda(
		[&CancelCallbackCount](URPGCharacterCreationWidget* SourceWidget)
		{
			if (SourceWidget)
			{
				++CancelCallbackCount;
			}
		});

	const int32 ActiveCountBefore = Inventory->PartyInventoryState.ActiveCharacters.Num();
	const int32 PoolCountBefore = Inventory->PartyInventoryState.CharacterPool.Num();

	Wizard->CancelWizard();

	TestEqual(TEXT("Custom cancel delegate fires once"), CancelCallbackCount, 1);
	TestEqual(TEXT("Custom cancel does not change active party"), Inventory->PartyInventoryState.ActiveCharacters.Num(), ActiveCountBefore);
	TestEqual(TEXT("Custom cancel does not change candidate pool"), Inventory->PartyInventoryState.CharacterPool.Num(), PoolCountBefore);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON205WizardNewGameSubmitRegressionTest, "Grimrock.MON20.5.CustomRecruit.WizardNewGameSubmitRegression",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON205WizardNewGameSubmitRegressionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPGCustomRecruitMON205WizardContextTests;

	UGridPartyInventoryComponent* Inventory = MakeNewGameParty();
	URPGCharacterCreationWizardWidget* Wizard =
		ConfigureWidget<URPGCharacterCreationWizardWidget>(Inventory, MakeRace(), MakeClass(), ERPGCharacterCreationContext::NewGameMainHero, TEXT("Héros"));
	Wizard->ResetAttributeAllocationToClassDefinition();

	int32 CustomCommitCallbackCount = 0;
	Wizard->OnCustomRecruitCommitted().AddLambda(
		[&CustomCommitCallbackCount](URPGCharacterCreationWidget* SourceWidget, int32 CharacterIndex)
		{
			(void)CharacterIndex;
			if (SourceWidget)
			{
				++CustomCommitCallbackCount;
			}
		});

	TestTrue(TEXT("New Game submit remains functional"), Wizard->SubmitCharacterCreation());
	TestTrue(TEXT("Initial character creation is marked completed"), Inventory->HasCompletedInitialCharacterCreation());
	TestEqual(TEXT("New Game creates exactly one active hero"), Inventory->PartyInventoryState.ActiveCharacters.Num(), 1);
	TestEqual(TEXT("New Game does not emit custom recruit callback"), CustomCommitCallbackCount, 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
