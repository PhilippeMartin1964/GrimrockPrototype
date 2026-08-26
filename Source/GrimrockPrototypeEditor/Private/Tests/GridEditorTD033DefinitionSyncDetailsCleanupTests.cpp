#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "UObject/Class.h"

namespace
{
	struct FGridTD033EditorTestWorld
	{
		UWorld* World = nullptr;

		FGridTD033EditorTestWorld()
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
				FName(*FString::Printf(TEXT("TD033DefinitionSyncWorld_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true,
				ERHIFeatureLevel::Num, &Values);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::EditorPreview);
				Context.SetCurrentWorld(World);
			}
		}

		~FGridTD033EditorTestWorld()
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD033DefinitionSyncContractTest,
	"Grimrock.TechnicalDebt.TD03_3.ObjectInspectorDetails.DefinitionSyncContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD033DefinitionSyncContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridTD033EditorTestWorld TestWorld;
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

	UGridItemDefinitionAsset* ItemDefinition = NewObject<UGridItemDefinitionAsset>(EditorActor);
	ItemDefinition->ItemDefinitionId = TEXT("TD033_Item");

	UGridMonsterDefinitionAsset* MonsterDefinition = NewObject<UGridMonsterDefinitionAsset>(EditorActor);
	MonsterDefinition->MonsterId = TEXT("TD033_Monster");

	FGridLevelObjectData ItemObject;
	ItemObject.ObjectId = FGuid::NewGuid();
	ItemObject.Type = EGridLevelObjectType::Item;
	ItemObject.CellX = 0;
	ItemObject.CellY = 0;
	ItemObject.Edge = EGridEdge::None;
	ItemObject.ItemDefinitionAsset = ItemDefinition;
	ItemObject.ItemDefinitionId = TEXT("Stale_Item");
	LevelAsset->Objects.Add(ItemObject);

	FGridLevelObjectData MonsterObject;
	MonsterObject.ObjectId = FGuid::NewGuid();
	MonsterObject.Type = EGridLevelObjectType::MonsterSpawn;
	MonsterObject.CellX = 1;
	MonsterObject.CellY = 0;
	MonsterObject.Edge = EGridEdge::None;
	MonsterObject.MonsterDefinitionAsset = MonsterDefinition;
	MonsterObject.MonsterDefinitionId = TEXT("Stale_Monster");
	LevelAsset->Objects.Add(MonsterObject);

	const UFunction* ItemSyncFunction = EditorActor->FindFunction(TEXT("SyncSelectedItemDefinitionIdFromAsset"));
	const UFunction* MonsterSyncFunction = EditorActor->FindFunction(TEXT("SyncSelectedMonsterDefinitionIdFromAsset"));
	TestNotNull(TEXT("SyncSelectedItemDefinitionIdFromAsset remains reflected"), ItemSyncFunction);
	TestNotNull(TEXT("SyncSelectedMonsterDefinitionIdFromAsset remains reflected"), MonsterSyncFunction);
	TestTrue(TEXT("Item definition sync is BlueprintCallable"),
		ItemSyncFunction && ItemSyncFunction->HasAnyFunctionFlags(FUNC_BlueprintCallable));
	TestTrue(TEXT("Monster definition sync is BlueprintCallable"),
		MonsterSyncFunction && MonsterSyncFunction->HasAnyFunctionFlags(FUNC_BlueprintCallable));
	TestFalse(TEXT("Item definition sync is no longer exposed as CallInEditor"),
		ItemSyncFunction && ItemSyncFunction->HasMetaData(TEXT("CallInEditor")));
	TestFalse(TEXT("Monster definition sync is no longer exposed as CallInEditor"),
		MonsterSyncFunction && MonsterSyncFunction->HasMetaData(TEXT("CallInEditor")));

	TestTrue(TEXT("The item object can be selected"), EditorActor->SelectObjectById(ItemObject.ObjectId));
	TestFalse(TEXT("Monster sync rejects an item selection"), EditorActor->SyncSelectedMonsterDefinitionIdFromAsset());
	TestTrue(TEXT("Item sync copies ItemDefinitionId from the selected asset"), EditorActor->SyncSelectedItemDefinitionIdFromAsset());
	TestEqual(TEXT("The item object stores the asset ItemDefinitionId"), LevelAsset->Objects[0].ItemDefinitionId, ItemDefinition->ItemDefinitionId);
	TestEqual(TEXT("The monster object remains untouched by item sync"), LevelAsset->Objects[1].MonsterDefinitionId, FName(TEXT("Stale_Monster")));

	TestTrue(TEXT("The monster object can be selected"), EditorActor->SelectObjectById(MonsterObject.ObjectId));
	TestFalse(TEXT("Item sync rejects a monster selection"), EditorActor->SyncSelectedItemDefinitionIdFromAsset());
	TestTrue(TEXT("Monster sync copies MonsterId from the selected asset"), EditorActor->SyncSelectedMonsterDefinitionIdFromAsset());
	TestEqual(TEXT("The monster object stores the asset MonsterId"), LevelAsset->Objects[1].MonsterDefinitionId, MonsterDefinition->MonsterId);
	TestEqual(TEXT("The item object remains synchronized"), LevelAsset->Objects[0].ItemDefinitionId, ItemDefinition->ItemDefinitionId);

	return true;
}

#endif
