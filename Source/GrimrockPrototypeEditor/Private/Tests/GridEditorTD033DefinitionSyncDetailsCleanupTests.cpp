#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Core/GridObjectArchetypeAsset.h"
#include "Core/GridObjectPaletteAsset.h"
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD033DefinitionSyncContractTest, "Grimrock.TechnicalDebt.TD03_3.ObjectInspectorDetails.DefinitionSyncContract",
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

	UGridObjectArchetypeAsset* ItemArchetype = NewObject<UGridObjectArchetypeAsset>(EditorActor);
	ItemArchetype->ArchetypeId = TEXT("TD033_ItemPickup");
	ItemArchetype->SupportedType = EGridLevelObjectType::Item;
	ItemArchetype->DefaultBehavior.Item.ItemDefinitionAsset = ItemDefinition;
	ItemArchetype->DefaultBehavior.Item.ItemDefinitionId = NAME_None;

	UGridObjectPaletteAsset* ObjectPalette = NewObject<UGridObjectPaletteAsset>(EditorActor);
	FGridObjectPaletteEntry& ItemPaletteEntry = ObjectPalette->Entries.AddDefaulted_GetRef();
	ItemPaletteEntry.EntryId = TEXT("TD033_Item");
	ItemPaletteEntry.DefaultArchetype = ItemArchetype;
	EditorActor->ObjectPalette = ObjectPalette;

	UGridMonsterDefinitionAsset* MonsterDefinition = NewObject<UGridMonsterDefinitionAsset>(EditorActor);
	MonsterDefinition->MonsterId = TEXT("TD033_Monster");

	FGridLooseItemInstance ItemObject;
	ItemObject.InstanceId = FGuid::NewGuid();
	ItemObject.ItemDefinition = ItemDefinition;
	ItemObject.CellX = 0;
	ItemObject.CellY = 0;
	LevelAsset->LooseItemInstances.Add(ItemObject);

	FGridMonsterSpawnInstance MonsterObject;
	MonsterObject.SpawnId = FGuid::NewGuid();
	MonsterObject.MonsterDefinition = MonsterDefinition;
	MonsterObject.CellX = 1;
	MonsterObject.CellY = 0;
	MonsterObject.Facing = EGridEdge::North;
	LevelAsset->MonsterSpawns.Add(MonsterObject);

	const UFunction* ItemSyncFunction = EditorActor->FindFunction(TEXT("SyncSelectedItemDefinitionIdFromAsset"));
	const UFunction* MonsterSyncFunction = EditorActor->FindFunction(TEXT("SyncSelectedMonsterDefinitionIdFromAsset"));
	TestNotNull(TEXT("SyncSelectedItemDefinitionIdFromAsset remains reflected"), ItemSyncFunction);
	TestNotNull(TEXT("SyncSelectedMonsterDefinitionIdFromAsset remains reflected"), MonsterSyncFunction);
	TestTrue(TEXT("Item definition repair is BlueprintCallable"), ItemSyncFunction && ItemSyncFunction->HasAnyFunctionFlags(FUNC_BlueprintCallable));
	TestTrue(TEXT("Monster definition sync is BlueprintCallable"), MonsterSyncFunction && MonsterSyncFunction->HasAnyFunctionFlags(FUNC_BlueprintCallable));
	TestFalse(TEXT("Item definition sync is no longer exposed as CallInEditor"), ItemSyncFunction && ItemSyncFunction->HasMetaData(TEXT("CallInEditor")));
	TestFalse(
		TEXT("Monster definition sync is no longer exposed as CallInEditor"), MonsterSyncFunction && MonsterSyncFunction->HasMetaData(TEXT("CallInEditor")));

	TestTrue(TEXT("The typed item can be selected"), EditorActor->SelectObjectById(ItemObject.InstanceId));
	TestFalse(TEXT("Monster sync rejects an item selection"), EditorActor->SyncSelectedMonsterDefinitionIdFromAsset());
	TestTrue(TEXT("Item repair accepts the canonical typed item definition"), EditorActor->SyncSelectedItemDefinitionIdFromAsset());
	TestTrue(TEXT("The typed item keeps the canonical definition asset"), LevelAsset->LooseItemInstances[0].ItemDefinition == ItemDefinition);
	TestTrue(TEXT("The monster spawn remains untouched by item sync"), LevelAsset->MonsterSpawns[0].MonsterDefinition == MonsterDefinition);

	TestTrue(TEXT("The typed monster spawn can be selected"), EditorActor->SelectObjectById(MonsterObject.SpawnId));
	TestFalse(TEXT("Item sync rejects a monster selection"), EditorActor->SyncSelectedItemDefinitionIdFromAsset());
	TestTrue(TEXT("Monster sync accepts the canonical typed definition asset"), EditorActor->SyncSelectedMonsterDefinitionIdFromAsset());
	TestTrue(TEXT("The typed monster keeps its definition asset"), LevelAsset->MonsterSpawns[0].MonsterDefinition == MonsterDefinition);
	TestTrue(TEXT("The typed item remains canonical after monster sync"), LevelAsset->LooseItemInstances[0].ItemDefinition == ItemDefinition);

	EditorActor->SelectedCellX = 2;
	EditorActor->SelectedCellY = 2;
	EditorActor->SelectedEdge = EGridEdge::None;
	TestTrue(TEXT("The item palette entry can be applied"), EditorActor->ApplyPaletteEntry(ItemPaletteEntry.EntryId));
	const int32 ItemCountBeforePlacement = LevelAsset->LooseItemInstances.Num();
	EditorActor->PlaceSelectedObject();
	TestEqual(TEXT("Placing the palette item adds exactly one typed loose item"), LevelAsset->LooseItemInstances.Num(), ItemCountBeforePlacement + 1);
	const FGridLooseItemInstance& PlacedItem = LevelAsset->LooseItemInstances.Last();
	TestTrue(TEXT("The placed item stores the canonical definition asset"), PlacedItem.ItemDefinition == ItemDefinition);
	TestEqual(TEXT("The placed item stores the source palette entry"), PlacedItem.PaletteEntryId, ItemPaletteEntry.EntryId);

	return true;
}

#endif
