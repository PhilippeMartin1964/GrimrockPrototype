#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include <initializer_list>

#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Quests/GridQuestDefinitionAsset.h"
#include "Quests/GridQuestSubsystem.h"
#include "Save/GrimrockPartySaveGame.h"
#include "UObject/UnrealType.h"

namespace GridQuestMON2142
{
	UGridQuestDefinitionAsset* MakeDefinition(
		UObject* Outer,
		FName QuestId,
		std::initializer_list<const TCHAR*> ObjectiveIds)
	{
		UGridQuestDefinitionAsset* Definition = NewObject<UGridQuestDefinitionAsset>(Outer);
		Definition->QuestId = QuestId;
		Definition->DisplayName = FText::FromName(QuestId);

		for (const TCHAR* ObjectiveText : ObjectiveIds)
		{
			FGridQuestObjectiveDefinition& Objective = Definition->Objectives.AddDefaulted_GetRef();
			Objective.ObjectiveId = FName(ObjectiveText);
			Objective.DisplayName = FText::FromName(Objective.ObjectiveId);
		}
		return Definition;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridQuestMON2142SaveSchemaTest,
	"Grimrock.Quests.MON21_4_2.SaveSchemaV26",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridQuestMON2142SaveSchemaTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	TestTrue(TEXT("Quest persistence remains part of every schema at or beyond v26"),
		UGrimrockPartySaveGame::CurrentSaveVersion >= 26);

	const FProperty* CampaignQuestStateProperty =
		UGrimrockPartySaveGame::StaticClass()->FindPropertyByName(TEXT("CampaignQuestState"));
	if (TestNotNull(TEXT("SaveGame owns CampaignQuestState"), CampaignQuestStateProperty))
	{
		TestTrue(TEXT("CampaignQuestState is SaveGame-persistent"),
			CampaignQuestStateProperty->HasAnyPropertyFlags(CPF_SaveGame));
	}

	UGrimrockPartySaveGame* Previous = NewObject<UGrimrockPartySaveGame>();
	Previous->SaveVersion = 25;
	FText Error;
	TestFalse(TEXT("Previous v25 is rejected without migration"), Previous->ValidateCurrentState(Error));
	TestFalse(TEXT("Previous v25 is incompatible"), Previous->IsCompatible());
	TestEqual(TEXT("Rejected v25 is never rewritten"), Previous->SaveVersion, 25);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridQuestMON2142SaveRoundTripTest,
	"Grimrock.Quests.MON21_4_2.SaveSnapshotRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridQuestMON2142SaveRoundTripTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGrimrockPartySaveGame* Source = NewObject<UGrimrockPartySaveGame>();
	FGridQuestRuntimeState Quest;
	Quest.QuestId = TEXT("Quest_MON2142_RoundTrip");
	Quest.Status = EGridQuestStatus::Completed;

	FGridQuestObjectiveRuntimeState Objective;
	Objective.ObjectiveId = TEXT("Objective_MON2142_RoundTrip");
	Objective.Status = EGridQuestObjectiveStatus::Completed;
	Quest.Objectives.Add(Objective);
	Source->CampaignQuestState.Quests.Add(Quest);

	TArray<uint8> Bytes;
	if (!TestTrue(TEXT("Current v26 SaveGame serializes campaign Quest snapshot"),
		UGameplayStatics::SaveGameToMemory(Source, Bytes)))
	{
		return false;
	}

	UGrimrockPartySaveGame* Loaded =
		Cast<UGrimrockPartySaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
	if (!TestNotNull(TEXT("Serialized v26 SaveGame loads"), Loaded))
	{
		return false;
	}

	TestTrue(TEXT("Loaded v26 SaveGame is compatible"), Loaded->IsCompatible());
	TestEqual(TEXT("One Quest snapshot survives"), Loaded->CampaignQuestState.Quests.Num(), 1);
	if (Loaded->CampaignQuestState.Quests.Num() == 1)
	{
		const FGridQuestRuntimeState& LoadedQuest = Loaded->CampaignQuestState.Quests[0];
		TestEqual(TEXT("QuestId survives"), LoadedQuest.QuestId, Quest.QuestId);
		TestEqual(TEXT("Quest status survives"), LoadedQuest.Status, EGridQuestStatus::Completed);
		TestEqual(TEXT("One objective survives"), LoadedQuest.Objectives.Num(), 1);
		if (LoadedQuest.Objectives.Num() == 1)
		{
			TestEqual(TEXT("ObjectiveId survives"), LoadedQuest.Objectives[0].ObjectiveId, Objective.ObjectiveId);
			TestEqual(TEXT("Objective status survives"), LoadedQuest.Objectives[0].Status, EGridQuestObjectiveStatus::Completed);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridQuestMON2142SubsystemSnapshotApiTest,
	"Grimrock.Quests.MON21_4_2.SubsystemSnapshotApi",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridQuestMON2142SubsystemSnapshotApiTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridQuestMON2142;

	UGameInstance* GameInstance = NewObject<UGameInstance>(GetTransientPackage());
	UGridQuestSubsystem* Subsystem = NewObject<UGridQuestSubsystem>(GameInstance);
	if (!TestNotNull(TEXT("Quest subsystem exists"), Subsystem))
	{
		return false;
	}

	UGridQuestDefinitionAsset* Definition =
		MakeDefinition(Subsystem, TEXT("Quest_MON2142_Main"), { TEXT("FindKey"), TEXT("OpenGate") });

	FString Error;
	TestTrue(TEXT("Quest definition registers"), Subsystem->RegisterQuestDefinition(Definition, Error));
	TestEqual(TEXT("Quest starts"), Subsystem->StartQuest(Definition->QuestId), EGridQuestMutationResult::Success);
	TestEqual(TEXT("First objective completes"),
		Subsystem->CompleteObjective(Definition->QuestId, TEXT("FindKey")),
		EGridQuestMutationResult::Success);

	FGridCampaignQuestRuntimeState Snapshot;
	TestTrue(TEXT("Valid runtime state can be captured"), Subsystem->CaptureCampaignQuestState(Snapshot, Error));
	TestEqual(TEXT("Captured snapshot contains one Quest"), Snapshot.Quests.Num(), 1);

	Subsystem->ResetCampaignQuestState();
	TestEqual(TEXT("Runtime reset clears campaign state"), Subsystem->CampaignState.Quests.Num(), 0);
	TestTrue(TEXT("Valid snapshot restores atomically"), Subsystem->RestoreCampaignQuestState(Snapshot, Error));

	FGridQuestRuntimeState Restored;
	TestTrue(TEXT("Restored Quest is present"), Subsystem->GetQuestRuntimeState(Definition->QuestId, Restored));
	TestEqual(TEXT("Restored Quest remains active"), Restored.Status, EGridQuestStatus::Active);
	TestEqual(TEXT("Restored first objective remains completed"),
		Restored.Objectives[0].Status, EGridQuestObjectiveStatus::Completed);
	TestEqual(TEXT("Restored second objective remains active"),
		Restored.Objectives[1].Status, EGridQuestObjectiveStatus::Active);

	const FGridCampaignQuestRuntimeState BeforeRejectedRestore = Subsystem->CampaignState;

	FGridCampaignQuestRuntimeState UnknownQuestSnapshot = Snapshot;
	UnknownQuestSnapshot.Quests[0].QuestId = TEXT("Quest_MON2142_Unknown");
	TestFalse(TEXT("Unknown QuestId is rejected before mutation"),
		Subsystem->RestoreCampaignQuestState(UnknownQuestSnapshot, Error));
	TestEqual(TEXT("Rejected unknown Quest leaves current QuestId unchanged"),
		Subsystem->CampaignState.Quests[0].QuestId,
		BeforeRejectedRestore.Quests[0].QuestId);
	TestEqual(TEXT("Rejected unknown Quest leaves current objective status unchanged"),
		Subsystem->CampaignState.Quests[0].Objectives[0].Status,
		BeforeRejectedRestore.Quests[0].Objectives[0].Status);

	FGridCampaignQuestRuntimeState WrongOrderSnapshot = Snapshot;
	Swap(WrongOrderSnapshot.Quests[0].Objectives[0], WrongOrderSnapshot.Quests[0].Objectives[1]);
	TestFalse(TEXT("Objective order mismatch is rejected before mutation"),
		Subsystem->RestoreCampaignQuestState(WrongOrderSnapshot, Error));
	TestEqual(TEXT("Rejected objective order leaves current second objective active"),
		Subsystem->CampaignState.Quests[0].Objectives[1].Status,
		EGridQuestObjectiveStatus::Active);

	TestTrue(TEXT("Current runtime state remains valid after rejected restores"),
		Subsystem->ValidateRuntimeState(Error));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
