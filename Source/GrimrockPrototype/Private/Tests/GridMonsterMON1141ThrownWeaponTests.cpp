#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "GridCanonicalLootTestScope.h"

#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Runtime/Combat/GridPlayerAttackPresentationComponent.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GridThrownItemActor.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Monsters/GridMonsterActor.h"

namespace
{
	struct FGridMON1141TestWorld
	{
		UWorld* World = nullptr;

		FGridMON1141TestWorld()
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
				FName(*FString::Printf(TEXT("MON1141ThrownWeaponWorld_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true,
				ERHIFeatureLevel::Num, &InitializationValues);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FGridMON1141TestWorld()
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

	FGridItemInstance MakeEquippedShuriken(const FGuid& CharacterId, int32 Quantity)
	{
		FGridItemInstance Item;
		Item.RuntimeObjectId = FGuid::NewGuid();
		Item.ItemDefinitionId = TEXT("Shuriken");
		Item.Quantity = Quantity;
		Item.OwnerType = EGridItemOwnerType::EquipmentSlot;
		Item.OwnerGuid = CharacterId;
		Item.OwnerCharacterIndex = 0;
		Item.EquipmentSlot = EGridEquipmentSlot::MainHand;
		return Item;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON1141ThrownWeaponLifecycleTest, "Grimrock.Monsters.MON11.Presentation.ThrownWeaponLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON1141ThrownWeaponLifecycleTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridMON1141TestWorld TestWorld;
	TestNotNull(TEXT("The transient world is created"), TestWorld.World);
	if (!TestWorld.World)
	{
		return false;
	}

	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	UGridLevelAsset* LevelAsset = NewObject<UGridLevelAsset>(Runtime);
	LevelAsset->Width = 4;
	LevelAsset->Height = 4;
	LevelAsset->EnsureCellCount();
	for (FGridLevelCellData& Cell : LevelAsset->Cells)
	{
		Cell.CellType = EGridCellType::Floor;
		Cell.bBlocksOccupancy = false;
	}
	Runtime->LevelAsset = LevelAsset;

	AGrimrockPartyPawn* Party = TestWorld.World->SpawnActor<AGrimrockPartyPawn>();
	Party->LevelRuntimeActor = Runtime;
	Party->CurrentCellX = 1;
	Party->CurrentCellY = 1;
	Party->Facing = EGridEdge::North;
	Party->SetActorLocation(Runtime->GetCellCenterWorld(Party->CurrentCellX, Party->CurrentCellY, Party->EyeHeight));
	auto* Inventory = Party->PartyInventoryComponent.Get();
	FGridCanonicalLootTestScope RegistryScope;
	if (!TestTrue(TEXT("Real ItemSystem attaches"), RegistryScope.Attach(TestWorld.World))) return false;
	const FName KnifeId(TEXT("Weapon_ThrowingKnife_01"));
	FGridResolvedItem Knife, Shuriken;
	if (!TestTrue(TEXT("N7 knife has a specialized projectile"),
		RegistryScope.Registry->ResolveItem(KnifeId, 7, Knife) && Knife.Weapon.Projectile.bEnabled) ||
		!TestTrue(TEXT("Registered shuriken is stackable and uses generic throw"),
			RegistryScope.Registry->ResolveItem(TEXT("Shuriken"), 0, Shuriken) &&
			!Shuriken.bIsArmament && Shuriken.MaxStack >= 2 && Shuriken.GenericThrow.InitialSpeed > 0.f)) return false;

	FGridCharacterInventoryState Character;
	Character.CharacterId = FGuid::NewGuid();
	Character.DisplayName = FText::FromString(TEXT("Mina"));
	Character.Attributes.Strength = 20;
	Character.InventorySlots.SetNum(2);
	Inventory->PartyInventoryState.ActiveCharacters = { Character };
	Inventory->PartyInventoryState.ActiveEquipment.SetNum(1);
	Inventory->PartyInventoryState.SelectedCharacterIndex = 0;
	auto& MainHand = Inventory->PartyInventoryState.ActiveEquipment[0].MainHand;
	MainHand = MakeEquippedShuriken(Character.CharacterId, 2);
	const FVector StackTarget = Runtime->GetCellCenterWorld(1, 2, 100.f);
	TestNull(TEXT("Generic equipped shuriken cannot launch as a specialized attack"),
		Party->TryLaunchEquippedItemForAttack(0, EGridEquipmentSlot::MainHand, Shuriken.ItemId, StackTarget, FIntPoint(1, 1)));
	TestEqual(TEXT("Refused wrong route leaves both units equipped"), MainHand.Quantity, 2);

	FGridItemInstance EquippedKnife = MakeEquippedShuriken(Character.CharacterId, 1);
	EquippedKnife.ItemDefinitionId = KnifeId;
	EquippedKnife.ItemLevel = 7;
	MainHand = EquippedKnife;
	Party->SyncHeldVisualFromSelectedCharacterEquipment();
	TestNull(TEXT("A non-light weapon has no permanent held visual"), Party->HeldItemActor.Get());

	FGridResolvedItem Torch;
	if (!TestTrue(TEXT("Real canonical torch resolves"), RegistryScope.Registry->ResolveItem(TEXT("Item_Torch"), 0, Torch))) return false;
	UStaticMesh* TorchMesh = Torch.EquippedMesh.LoadSynchronous();

	FGridItemInstance EquippedTorch;
	EquippedTorch.RuntimeObjectId = FGuid::NewGuid();
	EquippedTorch.ItemDefinitionId = Torch.ItemId;
	EquippedTorch.Quantity = 1;
	EquippedTorch.bLightsEnabled = true;
	EquippedTorch.OwnerType = EGridItemOwnerType::EquipmentSlot;
	EquippedTorch.OwnerGuid = Character.CharacterId;
	EquippedTorch.OwnerCharacterIndex = 0;
	EquippedTorch.EquipmentSlot = EGridEquipmentSlot::OffHand;
	Party->PartyInventoryComponent->PartyInventoryState.ActiveEquipment[0].OffHand = EquippedTorch;
	Party->SyncHeldVisualFromSelectedCharacterEquipment();
	TestNotNull(TEXT("An equipped light keeps its held visual"), Party->HeldItemActor.Get());
	TestEqual(TEXT("The held light uses its canonical equipped mesh"),
		Party->HeldItemActor && Party->HeldItemActor->MeshComponent ? Party->HeldItemActor->MeshComponent->GetStaticMesh().Get() : nullptr, TorchMesh);

	Party->PartyInventoryComponent->PartyInventoryState.ActiveEquipment[0].OffHand = FGridItemInstance();
	Party->SyncHeldVisualFromSelectedCharacterEquipment();
	TestNull(TEXT("Removing the light does not reveal the equipped weapon"), Party->HeldItemActor.Get());

	AGridMonsterActor* TargetMonster = TestWorld.World->SpawnActor<AGridMonsterActor>();
	TargetMonster->PersistentMonsterId = FGuid::NewGuid();
	TargetMonster->CurrentCell = FIntPoint(1, 2);
	TargetMonster->SetActorLocation(Runtime->GetCellCenterWorld(1, 2, 0.0f));

	UGridTurnManagerComponent* TurnManager = NewObject<UGridTurnManagerComponent>(Runtime, TEXT("MON1141TurnManager"));
	TurnManager->bAutoInitialize = false;
	Runtime->AddInstanceComponent(TurnManager);
	TurnManager->RegisterComponent();
	TestTrue(TEXT("The turn manager is initialized"), TurnManager->InitializeTurnManager(Runtime, Party));

	UGridPlayerAttackPresentationComponent* Presentation = Runtime->GetPlayerAttackPresentationComponent();
	TestNotNull(TEXT("The native presentation component exists"), Presentation);
	if (!Presentation)
	{
		return false;
	}
	Presentation->bNativeAudioPlaybackEnabled = false;
	Presentation->bNativeVFXSpawnEnabled = false;
	Presentation->bNativeFeedbackEnabled = false;
	Presentation->InitializePresentation(TurnManager);

	FGridPlayerAttackRequest Request;
	Request.RequestId = FGuid::NewGuid();
	Request.RoundNumber = 1;
	Request.AttackerCharacterIndex = 0;
	Request.AttackerCharacterId = Character.CharacterId;
	Request.TargetMonsterId = TargetMonster->ResolvePersistenceId();
	Request.PartyCell = FIntPoint(1, 1);
	Request.TargetCell = FIntPoint(1, 2);
	Request.PartyFacing = EGridEdge::North;
	Request.RangeCells = 3;
	Request.AttackId = TEXT("Attack_ThrowingKnife");
	Request.OffensiveItemDefinitionId = KnifeId;
	Request.OffensiveEquipmentSlot = EGridEquipmentSlot::MainHand;
	const FVector Target = TargetMonster->GetActorLocation();
	for (const int32 InvalidLevel : {0, 21})
	{
		MainHand.ItemLevel = InvalidLevel;
		TestNull(TEXT("An invalid physical level refuses specialized launch"),
			Party->TryLaunchEquippedItemForAttack(0, EGridEquipmentSlot::MainHand, KnifeId, Target, Request.PartyCell));
		TestEqual(TEXT("Level refusal preserves the source GUID"), MainHand.RuntimeObjectId, EquippedKnife.RuntimeObjectId);
	}
	MainHand = EquippedKnife;
	AddExpectedError(TEXT("GridPlayerAttack Throw Failed"), EAutomationExpectedErrorFlags::Contains, 1);
	TestNull(TEXT("An invalid launch cell triggers rollback after extraction"),
		Party->TryLaunchEquippedItemForAttack(0, EGridEquipmentSlot::MainHand, KnifeId, Target, FIntPoint(-1, -1)));
	TestTrue(TEXT("Spawn rollback restores all equipped fields"),
		FGridItemInstance::StaticStruct()->CompareScriptStruct(&MainHand, &EquippedKnife, 0));
	TestFalse(TEXT("Rejected launches create no projectile"), bool(TActorIterator<AGridThrownItemActor>(TestWorld.World)));
	Request.PreparedThrownItemActor =
		Party->TryLaunchEquippedItemForAttack(0, EGridEquipmentSlot::MainHand, KnifeId, Target, Request.PartyCell);
	if (!TestNotNull(TEXT("Gameplay commits a recoverable projectile"), Request.PreparedThrownItemActor.Get())) return false;

	TurnManager->OnPlayerAttackRequested.Broadcast(Request);
	TestEqual(TEXT("Exactly one thrown launch is requested"), Presentation->ThrownItemLaunchRequestCount, 1);
	TestEqual(TEXT("Exactly one thrown launch starts"), Presentation->ThrownItemLaunchStartedCount, 1);
	TestTrue(TEXT("The throw never starts boomerang held motion"), !Presentation->bHeldItemMotionStarted && !Presentation->IsHeldItemMotionActive());
	TestFalse(TEXT("The committed attack clears the equipped source"), Inventory->IsEquipmentSlotOccupied(0, EGridEquipmentSlot::MainHand));

	AGridThrownItemActor* ThrownItem = Request.PreparedThrownItemActor.Get();
	const auto& Physical = ThrownItem->ThrownItemInstance;
	TestEqual(TEXT("Specialized projectile carries one item"), Physical.Quantity, 1);
	TestEqual(TEXT("Specialized projectile preserves ItemId"), Physical.ItemDefinitionId, KnifeId);
	TestEqual(TEXT("Specialized projectile keeps physical level"), Physical.ItemLevel, 7);
	TestEqual(TEXT("Specialized projectile preserves its source GUID"), Physical.RuntimeObjectId, EquippedKnife.RuntimeObjectId);
	TestEqual(TEXT("Projectile has world ownership"), Physical.OwnerType, EGridItemOwnerType::World);
	UStaticMesh* ExpectedProjectileMesh = Knife.ProjectileMesh.LoadSynchronous();
	TestNotNull(TEXT("Registered projectile mesh loads"), ExpectedProjectileMesh);
	TestEqual(TEXT("Projectile uses the dedicated mesh"),
		ThrownItem->MeshComponent ? ThrownItem->MeshComponent->GetStaticMesh().Get() : nullptr, ExpectedProjectileMesh);
	TestTrue(TEXT("The projectile mesh is visible"), ThrownItem->MeshComponent && ThrownItem->MeshComponent->IsVisible() && !ThrownItem->IsHidden());
	TestTrue(TEXT("Projectile uses the authored presentation scale"),
		ThrownItem->MeshComponent && ThrownItem->MeshComponent->GetRelativeScale3D().Equals(Knife.ProjectileAppearance.RelativeScale, KINDA_SMALL_NUMBER));
	TestTrue(TEXT("Launch velocity follows the specialized policy"),
		ThrownItem->ProjectileMovementComponent &&
		FMath::IsNearlyEqual(ThrownItem->ProjectileMovementComponent->Velocity.Size(),
			Knife.Weapon.Projectile.InitialSpeed * FMath::Sqrt(2.0) * (1.0 - 0.35 * Knife.WeightKg / 5.0), 1.0));
	const FGuid ThrownRuntimeId = Physical.RuntimeObjectId;

	FGridAttackResult HitResult;
	HitResult.bHit = true;
	HitResult.TargetHealthBefore = 5;
	HitResult.TargetHealthAfter = 4;
	HitResult.HealthDamage = 1;
	TurnManager->OnPlayerAttackResolved.Broadcast(Request, TargetMonster, HitResult);
	TestTrue(TEXT("A hit configures target interception"), ThrownItem->bStopsAtCombatPresentationTarget);

	const FVector TargetLocation = ThrownItem->CombatPresentationTargetLocation;
	const FVector TravelDirection = (TargetLocation - ThrownItem->GetActorLocation()).GetSafeNormal();
	ThrownItem->SetActorLocation(TargetLocation + TravelDirection * 50.0f);
	ThrownItem->Tick(1.0f / 60.0f);
	TestTrue(TEXT("Crossing the hit target converts the projectile"), ThrownItem->HasCompletedImpactConversion());
	AGridItemActor* DroppedPickup = nullptr;
	for (TActorIterator<AGridItemActor> It(TestWorld.World); It; ++It)
	{
		AGridItemActor* Candidate = *It;
		if (Candidate && !Candidate->IsActorBeingDestroyed() && !Candidate->IsA<AGridThrownItemActor>() && Candidate->GetRuntimeObjectId() == ThrownRuntimeId)
		{
			DroppedPickup = Candidate;
			break;
		}
	}
	TestNotNull(TEXT("The impact creates one recoverable pickup"), DroppedPickup);
	TestTrue(TEXT("The recoverable pickup keeps a visible world mesh"),
		DroppedPickup && DroppedPickup->MeshComponent && DroppedPickup->MeshComponent->GetStaticMesh().Get() == Knife.WorldMesh.LoadSynchronous() &&
			DroppedPickup->MeshComponent->IsVisible() && !DroppedPickup->IsHidden());

	TurnManager->OnPlayerAttackRejected.Broadcast(0, EGridPlayerAttackRejectReason::PassageBlocked);
	TestEqual(TEXT("A rejected attack creates no additional projectile"), Presentation->ThrownItemLaunchStartedCount, 1);
	TestFalse(TEXT("Rejected diagnostics report no new launch"), Presentation->bThrownItemLaunchStarted);
	if (!TestNotNull(TEXT("Pickup preserves the projectile GUID"), DroppedPickup)) return false;
	TestEqual(TEXT("Pickup preserves the knife level"), DroppedPickup->WorldItemLevel, 7);
	TestTrue(TEXT("The physical pickup can be recovered"), Runtime->TryPickupItemActor(DroppedPickup, Party));
	TestEqual(TEXT("Exactly one knife is recovered"), Inventory->CountItemDefinitionInCharacterInventory(0, KnifeId), 1);
	TestNull(TEXT("No permanent weapon visual remains"), Party->HeldItemActor.Get());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON1142PlacedItemRebuildUniquenessTest, "Grimrock.Monsters.MON11.Presentation.PlacedItemRebuildUniqueness",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON1142PlacedItemRebuildUniquenessTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridMON1141TestWorld TestWorld;
	TestNotNull(TEXT("The transient rebuild world is created"), TestWorld.World);
	if (!TestWorld.World)
	{
		return false;
	}

	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	UGridLevelAsset* LevelAsset = NewObject<UGridLevelAsset>(Runtime);
	LevelAsset->Width = 2;
	LevelAsset->Height = 2;
	LevelAsset->EnsureCellCount();
	for (FGridLevelCellData& Cell : LevelAsset->Cells)
	{
		Cell.CellType = EGridCellType::Floor;
		Cell.bBlocksOccupancy = false;
	}

	FGridCanonicalLootTestScope RegistryScope;
	if (!TestTrue(TEXT("Canonical rebuild fixture registry"), RegistryScope.Attach(TestWorld.World)))
		return false;

	const FGuid PlacedObjectId = FGuid::NewGuid();
	FGridLooseItemInstance PlacedItem;
	PlacedItem.InstanceId = PlacedObjectId;
	PlacedItem.CellX = 0;
	PlacedItem.CellY = 0;
	PlacedItem.ItemId = TEXT("Shuriken");
	LevelAsset->LooseItemInstances.Add(PlacedItem);
	Runtime->LevelAsset = LevelAsset;

	const auto CountLivePlacedItems = [&TestWorld, &PlacedObjectId]()
	{
		int32 Count = 0;
		for (TActorIterator<AGridItemActor> It(TestWorld.World); It; ++It)
		{
			const AGridItemActor* Item = *It;
			if (Item && !Item->IsActorBeingDestroyed() && Item->GetRuntimeObjectId() == PlacedObjectId)
			{
				++Count;
			}
		}
		return Count;
	};

	Runtime->RebuildLevel(EGridRuntimeRebuildMode::Full);
	TestEqual(TEXT("The first runtime rebuild has generation one"), Runtime->GetRuntimeObjectRebuildGeneration(), 1);
	TestEqual(TEXT("The first rebuild creates one placed shuriken"), CountLivePlacedItems(), 1);

	Runtime->RebuildLevel(EGridRuntimeRebuildMode::Full);
	TestEqual(TEXT("The deferred runtime rebuild has generation two"), Runtime->GetRuntimeObjectRebuildGeneration(), 2);
	TestEqual(TEXT("A second rebuild replaces rather than duplicates the item"), CountLivePlacedItems(), 1);

	return true;
}

#endif
