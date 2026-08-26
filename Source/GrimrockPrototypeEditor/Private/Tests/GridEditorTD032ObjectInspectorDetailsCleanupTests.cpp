#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "UObject/Class.h"

namespace
{
	struct FGridTD032EditorTestWorld
	{
		UWorld* World = nullptr;

		FGridTD032EditorTestWorld()
		{
			const UWorld::InitializationValues Values = UWorld::InitializationValues()
				.AllowAudioPlayback(false)
				.RequiresHitProxies(false)
				.CreatePhysicsScene(false)
				.CreateNavigation(false)
				.CreateAISystem(false)
				.ShouldSimulatePhysics(false)
				.SetTransactional(false);

			World = UWorld::CreateWorld(EWorldType::EditorPreview, false,
				FName(*FString::Printf(TEXT("TD032ObjectInspectorWorld_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true,
				ERHIFeatureLevel::Num, &Values);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::EditorPreview);
				Context.SetCurrentWorld(World);
			}
		}

		~FGridTD032EditorTestWorld()
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD032ObjectInspectorMoveContractTest,
	"Grimrock.TechnicalDebt.TD03_2.ObjectInspectorDetails.MoveToCurrentCellContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD032ObjectInspectorMoveContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridTD032EditorTestWorld TestWorld;
	TestNotNull(TEXT("The editor preview world is created"), TestWorld.World);
	if (!TestWorld.World)
	{
		return false;
	}

	AGridLevelEditorActor* EditorActor = TestWorld.World->SpawnActor<AGridLevelEditorActor>();
	TestNotNull(TEXT("The grid editor actor is spawned"), EditorActor);
	if (!EditorActor)
	{
		return false;
	}

	UGridLevelAsset* LevelAsset = NewObject<UGridLevelAsset>(EditorActor);
	LevelAsset->Width = 3;
	LevelAsset->Height = 3;
	LevelAsset->EnsureCellCount();
	for (FGridLevelCellData& Cell : LevelAsset->Cells)
	{
		Cell.CellType = EGridCellType::Floor;
		Cell.bBlocksOccupancy = false;
	}
	EditorActor->LevelAsset = LevelAsset;

	FGridLevelObjectData Object;
	Object.ObjectId = FGuid::NewGuid();
	Object.Type = EGridLevelObjectType::Trigger;
	Object.CellX = 0;
	Object.CellY = 0;
	Object.Edge = EGridEdge::None;
	LevelAsset->Objects.Add(Object);

	TestTrue(TEXT("The object can be selected"), EditorActor->SelectObjectById(Object.ObjectId));

	const UFunction* MoveFunction = EditorActor->FindFunction(TEXT("MoveSelectedObjectToCurrentSelection"));
	TestNotNull(TEXT("MoveSelectedObjectToCurrentSelection remains reflected"), MoveFunction);
	TestTrue(TEXT("MoveSelectedObjectToCurrentSelection is BlueprintCallable"),
		MoveFunction && MoveFunction->HasAnyFunctionFlags(FUNC_BlueprintCallable));
	TestFalse(TEXT("MoveSelectedObjectToCurrentSelection is no longer exposed as CallInEditor"),
		MoveFunction && MoveFunction->HasMetaData(TEXT("CallInEditor")));

	EditorActor->SelectedCellX = 2;
	EditorActor->SelectedCellY = 1;
	EditorActor->SelectedEdge = EGridEdge::East;

	TestTrue(TEXT("The canonical Object Inspector move action succeeds"), EditorActor->MoveSelectedObjectToCurrentSelection());
	TestEqual(TEXT("The level still contains one object"), LevelAsset->Objects.Num(), 1);
	TestEqual(TEXT("The moved object keeps its identity"), LevelAsset->Objects[0].ObjectId, Object.ObjectId);
	TestEqual(TEXT("The moved object stores the selected X"), LevelAsset->Objects[0].CellX, 2);
	TestEqual(TEXT("The moved object stores the selected Y"), LevelAsset->Objects[0].CellY, 1);
	TestEqual(TEXT("A cell-centered object keeps Edge=None"), LevelAsset->Objects[0].Edge, EGridEdge::None);
	TestTrue(TEXT("Moving again to the same destination remains a successful no-op"), EditorActor->MoveSelectedObjectToCurrentSelection());

	return true;
}

#endif
