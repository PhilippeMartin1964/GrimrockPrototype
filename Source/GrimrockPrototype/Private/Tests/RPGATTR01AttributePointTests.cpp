#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "RPG/RPGAttributePointService.h"
#include "RPG/RPGAuthoringIdentityResolver.h"
#include "RPG/RPGCharacterRulesLibrary.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGRaceAsset.h"
#include "Runtime/GridInventoryTypes.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "UI/GridCharacterSheetWidget.h"
#include "UI/RPGProgressionFeedbackService.h"
#include "UObject/UnrealType.h"

namespace RPGATTR01Tests
{
	struct FRuntimeCacheGuard
	{
		FRuntimeCacheGuard()
		{
			FRPGAuthoringIdentityResolver::ResetRuntimeCache();
		}

		~FRuntimeCacheGuard()
		{
			FRPGAuthoringIdentityResolver::ResetRuntimeCache();
		}
	};

	UGridPartyInventoryComponent* MakeInventory(
		int32 Level,
		URPGClassAsset*& OutClassDefinition,
		URPGRaceAsset*& OutRaceDefinition)
	{
		UGridPartyInventoryComponent* Component = NewObject<UGridPartyInventoryComponent>();

		OutClassDefinition = NewObject<URPGClassAsset>(Component);
		OutClassDefinition->ClassId = TEXT("ATTR01_Class");
		OutClassDefinition->DisplayName = FText::FromString(TEXT("ATTR01 Class"));
		OutClassDefinition->BaseAttributes = FRPGAttributes{ 12, 12, 13, 12, 12, 12 };
		OutClassDefinition->HealthAtLevelOne = 20;
		OutClassDefinition->HealthPerLevel = 5;
		OutClassDefinition->ManaAtLevelOne = 6;
		OutClassDefinition->ManaPerLevel = 2;

		OutRaceDefinition = NewObject<URPGRaceAsset>(Component);
		OutRaceDefinition->RaceId = TEXT("ATTR01_Race");
		OutRaceDefinition->DisplayName = FText::FromString(TEXT("ATTR01 Race"));
		OutRaceDefinition->AttributeBonuses = FRPGAttributes{ 0, 0, 0, 0, 0, 0 };

		FRPGAuthoringIdentityResolver::RememberClassDefinition(OutClassDefinition);
		FRPGAuthoringIdentityResolver::RememberRaceDefinition(OutRaceDefinition);

		FGridCharacterInventoryState Character;
		Character.CharacterId = FGuid::NewGuid();
		Character.DisplayName = FText::FromString(TEXT("Elias"));
		Character.ClassId = OutClassDefinition->ClassId;
		Character.ClassDisplayName = OutClassDefinition->DisplayName;
		Character.ClassDefinition = OutClassDefinition;
		Character.RaceId = OutRaceDefinition->RaceId;
		Character.RaceDisplayName = OutRaceDefinition->DisplayName;
		Character.Level = Level;
		Character.Experience = URPGCharacterRulesLibrary::GetCumulativeExperienceRequiredForLevel(Level);
		Character.Attributes = URPGCharacterRulesLibrary::AddAttributes(
			OutClassDefinition->BaseAttributes,
			OutRaceDefinition->AttributeBonuses);
		Character.DerivedStats =
			URPGCharacterRulesLibrary::CalculateDerivedStats(
				Character.Attributes,
				OutClassDefinition,
				Character.Level);
		Character.Resources =
			URPGCharacterRulesLibrary::InitializeCharacterResources(
				Character.DerivedStats,
				OutClassDefinition);

		Component->PartyInventoryState.ActiveCharacters.Add(Character);
		Component->PartyInventoryState.ActiveEquipment.SetNum(1);
		Component->PartyInventoryState.SelectedCharacterIndex = 0;
		return Component;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGATTR01GrantScheduleTest,
	"Grimrock.RPG.ATTR01.Economy.GrantSchedule",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGATTR01GrantScheduleTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	TestEqual(TEXT("Level 1 grants no Attribute Point"), FRPGAttributePointService::GetTotalPointsGranted(1), 0);
	TestEqual(TEXT("Level 3 grants no Attribute Point"), FRPGAttributePointService::GetTotalPointsGranted(3), 0);
	TestEqual(TEXT("Level 4 grants one Attribute Point"), FRPGAttributePointService::GetTotalPointsGranted(4), 1);
	TestEqual(TEXT("Level 8 grants two Attribute Points"), FRPGAttributePointService::GetTotalPointsGranted(8), 2);
	TestEqual(TEXT("Level 12 grants three Attribute Points"), FRPGAttributePointService::GetTotalPointsGranted(12), 3);
	TestEqual(TEXT("Level 16 grants four Attribute Points"), FRPGAttributePointService::GetTotalPointsGranted(16), 4);
	TestEqual(TEXT("Level 20 grants five Attribute Points"), FRPGAttributePointService::GetTotalPointsGranted(20), 5);
	TestEqual(TEXT("Invalid level zero grants nothing"), FRPGAttributePointService::GetTotalPointsGranted(0), 0);
	TestEqual(TEXT("Invalid level twenty-one grants nothing"), FRPGAttributePointService::GetTotalPointsGranted(21), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGATTR01DerivedBalanceTest,
	"Grimrock.RPG.ATTR01.Economy.DerivedBalance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGATTR01DerivedBalanceTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPGATTR01Tests;
	FRuntimeCacheGuard Guard;

	URPGClassAsset* ClassDefinition = nullptr;
	URPGRaceAsset* RaceDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeInventory(8, ClassDefinition, RaceDefinition);
	FGridCharacterInventoryState& Character = Component->PartyInventoryState.ActiveCharacters[0];

	FRPGAttributePointBalance Balance;
	TestTrue(TEXT("Baseline balance resolves"), FRPGAttributePointService::TryGetBalance(Character, Balance));
	TestEqual(TEXT("Level eight grants two points"), Balance.GrantedPoints, 2);
	TestEqual(TEXT("Fresh character has spent zero progression points"), Balance.SpentPoints, 0);
	TestEqual(TEXT("Fresh character keeps both points"), Balance.RemainingPoints, 2);

	// Character creation may redistribute the fixed class budget. The total,
	// not the distribution, defines the progression baseline.
	++Character.Attributes.Strength;
	--Character.Attributes.Dexterity;
	TestTrue(TEXT("Redistributed creation budget still resolves"), FRPGAttributePointService::TryGetBalance(Character, Balance));
	TestEqual(TEXT("Redistribution does not spend progression points"), Balance.SpentPoints, 0);

	Character.Attributes.Wisdom += 2;
	TestTrue(TEXT("Two durable increases resolve"), FRPGAttributePointService::TryGetBalance(Character, Balance));
	TestEqual(TEXT("Two increases spend two points"), Balance.SpentPoints, 2);
	TestEqual(TEXT("No point remains"), Balance.RemainingPoints, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGATTR01PurchaseTest,
	"Grimrock.RPG.ATTR01.Mutation.Purchase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGATTR01PurchaseTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPGATTR01Tests;
	FRuntimeCacheGuard Guard;

	URPGClassAsset* ClassDefinition = nullptr;
	URPGRaceAsset* RaceDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeInventory(4, ClassDefinition, RaceDefinition);
	FGridCharacterInventoryState& Character = Component->PartyInventoryState.ActiveCharacters[0];

	const int32 PreviousConstitution = Character.Attributes.Constitution;
	const int32 PreviousMaxHealth = Character.DerivedStats.MaxHealth;
	Character.Resources.CurrentHealth = PreviousMaxHealth - 3;

	FRPGAttributePointMutationResult Result;
	TestTrue(
		TEXT("Level-four point can increase Constitution"),
		FRPGAttributePointService::TryPurchasePoint(
			Component,
			0,
			ERPGAttributePointTarget::Constitution,
			Result));

	TestTrue(TEXT("Mutation is committed"), Result.bCommitted);
	TestEqual(TEXT("Constitution increases by one"), Character.Attributes.Constitution, PreviousConstitution + 1);
	TestEqual(TEXT("One point is spent"), Result.SpentPoints, 1);
	TestEqual(TEXT("No point remains"), Result.RemainingPoints, 0);
	TestTrue(TEXT("Derived maximum health is rebuilt"), Character.DerivedStats.MaxHealth > PreviousMaxHealth);
	TestEqual(
		TEXT("Current health preserves the pre-allocation damage deficit"),
		Character.Resources.CurrentHealth,
		Character.DerivedStats.MaxHealth - 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGATTR01CapTest,
	"Grimrock.RPG.ATTR01.Mutation.Cap20",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGATTR01CapTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPGATTR01Tests;
	FRuntimeCacheGuard Guard;

	URPGClassAsset* ClassDefinition = nullptr;
	URPGRaceAsset* RaceDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeInventory(4, ClassDefinition, RaceDefinition);
	FGridCharacterInventoryState& Character = Component->PartyInventoryState.ActiveCharacters[0];

	ClassDefinition->BaseAttributes.Strength = 20;
	Character.Attributes.Strength = 20;
	Character.DerivedStats =
		URPGCharacterRulesLibrary::CalculateDerivedStats(Character.Attributes, ClassDefinition, Character.Level);

	TestEqual(
		TEXT("Base value twenty is capped"),
		FRPGAttributePointService::GetIncreaseAvailability(Character, ERPGAttributePointTarget::Strength),
		ERPGAttributePointMutationRejectReason::AttributeCapReached);

	FRPGAttributePointMutationResult Result;
	TestFalse(
		TEXT("Purchase above twenty is rejected"),
		FRPGAttributePointService::TryPurchasePoint(
			Component,
			0,
			ERPGAttributePointTarget::Strength,
			Result));
	TestEqual(TEXT("Rejection reason is the attribute cap"), Result.RejectReason, ERPGAttributePointMutationRejectReason::AttributeCapReached);
	TestEqual(TEXT("Strength stays at twenty"), Character.Attributes.Strength, 20);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGATTR01RefundTest,
	"Grimrock.RPG.ATTR01.Undo.RefundToSessionFloor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGATTR01RefundTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPGATTR01Tests;
	FRuntimeCacheGuard Guard;

	URPGClassAsset* ClassDefinition = nullptr;
	URPGRaceAsset* RaceDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeInventory(4, ClassDefinition, RaceDefinition);
	const int32 SessionFloor =
		FRPGAttributePointService::GetAttributeValue(
			Component->PartyInventoryState.ActiveCharacters[0],
			ERPGAttributePointTarget::Wisdom);

	FRPGAttributePointMutationResult Purchase;
	TestTrue(
		TEXT("Wisdom purchase succeeds"),
		FRPGAttributePointService::TryPurchasePoint(
			Component,
			0,
			ERPGAttributePointTarget::Wisdom,
			Purchase));

	FRPGAttributePointMutationResult Refund;
	TestTrue(
		TEXT("Current-session purchase can be refunded"),
		FRPGAttributePointService::TryRefundPurchasedPoint(
			Component,
			0,
			ERPGAttributePointTarget::Wisdom,
			SessionFloor,
			Refund));
	TestEqual(TEXT("Wisdom returns to session floor"), Refund.NewValue, SessionFloor);
	TestEqual(TEXT("Refund restores the point"), Refund.RemainingPoints, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGATTR01RefundFloorTest,
	"Grimrock.RPG.ATTR01.Undo.RejectBelowSessionFloor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGATTR01RefundFloorTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPGATTR01Tests;
	FRuntimeCacheGuard Guard;

	URPGClassAsset* ClassDefinition = nullptr;
	URPGRaceAsset* RaceDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeInventory(4, ClassDefinition, RaceDefinition);
	const int32 SessionFloor =
		FRPGAttributePointService::GetAttributeValue(
			Component->PartyInventoryState.ActiveCharacters[0],
			ERPGAttributePointTarget::Charisma);

	FRPGAttributePointMutationResult Purchase;
	TestTrue(
		TEXT("Charisma purchase succeeds"),
		FRPGAttributePointService::TryPurchasePoint(
			Component,
			0,
			ERPGAttributePointTarget::Charisma,
			Purchase));

	FRPGAttributePointMutationResult FirstRefund;
	TestTrue(
		TEXT("First refund reaches the floor"),
		FRPGAttributePointService::TryRefundPurchasedPoint(
			Component,
			0,
			ERPGAttributePointTarget::Charisma,
			SessionFloor,
			FirstRefund));

	FRPGAttributePointMutationResult SecondRefund;
	TestFalse(
		TEXT("A second refund cannot cross the floor"),
		FRPGAttributePointService::TryRefundPurchasedPoint(
			Component,
			0,
			ERPGAttributePointTarget::Charisma,
			SessionFloor,
			SecondRefund));
	TestEqual(
		TEXT("Floor rejection is explicit"),
		SecondRefund.RejectReason,
		ERPGAttributePointMutationRejectReason::NoSessionPurchaseToUndo);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGATTR01UIContractTest,
	"Grimrock.RPG.ATTR01.UI.CharacterSheetContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGATTR01UIContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UClass* SheetClass = UGridCharacterSheetWidget::StaticClass();
	TestNotNull(TEXT("Character Sheet exposes Attribute Point text"), FindFProperty<FProperty>(SheetClass, FName(TEXT("Text_AttributePoints"))));

	for (const TCHAR* PropertyName : {
		TEXT("Button_DecreaseStrength"), TEXT("Button_IncreaseStrength"),
		TEXT("Button_DecreaseDexterity"), TEXT("Button_IncreaseDexterity"),
		TEXT("Button_DecreaseConstitution"), TEXT("Button_IncreaseConstitution"),
		TEXT("Button_DecreaseIntelligence"), TEXT("Button_IncreaseIntelligence"),
		TEXT("Button_DecreaseWisdom"), TEXT("Button_IncreaseWisdom"),
		TEXT("Button_DecreaseCharisma"), TEXT("Button_IncreaseCharisma") })
	{
		TestNotNull(
			*FString::Printf(TEXT("%s is exposed"), PropertyName),
			FindFProperty<FProperty>(SheetClass, FName(PropertyName)));
	}

	TestNotNull(
		TEXT("Character Sheet exposes session reset"),
		SheetClass->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UGridCharacterSheetWidget, BeginAttributeAllocationSession)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGATTR01LevelUpFeedbackTest,
	"Grimrock.RPG.ATTR01.Feedback.LevelFour",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGATTR01LevelUpFeedbackTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const FRPGProgressionNotificationView Notification =
		FRPGProgressionFeedbackService::MakeLevelUpNotification(
			FText::FromString(TEXT("Elias")),
			3,
			4,
			1,
			1,
			1,
			0);

	TestTrue(TEXT("Level-four toast mentions Attribute Point"), Notification.Message.ToString().Contains(TEXT("caractéristique")));
	TestTrue(TEXT("Level-four toast still mentions Skill Point"), Notification.Message.ToString().Contains(TEXT("compétence")));
	TestTrue(TEXT("Level-four toast still mentions Talent Point"), Notification.Message.ToString().Contains(TEXT("talent")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGATTR01AuthorityTest,
	"Grimrock.RPG.ATTR01.Authority.NoPersistedCurrency",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGATTR01AuthorityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const UScriptStruct* CharacterStruct = FGridCharacterInventoryState::StaticStruct();
	TestNotNull(TEXT("Character state exists"), CharacterStruct);
	if (!CharacterStruct)
	{
		return false;
	}

	TestNull(TEXT("No AttributePoints field is persisted"), CharacterStruct->FindPropertyByName(TEXT("AttributePoints")));
	TestNull(TEXT("No SpentAttributePoints field is persisted"), CharacterStruct->FindPropertyByName(TEXT("SpentAttributePoints")));
	TestNull(TEXT("No AttributePointBalance field is persisted"), CharacterStruct->FindPropertyByName(TEXT("AttributePointBalance")));
	TestNotNull(TEXT("Attributes remains the durable authority"), CharacterStruct->FindPropertyByName(TEXT("Attributes")));
	return true;
}

#endif
