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
#include "Runtime/GridThrownItemActor.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "PhysicsEngine/BodyInstance.h"
#include "AssetCompilingManager.h"
#include "Kismet/GameplayStatics.h"
#include "RPG/RPGSkillAsset.h"
#include "RPG/RPGSkillCheckService.h"
#include "Save/GrimrockPartySaveGame.h"

namespace GridIssue181
{
	struct FWorld
	{
		const uint64 InitialFrameCounter = GFrameCounter;
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
			// This isolated world has no GameMode to dispatch Pawn BeginPlay.
			// Start it after clearing the save slot, so World::Tick runs its real movement/presentation.
			Runtime->bApplyLevelStartOnBeginPlay = false;
			Party->SetGridStart(Runtime, 27, 23, EGridEdge::North);
			if (!Party->HasActorBegunPlay())
				Party->DispatchBeginPlay();
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
			GFrameCounter = InitialFrameCounter;
		}

		void Tick(int32 Frames = 1) const
		{
			for (int32 Frame = 0; Frame < Frames; ++Frame)
			{
				World->Tick(LEVELTICK_All, 1.f / 60.f);
				++GFrameCounter; // Match Engine's FTestWorldWrapper: physics/timers must see distinct gameplay frames.
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
			Tick();
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
	F.Tick(120);
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
	TestTrue(TEXT("Authored explicit same-level destination enables local landing"),
		F.Runtime->CanRelocateThroughPitOnCurrentLevel(Pit->InstanceConfig.Relocation, 27, 24));
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
	const FName ToolId(TEXT("Item_DisarmTools_01"));
	const UGridItemSubsystem* Registry = F.GI
		? F.GI->GetSubsystem<UGridItemSubsystem>() : nullptr;
	FGridResolvedItem CanonicalTool;
	if (!TestNotNull(TEXT("Canonical ItemSystem is available"), Registry) ||
		!TestTrue(TEXT("Real N0 disarm tool resolves canonically"),
			Registry && Registry->ResolveItem(ToolId, 0, CanonicalTool)) ||
		!TestFalse(TEXT("Disarm tool is a non-ARM N0 item"), CanonicalTool.bIsArmament))
		return false;
	FGridItemInstance Tools;
	Tools.RuntimeObjectId = FGuid::NewGuid();
	Tools.ItemDefinitionId = ToolId;
	Tools.ItemLevel = 0;
	Tools.Quantity = 1;
	if (!TestTrue(TEXT("Canonical tool instance projected"),
			Inventory->ApplyItemDefinitionToInstance(Tools)) ||
		!TestTrue(TEXT("Operator carries their tools"),
			Inventory->AddItemToCharacterInventory(0, Tools)))
		return false;
	URPGSkillAsset* Skill = LoadObject<URPGSkillAsset>(nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/Skills/DA_Skill_Traps.DA_Skill_Traps"));
	TestTrue(TEXT("Trap detected before disarm"), F.Runtime->IsTriggerTrapDetected(F.TrapId) || F.Runtime->TryRecordDungeonTrapDetection(F.TrapId));
	FRandomStream Dice(181);
	FRPGSkillCheckResult Result;
	bool bTriggered = false;
	F.Party->SetGridStart(F.Runtime, 27, 24, EGridEdge::North);
	TestTrue(TEXT("Actual disarm pipeline resolves"), F.Runtime->TryDisarmTriggerTrap(F.Party, F.TrapId, 0, Skill, Dice, Result, nullptr, &bTriggered));
	TestTrue(TEXT("Unsafe failure actually opens the authored trap"), !Result.bSuccess && bTriggered && F.Runtime->IsPitOpen(F.PitId));
	TestTrue(TEXT("Unsafe disarm underfoot starts one animated group fall"), F.Party->IsPitFalling());
	F.Tick(120); // The opening itself must not teleport a supported rigid body.
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
	TestTrue(TEXT("Local pit starts the normal fall presentation"), F.Party->IsPitFalling());
	F.Tick(60);
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridIssue181LocalFallLifecycleTest, "Grimrock.Pit.Issue181.LocalWalkFallLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridIssue181LocalFallLifecycleTest::RunTest(const FString&)
{
	for (bool bOpenBeforeMove : { true, false })
	{
		GridIssue181::FWorld F;
		F.Level->FindWorldObjectInstanceById(F.PitId)->InstanceConfig.Relocation.TargetLevelId = F.UpperId;
		const int32 Generation = F.Runtime->GetRuntimeObjectRebuildGeneration();
		if (bOpenBeforeMove)
			TestTrue(TEXT("Combined trap pit opens before the walk"), F.Runtime->SetPitOpen(F.PitId, true, false));
		TestTrue(TEXT("Normal movement starts towards the open pit"), F.Party->TryStartMove(EGridEdge::North));
		F.Party->UpdateMove(F.Party->MoveDuration);
		TestEqual(TEXT("Actual pit X reached"), F.Party->CurrentCellX, 27);
		TestEqual(TEXT("Contact does not teleport directly to arrival"), F.Party->CurrentCellY, 24);
		TestTrue(TEXT("Movement completion starts BeginPitFall"), F.Party->IsPitFalling());
		TestFalse(TEXT("Repeated Open/fall notification cannot start a second fall"), F.Runtime->TryBeginPitFallAtCell(27, 24, F.Party));
		const FVector Contact = F.Party->GetActorLocation();
		F.Party->UpdatePitFall(F.Party->PitFallDuration * 0.5f);
		TestTrue(TEXT("Fall remains active during presentation"), F.Party->IsPitFalling());
		TestTrue(TEXT("Presentation moves the group down before landing"), F.Party->GetActorLocation().Z < Contact.Z - 10.f);
		F.Party->UpdatePitFall(F.Party->PitFallDuration);
		TestFalse(TEXT("Fall completes"), F.Party->IsPitFalling());
		TestEqual(TEXT("Arrival uses authored local cell"), F.Party->CurrentCellY, 23);
		TestEqual(TEXT("Landing does not rebuild the level"), F.Runtime->GetRuntimeObjectRebuildGeneration(), Generation);
		TestTrue(TEXT("Landing starts the existing camera impact"), F.Party->bPitFallLandingCameraImpactActive);
		F.Party->UpdatePitFallLandingCameraImpact(1.f);
		TestFalse(TEXT("Landing camera recovers"), F.Party->bPitFallLandingCameraImpactActive);
	}
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridIssue181AfterOpenChaosPassage, "Grimrock.Pit.Issue181.AfterOpenChaosPassage", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridIssue181AfterOpenChaosPassage::RunTest(const FString&)
{
	GridIssue181::FWorld F(true);
	F.Level->FindWorldObjectInstanceById(F.PitId)->InstanceConfig.Relocation.TargetLevelId = F.UpperId;
	AGridItemActor* Axe = F.Find(TEXT("Weapon_GreatAxe_01"));
	if (!TestNotNull(TEXT("Real placed axe"), Axe))
		return false;
	const FGuid Guid = Axe->RuntimeObjectId;
	TestTrue(TEXT("Opening occurs before the body enters the hole"), F.Runtime->SetPitOpen(F.PitId, true, false));
	AGridItemActor* Supported = F.Find(TEXT("Weapon_LongSword_01"));
	F.LayDown(Supported, FVector(5700.f, 4852.f, 8.f));
	F.Tick(30);
	TestTrue(TEXT("A supported tip over the open hole is not transferred"), Supported->GetWorldPickupLocation().Y >= 4800.f);
	F.LayDown(Axe, FVector(5652.f, 4900.f, 160.f));
	TestTrue(TEXT("Pivot remains in the neighbouring placement cell at release"), Axe->MeshComponent->GetComponentLocation().X > 5600.f);
	F.Tick(180);
	AddInfo(FString::Printf(TEXT("After-open body: center=%s velocity=%s minZ=%.2f"), *Axe->GetWorldPickupLocation().ToString(),
		*Axe->MeshComponent->GetPhysicsLinearVelocity().ToString(), Axe->MeshComponent->Bounds.GetBox().Min.Z));
	TestTrue(TEXT("The existing body arrives after physical descent"), F.Find(TEXT("Weapon_GreatAxe_01")) == Axe);
	const FVector Arrival = Axe->GetWorldPickupLocation();
	TestTrue(TEXT("Post-opening fall reaches local destination"), Arrival.X >= 5400.f && Arrival.X < 5600.f && Arrival.Y >= 4600.f && Arrival.Y < 4800.f);
	TestTrue(TEXT("Arrival is accessible above destination floor"), Axe->MeshComponent->Bounds.GetBox().Min.Z >= 0.f);
	TestEqual(TEXT("Physical transfer preserves GUID"), Axe->RuntimeObjectId, Guid);
	TestEqual(TEXT("Physical transfer preserves item level"), Axe->CanonicalWorldItemLevel, 1);
	TestTrue(TEXT("Local arrival never enters an orphaned inbound queue"), F.Runtime->DungeonRuntimeState.LevelStates[F.UpperId].PendingInboundItems.IsEmpty());
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridIssue181OffsetBodyPassage, "Grimrock.Pit.Issue181.CollisionPassageOutsideCenter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridIssue181OffsetBodyPassage::RunTest(const FString&)
{
	GridIssue181::FWorld F;
	F.Level->FindWorldObjectInstanceById(F.PitId)->InstanceConfig.Relocation.TargetLevelId = F.UpperId;
	AGridItemActor* Axe = F.Find(TEXT("Weapon_GreatAxe_01"));
	if (!TestNotNull(TEXT("Actual authored long body"), Axe))
		return false;
	TestTrue(TEXT("Already open pit"), F.Runtime->SetPitOpen(F.PitId, true, false));
	Axe->MeshComponent->SetWorldTransform(FTransform(FRotator(120.f, 0.f, 0.f), FVector(5675.f, 4900.f, 40.f)), false, nullptr, ETeleportType::TeleportPhysics);
	Axe->MeshComponent->SetPhysicsLinearVelocity(FVector(0.f, 0.f, -120.f));
	Axe->MeshComponent->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	TestTrue(TEXT("Both pivot and bounds centre remain outside the pit"),
		Axe->MeshComponent->GetComponentLocation().X > 5600.f && Axe->GetWorldPickupLocation().X > 5600.f);
	TestTrue(TEXT("Actual descending shape reaches below the pit floor"), Axe->MeshComponent->Bounds.GetBox().Min.Z < -20.f);
	F.Tick(12);
	TestTrue(TEXT("Collision passage is selected despite outside centre"), Axe->GetWorldPickupLocation().Y < 4800.f);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridIssue181FlyingProjectilePassage, "Grimrock.Pit.Issue181.FlyingProjectilePassage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridIssue181FlyingProjectilePassage::RunTest(const FString&)
{
	GridIssue181::FWorld F;
	F.Level->FindWorldObjectInstanceById(F.PitId)->InstanceConfig.Relocation.TargetLevelId = F.UpperId;
	TestTrue(TEXT("Pit is open before flight"), F.Runtime->SetPitOpen(F.PitId, true, false));
	FGridItemInstance Item;
	Item.RuntimeObjectId = FGuid::NewGuid();
	Item.ItemDefinitionId = TEXT("Weapon_Dagger_01");
	Item.ItemLevel = 20;
	Item.Quantity = 1;
	// X.39: already-placed ARM actors and thrown ARM instances require only
	// ItemId + physical N20. Do not read or attach any DA_Weapon_*.
	AGridItemActor* PlacedDagger = F.Find(Item.ItemDefinitionId);
	if (!TestNotNull(TEXT("Real authored dagger exists"), PlacedDagger) ||
		!TestNull(TEXT("Placed canonical dagger has no historical definition pointer"),
			PlacedDagger->GetItemDefinitionAsset()))
		return false;
	const UGridItemSubsystem* Registry = F.GI ? F.GI->GetSubsystem<UGridItemSubsystem>() : nullptr;
	FGridResolvedItem Canonical;
	if (!TestNotNull(TEXT("Configured ItemSystem in physics world"), Registry) ||
		!TestTrue(TEXT("Native Dagger N20 resolves without old asset"),
			Registry->ResolveItem(Item.ItemDefinitionId, Item.ItemLevel, Canonical)) ||
		!TestTrue(TEXT("Physical dagger identity and throw policy are canonical"),
			Canonical.IsValid() && Canonical.bIsArmament &&
			Canonical.ItemId == Item.ItemDefinitionId && Canonical.ItemLevel == 20 &&
			(Canonical.Weapon.Projectile.bEnabled ||
			 Canonical.Weapon.Projectile.bGenericThrowAuthored)))
		return false;
	AGridThrownItemActor* Projectile =
		F.Runtime->SpawnThrownItemProjectile(Item, nullptr, FVector(5500.f, 4900.f, 140.f), FVector(0.f, 0.f, -250.f), 27, 23);
	if (!TestNotNull(TEXT("Actual throwing pipeline creates the projectile"), Projectile))
		return false;
	Projectile->DispatchBeginPlay(); // The isolated test world has no GameMode.
	F.Tick(60);
	TestTrue(TEXT("Physical passage consumes the projectile before expiry"), Projectile->IsActorBeingDestroyed());
	int32 Count = 0;
	for (TActorIterator<AGridItemActor> It(F.World); It; ++It)
		if (!It->IsActorBeingDestroyed() && It->GetRuntimeObjectId() == Item.RuntimeObjectId)
		{
			++Count;
			TestEqual(TEXT("Thrown N20 preserved"), It->CanonicalWorldItemLevel, 20);
			TestTrue(TEXT("Thrown item arrives in authored local cell"), It->GetWorldPickupLocation().Y >= 4600.f && It->GetWorldPickupLocation().Y < 4800.f);
		}
	TestEqual(TEXT("Exactly one pickup replaces the projectile"), Count, 1);
	return !HasAnyErrors();
}

#endif
