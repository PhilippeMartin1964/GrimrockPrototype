#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Core/GridObjectArchetypeAsset.h"
#include "Core/GridObjectPaletteAsset.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

namespace
{
	struct FMIG06EditorTestWorld
	{
		UWorld* World = nullptr;

		FMIG06EditorTestWorld()
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
				FName(*FString::Printf(TEXT("MIG06EditorWorld_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true,
				ERHIFeatureLevel::Num, &Values);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::EditorPreview);
				Context.SetCurrentWorld(World);
			}
		}

		~FMIG06EditorTestWorld()
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
	FGridEditorWorldObjectMIG06SparsePlacementTest,
	"Grimrock.WorldObjects.MIG06.EditorSparsePlacement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorWorldObjectMIG06SparsePlacementTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FMIG06EditorTestWorld TestWorld;
	TestNotNull(TEXT("MIG06 editor world exists"), TestWorld.World);
	if (!TestWorld.World)
	{
		return false;
	}

	AGridLevelEditorActor* EditorActor = TestWorld.World->SpawnActor<AGridLevelEditorActor>();
	TestNotNull(TEXT("MIG06 editor actor exists"), EditorActor);
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

	UGridObjectArchetypeAsset* Definition = NewObject<UGridObjectArchetypeAsset>(EditorActor);
	Definition->ArchetypeId = TEXT("MIG06_Button");
	Definition->SupportedType = EGridLevelObjectType::Button;
	Definition->PlacementSurface = EGridObjectPlacementKind::Center;
	Definition->DefaultBehavior.ButtonAnimation.ButtonHoldTime = 0.77f;

	UGridObjectPaletteAsset* Palette = NewObject<UGridObjectPaletteAsset>(EditorActor);
	FGridObjectPaletteEntry& Entry = Palette->Entries.AddDefaulted_GetRef();
	Entry.EntryId = TEXT("MIG06_Button");
	Entry.DefaultArchetype = Definition;
	EditorActor->ObjectPalette = Palette;

	EditorActor->SelectedCellX = 1;
	EditorActor->SelectedCellY = 1;
	EditorActor->SelectedEdge = EGridEdge::None;
	TestTrue(TEXT("Definition palette entry can be applied"), EditorActor->ApplyPaletteEntry(Entry.EntryId));
	TestEqual(TEXT("Editor staging starts from definition behavior"), EditorActor->ObjectBehavior.ButtonAnimation.ButtonHoldTime, 0.77f);

	EditorActor->PlaceSelectedObject();
	TestEqual(TEXT("Placement creates one object"), Level->Objects.Num(), 1);
	if (Level->Objects.Num() != 1)
	{
		return false;
	}

	const FGuid ObjectId = Level->Objects[0].ObjectId;
	TestTrue(TEXT("New placement is marked as sparse"), Level->UsesSparseBehaviorOverrides(ObjectId));
	TestTrue(TEXT("Placed instance does not clone ButtonHoldTime"),
		!FMath::IsNearlyEqual(Level->Objects[0].Behavior.ButtonAnimation.ButtonHoldTime, 0.77f));

	TestTrue(TEXT("Sparse object can be selected by id"), EditorActor->SelectObjectById(ObjectId));
	TestEqual(TEXT("Selection resolves ButtonHoldTime back from definition"), EditorActor->ObjectBehavior.ButtonAnimation.ButtonHoldTime, 0.77f);
	const FGridLevelObjectData* InspectorView = EditorActor->GetSelectedObjectData();
	TestNotNull(TEXT("Inspector receives a selected object view"), InspectorView);
	if (InspectorView)
	{
		TestEqual(TEXT("Inspector view resolves definition-owned ButtonHoldTime"), InspectorView->Behavior.ButtonAnimation.ButtonHoldTime, 0.77f);
	}

	FGridObjectBehaviorParams EditedBehavior = EditorActor->ObjectBehavior;
	EditedBehavior.ButtonAnimation.ButtonHoldTime = 9.0f; // must not become an instance authority
	EditedBehavior.Transition.bIsTransition = true;
	EditedBehavior.Transition.TargetLevelId = TEXT("MIG06_Target");
	EditedBehavior.Transition.TargetCellX = 3;
	EditedBehavior.Transition.TargetCellY = 4;
	TestTrue(TEXT("Inspector behavior edit keeps sparse storage"), EditorActor->ApplyBehaviorToSelectedObject(EditedBehavior));
	TestTrue(TEXT("Edited object remains sparse"), Level->UsesSparseBehaviorOverrides(ObjectId));
	TestTrue(TEXT("Definition-owned ButtonHoldTime is still not stored"),
		!FMath::IsNearlyEqual(Level->Objects[0].Behavior.ButtonAnimation.ButtonHoldTime, 9.0f));
	TestEqual(TEXT("Transition target X is stored as instance data"), Level->Objects[0].Behavior.Transition.TargetCellX, 3);
	TestEqual(TEXT("Transition target Y is stored as instance data"), Level->Objects[0].Behavior.Transition.TargetCellY, 4);

	TestTrue(TEXT("Edited sparse object can be reselected"), EditorActor->SelectObjectById(ObjectId));
	TestEqual(TEXT("Reselection restores definition-owned ButtonHoldTime"), EditorActor->ObjectBehavior.ButtonAnimation.ButtonHoldTime, 0.77f);
	TestEqual(TEXT("Reselection preserves instance transition"), EditorActor->ObjectBehavior.Transition.TargetLevelId, FName(TEXT("MIG06_Target")));
	InspectorView = EditorActor->GetSelectedObjectData();
	TestNotNull(TEXT("Inspector view survives sparse edit/reselection"), InspectorView);
	if (InspectorView)
	{
		TestEqual(TEXT("Inspector view keeps definition value after sparse edit"), InspectorView->Behavior.ButtonAnimation.ButtonHoldTime, 0.77f);
		TestEqual(TEXT("Inspector view keeps instance transition after sparse edit"), InspectorView->Behavior.Transition.TargetLevelId, FName(TEXT("MIG06_Target")));
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
