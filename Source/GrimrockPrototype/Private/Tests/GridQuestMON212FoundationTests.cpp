#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include <initializer_list>

#include "Engine/DataAsset.h"
#include "Engine/GameInstance.h"
#include "Quests/GridQuestDefinitionAsset.h"
#include "Quests/GridQuestSubsystem.h"
#include "Subsystems/GameInstanceSubsystem.h"

namespace
{
	UGridQuestDefinitionAsset* GridQuestMON212MakeDefinition(UObject* Outer, FName QuestId, std::initializer_list<const TCHAR*> ObjectiveIdTexts)
	{
		UGridQuestDefinitionAsset* Definition = NewObject<UGridQuestDefinitionAsset>(Outer);
		Definition->QuestId = QuestId;
		Definition->DisplayName = FText::FromName(QuestId);
		Definition->Description = FText::FromString(TEXT("MON21.2 transient quest definition."));

		for (const TCHAR* ObjectiveIdText : ObjectiveIdTexts)
		{
			const FName ObjectiveId(ObjectiveIdText);
			FGridQuestObjectiveDefinition& Objective = Definition->Objectives.AddDefaulted_GetRef();
			Objective.ObjectiveId = ObjectiveId;
			Objective.DisplayName = FText::FromName(ObjectiveId);
			Objective.Description = FText::FromString(TEXT("MON21.2 transient objective."));
		}
		return Definition;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridQuestMON212DefinitionValidationTest, "Grimrock.Quests.MON21_2.DefinitionValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridQuestMON212DefinitionValidationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	TestTrue(TEXT("Quest definitions are PrimaryDataAssets"), UGridQuestDefinitionAsset::StaticClass()->IsChildOf(UPrimaryDataAsset::StaticClass()));
	TestTrue(TEXT("Quest runtime authority is a GameInstanceSubsystem"), UGridQuestSubsystem::StaticClass()->IsChildOf(UGameInstanceSubsystem::StaticClass()));

	UGridQuestDefinitionAsset* Definition =
		GridQuestMON212MakeDefinition(GetTransientPackage(), TEXT("Quest_MON212_Main"), { TEXT("FindKey"), TEXT("OpenGate") });
	FString Error;
	TestTrue(TEXT("A stable QuestId with ordered unique objectives is valid"), Definition->ValidateDefinition(Error));
	TestTrue(TEXT("Valid definition returns no validation error"), Error.IsEmpty());
	TestNotNull(TEXT("Objective lookup resolves the declared stable ObjectiveId"), Definition->FindObjective(TEXT("OpenGate")));
	TestNull(TEXT("Objective lookup rejects an unknown ObjectiveId"), Definition->FindObjective(TEXT("MissingObjective")));

	Definition->QuestId = NAME_None;
	TestFalse(TEXT("QuestId=NAME_None is rejected"), Definition->ValidateDefinition(Error));
	TestTrue(TEXT("QuestId validation reports the identity problem"), Error.Contains(TEXT("QuestId")));

	Definition->QuestId = TEXT("Quest_MON212_Main");
	Definition->Objectives[1].ObjectiveId = Definition->Objectives[0].ObjectiveId;
	TestFalse(TEXT("Duplicate ObjectiveIds are rejected"), Definition->ValidateDefinition(Error));
	TestTrue(TEXT("Duplicate objective validation names the duplicate contract"), Error.Contains(TEXT("duplicate ObjectiveId")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridQuestMON212CampaignRuntimeStateTest, "Grimrock.Quests.MON21_2.CampaignRuntimeState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridQuestMON212CampaignRuntimeStateTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGameInstance* GameInstance = NewObject<UGameInstance>(GetTransientPackage());
	TestNotNull(TEXT("Transient GameInstance exists for subsystem ownership"), GameInstance);
	if (!GameInstance)
	{
		return false;
	}

	UGridQuestSubsystem* QuestSubsystem = NewObject<UGridQuestSubsystem>(GameInstance);
	TestNotNull(TEXT("Campaign quest authority can exist without a world Actor"), QuestSubsystem);
	if (!QuestSubsystem)
	{
		return false;
	}

	UGridQuestDefinitionAsset* MainQuest = GridQuestMON212MakeDefinition(QuestSubsystem, TEXT("Quest_MON212_Main"), { TEXT("FindKey"), TEXT("OpenGate") });
	UGridQuestDefinitionAsset* DirectQuest = GridQuestMON212MakeDefinition(QuestSubsystem, TEXT("Quest_MON212_Direct"), {});
	UGridQuestDefinitionAsset* FailedQuest = GridQuestMON212MakeDefinition(QuestSubsystem, TEXT("Quest_MON212_Failed"), { TEXT("Survive") });

	FString Error;
	TestTrue(TEXT("Main quest registers by stable QuestId"), QuestSubsystem->RegisterQuestDefinition(MainQuest, Error));
	TestTrue(TEXT("Registering the same definition is idempotent"), QuestSubsystem->RegisterQuestDefinition(MainQuest, Error));
	TestTrue(TEXT("Zero-objective direct quest remains a valid definition"), QuestSubsystem->RegisterQuestDefinition(DirectQuest, Error));
	TestTrue(TEXT("Failure-path quest registers"), QuestSubsystem->RegisterQuestDefinition(FailedQuest, Error));

	UGridQuestDefinitionAsset* ConflictingQuest = GridQuestMON212MakeDefinition(QuestSubsystem, TEXT("Quest_MON212_Main"), { TEXT("Other") });
	TestFalse(TEXT("A second definition cannot claim an existing QuestId"), QuestSubsystem->RegisterQuestDefinition(ConflictingQuest, Error));
	TestTrue(TEXT("QuestId collision is diagnosed"), Error.Contains(TEXT("already registered")));

	TestFalse(TEXT("No runtime record exists before StartQuest"), QuestSubsystem->HasQuestRuntimeState(TEXT("Quest_MON212_Main")));
	TestEqual(TEXT("Unstarted known quest projects as Inactive"), QuestSubsystem->GetQuestStatus(TEXT("Quest_MON212_Main")), EGridQuestStatus::Inactive);
	TestEqual(TEXT("Unknown quest cannot start"), QuestSubsystem->StartQuest(TEXT("Quest_MON212_Unknown")), EGridQuestMutationResult::DefinitionNotFound);

	TestEqual(
		TEXT("StartQuest creates the unique campaign runtime state"), QuestSubsystem->StartQuest(TEXT("Quest_MON212_Main")), EGridQuestMutationResult::Success);
	TestEqual(TEXT("Starting an already active quest is idempotently rejected"), QuestSubsystem->StartQuest(TEXT("Quest_MON212_Main")),
		EGridQuestMutationResult::AlreadyInState);
	TestEqual(TEXT("Started quest is Active"), QuestSubsystem->GetQuestStatus(TEXT("Quest_MON212_Main")), EGridQuestStatus::Active);
	TestEqual(TEXT("Exactly one runtime record exists for the main QuestId"), QuestSubsystem->CampaignState.Quests.Num(), 1);

	EGridQuestObjectiveStatus ObjectiveStatus = EGridQuestObjectiveStatus::Failed;
	TestTrue(TEXT("First objective status is queryable"), QuestSubsystem->GetObjectiveStatus(TEXT("Quest_MON212_Main"), TEXT("FindKey"), ObjectiveStatus));
	TestEqual(TEXT("First ordered objective becomes Active"), ObjectiveStatus, EGridQuestObjectiveStatus::Active);
	TestTrue(TEXT("Second objective status is queryable"), QuestSubsystem->GetObjectiveStatus(TEXT("Quest_MON212_Main"), TEXT("OpenGate"), ObjectiveStatus));
	TestEqual(TEXT("Later ordered objective stays Inactive"), ObjectiveStatus, EGridQuestObjectiveStatus::Inactive);

	TestEqual(TEXT("A later objective cannot complete before the active objective"),
		QuestSubsystem->CompleteObjective(TEXT("Quest_MON212_Main"), TEXT("OpenGate")), EGridQuestMutationResult::InvalidTransition);
	TestEqual(TEXT("Completing the active objective succeeds"), QuestSubsystem->CompleteObjective(TEXT("Quest_MON212_Main"), TEXT("FindKey")),
		EGridQuestMutationResult::Success);
	TestTrue(TEXT("Second objective is now queryable"), QuestSubsystem->GetObjectiveStatus(TEXT("Quest_MON212_Main"), TEXT("OpenGate"), ObjectiveStatus));
	TestEqual(TEXT("Completing the first objective activates the next one"), ObjectiveStatus, EGridQuestObjectiveStatus::Active);
	TestEqual(TEXT("Quest remains Active while an objective remains"), QuestSubsystem->GetQuestStatus(TEXT("Quest_MON212_Main")), EGridQuestStatus::Active);

	TestEqual(TEXT("Completing the last active objective succeeds"), QuestSubsystem->CompleteObjective(TEXT("Quest_MON212_Main"), TEXT("OpenGate")),
		EGridQuestMutationResult::Success);
	TestEqual(
		TEXT("Completing the last objective completes the quest"), QuestSubsystem->GetQuestStatus(TEXT("Quest_MON212_Main")), EGridQuestStatus::Completed);
	TestEqual(TEXT("A completed quest cannot fail"), QuestSubsystem->FailQuest(TEXT("Quest_MON212_Main")), EGridQuestMutationResult::InvalidTransition);

	TestEqual(TEXT("Zero-objective quest can start"), QuestSubsystem->StartQuest(TEXT("Quest_MON212_Direct")), EGridQuestMutationResult::Success);
	TestEqual(TEXT("Explicit CompleteQuest supports direct completion"), QuestSubsystem->CompleteQuest(TEXT("Quest_MON212_Direct")),
		EGridQuestMutationResult::Success);
	TestEqual(TEXT("Direct quest is Completed"), QuestSubsystem->GetQuestStatus(TEXT("Quest_MON212_Direct")), EGridQuestStatus::Completed);

	TestEqual(TEXT("Failure-path quest can start"), QuestSubsystem->StartQuest(TEXT("Quest_MON212_Failed")), EGridQuestMutationResult::Success);
	TestEqual(TEXT("Active quest can fail"), QuestSubsystem->FailQuest(TEXT("Quest_MON212_Failed")), EGridQuestMutationResult::Success);
	TestEqual(TEXT("Failed quest status is authoritative"), QuestSubsystem->GetQuestStatus(TEXT("Quest_MON212_Failed")), EGridQuestStatus::Failed);
	TestTrue(TEXT("Failed objective status is queryable"), QuestSubsystem->GetObjectiveStatus(TEXT("Quest_MON212_Failed"), TEXT("Survive"), ObjectiveStatus));
	TestEqual(TEXT("The active objective becomes Failed with its quest"), ObjectiveStatus, EGridQuestObjectiveStatus::Failed);

	TestTrue(TEXT("Campaign state validates after normal progression"), QuestSubsystem->ValidateRuntimeState(Error));
	TestTrue(TEXT("Valid campaign runtime state returns no error"), Error.IsEmpty());

	QuestSubsystem->ResetCampaignQuestState();
	TestEqual(TEXT("Campaign reset clears runtime quest records"), QuestSubsystem->CampaignState.Quests.Num(), 0);
	TestNotNull(TEXT("Campaign reset keeps registered data definitions"), QuestSubsystem->FindQuestDefinition(TEXT("Quest_MON212_Main")));
	TestTrue(TEXT("Reset campaign state validates"), QuestSubsystem->ValidateRuntimeState(Error));

	return true;
}

#endif
