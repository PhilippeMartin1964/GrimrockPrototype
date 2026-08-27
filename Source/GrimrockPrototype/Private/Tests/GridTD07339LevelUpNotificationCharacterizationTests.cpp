#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "RPG/RPGCharacterRulesLibrary.h"
#include "RPG/RPGLevelUpNotificationSubsystem.h"
#include "Runtime/GridInventoryTypes.h"
#include "Save/GrimrockPartySaveGame.h"
#include "UObject/UnrealType.h"

namespace GridTD07339Characterization
{
	struct FTD07339PersistentMirrorGuard
	{
		FTD07339PersistentMirrorGuard()
		{
			URPGLevelUpNotificationSubsystem::RestorePersistentState({});
		}

		~FTD07339PersistentMirrorGuard()
		{
			URPGLevelUpNotificationSubsystem::RestorePersistentState({});
		}
	};

	FGuid MakeTD07339Id(uint32 Suffix)
	{
		return FGuid(7, 3, 39, Suffix);
	}

	FGridCharacterInventoryState MakeTD07339Character(uint32 Suffix, int32 Level)
	{
		FGridCharacterInventoryState Character;
		Character.CharacterId = MakeTD07339Id(Suffix);
		Character.Experience = URPGCharacterRulesLibrary::GetCumulativeExperienceRequiredForLevel(Level);
		Character.Level = Level;
		return Character;
	}

	FRPGPendingLevelUpSaveState MakeTD07339Pending(const FGuid& CharacterId, int32 PreviousLevel, int32 NewLevel)
	{
		FRPGPendingLevelUpSaveState State;
		State.CharacterId = CharacterId;
		State.PreviousLevel = PreviousLevel;
		State.NewLevel = NewLevel;
		State.LevelsGained = FMath::Max(0, NewLevel - PreviousLevel);
		return State;
	}

	bool LoadTD07339ProjectFile(const TCHAR* RelativePath, FString& OutText)
	{
		return FFileHelper::LoadFileToString(OutText, *FPaths::Combine(FPaths::ProjectDir(), RelativePath));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07339RepresentationMultiplicityTest,
	"Grimrock.TechnicalDebt.TD07_3_3_9.Characterization.RepresentationMultiplicity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07339RepresentationMultiplicityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD07339Characterization;

	TestNotNull(TEXT("SaveGame owns PendingLevelUpNotifications"),
		FindFProperty<FProperty>(UGrimrockPartySaveGame::StaticClass(), TEXT("PendingLevelUpNotifications")));
	TestNull(TEXT("Character has no minimal durable acknowledgement field yet"),
		FindFProperty<FProperty>(FGridCharacterInventoryState::StaticStruct(), TEXT("LastAcknowledgedLevel")));

	FString HeaderText;
	FString SourceText;
	TestTrue(TEXT("Level-up notification header loads"),
		LoadTD07339ProjectFile(TEXT("Source/GrimrockPrototype/Public/RPG/RPGLevelUpNotificationSubsystem.h"), HeaderText));
	TestTrue(TEXT("Level-up notification source loads"),
		LoadTD07339ProjectFile(TEXT("Source/GrimrockPrototype/Private/RPG/RPGLevelUpNotificationSubsystem.cpp"), SourceText));

	TestTrue(TEXT("Runtime queue exists"), HeaderText.Contains(TEXT("PendingNotifications")));
	TestTrue(TEXT("Active notification exists"), HeaderText.Contains(TEXT("ActiveNotification")));
	TestTrue(TEXT("Deferred persistent restore queue exists"), HeaderText.Contains(TEXT("PendingPersistentRestoreStates")));
	TestTrue(TEXT("Static persistent mirror exists"), SourceText.Contains(TEXT("PersistentNotificationMirror")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07339PersistentMirrorRoundTripTest,
	"Grimrock.TechnicalDebt.TD07_3_3_9.Characterization.PersistentMirrorRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07339PersistentMirrorRoundTripTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD07339Characterization;
	FTD07339PersistentMirrorGuard Guard;

	FGridPartyInventoryState Party;
	const FGridCharacterInventoryState Character = MakeTD07339Character(1, 2);
	Party.ActiveCharacters.Add(Character);

	const FRPGPendingLevelUpSaveState Pending = MakeTD07339Pending(Character.CharacterId, 1, 2);
	URPGLevelUpNotificationSubsystem::RestorePersistentState({ Pending });

	TArray<FRPGPendingLevelUpSaveState> Captured;
	FText Error;
	TestTrue(TEXT("Static persistent mirror captures for current active party"),
		URPGLevelUpNotificationSubsystem::CapturePersistentState(Party, Captured, Error));
	TestEqual(TEXT("One notification round-trips through the mirror"), Captured.Num(), 1);
	if (Captured.Num() == 1)
	{
		TestTrue(TEXT("CharacterId survives"), Captured[0].CharacterId == Character.CharacterId);
		TestEqual(TEXT("PreviousLevel survives"), Captured[0].PreviousLevel, 1);
		TestEqual(TEXT("NewLevel survives"), Captured[0].NewLevel, 2);
		TestEqual(TEXT("LevelsGained survives"), Captured[0].LevelsGained, 1);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07339ActiveOnlyPersistenceBoundaryTest,
	"Grimrock.TechnicalDebt.TD07_3_3_9.Characterization.ActiveOnlyPersistenceBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07339ActiveOnlyPersistenceBoundaryTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD07339Characterization;
	FTD07339PersistentMirrorGuard Guard;

	FGridPartyInventoryState Party;
	const FGridCharacterInventoryState Active = MakeTD07339Character(2, 2);
	const FGridCharacterInventoryState Reserve = MakeTD07339Character(3, 2);
	Party.ActiveCharacters.Add(Active);
	Party.CharacterPool.Add(Reserve);

	URPGLevelUpNotificationSubsystem::RestorePersistentState(
		{ MakeTD07339Pending(Active.CharacterId, 1, 2), MakeTD07339Pending(Reserve.CharacterId, 1, 2) });

	TArray<FRPGPendingLevelUpSaveState> Captured;
	FText Error;
	TestTrue(TEXT("Current persistent mirror can be captured"), URPGLevelUpNotificationSubsystem::CapturePersistentState(Party, Captured, Error));
	TestEqual(TEXT("Current persistence filters notifications to active characters"), Captured.Num(), 1);
	if (Captured.Num() == 1)
	{
		TestTrue(TEXT("Active notification is retained"), Captured[0].CharacterId == Active.CharacterId);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07339SaveValidationContractTest,
	"Grimrock.TechnicalDebt.TD07_3_3_9.Characterization.SaveValidationContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07339SaveValidationContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD07339Characterization;

	UGrimrockPartySaveGame* Save = NewObject<UGrimrockPartySaveGame>();
	const FGridCharacterInventoryState Character = MakeTD07339Character(4, 2);
	Save->PartyInventoryState.ActiveCharacters.Add(Character);
	Save->PendingLevelUpNotifications.Add(MakeTD07339Pending(Character.CharacterId, 1, 2));

	FText Error;
	TestTrue(TEXT("Notification whose NewLevel equals current character Level is valid"), Save->ValidateCurrentState(Error));

	Save->PendingLevelUpNotifications[0].NewLevel = 3;
	Save->PendingLevelUpNotifications[0].LevelsGained = 2;
	TestFalse(TEXT("Notification targeting a level other than current character Level is rejected"), Save->ValidateCurrentState(Error));
	TestTrue(TEXT("Invalid notification reports a reason"), !Error.IsEmpty());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
