#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Core/GridObjectPaletteAsset.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Runtime/GridItemDefinitionAsset.h"

namespace
{
	struct FMIG05EditorTestWorld
	{
		UWorld* World = nullptr;

		FMIG05EditorTestWorld()
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
				FName(*FString::Printf(TEXT("MIG05EditorItemWorld_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true,
				ERHIFeatureLevel::Num, &Values);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::EditorPreview);
				Context.SetCurrentWorld(World);
			}
		}

		~FMIG05EditorTestWorld()
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridEditorWorldObjectMIG05DirectItemPlacementTest,
	"Grimrock.WorldObjects.MIG05.EditorDirectItemPlacement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorWorldObjectMIG05DirectItemPlacementTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FMIG05EditorTestWorld TestWorld;
	TestNotNull(TEXT("MIG05 editor preview world exists"), TestWorld.World);
	if (!TestWorld.World)
	{
		return false;
	}

	AGridLevelEditorActor* EditorActor = TestWorld.World->SpawnActor<AGridLevelEditorActor>();
	TestNotNull(TEXT("MIG05 grid editor actor exists"), EditorActor);
	if (!EditorActor)
	{
		return false;
	}

	UGridLevelAsset* Level = NewObject<UGridLevelAsset>(EditorActor);
	Level->Width = 2;
	Level->Height = 2;
	Level->EnsureCellCount();
	for (FGridLevelCellData& Cell : Level->Cells)
	{
		Cell.CellType = EGridCellType::Floor;
		Cell.bBlocksOccupancy = false;
	}
	EditorActor->LevelAsset = Level;

	UGridItemDefinitionAsset* Definition = NewObject<UGridItemDefinitionAsset>(EditorActor);
	Definition->ItemDefinitionId = TEXT("BlueGem");
	Definition->DisplayName = FText::FromString(TEXT("Blue Gem"));
	Definition->ItemType = EGridItemType::Gem;
	Definition->WorldMesh = NewObject<UStaticMesh>(Definition);

	UGridObjectPaletteAsset* Palette = NewObject<UGridObjectPaletteAsset>(EditorActor);
	FGridObjectPaletteEntry& Entry = Palette->Entries.AddDefaulted_GetRef();
	Entry.EntryId = TEXT("BlueGem");
	Entry.DefaultItemDefinition = Definition;
	EditorActor->ObjectPalette = Palette;

	EditorActor->SelectedCellX = 1;
	EditorActor->SelectedCellY = 1;
	EditorActor->SelectedEdge = EGridEdge::None;

	TestTrue(TEXT("Direct ItemDefinition palette entry can be selected without DefaultArchetype"), EditorActor->ApplyPaletteEntry(Entry.EntryId));
	TestEqual(TEXT("Direct collectible palette selects Item paint type"), EditorActor->PaintObjectType, EGridLevelObjectType::Item);
	TestTrue(TEXT("Direct collectible palette leaves ObjectArchetypeId empty"), EditorActor->ObjectArchetypeId.IsNone());
	TestTrue(TEXT("Direct collectible palette leaves SelectedArchetypeId empty"), EditorActor->SelectedArchetypeId.IsNone());
	TestTrue(TEXT("Direct collectible staging references the ItemDefinition"), EditorActor->ObjectBehavior.Item.ItemDefinitionAsset == Definition);

	EditorActor->PlaceSelectedObject();
	TestEqual(TEXT("Direct collectible placement creates exactly one loose item"), Level->LooseItemInstances.Num(), 1);
	if (Level->LooseItemInstances.Num() != 1)
	{
		return false;
	}

	const FGridLooseItemInstance& PlacedItem = Level->LooseItemInstances[0];
	TestEqual(TEXT("Placed collectible type is Item"), Level->GetTypedPlacementType(PlacedItem.InstanceId), EGridLevelObjectType::Item);
	TestTrue(TEXT("Placed collectible has no companion world object"), Level->WorldObjectInstances.IsEmpty());
	TestTrue(TEXT("Placed collectible stores the canonical ItemDefinition asset"), PlacedItem.ItemDefinition == Definition);
	TestEqual(TEXT("Placement creates exactly one typed entry"), Level->GetTypedPlacementCount(), 1);
	TestEqual(TEXT("Placed collectible keeps the selected cell X"), PlacedItem.CellX, 1);
	TestEqual(TEXT("Placed collectible keeps the selected cell Y"), PlacedItem.CellY, 1);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
