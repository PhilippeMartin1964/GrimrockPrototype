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
	LevelAsset->Width = 3;
	LevelAsset->Height = 3;
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
	Runtime->WorldItemPickupReach = 210.0f;

	UGridItemDefinitionAsset* Definition = NewObject<UGridItemDefinitionAsset>(Runtime);
	Definition->ItemDefinitionId = TEXT("TD02_Stone");
	Definition->DisplayName = FText::FromString(TEXT("Stone"));
	Definition->Weight = 2.5f;
	Definition->WorldMesh = NewObject<UStaticMesh>(Runtime);

	FGridLevelObjectData DefinitionLookup;
	DefinitionLookup.Type = EGridLevelObjectType::Item;
	DefinitionLookup.ItemDefinitionAsset = Definition;
	DefinitionLookup.ItemDefinitionId = Definition->ItemDefinitionId;
	LevelAsset->Objects.Add(DefinitionLookup);

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
	TestTrue(TEXT("A free item one cell away is pickable within the physical reach regardless of facing"),
		Runtime->CanPartyPickupItemActor(NearbyFreeActor, Party));

	Runtime->WorldItemPickupReach = 190.0f;
	TestFalse(TEXT("The same free item is rejected when the physical reach is shorter than one cell"),
		Runtime->CanPartyPickupItemActor(NearbyFreeActor, Party));
	Runtime->WorldItemPickupReach = 210.0f;

	LevelAsset->GetCellMutable(1, 1).NorthWall = EGridWallType::Solid;
	TestFalse(TEXT("A wall blocks pickup of a free item in the neighbouring cell"), Runtime->CanPartyPickupItemActor(NearbyFreeActor, Party));
	LevelAsset->GetCellMutable(1, 1).NorthWall = EGridWallType::None;
	TestTrue(TEXT("Opening the grid edge restores neighbouring free-item pickup"), Runtime->CanPartyPickupItemActor(NearbyFreeActor, Party));

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
	TestFalse(TEXT("A diagonal free item is outside the immediate pickup neighbourhood"), Runtime->CanPartyPickupItemActor(DiagonalFreeActor, Party));

	TestTrue(TEXT("Picking up the neighbouring free actor transfers the actor actually clicked"),
		Runtime->TryPickupItemActor(NearbyFreeActor, Party));

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
