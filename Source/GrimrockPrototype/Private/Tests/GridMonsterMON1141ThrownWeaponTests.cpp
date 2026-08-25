#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Runtime/Combat/GridPlayerAttackPresentationComponent.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridItemDefinitionAsset.h"
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
		Item.DisplayName = FText::FromString(TEXT("Shuriken"));
		Item.Quantity = Quantity;
		Item.Weight = 0.1f;
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

	UGridItemDefinitionAsset* Definition = NewObject<UGridItemDefinitionAsset>();
	Definition->ItemDefinitionId = TEXT("Shuriken");
	Definition->DisplayName = FText::FromString(TEXT("Shuriken"));
	Definition->ItemType = EGridItemType::Weapon;
	Definition->Weight = 0.1f;
	Definition->bStackable = true;
	Definition->MaxStackSize = 20;
	Definition->bThrowable = true;
	Definition->ThrowSpeed = 1800.0f;
	Definition->ThrowArc = 0.08f;
	Definition->ThrowLifeSeconds = 5.0f;
	Definition->ThrowImpactDropOffset = 12.0f;
	Definition->CompatibleEquipmentSlots.Add(EGridEquipmentSlot::MainHand);
	Definition->bProvidesAttackPresentation = true;
	Definition->PlayerAttackPresentationProfile.MotionStyle = EGridPlayerAttackMotionStyle::Throw;
	Definition->PlayerAttackPresentationProfile.bAnimateHeldItem = true;

	UStaticMesh* WorldMesh = NewObject<UStaticMesh>();
	UStaticMesh* EquippedMesh = NewObject<UStaticMesh>();
	Definition->WorldMesh = WorldMesh;
	Definition->EquippedMesh = EquippedMesh;
	TestEqual(TEXT("EquippedMesh is preferred for the held visual"), Definition->LoadHeldMesh(), EquippedMesh);
	Definition->EquippedMesh.Reset();
	TestEqual(TEXT("WorldMesh is the held visual fallback"), Definition->LoadHeldMesh(), WorldMesh);
	Definition->EquippedMesh = EquippedMesh;
	TestTrue(TEXT("The default throw rotation faces a flat mesh toward the source"),
		Definition->ThrowVisualRelativeRotation.Equals(FRotator(-90.0f, 0.0f, 0.0f), KINDA_SMALL_NUMBER));
	TestTrue(TEXT("The default throw scale is readable"), Definition->ThrowVisualRelativeScale.Equals(FVector(1.5f), KINDA_SMALL_NUMBER));
	TestEqual(TEXT("The default throw spin is configured"), Definition->ThrowVisualSpinDegreesPerSecond, 1080.0f);
	TestTrue(TEXT("A throwable Throw presentation is valid"), Definition->HasValidPlayerAttackPresentation());
	Definition->bThrowable = false;
	TestFalse(TEXT("A non-throwable Throw presentation is rejected"), Definition->HasValidPlayerAttackPresentation());
	Definition->bThrowable = true;

	UGridPartyInventoryComponent* TransferInventory = NewObject<UGridPartyInventoryComponent>();
	FGridCharacterInventoryState TransferCharacter;
	TransferCharacter.CharacterId = FGuid::NewGuid();
	TransferInventory->PartyInventoryState.ActiveCharacters = { TransferCharacter };
	TransferInventory->PartyInventoryState.ActiveEquipment.SetNum(1);
	TransferInventory->PartyInventoryState.ActiveEquipment[0].MainHand = MakeEquippedShuriken(TransferCharacter.CharacterId, 2);
	const FGuid StackRuntimeId = TransferInventory->PartyInventoryState.ActiveEquipment[0].MainHand.RuntimeObjectId;

	FGridItemInstance ExtractedItem;
	TestTrue(TEXT("One equipped unit can be extracted"),
		TransferInventory->TryExtractOneEquippedItemForWorldTransfer(0, EGridEquipmentSlot::MainHand, TEXT("Shuriken"), ExtractedItem));
	TestEqual(TEXT("The equipped stack is decremented"), TransferInventory->PartyInventoryState.ActiveEquipment[0].MainHand.Quantity, 1);
	TestEqual(TEXT("The extracted unit belongs to the world"), ExtractedItem.OwnerType, EGridItemOwnerType::World);
	TestNotEqual(TEXT("A split unit has its own runtime identity"), ExtractedItem.RuntimeObjectId, StackRuntimeId);
	TestTrue(TEXT("A failed spawn can restore the extracted unit"),
		TransferInventory->TryRestoreExtractedItemToEquipment(0, EGridEquipmentSlot::MainHand, ExtractedItem));
	TestEqual(TEXT("Rollback restores the original stack quantity"), TransferInventory->PartyInventoryState.ActiveEquipment[0].MainHand.Quantity, 2);

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
	FGridLevelObjectData DefinitionLookup;
	DefinitionLookup.Type = EGridLevelObjectType::Item;
	DefinitionLookup.ItemDefinitionAsset = Definition;
	LevelAsset->Objects.Add(DefinitionLookup);
	Runtime->LevelAsset = LevelAsset;

	AGrimrockPartyPawn* Party = TestWorld.World->SpawnActor<AGrimrockPartyPawn>();
	Party->LevelRuntimeActor = Runtime;
	Party->CurrentCellX = 1;
	Party->CurrentCellY = 1;
	Party->Facing = EGridEdge::North;
	Party->SetActorLocation(Runtime->GetCellCenterWorld(Party->CurrentCellX, Party->CurrentCellY, Party->EyeHeight));
	Party->PartyInventoryComponent->RegisterItemDefinition(Definition);

	FGridCharacterInventoryState Character;
	Character.CharacterId = FGuid::NewGuid();
	Character.DisplayName = FText::FromString(TEXT("Mina"));
	Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters = { Character };
	Party->PartyInventoryComponent->PartyInventoryState.ActiveEquipment.SetNum(1);
	Party->PartyInventoryComponent->PartyInventoryState.SelectedCharacterIndex = 0;
	Party->PartyInventoryComponent->PartyInventoryState.ActiveEquipment[0].MainHand = MakeEquippedShuriken(Character.CharacterId, 2);

	Party->SyncHeldVisualFromSelectedCharacterEquipment();
	TestNull(TEXT("A non-light weapon has no permanent held visual"), Party->HeldItemActor.Get());

	UGridItemDefinitionAsset* TorchDefinition = NewObject<UGridItemDefinitionAsset>(Party);
	TorchDefinition->ItemDefinitionId = TEXT("Item_Torch");
	TorchDefinition->DisplayName = FText::FromString(TEXT("Torche"));
	TorchDefinition->ItemType = EGridItemType::Torch;
	TorchDefinition->Weight = 1.0f;
	TorchDefinition->bCanEmitLight = true;
	TorchDefinition->CompatibleEquipmentSlots.Add(EGridEquipmentSlot::OffHand);
	UStaticMesh* TorchMesh = NewObject<UStaticMesh>(Party);
	TorchDefinition->WorldMesh = TorchMesh;
	Party->PartyInventoryComponent->RegisterItemDefinition(TorchDefinition);

	FGridItemInstance EquippedTorch;
	EquippedTorch.RuntimeObjectId = FGuid::NewGuid();
	EquippedTorch.ItemDefinitionId = TorchDefinition->ItemDefinitionId;
	EquippedTorch.DisplayName = TorchDefinition->DisplayName;
	EquippedTorch.Quantity = 1;
	EquippedTorch.Weight = TorchDefinition->Weight;
	EquippedTorch.OwnerType = EGridItemOwnerType::EquipmentSlot;
	EquippedTorch.OwnerGuid = Character.CharacterId;
	EquippedTorch.OwnerCharacterIndex = 0;
	EquippedTorch.EquipmentSlot = EGridEquipmentSlot::OffHand;
	Party->PartyInventoryComponent->PartyInventoryState.ActiveEquipment[0].OffHand = EquippedTorch;
	Party->SyncHeldVisualFromSelectedCharacterEquipment();
	TestNotNull(TEXT("An equipped light keeps its held visual"), Party->HeldItemActor.Get());
	TestEqual(TEXT("The held light uses its world mesh"),
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
	Request.AttackId = TEXT("Attack_Shuriken");
	Request.OffensiveItemDefinitionId = TEXT("Shuriken");
	Request.OffensiveEquipmentSlot = EGridEquipmentSlot::MainHand;
	Request.PreparedThrownItemActor =
		Party->TryLaunchEquippedItemForAttack(0, EGridEquipmentSlot::MainHand, TEXT("Shuriken"), TargetMonster->GetActorLocation(), Request.PartyCell);
	TestNotNull(TEXT("Gameplay commits the recoverable projectile before presentation"), Request.PreparedThrownItemActor.Get());

	TurnManager->OnPlayerAttackRequested.Broadcast(Request);
	TestEqual(TEXT("Exactly one thrown launch is requested"), Presentation->ThrownItemLaunchRequestCount, 1);
	TestEqual(TEXT("Exactly one thrown launch starts"), Presentation->ThrownItemLaunchStartedCount, 1);
	TestTrue(TEXT("The throw never starts boomerang held motion"), !Presentation->bHeldItemMotionStarted && !Presentation->IsHeldItemMotionActive());
	TestEqual(TEXT("One equipped shuriken remains"), Party->PartyInventoryComponent->PartyInventoryState.ActiveEquipment[0].MainHand.Quantity, 1);

	AGridThrownItemActor* ThrownItem = nullptr;
	for (TActorIterator<AGridThrownItemActor> It(TestWorld.World); It; ++It)
	{
		ThrownItem = *It;
		break;
	}
	TestNotNull(TEXT("A recoverable thrown item actor exists"), ThrownItem);
	if (!ThrownItem)
	{
		return false;
	}
	TestEqual(TEXT("The projectile carries exactly one shuriken"), ThrownItem->ThrownItemInstance.Quantity, 1);
	TestEqual(TEXT("The projectile uses the world mesh"), ThrownItem->MeshComponent ? ThrownItem->MeshComponent->GetStaticMesh().Get() : nullptr, WorldMesh);
	TestTrue(TEXT("The projectile mesh is visible"), ThrownItem->MeshComponent && ThrownItem->MeshComponent->IsVisible() && !ThrownItem->IsHidden());
	TestTrue(TEXT("The projectile uses its readable visual scale"),
		ThrownItem->MeshComponent && ThrownItem->MeshComponent->GetRelativeScale3D().Equals(Definition->ThrowVisualRelativeScale, KINDA_SMALL_NUMBER));
	TestTrue(TEXT("The projectile turns the flat mesh toward the source"),
		ThrownItem->MeshComponent &&
			ThrownItem->MeshComponent->GetRelativeTransform().GetRotation().Equals(Definition->ThrowVisualRelativeRotation.Quaternion(), KINDA_SMALL_NUMBER));
	const FQuat RotationBeforeSpin = ThrownItem->MeshComponent ? ThrownItem->MeshComponent->GetRelativeRotation().Quaternion() : FQuat::Identity;
	ThrownItem->Tick(1.0f / 60.0f);
	TestFalse(TEXT("The projectile mesh spins while flying"),
		ThrownItem->MeshComponent && ThrownItem->MeshComponent->GetRelativeRotation().Quaternion().Equals(RotationBeforeSpin, KINDA_SMALL_NUMBER));
	const FGuid ThrownRuntimeId = ThrownItem->ThrownItemInstance.RuntimeObjectId;

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
	TestTrue(TEXT("The shuriken becomes a world pickup in the target cell"), Runtime->GetWorldItemWeightAtCell(1, 2) > 0.0f);
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
		DroppedPickup && DroppedPickup->MeshComponent && DroppedPickup->MeshComponent->GetStaticMesh().Get() == WorldMesh &&
			DroppedPickup->MeshComponent->IsVisible() && !DroppedPickup->IsHidden());

	TurnManager->OnPlayerAttackRejected.Broadcast(0, EGridPlayerAttackRejectReason::PassageBlocked);
	TestEqual(TEXT("A rejected attack does not consume the remaining shuriken"),
		Party->PartyInventoryComponent->PartyInventoryState.ActiveEquipment[0].MainHand.Quantity, 1);
	TestEqual(TEXT("A rejected attack creates no additional projectile"), Presentation->ThrownItemLaunchStartedCount, 1);
	TestFalse(TEXT("Rejected diagnostics report no new launch"), Presentation->bThrownItemLaunchStarted);

	AGridThrownItemActor* LastThrownItem =
		Party->TryLaunchEquippedItemForAttack(0, EGridEquipmentSlot::MainHand, TEXT("Shuriken"), TargetLocation, FIntPoint(1, 1));
	TestNotNull(TEXT("The final equipped shuriken can be launched"), LastThrownItem);
	TestFalse(TEXT("The final shuriken clears MainHand"), Party->PartyInventoryComponent->IsEquipmentSlotOccupied(0, EGridEquipmentSlot::MainHand));
	TestNull(TEXT("The held visual disappears with the final shuriken"), Party->HeldItemActor.Get());
	if (LastThrownItem)
	{
		LastThrownItem->Destroy();
	}

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

	UGridItemDefinitionAsset* Definition = NewObject<UGridItemDefinitionAsset>(Runtime);
	Definition->ItemDefinitionId = TEXT("Shuriken");
	Definition->DisplayName = FText::FromString(TEXT("Shuriken"));
	Definition->ItemType = EGridItemType::Weapon;
	Definition->Weight = 0.1f;
	Definition->WorldMesh = NewObject<UStaticMesh>(Runtime);

	const FGuid PlacedObjectId = FGuid::NewGuid();
	FGridLevelObjectData PlacedItem;
	PlacedItem.ObjectId = PlacedObjectId;
	PlacedItem.Type = EGridLevelObjectType::Item;
	PlacedItem.CellX = 0;
	PlacedItem.CellY = 0;
	PlacedItem.bInitiallyEnabled = true;
	PlacedItem.ItemDefinitionAsset = Definition;
	PlacedItem.ItemDefinitionId = Definition->ItemDefinitionId;
	LevelAsset->Objects.Add(PlacedItem);
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
