#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "RPG/RPGLevelUpService.h"
#include "RPGMON155TestHelpers.h"
#include "Runtime/GridInventoryTypes.h"
#include "Save/GrimrockPartySaveGame.h"
#include "UObject/UnrealType.h"

namespace GridTD07339Normalization
{
	bool LoadTD07339NProjectFile(const TCHAR* RelativePath, FString& OutText)
	{
		return FFileHelper::LoadFileToString(OutText, *FPaths::Combine(FPaths::ProjectDir(), RelativePath));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07339SchemaAuthorityTest,
	"Grimrock.TechnicalDebt.TD07_3_3_9.Normalization.SchemaAuthority",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07339SchemaAuthorityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD07339Normalization;

	const FProperty* AcknowledgementProperty =
		FindFProperty<FProperty>(FGridCharacterInventoryState::StaticStruct(), TEXT("LastAcknowledgedLevel"));
	TestNotNull(TEXT("LastAcknowledgedLevel exists on character state"), AcknowledgementProperty);
	TestTrue(TEXT("LastAcknowledgedLevel is durable and non-transient"),
		AcknowledgementProperty && !AcknowledgementProperty->HasAnyPropertyFlags(CPF_Transient));

	TestNull(TEXT("Separate PendingLevelUpNotifications SaveGame mirror is removed"),
		FindFProperty<FProperty>(UGrimrockPartySaveGame::StaticClass(), TEXT("PendingLevelUpNotifications")));

	FString HeaderText;
	FString SourceText;
	TestTrue(TEXT("Level-up subsystem header loads"),
		LoadTD07339NProjectFile(TEXT("Source/GrimrockPrototype/Public/RPG/RPGLevelUpNotificationSubsystem.h"), HeaderText));
	TestTrue(TEXT("Level-up subsystem source loads"),
		LoadTD07339NProjectFile(TEXT("Source/GrimrockPrototype/Private/RPG/RPGLevelUpNotificationSubsystem.cpp"), SourceText));
	TestFalse(TEXT("PersistentNotificationMirror is removed"), SourceText.Contains(TEXT("PersistentNotificationMirror")));
	TestFalse(TEXT("PendingPersistentRestoreStates is removed"), HeaderText.Contains(TEXT("PendingPersistentRestoreStates")));
	TestFalse(TEXT("CapturePersistentState is removed"), HeaderText.Contains(TEXT("CapturePersistentState")));
	TestFalse(TEXT("RestorePersistentState is removed"), HeaderText.Contains(TEXT("RestorePersistentState")));
	TestTrue(TEXT("Transient runtime queue remains"), HeaderText.Contains(TEXT("PendingNotifications")));
	TestTrue(TEXT("Transient active notification remains"), HeaderText.Contains(TEXT("ActiveNotification")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07339LevelUpGapTest,
	"Grimrock.TechnicalDebt.TD07_3_3_9.Normalization.LevelUpCreatesDurableGap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07339LevelUpGapTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FMON155RuntimeStateGuard RuntimeGuard;
	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeMON155Inventory(1, 1000, ClassDefinition);
	FGridCharacterInventoryState& Character = Component->PartyInventoryState.ActiveCharacters[0];
	Character.LastAcknowledgedLevel = 1;

	TestTrue(TEXT("Pending level applies"), FRPGLevelUpService::ApplyPendingLevelUp(Component, 0, false));
	TestEqual(TEXT("Character reaches level two"), Character.Level, 2);
	TestEqual(TEXT("Level-Up service deliberately leaves acknowledgement unchanged"), Character.LastAcknowledgedLevel, 1);
	TestEqual(TEXT("One unacknowledged level is derivable"), Character.Level - Character.LastAcknowledgedLevel, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07339ActivePoolDurabilityTest,
	"Grimrock.TechnicalDebt.TD07_3_3_9.Normalization.ActivePoolDurability",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07339ActivePoolDurabilityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridPartyInventoryState Party;

	FGridCharacterInventoryState Active;
	Active.CharacterId = FGuid(7, 3, 139, 1);
	Active.Experience = 1000;
	Active.Level = 2;
	Active.LastAcknowledgedLevel = 1;
	Party.ActiveCharacters.Add(Active);

	FGridCharacterInventoryState Reserve;
	Reserve.CharacterId = FGuid(7, 3, 139, 2);
	Reserve.Experience = 3000;
	Reserve.Level = 3;
	Reserve.LastAcknowledgedLevel = 3;
	Party.CharacterPool.Add(Reserve);

	UGrimrockPartySaveGame* Save = NewObject<UGrimrockPartySaveGame>();
	Save->PartyInventoryState = Party;
	FText Error;
	TestTrue(TEXT("Active + Pool durable acknowledgement state validates"), Save->ValidateCurrentState(Error));

	FGridCharacterInventoryState Moved = Save->PartyInventoryState.CharacterPool[0];
	Save->PartyInventoryState.CharacterPool.Reset();
	Save->PartyInventoryState.ActiveCharacters.Add(Moved);
	TestEqual(TEXT("Pool -> Active move carries acknowledgement naturally"),
		Save->PartyInventoryState.ActiveCharacters[1].LastAcknowledgedLevel, 3);
	TestEqual(TEXT("Moved character has no synthetic Level-Up pending"),
		Save->PartyInventoryState.ActiveCharacters[1].Level - Save->PartyInventoryState.ActiveCharacters[1].LastAcknowledgedLevel, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07339SaveSchemaVersionTest,
	"Grimrock.TechnicalDebt.TD07_3_3_9.Normalization.SaveSchemaVersion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07339SaveSchemaVersionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	TestTrue(TEXT("TD07.3.3.9 established SaveGame v19 or later"), UGrimrockPartySaveGame::CurrentSaveVersion >= 19);

	UGrimrockPartySaveGame* Current = NewObject<UGrimrockPartySaveGame>();
	TestEqual(TEXT("New SaveGame starts on the current schema"), Current->SaveVersion, UGrimrockPartySaveGame::CurrentSaveVersion);
	TestTrue(TEXT("Current schema is compatible"), Current->IsCompatible());

	UGrimrockPartySaveGame* Previous = NewObject<UGrimrockPartySaveGame>();
	Previous->SaveVersion = 18;
	FText Error;
	TestFalse(TEXT("Previous v18 is rejected without migration"), Previous->ValidateCurrentState(Error));
	TestFalse(TEXT("Previous v18 is incompatible"), Previous->IsCompatible());
	TestEqual(TEXT("Validation does not rewrite v18"), Previous->SaveVersion, 18);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
