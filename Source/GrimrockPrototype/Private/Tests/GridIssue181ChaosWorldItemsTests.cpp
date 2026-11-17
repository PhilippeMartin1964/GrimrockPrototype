#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Components/StaticMeshComponent.h"
#include "Core/GridDungeonAsset.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "PhysicsEngine/BodyInstance.h"
#include "AssetCompilingManager.h"
#include "Kismet/GameplayStatics.h"
#include "RPG/RPGSkillAsset.h"
#include "RPG/RPGSkillCheckService.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Save/GrimrockPartySaveGame.h"

namespace GridIssue181
{
	struct FWorld
	{
		UWorld* World = nullptr;
		UGameInstance* GI = nullptr;
		AGridLevelRuntimeActor* Runtime = nullptr;
		AGrimrockPartyPawn* Party = nullptr;
		UGridLevelAsset* Level = nullptr;
		FGuid PitId;
		FGuid TrapId;
		const FName UpperId = TEXT("Into_The_Dark");
		const FName LowerId = TEXT("Issue181_Lower");

		explicit FWorld(bool bSwordN20 = false)
		{
			const UWorld::InitializationValues Values = UWorld::InitializationValues()
															.AllowAudioPlayback(false)
															.RequiresHitProxies(false)
															.CreatePhysicsScene(true)
															.CreateNavigation(false)
															.CreateAISystem(false)
															.ShouldSimulatePhysics(true)
															.SetTransactional(false);
			World = UWorld::CreateWorld(EWorldType::Game, false, MakeUniqueObjectName(GetTransientPackage(), UWorld::StaticClass(), TEXT("Issue181")), nullptr,
				true, ERHIFeatureLevel::Num, &Values);
			if (!World || !GEngine)
				return;
			GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
			GI = NewObject<UGameInstance>();
			GI->Init();
			World->SetGameInstance(GI);
			World->InitializeActorsForPlay(FURL());
			World->BeginPlay(); // Initializes the Chaos solver; no GameMode or persistent save.
			Runtime = World->SpawnActor<AGridLevelRuntimeActor>();
			Party = World->SpawnActor<AGrimrockPartyPawn>();
			Party->PartySaveSlotName.Empty();
			Party->bAutoSaveOnInventoryClose = false;
			const UGridLevelAsset* Source =
				LoadObject<UGridLevelAsset>(nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_00.DA_GridLevel_00"));
			if (!Source)
				return;
			Level = DuplicateObject<UGridLevelAsset>(Source, Runtime);
			Level->LooseItemInstances.RemoveAll(
				[](const FGridLooseItemInstance& Item)
				{
					return Item.CellX != 28 || Item.CellY != 24;
				});
			if (bSwordN20)
				for (FGridLooseItemInstance& Item : Level->LooseItemInstances)
					if (Item.CanonicalItemId == TEXT("Weapon_LongSword_01"))
						Item.ItemLevel = 20;
			Level->MonsterSpawns.Reset();
			Level->ItemSpawns.Reset();
			Level->Links.Reset();
			Level->WorldObjectInstances.RemoveAll(
				[](const FGridWorldObjectInstance& Object)
				{
					return Object.CellX != 27 || Object.CellY != 24;
				});
			UGridLevelAsset* Lower = DuplicateObject<UGridLevelAsset>(Level, Runtime, TEXT("Lower"));
			Lower->LooseItemInstances.Reset();
			Lower->WorldObjectInstances.Reset();
			UGridDungeonAsset* Dungeon = NewObject<UGridDungeonAsset>(Runtime);
			FGridDungeonLevelEntry UpperEntry;
			UpperEntry.LevelId = UpperId;
			UpperEntry.LevelAsset = Level;
			UpperEntry.LogicalPosition = FIntVector(0, 0, 1);
			FGridDungeonLevelEntry LowerEntry;
			LowerEntry.LevelId = LowerId;
			LowerEntry.LevelAsset = Lower;
			Dungeon->Levels = { UpperEntry, LowerEntry };
			Dungeon->DefaultLevelId = UpperId;
			for (FGridWorldObjectInstance& Object : Level->WorldObjectInstances)
			{
				if (Object.Type == EGridLevelObjectType::Pit)
				{
					PitId = Object.InstanceId;
					Object.InstanceConfig.Relocation.TargetLevelId = LowerId;
				}
				else if (Object.Type == EGridLevelObjectType::Trigger)
					TrapId = Object.InstanceId;
			}
			Level->Links = Source->Links.FilterByPredicate(
				[this](const FGridObjectLink& Link)
				{
					return Link.SourceObjectId == TrapId && Link.TargetObjectId == PitId;
				});
			Runtime->DungeonAsset = Dungeon;
			Runtime->CurrentDungeonLevelId = UpperId;
			Runtime->LevelAsset = Level;
			Runtime->FloorMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/GrimrockPrototype/Meshes/Floor/SM_Floor_Stone_01.SM_Floor_Stone_01"));
			Runtime->WorldObjectDefinitions = { LoadObject<UGridWorldObjectDefinitionAsset>(nullptr,
													TEXT("/Game/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/DA_Pit_Stone.DA_Pit_Stone")),
				LoadObject<UGridWorldObjectDefinitionAsset>(
					nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/DA_Trap_Floor_Spikes_01.DA_Trap_Floor_Spikes_01")) };
			Runtime->BuildInitialRuntimeState();
			FAssetCompilingManager::Get().FinishAllCompilation();
			for (TActorIterator<AGridItemActor> It(World); It; ++It)
				It->MeshComponent->RecreatePhysicsState();
			Party->SetGridStart(Runtime, 27, 23, EGridEdge::North);
			FGridCharacterInventoryState Hero;
			Hero.CharacterId = FGuid::NewGuid();
			Hero.InventorySlots.SetNum(20);
			Hero.Resources.CurrentHealth = 30;
			Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters = { Hero };
			Party->PartyInventoryComponent->PartyInventoryState.ActiveEquipment.SetNum(1);
		}

		~FWorld()
		{
			if (World)
				World->SetGameInstance(nullptr);
			if (GI)
				GI->Shutdown();
			if (World)
			{
				World->DestroyWorld(false);
				if (GEngine)
					GEngine->DestroyWorldContext(World);
			}
		}

		AGridItemActor* Find(FName ItemId) const
		{
			for (TActorIterator<AGridItemActor> It(World); It; ++It)
				if (!It->IsActorBeingDestroyed() && It->ItemDefinitionId == ItemId)
					return *It;
			return nullptr;
		}

    // Frozen prone pose of the real mesh: its authored origin is at the
    // handle, so rolling moves the geometry across a cell before the pivot.
		void LayDown(AGridItemActor* Item, FVector Pivot) const
		{
			Item->MeshComponent->SetWorldTransform(FTransform(FRotator(90.f, 0.f, 0.f), Pivot), false, nullptr, ETeleportType::TeleportPhysics);
			Item->MeshComponent->SetPhysicsLinearVelocity(FVector::ZeroVector);
			Item->MeshComponent->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
			World->Tick(LEVELTICK_All, 1.f / 60.f);
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridIssue181PlacedMeshSelection, "Grimrock.Pit.Issue181.RealPlacedMeshSelection", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridIssue181PlacedMeshSelection::RunTest(const FString&)
{
	GridIssue181::FWorld F(true);
	if (!TestNotNull(TEXT("Real level copy"), F.Level))
		return false;
	TestEqual(TEXT("Eight actual placements, not inventory drops"), F.Level->LooseItemInstances.Num(), 8);
	AGridItemActor* Axe = F.Find(TEXT("Weapon_GreatAxe_01"));
	if (!TestNotNull(TEXT("Actual placed axe"), Axe))
		return false;
	const FGuid Guid = Axe->RuntimeObjectId;
	AGridItemActor* Sword = F.Find(TEXT("Weapon_LongSword_01"));
	if (!TestNotNull(TEXT("Actual placed N20 sword"), Sword))
		return false;
	const FGuid SwordGuid = Sword->RuntimeObjectId;
	F.LayDown(Axe, FVector(5652.f, 4900.f, 10.f));
	F.LayDown(Sword, FVector(5652.f, 4852.f, 6.f));
	TestTrue(TEXT("Real Chaos body exists"), Axe->MeshComponent->IsSimulatingPhysics() && Axe->MeshComponent->GetBodyInstance()->IsValidBodyInstance());
	TestTrue(TEXT("Mesh center is over cell 27 while its pivot is still in cell 28"),
		Axe->MeshComponent->Bounds.Origin.X < 5600.f && Axe->MeshComponent->GetComponentLocation().X > 5600.f);
	TestTrue(TEXT("Opening succeeds"), F.Runtime->SetPitOpen(F.PitId, true, false));
	const FGridLevelRuntimeState* Lower = F.Runtime->DungeonRuntimeState.LevelStates.Find(F.LowerId);
	TestTrue(TEXT("Placed axe geometry is selected and queued once"), Lower && Lower->PendingInboundItems.Contains(Guid));
	TestTrue(TEXT("N20 sword selected independently"), Lower && Lower->PendingInboundItems.Contains(SwordGuid));
	if (Lower && Lower->PendingInboundItems.Contains(SwordGuid))
		TestEqual(TEXT("Fixed N20 survives the inter-level queue"), Lower->PendingInboundItems[SwordGuid].ItemState.ItemLevel, 20);
	TestTrue(TEXT("Inter-level travel consumes the queue"), F.Runtime->TravelToDungeonLevel(F.LowerId, 27, 23, EGridEdge::North, F.Party));
	TestNotNull(TEXT("Axe arrives downstairs"), F.Find(TEXT("Weapon_GreatAxe_01")));
	AGridItemActor* ArrivedSword = F.Find(TEXT("Weapon_LongSword_01"));
	if (TestNotNull(TEXT("Sword arrives downstairs"), ArrivedSword))
	{
		TestEqual(TEXT("Same sword GUID downstairs"), ArrivedSword->RuntimeObjectId, SwordGuid);
		TestEqual(TEXT("Same N20 downstairs"), ArrivedSword->CanonicalWorldItemLevel, 20);
	}
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridIssue181ExplicitLocalDisarm, "Grimrock.Pit.Issue181.ExplicitLocalDisarmAndMemoryRestore",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridIssue181ExplicitLocalDisarm::RunTest(const FString&)
{
	GridIssue181::FWorld F(true);
	if (!TestNotNull(TEXT("Real copied level"), F.Level))
		return false;
	FGridWorldObjectInstance* Pit = F.Level->FindWorldObjectInstanceById(F.PitId);
	if (!TestNotNull(TEXT("Real pit placement"), Pit))
		return false;
	Pit->InstanceConfig.Relocation.TargetLevelId = F.UpperId;
	TestFalse(TEXT("Same-level mode is opt-in"), F.Runtime->CanRelocateThroughPitOnCurrentLevel(Pit->InstanceConfig.Relocation, 27, 24));
	F.Runtime->bEnableSameLevelPitRelocation = true;
	TestTrue(TEXT("Explicit authored local destination is accepted"), F.Runtime->CanRelocateThroughPitOnCurrentLevel(Pit->InstanceConfig.Relocation, 27, 24));
	FGridRelocationBehaviorParams Invalid = Pit->InstanceConfig.Relocation;
	Invalid.TargetCellY = 24;
	TestFalse(TEXT("Source-equals-target loop is rejected"), F.Runtime->CanRelocateThroughPitOnCurrentLevel(Invalid, 27, 24));
	Invalid.TargetCellY = -1;
	TestFalse(TEXT("Invalid destination is rejected"), F.Runtime->CanRelocateThroughPitOnCurrentLevel(Invalid, 27, 24));
	F.Level->GetCellMutable(27, 23).bBlocksOccupancy = true;
	TestFalse(TEXT("Blocked destination is rejected"), F.Runtime->CanRelocateThroughPitOnCurrentLevel(Pit->InstanceConfig.Relocation, 27, 24));
	F.Level->GetCellMutable(27, 23).bBlocksOccupancy = false;
	const FGridRelocationBehaviorParams LocalDestination = Pit->InstanceConfig.Relocation;
	FGridWorldObjectInstance DestinationPit = *Pit;
	DestinationPit.InstanceId = FGuid::NewGuid();
	DestinationPit.CellY = 23;
	DestinationPit.InstanceConfig.Pit.bInitiallyOpen = true;
	F.Level->WorldObjectInstances.Add(DestinationPit);
	TestFalse(TEXT("An open destination pit cannot create a cascade"), F.Runtime->CanRelocateThroughPitOnCurrentLevel(LocalDestination, 27, 24));
	F.Level->WorldObjectInstances.Pop();

	AGridItemActor* Axe = F.Find(TEXT("Weapon_GreatAxe_01"));
	AGridItemActor* Sword = F.Find(TEXT("Weapon_LongSword_01"));
	if (!TestNotNull(TEXT("Real axe"), Axe) || !TestNotNull(TEXT("Real sword"), Sword))
		return false;
	const FGuid AxeGuid = Axe->RuntimeObjectId, SwordGuid = Sword->RuntimeObjectId;
	F.LayDown(Axe, FVector(5652.f, 4900.f, 10.f));
	F.LayDown(Sword, FVector(5652.f, 4852.f, 6.f));
	const int32 Generation = F.Runtime->GetRuntimeObjectRebuildGeneration();
	UGridPartyInventoryComponent* Inventory = F.Party->PartyInventoryComponent;
	FGridCharacterInventoryState& Hero = Inventory->PartyInventoryState.ActiveCharacters[0];
	Hero.Attributes.Dexterity = 10;
	FRPGSkillRank Rank;
	Rank.SkillId = TEXT("Skill_Traps");
	Rank.Rank = 1;
	Hero.SkillRanks.Add(Rank);
	UGridItemDefinitionAsset* Tool =
		LoadObject<UGridItemDefinitionAsset>(nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_DisarmTools_01.DA_Item_DisarmTools_01"));
	if (!TestTrue(TEXT("Real tool registered"), Inventory->RegisterItemDefinition(Tool)))
		return false;
	FGridItemInstance Tools;
	Tools.RuntimeObjectId = FGuid::NewGuid();
	Tools.ItemDefinitionId = TEXT("Item_DisarmTools_01");
	Tools.Quantity = 1;
	TestTrue(TEXT("Operator carries their tools"), Inventory->AddItemToCharacterInventory(0, Tools));
	URPGSkillAsset* Skill = LoadObject<URPGSkillAsset>(nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/Skills/DA_Skill_Traps.DA_Skill_Traps"));
	TestTrue(TEXT("Trap detected before disarm"), F.Runtime->TryRecordDungeonTrapDetection(F.TrapId));
	FRandomStream Dice(181);
	FRPGSkillCheckResult Result;
	bool bTriggered = false;
	TestTrue(TEXT("Actual disarm pipeline resolves"), F.Runtime->TryDisarmTriggerTrap(F.Party, F.TrapId, 0, Skill, Dice, Result, nullptr, &bTriggered));
	TestTrue(TEXT("Unsafe failure actually opens the authored trap"), !Result.bSuccess && bTriggered && F.Runtime->IsPitOpen(F.PitId));
	TestTrue(TEXT("Axe actor survives the local move"), F.Find(TEXT("Weapon_GreatAxe_01")) == Axe);
	TestTrue(TEXT("Sword actor survives the local move"), F.Find(TEXT("Weapon_LongSword_01")) == Sword);
	for (AGridItemActor* Item : { Axe, Sword })
	{
		const FVector Center = Item->GetWorldPickupLocation();
		TestTrue(TEXT("Mesh center arrives in (27,23)"), Center.X >= 5400.f && Center.X < 5600.f && Center.Y >= 4600.f && Center.Y < 4800.f);
	}
	TestEqual(TEXT("Local transfer never rebuilds the level"), F.Runtime->GetRuntimeObjectRebuildGeneration(), Generation);
	F.Runtime->SetPitOpen(F.PitId, true, false);
	TestTrue(TEXT("Repeated Open does not duplicate the local item"), F.Find(TEXT("Weapon_GreatAxe_01")) == Axe);
	F.Party->SetGridStart(F.Runtime, 27, 24, EGridEdge::North);
	TestTrue(TEXT("Party uses guarded local relocation, not level travel"), F.Runtime->TryBeginPitFallAtCell(27, 24, F.Party));
	TestEqual(TEXT("Party arrives at local Y"), F.Party->CurrentCellY, 23);
	TestEqual(TEXT("Party relocation does not rebuild"), F.Runtime->GetRuntimeObjectRebuildGeneration(), Generation);
	FGridItemInstance Incoming;
	Incoming.RuntimeObjectId = FGuid::NewGuid();
	Incoming.ItemDefinitionId = TEXT("Weapon_StraightSword_01");
	Incoming.ItemLevel = 1;
	Incoming.Quantity = 1;
	Incoming.OwnerType = EGridItemOwnerType::World;
	TestTrue(TEXT("A new world drop into an already open local pit materializes immediately"),
		F.Runtime->TryDropItemInstanceAtCell(Incoming, 27, 24, EGridEdge::None, FVector::ZeroVector));
	AGridItemActor* IncomingActor = F.Find(Incoming.ItemDefinitionId);
	if (!TestNotNull(TEXT("New local arrival"), IncomingActor))
		return false;
	TestEqual(TEXT("New arrival GUID is unchanged"), IncomingActor->RuntimeObjectId, Incoming.RuntimeObjectId);
	TestEqual(TEXT("New arrival is already accessible in destination cell"), IncomingActor->RuntimeCellY, 23);
	TestTrue(TEXT("Capture local state"), F.Runtime->CaptureCurrentLevelRuntimeState());
	const FGridLevelRuntimeState* State = F.Runtime->FindRuntimeStateForCurrentLevel();
	if (!TestNotNull(TEXT("Current runtime state"), State))
		return false;
	TestTrue(TEXT("No unconsumed same-level inbound queue"), State->PendingInboundItems.IsEmpty());
	TestEqual(TEXT("Local axe retains N1"), State->Items[AxeGuid].ItemLevel, 1);
	TestEqual(TEXT("Local sword retains N20"), State->Items[SwordGuid].ItemLevel, 20);
	UGrimrockPartySaveGame* Save = NewObject<UGrimrockPartySaveGame>();
	Save->DungeonRuntimeState = F.Runtime->DungeonRuntimeState;
	TArray<uint8> Bytes;
	if (!TestTrue(TEXT("Save to memory only"), UGameplayStatics::SaveGameToMemory(Save, Bytes)))
		return false;
	UGrimrockPartySaveGame* Loaded = Cast<UGrimrockPartySaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
	if (!TestNotNull(TEXT("Reload from memory"), Loaded))
		return false;
	F.Runtime->DungeonRuntimeState = Loaded->DungeonRuntimeState;
	TestTrue(TEXT("Apply local saved poses"), F.Runtime->ApplyCurrentLevelRuntimeState());
	TestEqual(TEXT("Restored axe still has its GUID"), F.Find(TEXT("Weapon_GreatAxe_01"))->RuntimeObjectId, AxeGuid);
	TestEqual(TEXT("Restored sword still has its GUID"), F.Find(TEXT("Weapon_LongSword_01"))->RuntimeObjectId, SwordGuid);
	for (const FName Id : { FName(TEXT("Weapon_GreatAxe_01")), FName(TEXT("Weapon_LongSword_01")) })
	{
		const FVector Center = F.Find(Id)->GetWorldPickupLocation();
		TestTrue(TEXT("Memory restore preserves the local mesh pose"), Center.X >= 5400.f && Center.X < 5600.f && Center.Y >= 4600.f && Center.Y < 4800.f);
	}
	int32 Count = 0;
	for (TActorIterator<AGridItemActor> It(F.World); It; ++It)
		if (!It->IsActorBeingDestroyed() && (It->RuntimeObjectId == AxeGuid || It->RuntimeObjectId == SwordGuid))
			++Count;
	TestEqual(TEXT("Exactly two physical instances after restore"), Count, 2);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridIssue181PlacedMeshPickup, "Grimrock.Pit.Issue181.RealPlacedMeshPickup", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridIssue181PlacedMeshPickup::RunTest(const FString&)
{
	GridIssue181::FWorld F;
	AGridItemActor* Spear = F.Find(TEXT("Weapon_ShortSpear_01"));
	if (!TestNotNull(TEXT("Actual placed spear"), Spear))
		return false;
	const FGuid Guid = Spear->RuntimeObjectId;
	F.LayDown(Spear, FVector(5700.f, 4800.f, 6.f));
	F.Party->SetActorLocation(FVector(5500.f, 4700.f, 150.f));
	TestTrue(TEXT("Handle pivot is beyond hand reach"), FVector::Dist2D(F.Party->GetActorLocation(), Spear->MeshComponent->GetComponentLocation()) > 200.f);
	FHitResult Hit;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(Issue181Pickup), true, F.Party);
	const FBox Box = Spear->MeshComponent->Bounds.GetBox();
	bool bMeshHit = false;
	for (int32 X = 1; X < 10 && !bMeshHit; ++X)
		for (int32 Y = 1; Y < 10 && !bMeshHit; ++Y)
		{
			const FVector Point = Box.Min + Box.GetSize() * FVector(X / 10.f, Y / 10.f, 0.5f);
			bMeshHit = Spear->MeshComponent->LineTraceComponent(Hit, FVector(Point.X, Point.Y, 180.f), FVector(Point.X, Point.Y, -10.f), Query);
		}
	TestTrue(TEXT("Real query body exists"), Spear->MeshComponent->GetBodyInstance()->IsValidBodyInstance());
	TestTrue(TEXT("Complex component trace hits the final mesh"), bMeshHit);
	TestTrue(TEXT("Final visible geometry can interact"), Spear->CanInteract_Implementation(F.Party, Spear->MeshComponent));
	TestTrue(TEXT("Pickup succeeds at the final physical pose"), F.Runtime->TryPickupItemActor(Spear, F.Party));
	bool bFoundGuid = false;
	for (const FGridInventorySlot& Slot : F.Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters[0].InventorySlots)
		bFoundGuid |= Slot.bOccupied && Slot.Item.RuntimeObjectId == Guid;
	TestTrue(TEXT("Inventory preserves the same GUID"), bFoundGuid);
	return !HasAnyErrors();
}

#endif
