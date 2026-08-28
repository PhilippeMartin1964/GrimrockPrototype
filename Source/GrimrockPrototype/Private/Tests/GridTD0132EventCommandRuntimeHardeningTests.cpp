#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridActivationComponent.h"
#include "Runtime/GridLevelRuntimeActor.h"

namespace
{
	FGridLevelObjectData MakeTD0132Object(FGuid ObjectId, EGridLevelObjectType Type)
	{
		FGridLevelObjectData Object;
		Object.ObjectId = ObjectId;
		Object.Type = Type;
		Object.CellX = 0;
		Object.CellY = 0;
		Object.Edge = EGridEdge::None;
		Object.bInitiallyEnabled = true;
		return Object;
	}

	FGridObjectLink MakeTD0132Link(FGuid SourceId, FGuid TargetId, EGridObjectEvent SourceEvent, EGridObjectCommand Command)
	{
		FGridObjectLink Link;
		Link.SourceObjectId = SourceId;
		Link.TargetObjectId = TargetId;
		Link.SourceEvent = SourceEvent;
		Link.Command = Command;
		return Link;
	}

	struct FTD0132TestWorld
	{
		UWorld* World = nullptr;

		FTD0132TestWorld()
		{
			const UWorld::InitializationValues Values = UWorld::InitializationValues()
															.AllowAudioPlayback(false)
															.RequiresHitProxies(false)
															.CreatePhysicsScene(false)
															.CreateNavigation(false)
															.CreateAISystem(false)
															.ShouldSimulatePhysics(false)
															.SetTransactional(false);

			World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("TD0132EventCommandRuntimeWorld"), nullptr, true, ERHIFeatureLevel::Num, &Values);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FTD0132TestWorld()
		{
			if (!World)
			{
				return;
			}

			World->DestroyWorld(false);
			if (GEngine)
			{
				GEngine->DestroyWorldContext(World);
			}
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD0132StateOnlyRuntimeRejectionTest, "Grimrock.TechnicalDebt.TD01_3.EventCommandContract.RuntimeHardening",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD0132StateOnlyRuntimeRejectionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FTD0132TestWorld TestWorld;
	TestNotNull(TEXT("TD01.3.2 runtime world is created"), TestWorld.World);
	if (!TestWorld.World)
	{
		return false;
	}

	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	TestNotNull(TEXT("TD01.3.2 runtime actor is created"), Runtime);
	if (!Runtime)
	{
		return false;
	}

	UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Runtime);
	Level->Width = 1;
	Level->Height = 1;
	Level->EnsureCellCount();
	Level->Cells[0].CellType = EGridCellType::Floor;
	Runtime->LevelAsset = Level;
	Runtime->CurrentDungeonLevelId = TEXT("TD0132");

	const FGuid SourceId(1, 3, 2, 1);
	const FGuid TeleporterId(1, 3, 2, 2);
	const FGuid ItemSpawnId(1, 3, 2, 3);
	const FGuid LightId(1, 3, 2, 4);

	Level->Objects.Add(MakeTD0132Object(SourceId, EGridLevelObjectType::Trigger));
	Level->Objects.Add(MakeTD0132Object(TeleporterId, EGridLevelObjectType::Teleporter));
	Level->Objects.Add(MakeTD0132Object(ItemSpawnId, EGridLevelObjectType::ItemSpawn));
	Level->Objects.Add(MakeTD0132Object(LightId, EGridLevelObjectType::Light));

	for (const FGuid TargetId : { TeleporterId, ItemSpawnId, LightId })
	{
		Level->Links.Add(MakeTD0132Link(SourceId, TargetId, EGridObjectEvent::Activated, EGridObjectCommand::Activate));
		Level->Links.Add(MakeTD0132Link(SourceId, TargetId, EGridObjectEvent::Deactivated, EGridObjectCommand::Deactivate));
	}

	UGridActivationComponent* Activation = Runtime->FindComponentByClass<UGridActivationComponent>();
	TestNotNull(TEXT("TD01.3.2 activation component exists"), Activation);
	if (!Activation)
	{
		return false;
	}

	Activation->Initialize(Runtime);
	Activation->RebuildIndexes();

	AddExpectedError(TEXT("Grid link failed:"), EAutomationExpectedErrorFlags::Contains, 6);
	TestFalse(TEXT("StateOnly Activate links are rejected at runtime"), Activation->ExecuteLinksFromObjectForEvent(SourceId, EGridObjectEvent::Activated));
	TestFalse(TEXT("Teleporter was not activated by a rejected link"), Activation->GetActiveObjectIds().Contains(TeleporterId));
	TestFalse(TEXT("ItemSpawn was not activated by a rejected link"), Activation->GetActiveObjectIds().Contains(ItemSpawnId));
	TestFalse(TEXT("Light was not activated by a rejected link"), Activation->GetActiveObjectIds().Contains(LightId));

	TSet<FGuid> PreexistingActiveIds;
	PreexistingActiveIds.Add(TeleporterId);
	PreexistingActiveIds.Add(ItemSpawnId);
	PreexistingActiveIds.Add(LightId);
	Activation->SetActiveObjectIds(PreexistingActiveIds);

	TestFalse(TEXT("StateOnly Deactivate links are rejected at runtime"), Activation->ExecuteLinksFromObjectForEvent(SourceId, EGridObjectEvent::Deactivated));
	TestTrue(TEXT("Rejected Teleporter deactivation preserves preexisting state"), Activation->GetActiveObjectIds().Contains(TeleporterId));
	TestTrue(TEXT("Rejected ItemSpawn deactivation preserves preexisting state"), Activation->GetActiveObjectIds().Contains(ItemSpawnId));
	TestTrue(TEXT("Rejected Light deactivation preserves preexisting state"), Activation->GetActiveObjectIds().Contains(LightId));

	return true;
}

#endif
