#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Runtime/GridActivationComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/GrimrockPlayerController.h"
#include "Runtime/GridPressurePlateActor.h"
#include "Runtime/GridThrownItemActor.h"

namespace
{
	struct FGridTD021World
	{
		UWorld* World = nullptr;

		FGridTD021World()
		{
			const UWorld::InitializationValues InitializationValues = UWorld::InitializationValues()
																		  .AllowAudioPlayback(false)
																		  .RequiresHitProxies(false)
																		  .CreatePhysicsScene(false)
																		  .CreateNavigation(false)
																		  .CreateAISystem(false)
																		  .ShouldSimulatePhysics(false)
																		  .SetTransactional(false);
			World = UWorld::CreateWorld(EWorldType::Game, false,
				FName(*FString::Printf(TEXT("TD021WorldItems_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true, ERHIFeatureLevel::Num,
				&InitializationValues);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FGridTD021World()
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD021WorldItemsContractTest, "Grimrock.TechnicalDebt.TD02_1.WorldItemsContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD021WorldItemsContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridTD021World TestWorld;
	TestNotNull(TEXT("The transient world is created"), TestWorld.World);
	if (!TestWorld.World)
	{
		return false;
	}

	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	TestNotNull(TEXT("The runtime actor is created"), Runtime);
	if (!Runtime)
	{
		return false;
	}

	UGridLevelAsset* LevelAsset = NewObject<UGridLevelAsset>(Runtime);
	LevelAsset->Width = 4;
	LevelAsset->Height = 4;
	LevelAsset->CellSize = 200.0f;
	LevelAsset->EnsureCellCount();
	for (FGridLevelCellData& Cell : LevelAsset->Cells)
	{
		Cell.CellType = EGridCellType::Floor;
		Cell.NorthWall = EGridWallType::None;
		Cell.EastWall = EGridWallType::None;
		Cell.SouthWall = EGridWallType::None;
		Cell.WestWall = EGridWallType::None;
		Cell.bBlocksOccupancy = false;
	}
	Runtime->LevelAsset = LevelAsset;
	TestEqual(TEXT("World item hand reach defaults to 200 cm"), Runtime->WorldItemPickupReach, 200.0f);

	UGridItemDefinitionAsset* Definition = NewObject<UGridItemDefinitionAsset>(Runtime);
	Definition->ItemDefinitionId = TEXT("TD02_Stone");
	Definition->DisplayName = FText::FromString(TEXT("Stone"));
	Definition->Weight = 2.5f;
	Definition->WorldMesh = NewObject<UStaticMesh>(Runtime);

	FGridLooseItemInstance DefinitionLookup;
	DefinitionLookup.InstanceId = FGuid::NewGuid();
	DefinitionLookup.ItemDefinition = Definition;
	LevelAsset->LooseItemInstances.Add(DefinitionLookup);

	AGrimrockPartyPawn* Party = TestWorld.World->SpawnActor<AGrimrockPartyPawn>();
	TestNotNull(TEXT("The party pawn is created"), Party);
	if (!Party || !Party->PartyInventoryComponent)
	{
		return false;
	}
	Party->LevelRuntimeActor = Runtime;
	Party->CurrentCellX = 1;
	Party->CurrentCellY = 1;
	Party->Facing = EGridEdge::North;
	Party->SetActorLocation(Runtime->GetCellCenterWorld(1, 1, Party->EyeHeight));

	FGridCharacterInventoryState Character;
	Character.CharacterId = FGuid::NewGuid();
	Character.DisplayName = FText::FromString(TEXT("TD02 Tester"));
	Character.InventorySlots.SetNum(20);
	Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters = { Character };
	Party->PartyInventoryComponent->PartyInventoryState.ActiveEquipment.SetNum(1);
	Party->PartyInventoryComponent->PartyInventoryState.SelectedCharacterIndex = 0;
	Party->PartyInventoryComponent->RegisterItemDefinition(Definition);

	FGridItemInstance WorldItem;
	WorldItem.RuntimeObjectId = FGuid::NewGuid();
	WorldItem.ItemDefinitionId = Definition->ItemDefinitionId;
	WorldItem.DisplayName = Definition->DisplayName;
	WorldItem.Quantity = 2;
	WorldItem.Weight = Definition->Weight;
	WorldItem.OwnerType = EGridItemOwnerType::World;

	auto FindWorldItemActor = [&TestWorld](const FGuid& RuntimeObjectId) -> AGridItemActor*
	{
		for (TActorIterator<AGridItemActor> It(TestWorld.World); It; ++It)
		{
			AGridItemActor* Candidate = *It;
			if (Candidate && !Candidate->IsActorBeingDestroyed() && Candidate->GetRuntimeObjectId() == RuntimeObjectId)
			{
				return Candidate;
			}
		}
		return nullptr;
	};

	FGridItemInstance CursorAdjacentItem = WorldItem;
	CursorAdjacentItem.RuntimeObjectId = FGuid::NewGuid();
	CursorAdjacentItem.OwnerType = EGridItemOwnerType::Cursor;
	LevelAsset->GetCellMutable(1, 1).EastWall = EGridWallType::Solid;
	TestTrue(TEXT("A cursor item can be placed by hand exactly 200 cm away even when grid movement is blocked"),
		Runtime->TryDropItemInstanceAtCell(CursorAdjacentItem, Definition, 2, 1, EGridEdge::None, FVector::ZeroVector));
	TestNotNull(TEXT("The 200 cm hand placement creates its world actor"), FindWorldItemActor(CursorAdjacentItem.RuntimeObjectId));
	LevelAsset->GetCellMutable(1, 1).EastWall = EGridWallType::None;

	FGridItemInstance CursorJustBeyondReach = WorldItem;
	CursorJustBeyondReach.RuntimeObjectId = FGuid::NewGuid();
	CursorJustBeyondReach.OwnerType = EGridItemOwnerType::Cursor;
	TestFalse(TEXT("A cursor item is not placed by hand at 201 cm"),
		Runtime->TryDropItemInstanceAtCell(CursorJustBeyondReach, Definition, 2, 1, EGridEdge::None, FVector(1.0f, 0.0f, 0.0f)));
	TestNull(TEXT("Rejected 201 cm hand placement creates no world actor"), FindWorldItemActor(CursorJustBeyondReach.RuntimeObjectId));

	FGridItemInstance CursorDiagonalNearCorner = WorldItem;
	CursorDiagonalNearCorner.RuntimeObjectId = FGuid::NewGuid();
	CursorDiagonalNearCorner.OwnerType = EGridItemOwnerType::Cursor;
	TestTrue(TEXT("Physical distance, not cell adjacency, owns hand placement when a diagonal point is within 200 cm"),
		Runtime->TryDropItemInstanceAtCell(CursorDiagonalNearCorner, Definition, 2, 2, EGridEdge::None, FVector(-60.0f, -60.0f, 0.0f)));

	FGridItemInstance CursorTwoCellsAway = WorldItem;
	CursorTwoCellsAway.RuntimeObjectId = FGuid::NewGuid();
	CursorTwoCellsAway.OwnerType = EGridItemOwnerType::Cursor;
	TestFalse(TEXT("A cursor item cannot be placed directly two cell centres away"),
		Runtime->TryDropItemInstanceAtCell(CursorTwoCellsAway, Definition, 3, 1, EGridEdge::None, FVector::ZeroVector));
	TestNull(TEXT("Rejected two-cell hand placement creates no world actor"), FindWorldItemActor(CursorTwoCellsAway.RuntimeObjectId));

	FGridItemInstance CursorDiagonalItem = WorldItem;
	CursorDiagonalItem.RuntimeObjectId = FGuid::NewGuid();
	CursorDiagonalItem.OwnerType = EGridItemOwnerType::Cursor;
	TestFalse(TEXT("A diagonal cell centre is outside the 200 cm hand reach"),
		Runtime->TryDropItemInstanceAtCell(CursorDiagonalItem, Definition, 2, 2, EGridEdge::None, FVector::ZeroVector));

	FGridItemInstance NearbyFreeItem = WorldItem;
	NearbyFreeItem.RuntimeObjectId = FGuid::NewGuid();
	TestTrue(TEXT("A free item can be dropped in the neighbouring cell"),
		Runtime->TryDropItemInstanceAtCell(NearbyFreeItem, Definition, 1, 2, EGridEdge::None, FVector::ZeroVector));
	AGridItemActor* NearbyFreeActor = FindWorldItemActor(NearbyFreeItem.RuntimeObjectId);
	TestNotNull(TEXT("The neighbouring free pickup actor exists"), NearbyFreeActor);
	if (!NearbyFreeActor)
	{
		return false;
	}

	Party->Facing = EGridEdge::East;
	TestTrue(TEXT("A free item exactly 200 cm away is pickable regardless of facing"), Runtime->CanPartyPickupItemActor(NearbyFreeActor, Party));

	const FVector PickupLocation = NearbyFreeActor->GetActorLocation();
	NearbyFreeActor->SetActorLocation(PickupLocation + FVector(0.0f, 0.01f, 0.0f));
	TestFalse(TEXT("A free world item just beyond 200 cm is not pickable"), Runtime->CanPartyPickupItemActor(NearbyFreeActor, Party));
	NearbyFreeActor->SetActorLocation(PickupLocation);

	Runtime->WorldItemPickupReach = 199.0f;
	TestFalse(TEXT("The same free item is rejected when the hand reach is shorter than 200 cm"), Runtime->CanPartyPickupItemActor(NearbyFreeActor, Party));
	Runtime->WorldItemPickupReach = 200.0f;

	LevelAsset->GetCellMutable(1, 1).NorthWall = EGridWallType::Solid;
	TestTrue(TEXT("Grid movement blockage does not veto a visible pickup within hand reach; the mouse visibility hit owns obstacle rejection"),
		Runtime->CanPartyPickupItemActor(NearbyFreeActor, Party));
	LevelAsset->GetCellMutable(1, 1).NorthWall = EGridWallType::None;

	FGridItemInstance DiagonalFreeItem = WorldItem;
	DiagonalFreeItem.RuntimeObjectId = FGuid::NewGuid();
	TestTrue(TEXT("A free item can be dropped on a diagonal cell"),
		Runtime->TryDropItemInstanceAtCell(DiagonalFreeItem, Definition, 2, 2, EGridEdge::None, FVector::ZeroVector));
	AGridItemActor* DiagonalFreeActor = FindWorldItemActor(DiagonalFreeItem.RuntimeObjectId);
	TestNotNull(TEXT("The diagonal free pickup actor exists"), DiagonalFreeActor);
	if (!DiagonalFreeActor)
	{
		return false;
	}
	TestFalse(TEXT("A diagonal cell-centre item is outside the 200 cm pickup reach"), Runtime->CanPartyPickupItemActor(DiagonalFreeActor, Party));

	TestTrue(TEXT("Picking up the neighbouring free actor transfers the actor actually clicked"), Runtime->TryPickupItemActor(NearbyFreeActor, Party));

	TestTrue(TEXT("A valid item can be dropped on the edge facing the party"),
		Runtime->TryDropItemInstanceAtCell(WorldItem, Definition, 1, 2, EGridEdge::South, FVector::ZeroVector));
	TestEqual(TEXT("Edge items are excluded from the default pressure-plate weight query"), Runtime->GetWorldItemWeightAtCell(1, 2), 0.0f);
	TestEqual(TEXT("Edge items contribute their full stack weight when requested"), Runtime->GetWorldItemWeightAtCell(1, 2, true), 5.0f);

	AGridItemActor* DroppedItem = nullptr;
	for (TActorIterator<AGridItemActor> It(TestWorld.World); It; ++It)
	{
		AGridItemActor* Candidate = *It;
		if (Candidate && !Candidate->IsActorBeingDestroyed() && Candidate->GetRuntimeObjectId() == WorldItem.RuntimeObjectId)
		{
			DroppedItem = Candidate;
			break;
		}
	}
	TestNotNull(TEXT("The drop creates one world pickup with the same runtime identity"), DroppedItem);
	if (!DroppedItem)
	{
		return false;
	}

	Party->Facing = EGridEdge::East;
	TestFalse(TEXT("A front-cell edge item cannot be picked up when the party is facing elsewhere"), Runtime->CanPartyPickupItemActor(DroppedItem, Party));
	Party->Facing = EGridEdge::North;
	TestTrue(TEXT("A front-cell edge item is pickable when its edge faces the party"), Runtime->CanPartyPickupItemActor(DroppedItem, Party));
	TestTrue(TEXT("Picking up the actor transfers it to the selected character"), Runtime->TryPickupItemActor(DroppedItem, Party));
	TestTrue(TEXT("The selected character inventory now contains the item definition"), Party->HasInventoryItem(Definition->ItemDefinitionId));
	TestEqual(TEXT("Picking up the item removes its world weight"), Runtime->GetWorldItemWeightAtCell(1, 2, true), 0.0f);

	int32 ResolvedX = INDEX_NONE;
	int32 ResolvedY = INDEX_NONE;
	FVector LocalOffset = FVector::ZeroVector;
	const FVector ResolvePoint = Runtime->GetCellCenterWorld(2, 0, 12.0f) + FVector(30.0f, -25.0f, 0.0f);
	TestTrue(TEXT("A world impact point resolves to its walkable grid cell"),
		Runtime->TryResolveWorldCellFromImpactPoint(ResolvePoint, ResolvedX, ResolvedY, LocalOffset));
	TestEqual(TEXT("Resolved impact X is stable"), ResolvedX, 2);
	TestEqual(TEXT("Resolved impact Y is stable"), ResolvedY, 0);
	TestTrue(TEXT("Resolved impact local offset is preserved inside the drop clamp"), LocalOffset.Equals(FVector(30.0f, -25.0f, 0.0f), KINDA_SMALL_NUMBER));

	// The hotbar uses physical inventory aiming, bypassing the ordinary cursor-item resolver.
	UClass* ControllerClass = LoadClass<AGrimrockPlayerController>(
		nullptr, TEXT("/Game/GrimrockPrototype/Blueprints/Runtime/BP_GrimrockPlayerController.BP_GrimrockPlayerController_C"));
	TestNotNull(TEXT("The shipped player-controller Blueprint loads"), ControllerClass);
	UClass* RuntimeClass = LoadClass<AGridLevelRuntimeActor>(nullptr,
		TEXT("/Game/GrimrockPrototype/Blueprints/Runtime/BP_GridLevelRuntimeActor.BP_GridLevelRuntimeActor_C"));
	if (!TestNotNull(TEXT("The shipped runtime Blueprint loads"), RuntimeClass))
	{
		return false;
	}
	TestEqual(TEXT("The serialized runtime hand reach defaults to 200 cm"),
		RuntimeClass->GetDefaultObject<AGridLevelRuntimeActor>()->WorldItemPickupReach, 200.0f);
	if (!ControllerClass)
	{
		return false;
	}
	AGrimrockPlayerController* Controller = TestWorld.World->SpawnActor<AGrimrockPlayerController>(ControllerClass);
	if (!TestNotNull(TEXT("The hotbar controller is created"), Controller))
	{
		return false;
	}
	Controller->Possess(Party);
	Definition->bThrowable = true;
	Definition->Weight = 0.5f;
	Definition->bStackable = true;
	Definition->MaxStackSize = 99;
	Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters = { Character };
	FGridItemInstance HotbarStack = WorldItem;
	HotbarStack.RuntimeObjectId = FGuid::NewGuid();
	HotbarStack.Weight = Definition->Weight;
	HotbarStack.Quantity = 3;
	TestTrue(TEXT("The hotbar source stack is stored in inventory"), Party->PartyInventoryComponent->AddItemToCharacterInventory(0, HotbarStack));
	auto CountSource = [Party, Definition]()
	{
		return Party->PartyInventoryComponent->CountItemDefinitionInCharacterInventory(0, Definition->ItemDefinitionId);
	};
	auto CountProjectiles = [&TestWorld]()
	{
		int32 Count = 0;
		for (TActorIterator<AGridThrownItemActor> It(TestWorld.World); It; ++It)
		{
			Count += !It->IsActorBeingDestroyed() ? 1 : 0;
		}
		return Count;
	};
	TestTrue(TEXT("The same aiming entry point as a numbered hotbar slot accepts the Stone"),
		Party->BeginSelectedCharacterInventoryItemThrowAiming(Definition->ItemDefinitionId));
	TestEqual(TEXT("Aiming does not consume the inventory stack"), CountSource(), 3);
	TestFalse(TEXT("Hotbar aiming does not put an inventory instance on the ordinary cursor"), Party->HasCursorItem());

	AGridPressurePlateActor* Plate = TestWorld.World->SpawnActor<AGridPressurePlateActor>();
	FHitResult PlateHit(Plate, nullptr, FVector::ZeroVector, FVector::UpVector);
	PlateHit.bBlockingHit = true;
	LevelAsset->GetCellMutable(1, 1).EastWall = EGridWallType::Solid;
	for (const float Distance : { 199.0f, 200.0f, 200.01f, 201.0f })
	{
		PlateHit.ImpactPoint = Party->GetActorLocation() + FVector(Distance, 0.0f, -Party->EyeHeight);
		TestEqual(FString::Printf(TEXT("Hotbar Visibility hit at %.2f cm selects the distance-based cursor despite a blocked grid edge"), Distance),
			Controller->ResolvePhysicalThrowTargetCursor(Party, PlateHit),
			Distance <= 200.0f ? EGridInteractionCursor::PlaceItem : EGridInteractionCursor::AimThrow);
	}
	PlateHit.ImpactPoint = Party->GetActorLocation() + FVector(140.0f, 140.0f, -Party->EyeHeight);
	TestEqual(TEXT("Diagonal horizontal distance below 200 cm also selects PlaceItem"), Controller->ResolvePhysicalThrowTargetCursor(Party, PlateHit),
		EGridInteractionCursor::PlaceItem);

	PlateHit.ImpactPoint = Runtime->GetCellCenterWorld(2, 1, 0.0f);
	Runtime->WorldItemPickupReach = 199.0f;
	TestEqual(TEXT("Hotbar targeting reads the canonical runtime reach"),
		Controller->ResolvePhysicalThrowTargetCursor(Party, PlateHit), EGridInteractionCursor::AimThrow);
	TestFalse(TEXT("Ordinary cursor placement reads the same runtime reach"), Controller->IsWithinHandPlacementReach(Party, PlateHit));
	Runtime->WorldItemPickupReach = 200.0f;
	TestTrue(TEXT("Ordinary cursor placement includes the canonical boundary"), Controller->IsWithinHandPlacementReach(Party, PlateHit));
	const float PreviousWeight = Runtime->GetWorldItemWeightAtCell(2, 1);
	FGridWorldObjectInstance WeightPlate;
	WeightPlate.InstanceId = FGuid::NewGuid();
	WeightPlate.Type = EGridLevelObjectType::PressurePlate;
	WeightPlate.WorldObjectDefinitionId = TEXT("TD021_WeightPlate");
	WeightPlate.CellX = 2;
	WeightPlate.CellY = 1;
	WeightPlate.InstanceConfig.InteractionOverrides.bOverridePressurePlateWeight = true;
	WeightPlate.InstanceConfig.InteractionOverrides.PressurePlateWeight.bActivateWhenPartyPresent = false;
	WeightPlate.InstanceConfig.InteractionOverrides.PressurePlateWeight.bUseItemWeight = true;
	WeightPlate.InstanceConfig.InteractionOverrides.PressurePlateWeight.RequiredItemWeight = PreviousWeight + Definition->Weight;
	LevelAsset->WorldObjectInstances.Add(WeightPlate);
	UGridWorldObjectDefinitionAsset* PlateDefinition = NewObject<UGridWorldObjectDefinitionAsset>(Runtime);
	PlateDefinition->DefinitionId = WeightPlate.WorldObjectDefinitionId;
	PlateDefinition->SupportedType = EGridLevelObjectType::PressurePlate;
	PlateDefinition->PlacementSurface = EGridObjectPlacementKind::Floor;
	Runtime->WorldObjectDefinitions.Add(PlateDefinition);
	UGridActivationComponent* Activation = Runtime->FindComponentByClass<UGridActivationComponent>();
	if (!TestNotNull(TEXT("Pressure plate activation component exists"), Activation))
	{
		return false;
	}
	Activation->Initialize(Runtime);
	Activation->RebuildIndexes();
	Activation->RefreshAllPressurePlates();
	TestFalse(TEXT("The weight plate starts below its threshold"), Activation->GetActiveObjectIds().Contains(WeightPlate.InstanceId));
	TestTrue(TEXT("Clicking the plate at exactly 200 cm is handled"), Controller->HandlePhysicalThrowAimingHit(PlateHit));
	TestEqual(TEXT("The close hotbar click places exactly one Stone in the targeted cell"), Runtime->GetWorldItemWeightAtCell(2, 1),
		PreviousWeight + Definition->Weight);
	TestTrue(TEXT("WorldDrop immediately refreshes and activates the weight plate without a tick"),
		Activation->GetActiveObjectIds().Contains(WeightPlate.InstanceId));
	TestEqual(TEXT("Placement consumes exactly one inventory unit"), CountSource(), 2);
	TestEqual(TEXT("The close hotbar click never spawns a projectile"), CountProjectiles(), 0);
	TestFalse(TEXT("Successful placement exits aiming"), Controller->IsPhysicalThrowAimingActive());
	TestFalse(TEXT("Successful placement leaves no temporary cursor item"), Party->HasCursorItem());

	TestFalse(TEXT("Failed physical placement restores the original source slot"),
		Party->TryDropSelectedCharacterInventoryItemAtCell(Definition->ItemDefinitionId, -1, -1, FVector::ZeroVector));
	TestEqual(TEXT("Failed placement preserves the inventory quantity"), CountSource(), 2);
	TestEqual(TEXT("Failed placement preserves the source stack identity and slot"),
		Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters[0].InventorySlots[0].Item.RuntimeObjectId, HotbarStack.RuntimeObjectId);
	TestFalse(TEXT("Failed placement leaves no temporary cursor item"), Party->HasCursorItem());

	TestTrue(TEXT("The remaining hotbar stack can be targeted again"), Party->BeginSelectedCharacterInventoryItemThrowAiming(Definition->ItemDefinitionId));
	LevelAsset->GetCellMutable(2, 1).bBlocksOccupancy = true;
	Controller->HandlePhysicalThrowAimingHit(PlateHit);
	TestEqual(TEXT("An invalid near cell never falls back to throwing"), CountProjectiles(), 0);
	TestEqual(TEXT("An invalid near click preserves the source"), CountSource(), 2);
	LevelAsset->GetCellMutable(2, 1).bBlocksOccupancy = false;
	PlateHit.ImpactPoint.X += 1.0f;
	TestTrue(TEXT("The 201 cm hotbar click is handled"), Controller->HandlePhysicalThrowAimingHit(PlateHit));
	TestEqual(TEXT("The 201 cm hotbar click creates a projectile"), CountProjectiles(), 1);
	TestEqual(TEXT("Throwing consumes exactly one inventory unit"), CountSource(), 1);
	TestFalse(TEXT("Successful throwing exits aiming"), Controller->IsPhysicalThrowAimingActive());

	TestTrue(TEXT("The last unit can be aimed at a cell corner"), Party->BeginSelectedCharacterInventoryItemThrowAiming(Definition->ItemDefinitionId));
	PlateHit.ImpactPoint = Party->GetActorLocation() + FVector(101.0f, 172.0f, -Party->EyeHeight);
	TestEqual(
		TEXT("A visible corner at 199.56 cm is placeable"), Controller->ResolvePhysicalThrowTargetCursor(Party, PlateHit), EGridInteractionCursor::PlaceItem);
	const float PreviousCornerWeight = Runtime->GetWorldItemWeightAtCell(2, 2);
	Controller->HandlePhysicalThrowAimingHit(PlateHit);
	TestEqual(TEXT("Placement preserves the close impact instead of clamping it out of reach"), Runtime->GetWorldItemWeightAtCell(2, 2),
		PreviousCornerWeight + Definition->Weight);
	TestEqual(TEXT("Corner placement consumes the last unit"), CountSource(), 0);
	TestEqual(TEXT("Corner placement creates no additional projectile"), CountProjectiles(), 1);
	LevelAsset->GetCellMutable(1, 1).EastWall = EGridWallType::None;

	return true;
}

#endif
