#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "RPG/RPGClassAsset.h"
#include "RPG/RPGRaceAsset.h"
#include "RPG/RPGStoryCompanionAsset.h"
#include "Runtime/GridActivationComponent.h"
#include "Runtime/GridInventoryTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON2046RecruitmentOfferDeclineSourceScopeTest, "Grimrock.MON20.4.RecruitmentUI.OfferDeclineSourceScope",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON2046RecruitmentOfferDeclineSourceScopeTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridActivationComponent* Activation = NewObject<UGridActivationComponent>(GetTransientPackage());
	TestNotNull(TEXT("Activation component exists"), Activation);
	if (!Activation)
	{
		return false;
	}

	const FGuid SourceA = FGuid::NewGuid();
	const FGuid SourceB = FGuid::NewGuid();
	const FGuid CharacterId = FGuid::NewGuid();

	TestFalse(TEXT("Offer is not suppressed before a decline"), Activation->IsStoryCompanionOfferDeclined(SourceA, CharacterId));

	Activation->RememberStoryCompanionOfferDeclined(SourceA, CharacterId);

	TestTrue(TEXT("Decline suppresses the same source and companion"), Activation->IsStoryCompanionOfferDeclined(SourceA, CharacterId));
	TestFalse(TEXT("A different source may re-offer the same companion"), Activation->IsStoryCompanionOfferDeclined(SourceB, CharacterId));

	Activation->ResetRuntimeState();
	TestFalse(TEXT("Runtime reset clears the non-persistent decline suppression"), Activation->IsStoryCompanionOfferDeclined(SourceA, CharacterId));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON2046RecruitmentAlreadyActiveSuppressionTest, "Grimrock.MON20.4.RecruitmentUI.OfferAlreadyActiveSuppression",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON2046RecruitmentAlreadyActiveSuppressionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridActivationComponent* Activation = NewObject<UGridActivationComponent>(GetTransientPackage());
	URPGStoryCompanionAsset* Definition = NewObject<URPGStoryCompanionAsset>(GetTransientPackage());
	URPGRaceAsset* Race = NewObject<URPGRaceAsset>(Definition);
	URPGClassAsset* Class = NewObject<URPGClassAsset>(Definition);

	TestNotNull(TEXT("Activation component exists"), Activation);
	TestNotNull(TEXT("Story companion definition exists"), Definition);
	TestNotNull(TEXT("Race definition exists"), Race);
	TestNotNull(TEXT("Class definition exists"), Class);
	if (!Activation || !Definition || !Race || !Class)
	{
		return false;
	}

	Definition->CharacterId = FGuid::NewGuid();
	Race->RaceId = TEXT("Race_Test");
	Class->ClassId = TEXT("Class_Test");
	Definition->RaceDefinition = Race;
	Definition->ClassDefinition = Class;

	FGridCharacterInventoryState Character;
	Character.CharacterId = Definition->CharacterId;
	Character.RaceId = Race->RaceId;
	Character.ClassId = Class->ClassId;

	FGridPartyInventoryState PartyState;
	PartyState.ActiveCharacters.Add(Character);

	TestTrue(TEXT("Exact active companion identity suppresses a new offer"), Activation->IsStoryCompanionAlreadyActive(PartyState, *Definition));

	PartyState.ActiveCharacters[0].ClassId = TEXT("Class_Collision");
	TestFalse(
		TEXT("GUID collision with another identity is not treated as already active"), Activation->IsStoryCompanionAlreadyActive(PartyState, *Definition));

	PartyState.ActiveCharacters.Reset();
	Character.ClassId = Class->ClassId;
	PartyState.CharacterPool.Add(Character);
	TestFalse(TEXT("Reserve candidate remains offerable"), Activation->IsStoryCompanionAlreadyActive(PartyState, *Definition));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
