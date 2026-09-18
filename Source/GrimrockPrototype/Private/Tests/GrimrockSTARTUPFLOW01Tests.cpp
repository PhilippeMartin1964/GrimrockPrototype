#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Runtime/GrimrockGameInstance.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGrimrockSTARTUPFLOW01PendingPartyTest,
	"Grimrock.StartupFlow.STARTUPFLOW01.PendingParty",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGrimrockSTARTUPFLOW01PendingPartyTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGrimrockGameInstance* GameInstance = NewObject<UGrimrockGameInstance>();
	TestNotNull(TEXT("GameInstance is created"), GameInstance);
	if (!GameInstance)
	{
		return false;
	}

	TestEqual(TEXT("The canonical gameplay map is L_GrimrockEditor"), GameInstance->GetDungeonLevelName(),
		FName(TEXT("/Game/GrimrockPrototype/Maps/L_GrimrockEditor")));

	FGridPartyInventoryState PartyState;
	PartyState.bInitialCharacterCreationCompleted = true;
	PartyState.MaxActiveCharacters = 6;
	PartyState.SelectedCharacterIndex = 0;

	FGridCharacterInventoryState Character;
	Character.CharacterId = FGuid::NewGuid();
	Character.DisplayName = FText::FromString(TEXT("Startup Hero"));
	PartyState.ActiveCharacters.Add(Character);
	PartyState.ActiveEquipment.SetNum(1);

	TestTrue(TEXT("A completed frontend party can be staged"), GameInstance->SetPendingNewPartyState(PartyState));
	TestTrue(TEXT("The staged party is visible"), GameInstance->HasPendingNewPartyState());

	FGridPartyInventoryState ConsumedState;
	TestTrue(TEXT("The staged party is consumed once"), GameInstance->ConsumePendingNewPartyState(ConsumedState));
	TestEqual(TEXT("The consumed party preserves its character"), ConsumedState.ActiveCharacters.Num(), 1);
	TestEqual(TEXT("The consumed character id is preserved"), ConsumedState.ActiveCharacters[0].CharacterId, Character.CharacterId);
	TestFalse(TEXT("No pending party remains after consume"), GameInstance->HasPendingNewPartyState());

	return true;
}

#endif
