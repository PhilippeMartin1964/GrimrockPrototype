#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridObjectArchetypeAsset.h"
#include "Core/GridObjectPaletteAsset.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridItemDefinitionAsset.h"

namespace
{
	struct FMIG09AuthorityWorld
	{
		UWorld* World = nullptr;
		AGridLevelEditorActor* Editor = nullptr;
		UGridLevelAsset* Level = nullptr;

		FMIG09AuthorityWorld()
		{
			const UWorld::InitializationValues Values = UWorld::InitializationValues()
				.AllowAudioPlayback(false).RequiresHitProxies(false).CreatePhysicsScene(false)
				.CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
			World = UWorld::CreateWorld(EWorldType::EditorPreview, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
			if (!World) return;
			if (GEngine) GEngine->CreateNewWorldContext(EWorldType::EditorPreview).SetCurrentWorld(World);
			Editor = World->SpawnActor<AGridLevelEditorActor>();
			if (!Editor) return;
			Level = NewObject<UGridLevelAsset>(Editor);
			Level->Width = 5;
			Level->Height = 3;
			Level->EnsureCellCount();
			for (FGridLevelCellData& Cell : Level->Cells) { Cell.CellType = EGridCellType::Floor; Cell.bBlocksOccupancy = false; }
			Editor->LevelAsset = Level;
		}

		~FMIG09AuthorityWorld()
		{
			if (!World) return;
			World->DestroyWorld(false);
			if (GEngine) GEngine->DestroyWorldContext(World);
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMIG09EditorTypedMutationsTest, "Grimrock.WorldObjects.MIG09.EditorTypedMutations",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMIG09EditorTypedMutationsTest::RunTest(const FString& Parameters)
{
	FMIG09AuthorityWorld Fixture;
	if (!TestNotNull(TEXT("Editor exists"), Fixture.Editor)) return false;
	AGridLevelEditorActor* Editor = Fixture.Editor;
	UGridLevelAsset* Level = Fixture.Level;
	UGridItemDefinitionAsset* ItemDefinition = NewObject<UGridItemDefinitionAsset>(Level);
	ItemDefinition->ItemDefinitionId = TEXT("MIG09_Item");
	UGridObjectArchetypeAsset* Definition = NewObject<UGridObjectArchetypeAsset>(Level);
	Definition->ArchetypeId = TEXT("MIG09_Receptacle");
	Definition->SupportedType = EGridLevelObjectType::Receptacle;
	Definition->DefaultBehavior.Receptacle.MaxContainedItems = 4;
	UGridObjectPaletteAsset* Palette = NewObject<UGridObjectPaletteAsset>(Editor);
	FGridObjectPaletteEntry& Entry = Palette->Entries.AddDefaulted_GetRef();
	Entry.EntryId = Definition->ArchetypeId;
	Entry.DefaultArchetype = Definition;
	Editor->ObjectPalette = Palette;

	FGridWorldObjectInstance& WorldObject = Level->WorldObjectInstances.AddDefaulted_GetRef();
	WorldObject.InstanceId = FGuid::NewGuid();
	WorldObject.Type = EGridLevelObjectType::Receptacle;
	WorldObject.WorldObjectDefinitionId = Definition->ArchetypeId;
	WorldObject.bHasLocalTransformOverride = true;
	WorldObject.LocalTransformOverride = FTransform(FRotator(10.f, 20.f, 30.f), FVector(1.f, 2.f, 3.f), FVector(2.f));
	FGridLooseItemInstance& LooseItem = Level->LooseItemInstances.AddDefaulted_GetRef();
	LooseItem.InstanceId = FGuid::NewGuid();
	LooseItem.CellX = 1;
	LooseItem.ItemDefinition = ItemDefinition;
	LooseItem.Quantity = 7;
	LooseItem.LocalOffset = FVector(3.f, 4.f, 5.f);
	LooseItem.ReadTextOverride = FText::FromString(TEXT("Keep this text"));
	FGridMonsterSpawnInstance& Monster = Level->MonsterSpawns.AddDefaulted_GetRef();
	Monster.SpawnId = FGuid::NewGuid();
	Monster.CellX = 2;
	Monster.EncounterGroupId = TEXT("Encounter");
	Monster.PatrolWaypoints.AddDefaulted();
	FGridItemSpawnInstance& ItemSpawn = Level->ItemSpawns.AddDefaulted_GetRef();
	ItemSpawn.SpawnId = FGuid::NewGuid();
	ItemSpawn.CellX = 3;
	ItemSpawn.ItemDefinition = ItemDefinition;
	ItemSpawn.Quantity = 9;
	FGridLogicObjectInstance& Logic = Level->LogicObjects.AddDefaulted_GetRef();
	Logic.InstanceId = FGuid::NewGuid();
	Logic.CellX = 4;
	Logic.Logic.NodeType = EGridLogicNodeType::Latch;

	const FGuid Ids[] = { WorldObject.InstanceId, LooseItem.InstanceId, Monster.SpawnId, ItemSpawn.SpawnId, Logic.InstanceId };
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Ids); ++Index)
	{
		TestTrue(TEXT("Select native placement"), Editor->SelectObjectById(Ids[Index]));
		TestTrue(TEXT("Edit tag on native placement"), Editor->SetSelectedObjectTag(TEXT("Edited")));
		Editor->SelectedCellX = Index;
		Editor->SelectedCellY = 1;
		Editor->SelectedEdge = EGridEdge::North;
		TestTrue(TEXT("Move native placement"), Editor->MoveSelectedObjectToCurrentSelection());
		int32 X = -1, Y = -1;
		EGridEdge Edge = EGridEdge::None;
		TestTrue(TEXT("Locate moved placement"), Level->TryGetTypedPlacementLocation(Ids[Index], X, Y, Edge));
		TestEqual(TEXT("Moved cell Y"), Y, 1);
	}
	TestEqual(TEXT("Loose item quantity survives editing"), LooseItem.Quantity, 7);
	TestTrue(TEXT("Loose item offset survives editing"), LooseItem.LocalOffset.Equals(FVector(3.f, 4.f, 5.f)));
	TestEqual(TEXT("Loose item readable text survives editing"), LooseItem.ReadTextOverride.ToString(), FString(TEXT("Keep this text")));
	TestEqual(TEXT("Spawner quantity survives editing"), ItemSpawn.Quantity, 9);
	TestEqual(TEXT("Monster route survives editing"), Monster.PatrolWaypoints.Num(), 1);
	TestEqual(TEXT("Monster encounter survives editing"), Monster.EncounterGroupId, FName(TEXT("Encounter")));
	TestEqual(TEXT("Logic node survives editing"), Logic.Logic.NodeType, EGridLogicNodeType::Latch);
	TestTrue(TEXT("World transform survives editing"), WorldObject.LocalTransformOverride.GetScale3D().Equals(FVector(2.f)));
	TestTrue(TEXT("Select world object"), Editor->SelectObjectById(WorldObject.InstanceId));
	FGridObjectBehaviorParams EditedBehavior = Editor->ObjectBehavior;
	EditedBehavior.Receptacle.MaxContainedItems = 99;
	FGridReceptacleInitialItemConfig& Content = EditedBehavior.Receptacle.InitialContent.AddDefaulted_GetRef();
	Content.ItemDefinition = ItemDefinition;
	Content.Quantity = 2;
	TestTrue(TEXT("Apply only instance behavior"), Editor->ApplyBehaviorToSelectedObject(EditedBehavior));
	TestEqual(TEXT("Definition rule remains authoritative"), Editor->ObjectBehavior.Receptacle.MaxContainedItems, 4);
	TestEqual(TEXT("Local content is editable"), WorldObject.InstanceConfig.ReceptacleInitialContent.Num(), 1);
	TestEqual(TEXT("No placement was converted or duplicated"), Level->GetTypedPlacementCount(), 5);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMIG09EditorTypedValidationTest, "Grimrock.WorldObjects.MIG09.EditorTypedValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMIG09EditorTypedValidationTest::RunTest(const FString& Parameters)
{
	FMIG09AuthorityWorld Fixture;
	if (!TestNotNull(TEXT("Editor exists"), Fixture.Editor)) return false;
	UGridLevelAsset* Level = Fixture.Level;
	FGridLooseItemInstance& LooseItem = Level->LooseItemInstances.AddDefaulted_GetRef();
	LooseItem.InstanceId = FGuid::NewGuid();
	LooseItem.CellX = 1;
	LooseItem.ItemDefinition = NewObject<UGridItemDefinitionAsset>(Level);
	LooseItem.ItemDefinition->ItemDefinitionId = TEXT("MIG09_DirectItem");
	FGridItemSpawnInstance& ItemSpawn = Level->ItemSpawns.AddDefaulted_GetRef();
	ItemSpawn.SpawnId = FGuid::NewGuid();
	ItemSpawn.CellX = 2;
	FGridLogicObjectInstance& Logic = Level->LogicObjects.AddDefaulted_GetRef();
	Logic.InstanceId = FGuid::NewGuid();
	Logic.CellX = 3;
	Logic.Logic.NodeType = EGridLogicNodeType::Relay;
	FGridObjectLink& Link = Level->Links.AddDefaulted_GetRef();
	Link.SourceObjectId = Logic.InstanceId;
	Link.TargetObjectId = Logic.InstanceId;
	Link.SourceEvent = EGridObjectEvent::Activated;
	Link.Command = EGridObjectCommand::LogicReset;
	const auto HasError = [](const TArray<FGridLevelValidationMessage>& Messages, FGuid Id, const TCHAR* Text)
	{
		return Messages.ContainsByPredicate([Id, Text](const FGridLevelValidationMessage& Message)
		{
			return Message.OptionalObjectId == Id && Message.Severity == EGridLevelValidationSeverity::Error && Message.Message.Contains(Text);
		});
	};
	TArray<FGridLevelValidationMessage> Messages = Fixture.Editor->ValidateCurrentLevel();
	TestFalse(TEXT("Direct item needs no world definition"), HasError(Messages, LooseItem.InstanceId, TEXT("definition")));
	TestTrue(TEXT("Spawner requires its own item definition"), HasError(Messages, ItemSpawn.SpawnId, TEXT("ItemDefinition")));
	TestTrue(TEXT("Relay cannot reset like a Latch"), HasError(Messages, Logic.InstanceId, TEXT("not supported")));
	TestTrue(TEXT("Diagnostic resolves native location"), Messages.ContainsByPredicate([&ItemSpawn](const FGridLevelValidationMessage& Message)
	{
		return Message.OptionalObjectId == ItemSpawn.SpawnId && Message.CellX == 2 && Message.CellY == 0;
	}));
	Logic.Logic.NodeType = EGridLogicNodeType::Latch;
	ItemSpawn.ItemDefinition = LooseItem.ItemDefinition;
	ItemSpawn.SpawnId = LooseItem.InstanceId;
	Messages = Fixture.Editor->ValidateCurrentLevel();
	TestFalse(TEXT("Latch supports reset"), HasError(Messages, Logic.InstanceId, TEXT("not supported")));
	TestTrue(TEXT("Duplicate ids across collections are rejected"), HasError(Messages, LooseItem.InstanceId, TEXT("Duplicate ObjectId")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
