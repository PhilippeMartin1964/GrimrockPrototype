#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Quests/GridQuestDefinitionAsset.h"
#include "Quests/GridQuestSubsystem.h"
#include "Runtime/GridLevelRuntimeActor.h"

namespace
{
	struct FGridQuestMON213World
	{
		UWorld* World = nullptr;
		UGameInstance* GameInstance = nullptr;

		FGridQuestMON213World()
		{
			const UWorld::InitializationValues Values = UWorld::InitializationValues()
				.AllowAudioPlayback(false)
				.RequiresHitProxies(false)
				.CreatePhysicsScene(false)
				.CreateNavigation(false)
				.CreateAISystem(false)
				.ShouldSimulatePhysics(false)
				.SetTransactional(false);

			World = UWorld::CreateWorld(EWorldType::Game, false,
				FName(*FString::Printf(TEXT("MON213_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true,
				ERHIFeatureLevel::Num, &Values);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}

			GameInstance = NewObject<UGameInstance>(GetTransientPackage());
			if (GameInstance)
			{
				GameInstance->Init();
			}
			if (World)
			{
				World->SetGameInstance(GameInstance);
			}
		}

		~FGridQuestMON213World()
		{
			if (World)
			{
				World->SetGameInstance(nullptr);
			}
			if (GameInstance)
			{
				GameInstance->Shutdown();
			}
			if (World)
			{
				World->DestroyWorld(false);
				if (GEngine)
				{
					GEngine->DestroyWorldContext(World);
				}
			}
		}
	};

	UGridQuestDefinitionAsset* MakeQuest(UObject* Outer, FName QuestId, FName ObjectiveA = NAME_None, FName ObjectiveB = NAME_None)
	{
		UGridQuestDefinitionAsset* Quest = NewObject<UGridQuestDefinitionAsset>(Outer);
		Quest->QuestId = QuestId;
		Quest->DisplayName = FText::FromName(QuestId);

		for (const FName ObjectiveId : {ObjectiveA, ObjectiveB})
		{
			if (ObjectiveId.IsNone())
			{
				continue;
			}
			FGridQuestObjectiveDefinition& Objective = Quest->Objectives.AddDefaulted_GetRef();
			Objective.ObjectiveId = ObjectiveId;
			Objective.DisplayName = FText::FromName(ObjectiveId);
		}
		return Quest;
	}

	FGridObjectLink MakeQuestLink(FGuid SourceId, EGridObjectEvent Event, EGridObjectCommand Command, FName QuestId, FName ObjectiveId = NAME_None)
	{
		FGridObjectLink Link;
		Link.SourceObjectId = SourceId;
		Link.SourceEvent = Event;
		Link.Command = Command;
		Link.QuestId = QuestId;
		Link.QuestObjectiveId = ObjectiveId;
		Link.TargetObjectId.Invalidate();
		return Link;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridQuestMON213EventCommandIntegrationTest, "Grimrock.Quests.MON21_3.EventCommandIntegration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridQuestMON213EventCommandIntegrationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	TestEqual(TEXT("QuestStart serialized value"), static_cast<uint8>(EGridObjectCommand::QuestStart), static_cast<uint8>(25));
	TestEqual(TEXT("QuestCompleteObjective serialized value"), static_cast<uint8>(EGridObjectCommand::QuestCompleteObjective), static_cast<uint8>(26));
	TestEqual(TEXT("QuestComplete serialized value"), static_cast<uint8>(EGridObjectCommand::QuestComplete), static_cast<uint8>(27));
	TestEqual(TEXT("QuestFail serialized value"), static_cast<uint8>(EGridObjectCommand::QuestFail), static_cast<uint8>(28));

	FGridQuestMON213World TestWorld;
	TestNotNull(TEXT("Transient Game world exists"), TestWorld.World);
	TestNotNull(TEXT("Transient GameInstance exists"), TestWorld.GameInstance);
	if (!TestWorld.World || !TestWorld.GameInstance)
	{
		return false;
	}

	UGridQuestSubsystem* QuestSubsystem = TestWorld.GameInstance->GetSubsystem<UGridQuestSubsystem>();
	TestNotNull(TEXT("GameInstance owns UGridQuestSubsystem"), QuestSubsystem);
	if (!QuestSubsystem)
	{
		return false;
	}

	AGridLevelRuntimeActor* RuntimeActor = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	TestNotNull(TEXT("RuntimeActor is spawned"), RuntimeActor);
	if (!RuntimeActor)
	{
		return false;
	}

	UGridLevelAsset* Level = NewObject<UGridLevelAsset>(RuntimeActor);
	Level->Width = 1;
	Level->Height = 1;
	Level->EnsureCellCount();

	const FGuid SourceId = FGuid::NewGuid();
	FGridLevelObjectData& Source = Level->Objects.AddDefaulted_GetRef();
	Source.ObjectId = SourceId;
	Source.Type = EGridLevelObjectType::Logic;
	Source.LogicId = TEXT("MON213_Source");

	UGridQuestDefinitionAsset* MainQuest = MakeQuest(Level, TEXT("Quest_MON213_Main"), TEXT("FindKey"), TEXT("OpenGate"));
	UGridQuestDefinitionAsset* FailedQuest = MakeQuest(Level, TEXT("Quest_MON213_Failed"), TEXT("Survive"));
	UGridQuestDefinitionAsset* DirectQuest = MakeQuest(Level, TEXT("Quest_MON213_Direct"));
	Level->QuestDefinitions = {MainQuest, FailedQuest, DirectQuest};

	Level->Links.Add(MakeQuestLink(SourceId, EGridObjectEvent::Activated, EGridObjectCommand::QuestStart, TEXT("Quest_MON213_Main")));
	Level->Links.Add(
		MakeQuestLink(SourceId, EGridObjectEvent::Used, EGridObjectCommand::QuestCompleteObjective, TEXT("Quest_MON213_Main"), TEXT("FindKey")));
	Level->Links.Add(
		MakeQuestLink(SourceId, EGridObjectEvent::Opened, EGridObjectCommand::QuestCompleteObjective, TEXT("Quest_MON213_Main"), TEXT("OpenGate")));
	Level->Links.Add(MakeQuestLink(SourceId, EGridObjectEvent::Entered, EGridObjectCommand::QuestStart, TEXT("Quest_MON213_Failed")));
	Level->Links.Add(MakeQuestLink(SourceId, EGridObjectEvent::Disabled, EGridObjectCommand::QuestCompleteObjective, TEXT("Quest_MON213_Failed"),
		TEXT("MissingObjective")));
	Level->Links.Add(MakeQuestLink(SourceId, EGridObjectEvent::Exited, EGridObjectCommand::QuestFail, TEXT("Quest_MON213_Failed")));
	Level->Links.Add(MakeQuestLink(SourceId, EGridObjectEvent::MonsterSpawned, EGridObjectCommand::QuestStart, TEXT("Quest_MON213_Direct")));
	Level->Links.Add(MakeQuestLink(SourceId, EGridObjectEvent::MonsterDespawned, EGridObjectCommand::QuestComplete, TEXT("Quest_MON213_Direct")));
	Level->Links.Add(MakeQuestLink(SourceId, EGridObjectEvent::MonsterDied, EGridObjectCommand::QuestStart, TEXT("Quest_MON213_Unknown")));

	RuntimeActor->LevelAsset = Level;
	RuntimeActor->RebuildLevel();

	TestNotNull(TEXT("Rebuild registers MainQuest"), QuestSubsystem->FindQuestDefinition(TEXT("Quest_MON213_Main")));
	TestNotNull(TEXT("Rebuild registers FailedQuest"), QuestSubsystem->FindQuestDefinition(TEXT("Quest_MON213_Failed")));
	TestNotNull(TEXT("Rebuild registers DirectQuest"), QuestSubsystem->FindQuestDefinition(TEXT("Quest_MON213_Direct")));

	TestTrue(TEXT("Event -> QuestStart succeeds without TargetObjectId"), RuntimeActor->ExecuteLinksFromRuntimeObject(SourceId, EGridObjectEvent::Activated));
	TestEqual(TEXT("Main quest is Active"), QuestSubsystem->GetQuestStatus(TEXT("Quest_MON213_Main")), EGridQuestStatus::Active);
	TestTrue(TEXT("Repeated QuestStart is adapter-idempotent"), RuntimeActor->ExecuteLinksFromRuntimeObject(SourceId, EGridObjectEvent::Activated));
	TestEqual(TEXT("Repeated start does not duplicate runtime state"), QuestSubsystem->CampaignState.Quests.Num(), 1);

	TestTrue(TEXT("Event completes first objective"), RuntimeActor->ExecuteLinksFromRuntimeObject(SourceId, EGridObjectEvent::Used));
	EGridQuestObjectiveStatus ObjectiveStatus = EGridQuestObjectiveStatus::Failed;
	TestTrue(TEXT("Second objective status can be read"), QuestSubsystem->GetObjectiveStatus(TEXT("Quest_MON213_Main"), TEXT("OpenGate"), ObjectiveStatus));
	TestEqual(TEXT("Second objective is activated"), ObjectiveStatus, EGridQuestObjectiveStatus::Active);

	TestTrue(TEXT("Event completes final objective"), RuntimeActor->ExecuteLinksFromRuntimeObject(SourceId, EGridObjectEvent::Opened));
	TestEqual(TEXT("Main quest completes"), QuestSubsystem->GetQuestStatus(TEXT("Quest_MON213_Main")), EGridQuestStatus::Completed);

	TestTrue(TEXT("Failure-path quest starts"), RuntimeActor->ExecuteLinksFromRuntimeObject(SourceId, EGridObjectEvent::Entered));
	AddExpectedError(TEXT("Grid quest command result: Quest=Quest_MON213_Failed"), EAutomationExpectedErrorFlags::Contains, 1);
	AddExpectedError(TEXT("Grid link failed:"), EAutomationExpectedErrorFlags::Contains, 2);
	TestFalse(TEXT("Unknown objective is rejected"), RuntimeActor->ExecuteLinksFromRuntimeObject(SourceId, EGridObjectEvent::Disabled));
	TestEqual(TEXT("Rejected objective leaves quest active"), QuestSubsystem->GetQuestStatus(TEXT("Quest_MON213_Failed")), EGridQuestStatus::Active);
	TestTrue(TEXT("QuestFail event succeeds"), RuntimeActor->ExecuteLinksFromRuntimeObject(SourceId, EGridObjectEvent::Exited));
	TestEqual(TEXT("Failure-path quest failed"), QuestSubsystem->GetQuestStatus(TEXT("Quest_MON213_Failed")), EGridQuestStatus::Failed);

	TestTrue(TEXT("Direct quest starts"), RuntimeActor->ExecuteLinksFromRuntimeObject(SourceId, EGridObjectEvent::MonsterSpawned));
	TestTrue(TEXT("QuestComplete event succeeds"), RuntimeActor->ExecuteLinksFromRuntimeObject(SourceId, EGridObjectEvent::MonsterDespawned));
	TestEqual(TEXT("Direct quest completes"), QuestSubsystem->GetQuestStatus(TEXT("Quest_MON213_Direct")), EGridQuestStatus::Completed);

	AddExpectedError(TEXT("Grid quest command result: Quest=Quest_MON213_Unknown"), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("Unknown QuestId is rejected"), RuntimeActor->ExecuteLinksFromRuntimeObject(SourceId, EGridObjectEvent::MonsterDied));

	FString Error;
	TestTrue(TEXT("Campaign quest state remains valid"), QuestSubsystem->ValidateRuntimeState(Error));
	TestTrue(TEXT("Valid runtime state has no error"), Error.IsEmpty());
	return true;
}

#endif