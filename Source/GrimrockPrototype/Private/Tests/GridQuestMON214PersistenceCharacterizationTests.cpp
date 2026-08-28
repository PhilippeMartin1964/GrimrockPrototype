#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include <initializer_list>

#include "Engine/GameInstance.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Quests/GridQuestDefinitionAsset.h"
#include "Quests/GridQuestSubsystem.h"
#include "Save/GrimrockPartySaveGame.h"
#include "UObject/UnrealType.h"

namespace GridQuestMON214Characterization
{
	UGridQuestDefinitionAsset* MakeDefinition(UObject* Outer, FName QuestId, std::initializer_list<const TCHAR*> ObjectiveIds)
	{
		UGridQuestDefinitionAsset* Definition = NewObject<UGridQuestDefinitionAsset>(Outer);
		Definition->QuestId = QuestId;
		Definition->DisplayName = FText::FromName(QuestId);

		for (const TCHAR* ObjectiveText : ObjectiveIds)
		{
			const FName ObjectiveId(ObjectiveText);
			FGridQuestObjectiveDefinition& Objective = Definition->Objectives.AddDefaulted_GetRef();
			Objective.ObjectiveId = ObjectiveId;
			Objective.DisplayName = FText::FromName(ObjectiveId);
		}
		return Definition;
	}

	bool LoadProjectFile(const TCHAR* RelativePath, FString& OutText)
	{
		return FFileHelper::LoadFileToString(OutText, *FPaths::Combine(FPaths::ProjectDir(), RelativePath));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridQuestMON214SaveEnvelopeCharacterizationTest, "Grimrock.Quests.MON21_4.Characterization.SaveEnvelopeGap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridQuestMON214SaveEnvelopeCharacterizationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	TestEqual(TEXT("MON21.4 starts from exact-match SaveGame v22"), UGrimrockPartySaveGame::CurrentSaveVersion, 22);

	UClass* SaveClass = UGrimrockPartySaveGame::StaticClass();
	TestNull(TEXT("Current v22 SaveGame has no CampaignQuestState snapshot yet"), SaveClass->FindPropertyByName(TEXT("CampaignQuestState")));
	TestNull(TEXT("Current v22 SaveGame has no QuestRuntimeState mirror"), SaveClass->FindPropertyByName(TEXT("QuestRuntimeState")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridQuestMON214RuntimeAuthorityCharacterizationTest, "Grimrock.Quests.MON21_4.Characterization.RuntimeAuthority",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridQuestMON214RuntimeAuthorityCharacterizationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UClass* SubsystemClass = UGridQuestSubsystem::StaticClass();
	const FProperty* CampaignStateProperty = SubsystemClass->FindPropertyByName(TEXT("CampaignState"));
	const FProperty* DefinitionsProperty = SubsystemClass->FindPropertyByName(TEXT("QuestDefinitionsById"));

	TestNotNull(TEXT("Quest subsystem exposes one CampaignState authority"), CampaignStateProperty);
	TestTrue(
		TEXT("CampaignState remains transient before MON21.4 persistence"), CampaignStateProperty && CampaignStateProperty->HasAnyPropertyFlags(CPF_Transient));
	TestNotNull(TEXT("Quest definition registry exists"), DefinitionsProperty);
	TestTrue(TEXT("Definition registry remains transient"), DefinitionsProperty && DefinitionsProperty->HasAnyPropertyFlags(CPF_Transient));

	UScriptStruct* QuestStateStruct = FGridQuestRuntimeState::StaticStruct();
	TestNotNull(TEXT("Quest runtime state struct exists"), QuestStateStruct);
	if (!QuestStateStruct)
	{
		return false;
	}

	TestNotNull(TEXT("Quest runtime state persists stable QuestId"), QuestStateStruct->FindPropertyByName(TEXT("QuestId")));
	TestNotNull(TEXT("Quest runtime state persists status"), QuestStateStruct->FindPropertyByName(TEXT("Status")));
	TestNotNull(TEXT("Quest runtime state persists ordered objectives"), QuestStateStruct->FindPropertyByName(TEXT("Objectives")));
	TestNull(TEXT("Quest runtime state contains no definition pointer"), QuestStateStruct->FindPropertyByName(TEXT("Definition")));
	TestNull(TEXT("Quest runtime state contains no DefinitionAsset pointer"), QuestStateStruct->FindPropertyByName(TEXT("DefinitionAsset")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridQuestMON214ValidationCharacterizationTest, "Grimrock.Quests.MON21_4.Characterization.SnapshotValidationContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridQuestMON214ValidationCharacterizationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridQuestMON214Characterization;

	UGameInstance* GameInstance = NewObject<UGameInstance>(GetTransientPackage());
	UGridQuestSubsystem* Subsystem = NewObject<UGridQuestSubsystem>(GameInstance);
	TestNotNull(TEXT("Quest subsystem exists"), Subsystem);
	if (!Subsystem)
	{
		return false;
	}

	UGridQuestDefinitionAsset* Definition = MakeDefinition(Subsystem, TEXT("Quest_MON214_Main"), { TEXT("FindKey"), TEXT("OpenGate") });

	FString Error;
	TestTrue(TEXT("Current definition registers"), Subsystem->RegisterQuestDefinition(Definition, Error));
	TestEqual(TEXT("Quest starts"), Subsystem->StartQuest(TEXT("Quest_MON214_Main")), EGridQuestMutationResult::Success);
	TestTrue(TEXT("Current runtime state validates"), Subsystem->ValidateRuntimeState(Error));

	const FGridCampaignQuestRuntimeState ValidSnapshot = Subsystem->CampaignState;

	FGridQuestRuntimeState UnknownQuest;
	UnknownQuest.QuestId = TEXT("Quest_MON214_Unknown");
	UnknownQuest.Status = EGridQuestStatus::Completed;
	Subsystem->CampaignState.Quests.Add(UnknownQuest);
	TestFalse(TEXT("Unknown QuestId snapshot is rejected"), Subsystem->ValidateRuntimeState(Error));
	TestTrue(TEXT("Unknown QuestId rejection identifies missing definition"), Error.Contains(TEXT("no registered definition")));

	Subsystem->CampaignState = ValidSnapshot;
	Subsystem->CampaignState.Quests[0].Objectives[0].ObjectiveId = TEXT("UnknownObjective");
	TestFalse(TEXT("Unknown/reordered ObjectiveId snapshot is rejected"), Subsystem->ValidateRuntimeState(Error));
	TestTrue(TEXT("Objective mismatch rejection identifies definition order"), Error.Contains(TEXT("objective order differs")));

	Subsystem->CampaignState = ValidSnapshot;
	TestTrue(TEXT("Valid snapshot remains valid after rejected probes"), Subsystem->ValidateRuntimeState(Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridQuestMON214PipelineGapCharacterizationTest, "Grimrock.Quests.MON21_4.Characterization.SaveLoadPipelineGap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridQuestMON214PipelineGapCharacterizationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridQuestMON214Characterization;

	FString PawnSaveSource;
	FString ActivationSource;
	TestTrue(
		TEXT("Party save pipeline source loads"), LoadProjectFile(TEXT("Source/GrimrockPrototype/Private/Runtime/GrimrockPartyPawnSave.cpp"), PawnSaveSource));
	TestTrue(TEXT("Activation source loads"), LoadProjectFile(TEXT("Source/GrimrockPrototype/Private/Runtime/GridActivationComponent.cpp"), ActivationSource));

	TestFalse(TEXT("Current save pipeline does not capture CampaignQuestState yet"), PawnSaveSource.Contains(TEXT("CampaignQuestState")));
	TestFalse(TEXT("Current save pipeline does not access UGridQuestSubsystem yet"), PawnSaveSource.Contains(TEXT("UGridQuestSubsystem")));

	TestTrue(TEXT("Current quest registration is level-scoped"), ActivationSource.Contains(TEXT("RuntimeActor->LevelAsset->QuestDefinitions")));
	TestTrue(TEXT("Current registration helper is explicitly current-level scoped"), ActivationSource.Contains(TEXT("RegisterCurrentLevelQuestDefinitions")));
	TestFalse(TEXT("Current activation registration does not scan DungeonAsset levels"), ActivationSource.Contains(TEXT("DungeonAsset->Levels")));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
