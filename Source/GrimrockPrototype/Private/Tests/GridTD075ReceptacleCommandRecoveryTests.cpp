#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Core/GridObjectArchetypeAsset.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridReceptacleActor.h"

namespace GridTD075ReceptacleRecovery
{
	const FGuid SourceObjectId(7, 5, 1, 1);
	const FGuid ReceptacleObjectId(7, 5, 2, 2);
	const FName ReceptacleArchetypeId(TEXT("TD07_5_Receptacle"));

	struct FTestWorld
	{
		UWorld* World = nullptr;

		FTestWorld()
		{
			const UWorld::InitializationValues Values = UWorld::InitializationValues()
				.AllowAudioPlayback(false)
				.RequiresHitProxies(false)
				.CreatePhysicsScene(false)
				.CreateNavigation(false)
				.CreateAISystem(false)
				.ShouldSimulatePhysics(false)
				.SetTransactional(false);

			World = UWorld::CreateWorld(
				EWorldType::Game,
				false,
				FName(*FString::Printf(TEXT("TD075_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
				nullptr,
				true,
				ERHIFeatureLevel::Num,
				&Values);

			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FTestWorld()
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

	UGridItemDefinitionAsset* MakeItemDefinition(UObject* Outer, FName ItemId)
	{
		UGridItemDefinitionAsset* Definition = NewObject<UGridItemDefinitionAsset>(Outer);
		Definition->ItemDefinitionId = ItemId;
		Definition->DisplayName = FText::FromName(ItemId);
		Definition->Description = FText::FromString(TEXT("TD07.5 transient receptacle command fixture."));
		Definition->ItemType = EGridItemType::Component;
		Definition->Weight = 0.0f;
		Definition->bStackable = false;
		Definition->MaxStackSize = 1;
		return Definition;
	}

	FGridObjectLink MakeLink(EGridObjectEvent Event, EGridObjectCommand Command)
	{
		FGridObjectLink Link;
		Link.SourceObjectId = SourceObjectId;
		Link.TargetObjectId = ReceptacleObjectId;
		Link.SourceEvent = Event;
		Link.Command = Command;
		return Link;
	}

	AGridLevelRuntimeActor* BuildRuntime(FAutomationTestBase& Test, UWorld* World)
	{
		if (!World)
		{
			return nullptr;
		}

		AGridLevelRuntimeActor* Runtime = World->SpawnActor<AGridLevelRuntimeActor>();
		if (!Runtime)
		{
			Test.AddError(TEXT("TD07.5 could not spawn GridLevelRuntimeActor."));
			return nullptr;
		}

		UGridItemDefinitionAsset* ItemA = MakeItemDefinition(Runtime, TEXT("Item_TD075_A"));
		UGridItemDefinitionAsset* ItemB = MakeItemDefinition(Runtime, TEXT("Item_TD075_B"));

		UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Runtime);
		Level->Width = 1;
		Level->Height = 1;
		Level->EnsureCellCount();
		Level->Cells[0].CellType = EGridCellType::Floor;

		FGridLevelObjectData& Source = Level->Objects.AddDefaulted_GetRef();
		Source.ObjectId = SourceObjectId;
		Source.Type = EGridLevelObjectType::Logic;
		Source.LogicId = TEXT("TD07_5_Source");
		Source.CellX = 0;
		Source.CellY = 0;
		Source.bInitiallyEnabled = true;

		FGridLevelObjectData& Receptacle = Level->Objects.AddDefaulted_GetRef();
		Receptacle.ObjectId = ReceptacleObjectId;
		Receptacle.ArchetypeId = ReceptacleArchetypeId;
		Receptacle.Type = EGridLevelObjectType::Receptacle;
		Receptacle.CellX = 0;
		Receptacle.CellY = 0;
		Receptacle.Edge = EGridEdge::North;
		Receptacle.bInitiallyEnabled = true;
		Receptacle.Behavior.Receptacle.MaxContainedItems = 2;

		FGridReceptacleInitialItemConfig& InitialA =
			Receptacle.Behavior.Receptacle.InitialContent.AddDefaulted_GetRef();
		InitialA.ItemDefinition = ItemA;
		InitialA.Quantity = 1;

		FGridReceptacleInitialItemConfig& InitialB =
			Receptacle.Behavior.Receptacle.InitialContent.AddDefaulted_GetRef();
		InitialB.ItemDefinition = ItemB;
		InitialB.Quantity = 1;

		Level->Links.Add(MakeLink(EGridObjectEvent::Activated, EGridObjectCommand::ReceptacleConsumeItem));
		Level->Links.Add(MakeLink(EGridObjectEvent::Used, EGridObjectCommand::ReceptacleDisableRemoval));
		Level->Links.Add(MakeLink(EGridObjectEvent::Opened, EGridObjectCommand::ReceptacleEnableRemoval));
		Level->Links.Add(MakeLink(EGridObjectEvent::Closed, EGridObjectCommand::ReceptacleConsumeAllItems));

		UGridObjectArchetypeAsset* Archetype = NewObject<UGridObjectArchetypeAsset>(Runtime);
		Archetype->ArchetypeId = ReceptacleArchetypeId;
		Archetype->SupportedType = EGridLevelObjectType::Receptacle;
		Archetype->PlacementKind = EGridObjectPlacementKind::Wall;
		Archetype->bIsInteractable = true;
		Archetype->RuntimeActorClass = AGridReceptacleActor::StaticClass();
		Archetype->PreviewMesh = NewObject<UStaticMesh>(Runtime);

		Runtime->LevelAsset = Level;
		Runtime->ObjectArchetypes.Add(Archetype);
		Runtime->RebuildLevel();
		return Runtime;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridTD075ReceptacleCommandRecoveryTest,
	"Grimrock.TechnicalDebt.TD07_5.Recovery.ReceptacleCommands",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD075ReceptacleCommandRecoveryTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD075ReceptacleRecovery;

	FTestWorld TestWorld;
	TestNotNull(TEXT("TD07.5 transient world exists"), TestWorld.World);
	if (!TestWorld.World)
	{
		return false;
	}

	AGridLevelRuntimeActor* Runtime = BuildRuntime(*this, TestWorld.World);
	TestNotNull(TEXT("TD07.5 runtime exists"), Runtime);
	if (!Runtime)
	{
		return false;
	}

	AGridReceptacleActor* Receptacle =
		Runtime->FindRuntimeObjectActor<AGridReceptacleActor>(ReceptacleObjectId);
	TestNotNull(TEXT("TD07.5 receptacle spawned from transient fixture"), Receptacle);
	if (!Receptacle)
	{
		return false;
	}

	TestEqual(TEXT("Fixture starts with two contained items"), Receptacle->GetContainedItemCount(), 2);
	TestTrue(TEXT("Fixture starts with removal enabled"), Receptacle->bCanRemoveItem);

	TestTrue(
		TEXT("Event -> ReceptacleConsumeItem succeeds"),
		Runtime->ExecuteLinksFromRuntimeObject(SourceObjectId, EGridObjectEvent::Activated));
	TestEqual(TEXT("ConsumeItem removes exactly one contained entry"), Receptacle->GetContainedItemCount(), 1);

	TestTrue(
		TEXT("Event -> ReceptacleDisableRemoval succeeds"),
		Runtime->ExecuteLinksFromRuntimeObject(SourceObjectId, EGridObjectEvent::Used));
	TestFalse(TEXT("DisableRemoval updates receptacle runtime state"), Receptacle->bCanRemoveItem);

	TestTrue(
		TEXT("Event -> ReceptacleEnableRemoval succeeds"),
		Runtime->ExecuteLinksFromRuntimeObject(SourceObjectId, EGridObjectEvent::Opened));
	TestTrue(TEXT("EnableRemoval updates receptacle runtime state"), Receptacle->bCanRemoveItem);

	TestTrue(
		TEXT("Event -> ReceptacleConsumeAllItems succeeds"),
		Runtime->ExecuteLinksFromRuntimeObject(SourceObjectId, EGridObjectEvent::Closed));
	TestEqual(TEXT("ConsumeAllItems empties the receptacle"), Receptacle->GetContainedItemCount(), 0);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
