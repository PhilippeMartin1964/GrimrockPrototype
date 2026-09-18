#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Runtime/GrimrockGameInstance.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGrimrockSTARTUPFLOW02PendingDungeonBuildTest,
	"Grimrock.StartupFlow.STARTUPFLOW02.PendingDungeonBuild",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGrimrockSTARTUPFLOW02PendingDungeonBuildTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGrimrockGameInstance* GameInstance = NewObject<UGrimrockGameInstance>();
	TestNotNull(TEXT("GameInstance is created"), GameInstance);
	if (!GameInstance)
	{
		return false;
	}

	FGridPartyInventoryState PartyState;
	PartyState.bInitialCharacterCreationCompleted = true;
	PartyState.MaxActiveCharacters = 6;
	PartyState.SelectedCharacterIndex = 0;

	FGridCharacterInventoryState Character;
	Character.CharacterId = FGuid::NewGuid();
	Character.DisplayName = FText::FromString(TEXT("Startup Flow 02 Hero"));
	PartyState.ActiveCharacters.Add(Character);
	PartyState.ActiveEquipment.SetNum(1);

	TestFalse(TEXT("No dungeon build handoff exists initially"), GameInstance->IsNewGameDungeonBuildPending());
	TestTrue(TEXT("A valid frontend party can be staged"), GameInstance->SetPendingNewPartyState(PartyState));
	TestFalse(TEXT("Staging alone does not arm dungeon construction"), GameInstance->IsNewGameDungeonBuildPending());

	GameInstance->SetPendingStartupMode(EGrimrockPartyStartupMode::NewGame);
	TestTrue(TEXT("New Game with a staged party arms the dungeon build handoff"), GameInstance->IsNewGameDungeonBuildPending());

	FGridPartyInventoryState ConsumedState;
	TestTrue(TEXT("The frontend party can be consumed by the runtime pawn"), GameInstance->ConsumePendingNewPartyState(ConsumedState));
	TestTrue(TEXT("Party consumption preserves the independent build handoff"), GameInstance->IsNewGameDungeonBuildPending());

	GameInstance->CompletePendingNewGameDungeonBuild();
	TestFalse(TEXT("Runtime build completion clears the handoff"), GameInstance->IsNewGameDungeonBuildPending());

	TestTrue(TEXT("A second valid frontend party can be staged"), GameInstance->SetPendingNewPartyState(PartyState));
	GameInstance->SetPendingStartupMode(EGrimrockPartyStartupMode::NewGame);
	TestTrue(TEXT("The second New Game arms the handoff"), GameInstance->IsNewGameDungeonBuildPending());
	GameInstance->ClearPendingStartupMode();
	TestFalse(TEXT("Clearing startup mode disarms the build handoff"), GameInstance->IsNewGameDungeonBuildPending());
	GameInstance->ClearPendingNewPartyState();
	TestFalse(TEXT("Cancelling pending New Game keeps the build handoff clear"), GameInstance->IsNewGameDungeonBuildPending());

	return true;
}

#endif
