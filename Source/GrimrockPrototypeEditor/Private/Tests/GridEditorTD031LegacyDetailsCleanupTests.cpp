#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "UObject/Class.h"

namespace
{
	struct FGridTD031EditorTestWorld
	{
		UWorld* World = nullptr;

		FGridTD031EditorTestWorld()
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
				FName(*FString::Printf(TEXT("TD031EditorDetailsWorld_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true,
				ERHIFeatureLevel::Num, &Values);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::EditorPreview);
				Context.SetCurrentWorld(World);
			}
		}

		~FGridTD031EditorTestWorld()
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD031CanonicalEditorActionsContractTest,
	"Grimrock.TechnicalDebt.TD03_1.EditorDetailsRedundancy.CanonicalActionsContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD031CanonicalEditorActionsContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridTD031EditorTestWorld TestWorld;
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

	const UFunction* SetStartFunction = EditorActor->FindFunction(TEXT("SetStartFromSelection"));
	const UFunction* ValidateFunction = EditorActor->FindFunction(TEXT("ValidateCurrentLevel"));
	TestNotNull(TEXT("SetStartFromSelection remains reflected"), SetStartFunction);
	TestNotNull(TEXT("ValidateCurrentLevel remains reflected"), ValidateFunction);
	TestTrue(TEXT("SetStartFromSelection remains BlueprintCallable"),
		SetStartFunction && SetStartFunction->HasAnyFunctionFlags(FUNC_BlueprintCallable));
	TestTrue(TEXT("ValidateCurrentLevel remains BlueprintCallable"),
		ValidateFunction && ValidateFunction->HasAnyFunctionFlags(FUNC_BlueprintCallable));
	TestFalse(TEXT("SetStartFromSelection is no longer exposed as CallInEditor"),
		SetStartFunction && SetStartFunction->HasMetaData(TEXT("CallInEditor")));
	TestFalse(TEXT("ValidateCurrentLevel is no longer exposed as CallInEditor"),
		ValidateFunction && ValidateFunction->HasMetaData(TEXT("CallInEditor")));

	EditorActor->SelectedCellX = 2;
	EditorActor->SelectedCellY = 1;
	EditorActor->SelectedEdge = EGridEdge::West;
	EditorActor->SetStartFromSelection();
	TestEqual(TEXT("The canonical Set Start action stores the selected X"), LevelAsset->StartCellX, 2);
	TestEqual(TEXT("The canonical Set Start action stores the selected Y"), LevelAsset->StartCellY, 1);
	TestEqual(TEXT("The canonical Set Start action stores the selected facing"), LevelAsset->StartFacing, EGridEdge::West);

	const TArray<FGridLevelValidationMessage> Messages = EditorActor->ValidateCurrentLevel();
	TestEqual(TEXT("Validation publishes the same message snapshot it returns"), Messages.Num(), EditorActor->LastValidationMessages.Num());
	TestTrue(TEXT("Validation still reports missing DungeonAsset as a warning"),
		Messages.ContainsByPredicate([](const FGridLevelValidationMessage& Message)
		{
			return Message.Severity == EGridLevelValidationSeverity::Warning && Message.Message.Contains(TEXT("DungeonAsset is missing"));
		}));
	TestEqual(TEXT("Validation does not alter the configured start X"), LevelAsset->StartCellX, 2);
	TestEqual(TEXT("Validation does not alter the configured start Y"), LevelAsset->StartCellY, 1);
	TestEqual(TEXT("Validation does not alter the configured start facing"), LevelAsset->StartFacing, EGridEdge::West);

	return true;
}

#endif
