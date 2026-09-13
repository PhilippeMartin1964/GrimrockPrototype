#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"

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

	return true;
}

#endif