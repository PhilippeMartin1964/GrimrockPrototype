#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGAttributePointService.h"
#include "RPG/RPGLevelUpService.h"
#include "RPG/RPGSkillPointService.h"
#include "RPG/RPGProgressionPIESimulator.h"
#include "RPGMON155TestHelpers.h"
#include "Runtime/GridInventoryTypes.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Save/GrimrockPartySaveGame.h"
#include "UI/GridPersistentHudWidget.h"
#include "UI/GridPartyPortraitHudWidget.h"
#include "UI/RPGProgressionFeedbackService.h"
#include "UObject/UnrealType.h"

namespace RPGLEVELUX01Tests
{
	bool ContainsAnsiToken(const TArray<uint8>& Bytes, const ANSICHAR* Token)
	{
		const int32 TokenLength = FCStringAnsi::Strlen(Token);
		if (TokenLength <= 0 || Bytes.Num() < TokenLength)
		{
			return false;
		}

		for (int32 Index = 0; Index <= Bytes.Num() - TokenLength; ++Index)
		{
			bool bMatch = true;
			for (int32 Offset = 0; Offset < TokenLength; ++Offset)
			{
				if (Bytes[Index + Offset] != static_cast<uint8>(Token[Offset]))
				{
					bMatch = false;
					break;
				}
			}
			if (bMatch)
			{
				return true;
			}
		}
		return false;
	}

	bool ContainsUtf16LeToken(const TArray<uint8>& Bytes, const TCHAR* Token)
	{
		const int32 TokenLength = FCString::Strlen(Token);
		if (TokenLength <= 0 || Bytes.Num() < TokenLength * 2)
		{
			return false;
		}

		for (int32 Index = 0; Index <= Bytes.Num() - TokenLength * 2; ++Index)
		{
			bool bMatch = true;
			for (int32 Offset = 0; Offset < TokenLength; ++Offset)
			{
				const uint16 CodeUnit = static_cast<uint16>(Token[Offset]);
				if (Bytes[Index + Offset * 2] != static_cast<uint8>(CodeUnit & 0xff) ||
					Bytes[Index + Offset * 2 + 1] != static_cast<uint8>((CodeUnit >> 8) & 0xff))
				{
					bMatch = false;
					break;
				}
			}
			if (bMatch)
			{
				return true;
			}
		}
		return false;
	}

	bool ContainsToken(const TArray<uint8>& Bytes, const ANSICHAR* AnsiToken, const TCHAR* WideToken)
	{
		return ContainsAnsiToken(Bytes, AnsiToken) || ContainsUtf16LeToken(Bytes, WideToken);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGLEVELUX01NoDurableAcknowledgementTest,
	"Grimrock.RPG.LEVELUX01.LevelUp.NoDurableAcknowledgement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGLEVELUX01NoDurableAcknowledgementTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FMON155RuntimeStateGuard Guard;

	TestNull(
		TEXT("Legacy LastAcknowledgedLevel field is absent from character state"),
		FGridCharacterInventoryState::StaticStruct()->FindPropertyByName(TEXT("LastAcknowledgedLevel")));

	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeMON155Inventory(1, 1000, ClassDefinition);
	FGridCharacterInventoryState& Character = Component->PartyInventoryState.ActiveCharacters[0];

	TestTrue(TEXT("Canonical Level-Up applies"), FRPGLevelUpService::ApplyPendingLevelUp(Component, 0, false));
	TestEqual(TEXT("Character reaches level two"), Character.Level, 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGLEVELUX01OddLevelProgressionAvailabilityTest,
	"Grimrock.RPG.LEVELUX01.Progression.OddLevelBalances",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGLEVELUX01OddLevelProgressionAvailabilityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FMON155RuntimeStateGuard Guard;
	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeMON155Inventory(5, 0, ClassDefinition);
	const FGridCharacterInventoryState& Character = Component->PartyInventoryState.ActiveCharacters[0];
	FRPGSkillPointBalance SkillBalance;
	TestTrue(TEXT("Skill currency derives from the level-five character"),
		FRPGSkillPointService::TryGetBalance(Character, SkillBalance));
	TestEqual(TEXT("Level five grants eight total Skill Points"), SkillBalance.GrantedPoints, 8);
	TestEqual(TEXT("Level five raises the canonical Skill rank cap"), SkillBalance.RankCap, 3);
	TestEqual(TEXT("Level-five skill allocation is still available"), SkillBalance.RemainingPoints, 8);
	TestEqual(TEXT("Level five grants no new Attribute Point beyond level four"),
		FRPGAttributePointService::GetTotalPointsGranted(5),
		FRPGAttributePointService::GetTotalPointsGranted(4));
	FGridInventoryCharacterSummary Summary;
	TestTrue(TEXT("Character summary remains available"), Component->GetCharacterSummary(0, Summary));
	TestTrue(TEXT("Unspent currency is projected to the portrait badge"), Summary.bHasUnspentProgressionPoints);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGLEVELUX01EvenLevelProgressionAvailabilityTest,
	"Grimrock.RPG.LEVELUX01.Progression.EvenLevelBalances",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGLEVELUX01EvenLevelProgressionAvailabilityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FMON155RuntimeStateGuard Guard;
	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeMON155Inventory(6, 0, ClassDefinition);
	const FGridCharacterInventoryState& Character = Component->PartyInventoryState.ActiveCharacters[0];
	FRPGSkillPointBalance SkillBalance;
	TestTrue(TEXT("Skill currency derives from the level-six character"),
		FRPGSkillPointService::TryGetBalance(Character, SkillBalance));
	TestEqual(TEXT("Level six grants nine total Skill Points"), SkillBalance.GrantedPoints, 9);
	TestEqual(TEXT("Level six does not invent a new Skill rank cap"), SkillBalance.RankCap, 3);
	TestEqual(TEXT("Each new level grants exactly one more Skill Point"),
		FRPGSkillPointService::GetTotalPointsGranted(6) -
			FRPGSkillPointService::GetTotalPointsGranted(5), 1);
	FGridInventoryCharacterSummary Summary;
	TestTrue(TEXT("Character summary remains available"), Component->GetCharacterSummary(0, Summary));
	TestTrue(TEXT("Pending progression stays visible without a Level-Up toast"),
		Summary.bHasUnspentProgressionPoints);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGLEVELUX01PersistentHudContractTest,
	"Grimrock.RPG.LEVELUX01.PersistentHud.NotificationContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGLEVELUX01PersistentHudContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UClass* HudClass = UGridPersistentHudWidget::StaticClass();
	TestNotNull(
		TEXT("Persistent HUD retains generic non-Level-Up progression feedback"),
		FindFProperty<FProperty>(HudClass, FName(TEXT("Notification_Progression"))));
	TestNotNull(TEXT("Portrait presenter exposes persistent progression badge"),
		UGridPartyPortraitHudWidget::StaticClass()->FindPropertyByName(TEXT("Image_LevelUpAlert")));
	TestNotNull(TEXT("Portrait presenter exposes weight warning badge"),
		UGridPartyPortraitHudWidget::StaticClass()->FindPropertyByName(TEXT("Image_WeightAlert")));
	TestNotNull(
		TEXT("Persistent HUD exposes the non-modal notification entry point"),
		HudClass->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UGridPersistentHudWidget, ShowProgressionNotification)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGLEVELUX01SimulatorSuccessiveLevelsTest,
	"Grimrock.RPG.LEVELUX01.PIESimulator.SuccessiveLevels",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGLEVELUX01SimulatorSuccessiveLevelsTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FMON155RuntimeStateGuard Guard;

	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeMON155Inventory(1, 0, ClassDefinition);

	FText Feedback;
	TestTrue(TEXT("PIE simulator reaches level two"), FRPGProgressionPIESimulator::TrySetSelectedCharacterLevel(Component, 2, Feedback));
	TestTrue(TEXT("PIE simulator immediately continues to level six"), FRPGProgressionPIESimulator::TrySetSelectedCharacterLevel(Component, 6, Feedback));

	const FGridCharacterInventoryState& Character = Component->PartyInventoryState.ActiveCharacters[0];
	TestEqual(TEXT("Successive simulation reaches level six"), Character.Level, 6);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGLEVELUX01NoModalRuntimePathTest,
	"Grimrock.RPG.LEVELUX01.Runtime.NoModalCreationPath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGLEVELUX01NoModalRuntimePathTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const FString ProjectDir = FPaths::ProjectDir();
	TestFalse(TEXT("Legacy RPGLevelUpWidget header is deleted"),
		FPaths::FileExists(FPaths::Combine(ProjectDir, TEXT("Source/GrimrockPrototype/Public/UI/RPGLevelUpWidget.h"))));
	TestFalse(TEXT("Legacy RPGLevelUpWidget implementation is deleted"),
		FPaths::FileExists(FPaths::Combine(ProjectDir, TEXT("Source/GrimrockPrototype/Private/UI/RPGLevelUpWidget.cpp"))));
	TestFalse(TEXT("Legacy RPGLevelUpWidget Slate fallback is deleted"),
		FPaths::FileExists(FPaths::Combine(ProjectDir, TEXT("Source/GrimrockPrototype/Private/UI/RPGLevelUpWidgetSlate.cpp"))));

	TestFalse(TEXT("Obsolete Level-Up toast subsystem header is deleted"),
		FPaths::FileExists(FPaths::Combine(ProjectDir, TEXT("Source/GrimrockPrototype/Public/RPG/RPGLevelUpNotificationSubsystem.h"))));
	TestFalse(TEXT("Obsolete Level-Up toast subsystem implementation is deleted"),
		FPaths::FileExists(FPaths::Combine(ProjectDir, TEXT("Source/GrimrockPrototype/Private/RPG/RPGLevelUpNotificationSubsystem.cpp"))));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGLEVELUX01NoSerializedLegacyReferenceTest,
	"Grimrock.RPG.LEVELUX01.Cleanup.NoSerializedLegacyReference",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGLEVELUX01NoSerializedLegacyReferenceTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPGLEVELUX01Tests;

	TArray<FString> Files;
	IFileManager::Get().FindFilesRecursive(Files, *FPaths::ProjectContentDir(), TEXT("*.uasset"), true, false, false);
	IFileManager::Get().FindFilesRecursive(Files, *FPaths::ProjectContentDir(), TEXT("*.umap"), true, false, false);

	int32 ScannedFiles = 0;
	int32 LegacyReferenceCount = 0;
	for (const FString& FilePath : Files)
	{
		TArray<uint8> Bytes;
		if (!FFileHelper::LoadFileToArray(Bytes, *FilePath))
		{
			AddError(FString::Printf(TEXT("Impossible de lire %s pour l'audit legacy."), *FilePath));
			continue;
		}

		++ScannedFiles;
		const bool bReferencesWidget =
			ContainsToken(Bytes, "RPGLevelUpWidget", TEXT("RPGLevelUpWidget"));
		const bool bReferencesAcknowledgement =
			ContainsToken(Bytes, "LastAcknowledgedLevel", TEXT("LastAcknowledgedLevel"));
		if (bReferencesWidget || bReferencesAcknowledgement)
		{
			++LegacyReferenceCount;
			AddError(FString::Printf(
				TEXT("Référence sérialisée legacy détectée dans %s%s%s"),
				*FilePath,
				bReferencesWidget ? TEXT(" [RPGLevelUpWidget]") : TEXT(""),
				bReferencesAcknowledgement ? TEXT(" [LastAcknowledgedLevel]") : TEXT("")));
		}
	}

	TestTrue(TEXT("Content audit scanned at least one package"), ScannedFiles > 0);
	TestEqual(TEXT("No uasset/umap retains legacy Level-Up references"), LegacyReferenceCount, 0);
	AddInfo(FString::Printf(TEXT("RPG-LEVELUX01.3 scanned %d serialized Content packages."), ScannedFiles));
	return LegacyReferenceCount == 0;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGLEVELUX01SaveSchemaV24Test,
	"Grimrock.RPG.LEVELUX01.Cleanup.SaveSchemaV24",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGLEVELUX01SaveSchemaV24Test::RunTest(const FString& Parameters)
{
	(void)Parameters;

	TestTrue(TEXT("Legacy Level-Up removal v24 baseline remains part of the current exact-match schema"),
		UGrimrockPartySaveGame::CurrentSaveVersion >= 24);

	UGrimrockPartySaveGame* Current = NewObject<UGrimrockPartySaveGame>();
	TestEqual(TEXT("Fresh SaveGame defaults to the current schema"),
		Current->SaveVersion, UGrimrockPartySaveGame::CurrentSaveVersion);
	TestTrue(TEXT("Fresh current-schema SaveGame is compatible"), Current->IsCompatible());

	UGrimrockPartySaveGame* Previous = NewObject<UGrimrockPartySaveGame>();
	Previous->SaveVersion = 23;
	FText Error;
	TestFalse(TEXT("Former v23 schema is rejected without migration"), Previous->ValidateCurrentState(Error));
	TestFalse(TEXT("Former v23 schema is incompatible"), Previous->IsCompatible());
	return true;
}

#endif
