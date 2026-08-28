#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "RPG/RPGCharacterRulesLibrary.h"
#include "Runtime/GridInventoryTypes.h"
#include "Save/GrimrockPartySaveGame.h"
#include "UObject/UnrealType.h"

namespace GridTD07339Characterization
{
	FGuid MakeTD07339Id(uint32 Suffix)
	{
		return FGuid(7, 3, 39, Suffix);
	}

	FGridCharacterInventoryState MakeTD07339Character(uint32 Suffix, int32 Level, int32 LastAcknowledgedLevel)
	{
		FGridCharacterInventoryState Character;
		Character.CharacterId = MakeTD07339Id(Suffix);
		Character.Experience = URPGCharacterRulesLibrary::GetCumulativeExperienceRequiredForLevel(Level);
		Character.Level = Level;
		Character.LastAcknowledgedLevel = LastAcknowledgedLevel;
		return Character;
	}

	bool LoadTD07339ProjectFile(const TCHAR* RelativePath, FString& OutText)
	{
		return FFileHelper::LoadFileToString(OutText, *FPaths::Combine(FPaths::ProjectDir(), RelativePath));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07339RepresentationMultiplicityTest, "Grimrock.TechnicalDebt.TD07_3_3_9.Characterization.RepresentationMultiplicity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07339RepresentationMultiplicityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD07339Characterization;

	const FProperty* AcknowledgementProperty = FindFProperty<FProperty>(FGridCharacterInventoryState::StaticStruct(), TEXT("LastAcknowledgedLevel"));
	TestNotNull(TEXT("Character owns minimal durable acknowledgement state"), AcknowledgementProperty);
	TestTrue(TEXT("Acknowledgement state is non-transient"), AcknowledgementProperty && !AcknowledgementProperty->HasAnyPropertyFlags(CPF_Transient));
	TestNull(TEXT("SaveGame owns no PendingLevelUpNotifications mirror"),
		FindFProperty<FProperty>(UGrimrockPartySaveGame::StaticClass(), TEXT("PendingLevelUpNotifications")));

	FString HeaderText;
	FString SourceText;
	TestTrue(TEXT("Level-up notification header loads"),
		LoadTD07339ProjectFile(TEXT("Source/GrimrockPrototype/Public/RPG/RPGLevelUpNotificationSubsystem.h"), HeaderText));
	TestTrue(TEXT("Level-up notification source loads"),
		LoadTD07339ProjectFile(TEXT("Source/GrimrockPrototype/Private/RPG/RPGLevelUpNotificationSubsystem.cpp"), SourceText));

	TestTrue(TEXT("Transient runtime queue remains"), HeaderText.Contains(TEXT("PendingNotifications")));
	TestTrue(TEXT("Transient active notification remains"), HeaderText.Contains(TEXT("ActiveNotification")));
	TestFalse(TEXT("Deferred persistent restore queue is removed"), HeaderText.Contains(TEXT("PendingPersistentRestoreStates")));
	TestFalse(TEXT("Static persistent mirror is removed"), SourceText.Contains(TEXT("PersistentNotificationMirror")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07339PersistentMirrorRoundTripTest, "Grimrock.TechnicalDebt.TD07_3_3_9.Characterization.PersistentMirrorRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07339PersistentMirrorRoundTripTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD07339Characterization;

	FGridPartyInventoryState Party;
	Party.ActiveCharacters.Add(MakeTD07339Character(1, 3, 1));

	const FGridPartyInventoryState DurableCopy = Party;
	const FGridCharacterInventoryState& Character = DurableCopy.ActiveCharacters[0];
	TestEqual(TEXT("Acknowledged level survives ordinary party-state copy"), Character.LastAcknowledgedLevel, 1);
	TestEqual(TEXT("Current level remains three"), Character.Level, 3);
	TestEqual(TEXT("PreviousLevel is derivable from acknowledgement"), Character.LastAcknowledgedLevel, 1);
	TestEqual(TEXT("NewLevel is derivable from current Level"), Character.Level, 3);
	TestEqual(TEXT("LevelsGained is derivable"), Character.Level - Character.LastAcknowledgedLevel, 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07339ActiveOnlyPersistenceBoundaryTest,
	"Grimrock.TechnicalDebt.TD07_3_3_9.Characterization.ActiveOnlyPersistenceBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07339ActiveOnlyPersistenceBoundaryTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD07339Characterization;

	FGridPartyInventoryState Party;
	Party.ActiveCharacters.Add(MakeTD07339Character(2, 2, 1));
	Party.CharacterPool.Add(MakeTD07339Character(3, 3, 2));

	TestEqual(TEXT("Active character carries pending acknowledgement state directly"),
		Party.ActiveCharacters[0].Level - Party.ActiveCharacters[0].LastAcknowledgedLevel, 1);
	TestEqual(TEXT("Pool character also carries pending acknowledgement state directly"),
		Party.CharacterPool[0].Level - Party.CharacterPool[0].LastAcknowledgedLevel, 1);

	FGridCharacterInventoryState Recruited = Party.CharacterPool[0];
	Party.CharacterPool.Reset();
	Party.ActiveCharacters.Add(Recruited);
	TestEqual(TEXT("Pool -> Active naturally preserves the pending Level-Up delta"),
		Party.ActiveCharacters[1].Level - Party.ActiveCharacters[1].LastAcknowledgedLevel, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07339SaveValidationContractTest, "Grimrock.TechnicalDebt.TD07_3_3_9.Characterization.SaveValidationContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07339SaveValidationContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD07339Characterization;

	UGrimrockPartySaveGame* Save = NewObject<UGrimrockPartySaveGame>();
	Save->PartyInventoryState.ActiveCharacters.Add(MakeTD07339Character(4, 2, 1));

	FText Error;
	TestTrue(TEXT("Acknowledgement below current Level is a valid unacknowledged Level-Up"), Save->ValidateCurrentState(Error));

	Save->PartyInventoryState.ActiveCharacters[0].LastAcknowledgedLevel = 3;
	TestFalse(TEXT("Acknowledgement above current Level is rejected"), Save->ValidateCurrentState(Error));
	TestTrue(TEXT("Invalid acknowledgement reports a reason"), !Error.IsEmpty());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
