#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridDungeonAsset.h"
#include "Core/GridLevelAsset.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "UObject/Class.h"

namespace
{
	struct FGridTD034EditorTestWorld
	{
		UWorld* World = nullptr;

		FGridTD034EditorTestWorld()
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
				FName(*FString::Printf(TEXT("TD034DungeonLevelWorld_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true,
				ERHIFeatureLevel::Num, &Values);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::EditorPreview);
				Context.SetCurrentWorld(World);
			}
		}

		~FGridTD034EditorTestWorld()
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

	UGridLevelAsset* GridTD034MakeWalkableLevel(UObject* Outer)
	{
		UGridLevelAsset* LevelAsset = NewObject<UGridLevelAsset>(Outer);
		LevelAsset->Width = 2;
		LevelAsset->Height = 2;
		LevelAsset->EnsureCellCount();
		for (FGridLevelCellData& Cell : LevelAsset->Cells)
		{
			Cell.CellType = EGridCellType::Floor;
			Cell.bBlocksOccupancy = false;
		}
		return LevelAsset;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD034DungeonLevelContractTest,
	"Grimrock.TechnicalDebt.TD03_4.DungeonLevelsDetails.ApplyCurrentLevelContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD034DungeonLevelContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridTD034EditorTestWorld TestWorld;
	TestNotNull(TEXT("The editor preview world is created"), TestWorld.World);
	if (!TestWorld.World)
	{
		return false;
	}

	AGridLevelEditorActor* EditorActor = TestWorld.World->SpawnActor<AGridLevelEditorActor>();
	AGridLevelRuntimeActor* RuntimeActor = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	TestNotNull(TEXT("The grid editor actor is spawned"), EditorActor);
	TestNotNull(TEXT("The preview runtime actor is spawned"), RuntimeActor);
	if (!EditorActor || !RuntimeActor)
	{
		return false;
	}

	UGridDungeonAsset* DungeonAsset = NewObject<UGridDungeonAsset>(EditorActor);
	UGridLevelAsset* LevelA = GridTD034MakeWalkableLevel(EditorActor);
	UGridLevelAsset* LevelB = GridTD034MakeWalkableLevel(EditorActor);

	FGridDungeonLevelEntry EntryA;
	EntryA.LevelId = TEXT("TD034_A");
	EntryA.LevelAsset = LevelA;
	EntryA.bEnabled = true;
	DungeonAsset->Levels.Add(EntryA);

	FGridDungeonLevelEntry EntryB;
	EntryB.LevelId = TEXT("TD034_B");
	EntryB.LevelAsset = LevelB;
	EntryB.bEnabled = true;
	DungeonAsset->Levels.Add(EntryB);
	DungeonAsset->DefaultLevelId = EntryA.LevelId;

	EditorActor->DungeonAsset = DungeonAsset;
	EditorActor->PreviewRuntimeActor = RuntimeActor;
	EditorActor->LevelAsset = LevelB;
	EditorActor->CurrentDungeonLevelId = NAME_None;

	const UFunction* ApplyFunction = EditorActor->FindFunction(TEXT("ApplyCurrentDungeonLevel"));
	TestNotNull(TEXT("ApplyCurrentDungeonLevel remains reflected"), ApplyFunction);
	TestTrue(TEXT("ApplyCurrentDungeonLevel remains BlueprintCallable"),
		ApplyFunction && ApplyFunction->HasAnyFunctionFlags(FUNC_BlueprintCallable));
	TestTrue(TEXT("Baseline exposes ApplyCurrentDungeonLevel as CallInEditor"),
		ApplyFunction && ApplyFunction->HasMetaData(TEXT("CallInEditor")));

	TestTrue(TEXT("No explicit level selects the dungeon default"), EditorActor->ApplyCurrentDungeonLevel());
	TestEqual(TEXT("The editor stores the default level id"), EditorActor->CurrentDungeonLevelId, EntryA.LevelId);
	TestEqual(TEXT("The editor switches to the default level asset"), EditorActor->LevelAsset.Get(), LevelA);
	TestEqual(TEXT("The preview runtime receives the default level asset"), RuntimeActor->LevelAsset.Get(), LevelA);
	TestEqual(TEXT("The preview runtime receives the dungeon asset"), RuntimeActor->DungeonAsset.Get(), DungeonAsset);
	TestEqual(TEXT("The preview runtime receives the default level id"), RuntimeActor->CurrentDungeonLevelId, EntryA.LevelId);

	EditorActor->CurrentDungeonLevelId = EntryB.LevelId;
	TestTrue(TEXT("An explicit enabled level can be applied"), EditorActor->ApplyCurrentDungeonLevel());
	TestEqual(TEXT("The editor keeps the explicit level id"), EditorActor->CurrentDungeonLevelId, EntryB.LevelId);
	TestEqual(TEXT("The editor switches to the explicit level asset"), EditorActor->LevelAsset.Get(), LevelB);
	TestEqual(TEXT("The preview runtime receives the explicit level asset"), RuntimeActor->LevelAsset.Get(), LevelB);
	TestEqual(TEXT("The preview runtime receives the explicit level id"), RuntimeActor->CurrentDungeonLevelId, EntryB.LevelId);

	return true;
}

#endif
