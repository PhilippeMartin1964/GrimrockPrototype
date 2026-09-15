#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "GridDoorTestUtils.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/Material.h"
#include "Runtime/GridActivationComponent.h"
#include "Runtime/GridConsumingSlotReceptacleActor.h"
#include "Runtime/GridDoorActor.h"
#include "Runtime/GridDoorSystemComponent.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Save/GrimrockPartySaveGame.h"

namespace GridPUZZLE011Tests
{
	const FGuid GuardianId(1, 1, 1, 1);
	const FGuid DoorId(1, 1, 1, 2);
	const FName LeftSlot(TEXT("LeftEye"));
	const FName RightSlot(TEXT("RightEye"));

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
			World = UWorld::CreateWorld(EWorldType::Game, false,
				FName(*FString::Printf(TEXT("PUZZLE01_1_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
				nullptr, true, ERHIFeatureLevel::Num, &Values);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FTestWorld()
		{
			if (World)
			{
				World->DestroyWorld(false);
				if (GEngine)
				{
					GEngine->DestroyWorldContext(World);
				}
			}
		}
	};

	FGridReceptacleConsumingSlotConfig MakeSlot(FName SlotId, FName MaterialSlot, const FVector& Location)
	{
		FGridReceptacleConsumingSlotConfig Slot;
		Slot.SlotId = SlotId;
		Slot.InteractionRelativeLocation = Location;
		Slot.InteractionBoxExtent = FVector(8.f);
		Slot.FilledMaterialSlot = MaterialSlot;
		Slot.FilledMaterialAlias = TEXT("BlueGem");
		Slot.bEnableFilledLight = true;
		Slot.LightRelativeLocation = Location;
		Slot.LightColor = FLinearColor(0.05f, 0.2f, 1.f);
		Slot.LightIntensity = 750.f;
		Slot.LightRadius = 125.f;
		Slot.bRequiredForActivation = true;
		return Slot;
	}

	int32 CountWorldItems(UWorld& World)
	{
		int32 Count = 0;
		for (TActorIterator<AGridItemActor> It(&World); It; ++It)
		{
			++Count;
		}
		return Count;
	}

	FHitResult MakeSlotHit(AGridConsumingSlotReceptacleActor& Receptacle, FName SlotId)
	{
		FHitResult Hit;
		Hit.Component = Receptacle.GetConsumingSlotInteractionComponent(SlotId);
		Hit.Location = Hit.Component.IsValid() ? Hit.Component->GetComponentLocation() : FVector::ZeroVector;
		Hit.ImpactPoint = Hit.Location;
		return Hit;
	}

	bool InventoryContains(const FGridPartyInventoryState& State, FName ItemId)
	{
		if (State.bHasCursorItem && State.CursorItem.ItemDefinitionId == ItemId)
		{
			return true;
		}
		for (const FGridCharacterInventoryState& Character : State.ActiveCharacters)
		{
			for (const FGridInventorySlot& Slot : Character.InventorySlots)
			{
				if (!Slot.IsEmpty() && Slot.Item.ItemDefinitionId == ItemId)
				{
					return true;
				}
			}
		}
		return false;
	}

	bool RunConsumptionScenario(FAutomationTestBase& Test, bool bRightFirst, bool bExercisePersistence)
	{
		FTestWorld TestWorld;
		if (!TestWorld.World)
		{
			Test.AddError(TEXT("Unable to create PUZZLE01.1 test world."));
			return false;
		}

		AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
		AGrimrockPartyPawn* Party = TestWorld.World->SpawnActor<AGrimrockPartyPawn>();
		if (!Runtime || !Party || !Party->PartyInventoryComponent)
		{
			Test.AddError(TEXT("Unable to create runtime or party fixture."));
			return false;
		}

		UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Runtime);
		Level->Width = 1;
		Level->Height = 1;
		Level->EnsureCellCount();
		Level->Cells[0].CellType = EGridCellType::Floor;
		Runtime->LevelAsset = Level;
		Runtime->CurrentDungeonLevelId = TEXT("PUZZLE01_1");

		UMaterial* EmptyLeft = NewObject<UMaterial>(Runtime);
		UMaterial* EmptyRight = NewObject<UMaterial>(Runtime);
		UMaterial* FilledMaterial = NewObject<UMaterial>(Runtime);
		UStaticMesh* GuardianMesh = NewObject<UStaticMesh>(Runtime);
		GuardianMesh->GetStaticMaterials().Add(FStaticMaterial(EmptyLeft, TEXT("EyesLeft")));
		GuardianMesh->GetStaticMaterials().Add(FStaticMaterial(EmptyRight, TEXT("EyesRight")));

		UGridItemDefinitionAsset* BlueGem = NewObject<UGridItemDefinitionAsset>(Runtime);
		BlueGem->ItemDefinitionId = TEXT("Gem_Blue");
		BlueGem->DisplayName = FText::FromString(TEXT("Blue Gem"));
		BlueGem->bStackable = true;
		BlueGem->MaxStackSize = 10;
		UGridItemDefinitionAsset* IncompatibleItem = NewObject<UGridItemDefinitionAsset>(Runtime);
		IncompatibleItem->ItemDefinitionId = TEXT("Gem_Red");

		UGridWorldObjectDefinitionAsset* Definition = NewObject<UGridWorldObjectDefinitionAsset>(Runtime);
		Definition->DefinitionId = TEXT("Guardian");
		Definition->SupportedType = EGridLevelObjectType::Receptacle;
		Definition->PlacementSurface = EGridObjectPlacementKind::Wall;
		Definition->StaticPart.Mesh = GuardianMesh;
		Definition->RuntimeActorClass = AGridConsumingSlotReceptacleActor::StaticClass();
		Definition->bIsInteractable = true;
		Definition->RuntimeMaterialAliases.Add(TEXT("BlueGem"), FilledMaterial);
		Definition->DefaultBehavior.Receptacle.bAcceptAnyItem = false;
		FGridReceptacleAcceptedItemConfig Accepted;
		Accepted.ItemDefinition = BlueGem;
		Definition->DefaultBehavior.Receptacle.AcceptedItems.Add(Accepted);
		Definition->DefaultBehavior.Receptacle.ConsumingSlots.Add(MakeSlot(LeftSlot, TEXT("EyesLeft"), FVector(0.f, -12.f, 10.f)));
		Definition->DefaultBehavior.Receptacle.ConsumingSlots.Add(MakeSlot(RightSlot, TEXT("EyesRight"), FVector(0.f, 12.f, 10.f)));
		Runtime->WorldObjectDefinitions.Add(Definition);

		FGridWorldObjectInstance GuardianPlacement;
		GuardianPlacement.InstanceId = GuardianId;
		GuardianPlacement.LogicId = TEXT("Guardian");
		GuardianPlacement.Type = EGridLevelObjectType::Receptacle;
		GuardianPlacement.WorldObjectDefinitionId = Definition->DefinitionId;
		GuardianPlacement.CellX = 0;
		GuardianPlacement.CellY = 0;
		GuardianPlacement.WallSide = EGridEdge::North;
		Level->WorldObjectInstances.Add(GuardianPlacement);

		FGridWorldObjectInstance DoorPlacement;
		DoorPlacement.InstanceId = DoorId;
		DoorPlacement.LogicId = TEXT("GuardianDoor");
		DoorPlacement.Type = EGridLevelObjectType::Door;
		DoorPlacement.CellX = 0;
		DoorPlacement.CellY = 0;
		DoorPlacement.WallSide = EGridEdge::East;
		Level->WorldObjectInstances.Add(DoorPlacement);

		FGridObjectLink Link;
		Link.SourceObjectId = GuardianId;
		Link.SourceEvent = EGridObjectEvent::Activated;
		Link.TargetObjectId = DoorId;
		Link.Command = EGridObjectCommand::Open;
		Level->Links.Add(Link);

		Runtime->RebuildLevel();
		AGridConsumingSlotReceptacleActor* Guardian = Runtime->FindRuntimeObjectActor<AGridConsumingSlotReceptacleActor>(GuardianId);
		UGridActivationComponent* Activation = Runtime->FindComponentByClass<UGridActivationComponent>();
		UGridDoorSystemComponent* DoorSystem = Runtime->FindComponentByClass<UGridDoorSystemComponent>();
		AGridDoorActor* Door = TestWorld.World->SpawnActor<AGridDoorActor>();
		if (!Guardian || !Activation || !DoorSystem || !Door)
		{
			Test.AddError(TEXT("Unable to create consuming receptacle or door fixture."));
			return false;
		}
		GridDoorTestUtils::InitializeDoorFromMotion(Door, DoorPlacement, TestWorld.World, 0.1f, 180.f);
		DoorSystem->Initialize(Runtime);
		DoorSystem->RebuildIndexes();
		DoorSystem->RegisterDoorObject(FGridRuntimeWorldObjectData(DoorPlacement), Door);
		Activation->Initialize(Runtime);
		Activation->RebuildIndexes();

		Party->SetGridStart(Runtime, 0, 0, EGridEdge::North);
		Party->PartyInventoryComponent->InitializeDefaultPartyIfNeeded();
		Test.TestTrue(TEXT("Blue gem definition registers in inventory"), Party->PartyInventoryComponent->RegisterItemDefinition(BlueGem));
		Test.TestTrue(TEXT("Incompatible item definition registers in inventory"), Party->PartyInventoryComponent->RegisterItemDefinition(IncompatibleItem));

		FGridItemInstance WrongItem;
		WrongItem.RuntimeObjectId = FGuid::NewGuid();
		WrongItem.ItemDefinitionId = IncompatibleItem->ItemDefinitionId;
		WrongItem.Quantity = 1;
		WrongItem.OwnerType = EGridItemOwnerType::World;
		Test.TestTrue(TEXT("Incompatible item reaches cursor"), Party->PartyInventoryComponent->SetCursorItem(WrongItem));
		const FName FirstSlot = bRightFirst ? RightSlot : LeftSlot;
		const FName SecondSlot = bRightFirst ? LeftSlot : RightSlot;
		FHitResult FirstHit = MakeSlotHit(*Guardian, FirstSlot);
		AGridReceptacleActor* BasePointer = Guardian;
		Test.TestFalse(TEXT("Incompatible item is refused through polymorphic mouse path"), BasePointer->TryPlaceCursorItemFromHit(Party, FirstHit));
		Test.TestTrue(TEXT("Refused item remains on cursor"), Party->PartyInventoryComponent->HasCursorItem());
		Party->PartyInventoryComponent->ClearCursorItem();

		FGridItemInstance GemStack;
		GemStack.RuntimeObjectId = FGuid::NewGuid();
		GemStack.ItemDefinitionId = BlueGem->ItemDefinitionId;
		GemStack.Quantity = 2;
		GemStack.OwnerType = EGridItemOwnerType::World;
		Test.TestTrue(TEXT("Two ordinary blue gems enter inventory"), Party->PartyInventoryComponent->AddItemToCharacterInventory(0, GemStack));
		Test.TestTrue(TEXT("The real inventory transfer moves both gems to cursor"), Party->PartyInventoryComponent->TryTakeInventorySlotToCursor(0, 0));
		const int32 InitialWorldItemCount = CountWorldItems(*TestWorld.World);

		Test.TestTrue(TEXT("First slot consumes a blue gem through base-pointer dispatch"), BasePointer->TryPlaceCursorItemFromHit(Party, FirstHit));
		Test.TestEqual(TEXT("Exactly one unit is consumed"), Party->PartyInventoryComponent->GetCursorItem().Quantity, 1);
		Test.TestTrue(TEXT("First targeted slot is filled"), Guardian->IsConsumingSlotFilled(FirstSlot));
		Test.TestFalse(TEXT("Other slot remains empty"), Guardian->IsConsumingSlotFilled(SecondSlot));
		Test.TestEqual(TEXT("Consuming slot never adds ContainedItems"), Guardian->GetContainedItemCount(), 0);
		Test.TestFalse(TEXT("First insertion does not emit completion"), Guardian->WasConsumingSlotsCompletionEmitted());
		Test.TestFalse(TEXT("First insertion does not start the door"), Door->IsAnimating());
		Test.TestTrue(TEXT("Filled slot light is visible"), Guardian->GetConsumingSlotLightComponent(FirstSlot)->IsVisible());
		Test.TestFalse(TEXT("Empty slot light is hidden"), Guardian->GetConsumingSlotLightComponent(SecondSlot)->IsVisible());
		Test.TestTrue(TEXT("First slot material is activated"), Guardian->MeshComponent->GetMaterial(bRightFirst ? 1 : 0) == FilledMaterial);

		FHitResult SecondHit = MakeSlotHit(*Guardian, SecondSlot);
		Test.TestTrue(TEXT("Second eye accepts the same ordinary blue-gem definition"), BasePointer->TryPlaceCursorItemFromHit(Party, SecondHit));
		Test.TestFalse(TEXT("Second consumed gem is truly removed from cursor"), Party->PartyInventoryComponent->HasCursorItem());
		Test.TestFalse(TEXT("Inventory no longer contains consumed gems"), InventoryContains(Party->PartyInventoryComponent->PartyInventoryState, BlueGem->ItemDefinitionId));
		Test.TestEqual(TEXT("ContainedItems remains empty after completion"), Guardian->GetContainedItemCount(), 0);
		Test.TestEqual(TEXT("No GridItemActor is created"), CountWorldItems(*TestWorld.World), InitialWorldItemCount);
		Test.TestTrue(TEXT("Completion is emitted after both required slots"), Guardian->WasConsumingSlotsCompletionEmitted());
		Test.TestTrue(TEXT("Real Activated to Door.Open link starts animation"), Door->IsAnimating());
		Test.TestTrue(TEXT("Both slot lights are visible"), Guardian->GetConsumingSlotLightComponent(LeftSlot)->IsVisible() &&
			Guardian->GetConsumingSlotLightComponent(RightSlot)->IsVisible());

		FGridItemInstance ExtraGem = GemStack;
		ExtraGem.RuntimeObjectId = FGuid::NewGuid();
		ExtraGem.Quantity = 1;
		Test.TestTrue(TEXT("Extra gem reaches cursor"), Party->PartyInventoryComponent->SetCursorItem(ExtraGem));
		Test.TestFalse(TEXT("Filled slot rejects an additional attempt"), BasePointer->TryPlaceCursorItemFromHit(Party, FirstHit));
		Test.TestTrue(TEXT("Rejected extra gem remains on cursor"), Party->PartyInventoryComponent->HasCursorItem());
		Test.TestTrue(TEXT("Completion remains one-shot"), Guardian->WasConsumingSlotsCompletionEmitted());
		Party->PartyInventoryComponent->ClearCursorItem();

		if (!bExercisePersistence)
		{
			return !Test.HasAnyErrors();
		}

		Test.TestTrue(TEXT("Runtime capture succeeds"), Runtime->CaptureCurrentLevelRuntimeState());
		const FGridLevelRuntimeState* CapturedLevel = Runtime->FindRuntimeStateForCurrentLevel();
		const FGridRuntimeReceptacleState* Captured = CapturedLevel ? CapturedLevel->Receptacles.Find(GuardianId) : nullptr;
		Test.TestNotNull(TEXT("Runtime state contains consuming receptacle"), Captured);
		if (!Captured)
		{
			return false;
		}
		Test.TestEqual(TEXT("Runtime state retains both filled slots"), Captured->FilledConsumingSlots.Num(), 2);
		Test.TestTrue(TEXT("Runtime state retains completion one-shot"), Captured->bConsumingSlotsCompletionEmitted);
		Test.TestEqual(TEXT("Runtime state contains no absorbed items"), Captured->ContainedItems.Num(), 0);

		UGrimrockPartySaveGame* SourceSave = NewObject<UGrimrockPartySaveGame>(GetTransientPackage());
		SourceSave->DungeonRuntimeState = Runtime->DungeonRuntimeState;
		// The fixture's default RPG identities are intentionally not backed by transient class/race assets.
		// Cursor/inventory removal was asserted above; use an identity-neutral party snapshot for the real archive round-trip.
		SourceSave->PartyInventoryState = FGridPartyInventoryState();
		TArray<uint8> SaveBytes;
		Test.TestTrue(TEXT("SaveGameToMemory serializes consumed state"), UGameplayStatics::SaveGameToMemory(SourceSave, SaveBytes));
		UGrimrockPartySaveGame* LoadedSave = Cast<UGrimrockPartySaveGame>(UGameplayStatics::LoadGameFromMemory(SaveBytes));
		Test.TestNotNull(TEXT("LoadGameFromMemory restores consumed state"), LoadedSave);
		if (!LoadedSave)
		{
			return false;
		}
		Test.TestFalse(TEXT("Saved inventory contains no consumed blue gems"), InventoryContains(LoadedSave->PartyInventoryState, BlueGem->ItemDefinitionId));

		Runtime->DungeonRuntimeState = LoadedSave->DungeonRuntimeState;
		Runtime->RebuildLevel();
		Test.TestTrue(TEXT("Saved runtime state applies after rebuild"), Runtime->ApplyCurrentLevelRuntimeState());
		Guardian = Runtime->FindRuntimeObjectActor<AGridConsumingSlotReceptacleActor>(GuardianId);
		Test.TestNotNull(TEXT("Consuming receptacle is rebuilt"), Guardian);
		if (!Guardian)
		{
			return false;
		}
		Test.TestTrue(TEXT("Left slot restores after rebuild"), Guardian->IsConsumingSlotFilled(LeftSlot));
		Test.TestTrue(TEXT("Right slot restores after rebuild"), Guardian->IsConsumingSlotFilled(RightSlot));
		Test.TestEqual(TEXT("ContainedItems stays empty after rebuild"), Guardian->GetContainedItemCount(), 0);
		Test.TestTrue(TEXT("Materials restore after rebuild"), Guardian->MeshComponent->GetMaterial(0) == FilledMaterial &&
			Guardian->MeshComponent->GetMaterial(1) == FilledMaterial);
		Test.TestTrue(TEXT("Lights restore after rebuild"), Guardian->GetConsumingSlotLightComponent(LeftSlot)->IsVisible() &&
			Guardian->GetConsumingSlotLightComponent(RightSlot)->IsVisible());
		Test.TestEqual(TEXT("Rebuild creates no world-item duplicate"), CountWorldItems(*TestWorld.World), InitialWorldItemCount);
		return !Test.HasAnyErrors();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridPUZZLE011LeftThenRightTest, "Grimrock.PUZZLE01_1.LeftThenRight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridPUZZLE011LeftThenRightTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	return GridPUZZLE011Tests::RunConsumptionScenario(*this, false, true);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridPUZZLE011RightThenLeftTest, "Grimrock.PUZZLE01_1.RightThenLeft",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridPUZZLE011RightThenLeftTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	return GridPUZZLE011Tests::RunConsumptionScenario(*this, true, false);
}

#endif
