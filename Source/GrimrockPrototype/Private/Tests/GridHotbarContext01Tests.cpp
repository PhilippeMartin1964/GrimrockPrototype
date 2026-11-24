#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "GridCanonicalWeightTestWorld.h"
#include "../Runtime/GridPhysicalThrowPreflightReadModel.h"
#include "../Runtime/GridItemInteractionReadModel.h"

#include "Core/GridLevelAsset.h"
#include "EngineUtils.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Engine/World.h"
#include "Runtime/GridItemContextActionLibrary.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridThrownItemActor.h"
#include "Runtime/GrimrockPlayerController.h"
#include "UI/GridCombatHudWidget.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "UI/GridInventoryWidget.h"

namespace
{
	const FName StoneId(TEXT("Stone_HOTBAR_CONTEXT01"));
	const FName KeyId(TEXT("Key_Copper"));
	const FSoftObjectPath IconPath(TEXT("/Game/GrimrockPrototype/UI/Items/T_Item_Stone.T_Item_Stone"));

	void DefineStone(UGridItemSystemAsset& Root)
	{
		auto* Stone = NewObject<UGridOtherEquipmentDefinitionAsset>(&Root);
		Stone->ItemId = StoneId;
		Stone->DisplayName = FText::FromString(TEXT("Pierre HOTBAR-CONTEXT01"));
		Stone->Icon = TSoftObjectPtr<UTexture2D>(IconPath);
		Stone->WeightKg = 1.0f;
		Stone->HandPolicy = EGridNonArmamentHandPolicy::OneHanded;
		Stone->EquipmentSlots.Add(EGridEquipmentSlot::MainHand);
		Stone->MaxStack = 10;
		Stone->GenericThrow.InitialSpeed = 800.0f;
		Root.ItemDefinitions.Add(Stone);
	}

	const FGridItemContextAction* FindAddToHotbarAction(const TArray<FGridItemContextAction>& Actions)
	{
		return Actions.FindByPredicate(
			[](const FGridItemContextAction& Action)
			{
				return Action.ActionType == EGridItemActionType::AddToHotbar;
			});
	}

	int32 FindAddToHotbarActionIndex(const TArray<FGridItemContextAction>& Actions)
	{
		return Actions.IndexOfByPredicate(
			[](const FGridItemContextAction& Action)
			{
				return Action.ActionType == EGridItemActionType::AddToHotbar;
			});
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridHotbarContext01InventoryActionTest, "Grimrock.Hotbar.HOTBAR_CONTEXT01.InventoryContextAction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridHotbarContext01InventoryActionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridCanonicalWeightTestWorld TestWorld({}, DefineStone);
	if (!TestNotNull(TEXT("The isolated registry is configured"), TestWorld.Scope.Registry))
	{
		return false;
	}

	AGrimrockPartyPawn* Party = TestWorld.World->SpawnActor<AGrimrockPartyPawn>();
	if (!TestNotNull(TEXT("The party pawn is spawned"), Party) || !TestNotNull(TEXT("The party inventory exists"), Party ? Party->PartyInventoryComponent.Get() : nullptr))
	{
		return false;
	}

	UGridPartyInventoryComponent* Inventory = Party->PartyInventoryComponent;
	Inventory->InitializeDefaultPartyIfNeeded();
	const int32 CharacterIndex = Inventory->GetSelectedCharacterIndex();
	TestEqual(TEXT("The default selected character is zero"), CharacterIndex, 0);

	UGridInventoryWidget* InventoryWidget = CreateWidget<UGridInventoryWidget>(TestWorld.World, UGridInventoryWidget::StaticClass());
	if (!TestNotNull(TEXT("The inventory widget is created"), InventoryWidget))
	{
		return false;
	}
	InventoryWidget->InitializeInventoryWidget(Party);

	FGridItemInstance Stone;
	Stone.RuntimeObjectId = FGuid::NewGuid();
	Stone.ItemDefinitionId = StoneId;
	Stone.Quantity = 4;
	TestTrue(TEXT("The stone stack enters inventory"), Inventory->AddItemToCharacterInventory(CharacterIndex, Stone));
	TestEqual(TEXT("The inventory owns four stones"), Inventory->CountItemDefinitionInCharacterInventory(CharacterIndex, Stone.ItemDefinitionId), 4);

	FGridFacingTargetContext FacingTarget;
	TArray<FGridItemContextAction> Actions;
	TestTrue(TEXT("The stone context menu is built through the inventory widget"),
		InventoryWidget->BuildContextActionsForSlot(EGridInventoryUiSlotType::Inventory, 0, FacingTarget, Actions));
	const FGridItemContextAction* AddAction = FindAddToHotbarAction(Actions);
	if (!TestNotNull(TEXT("A physical throwable exposes AddToHotbar"), AddAction))
	{
		return false;
	}
	TestTrue(TEXT("AddToHotbar is enabled while a configurable slot is free"), AddAction->bEnabled);
	const int32 AddActionIndex = FindAddToHotbarActionIndex(Actions);
	TestTrue(TEXT("AddToHotbar owns a stable menu index"), AddActionIndex != INDEX_NONE);

	TestTrue(TEXT("Executing the menu action binds the stone into the first configurable slot"),
		InventoryWidget->ExecuteInventoryContextActionByIndex(EGridInventoryUiSlotType::Inventory, 0, AddActionIndex));
	FGridCombatHotbarBinding BoundStone;
	TestTrue(TEXT("The new shortcut can be read"), Inventory->GetCharacterCombatHotbarBinding(CharacterIndex, 1, BoundStone));
	TestTrue(TEXT("The stone shortcut uses QuickItem policy"), BoundStone.SourcePolicy == EGridCombatActionSourcePolicy::QuickItem);
	TestEqual(TEXT("The stone shortcut keeps definition identity"), BoundStone.SourceDefinitionId, Stone.ItemDefinitionId);
	TestTrue(TEXT("The stone shortcut has the persistent throw identity"), BoundStone.IsPhysicalThrowItemBinding());
	TestEqual(TEXT("The persistent action ID is unchanged"), BoundStone.ActionId, FName(TEXT("ThrowItem_Stone_HOTBAR_CONTEXT01")));
	TestEqual(TEXT("Creating the shortcut does not consume the stack"), Inventory->CountItemDefinitionInCharacterInventory(CharacterIndex, Stone.ItemDefinitionId), 4);

	Actions.Reset();
	TestTrue(TEXT("The stone context menu rebuilds after assignment"),
		InventoryWidget->BuildContextActionsForSlot(EGridInventoryUiSlotType::Inventory, 0, FacingTarget, Actions));
	AddAction = FindAddToHotbarAction(Actions);
	if (!TestNotNull(TEXT("AddToHotbar remains visible after assignment"), AddAction))
	{
		return false;
	}
	TestFalse(TEXT("AddToHotbar is disabled when the definition is already assigned"), AddAction->bEnabled);
	TestTrue(TEXT("The already-assigned state exposes a reason"), !AddAction->DisabledReason.IsEmpty());

	TestTrue(TEXT("The stone shortcut can be cleared for the full-bar scenario"), Inventory->ClearCharacterCombatHotbarBinding(CharacterIndex, 1));
	for (int32 SlotIndex = 1; SlotIndex < FGridCombatHotbarBinding::MinimumSlotCount; ++SlotIndex)
	{
		FGridCombatHotbarBinding OccupiedBinding;
		OccupiedBinding.ActionId = FName(*FString::Printf(TEXT("HOTBAR_CONTEXT01_Universal_%d"), SlotIndex));
		OccupiedBinding.SourcePolicy = EGridCombatActionSourcePolicy::Universal;
		TestTrue(FString::Printf(TEXT("Configurable hotbar slot %d can be occupied"), SlotIndex),
			Inventory->SetCharacterCombatHotbarBinding(CharacterIndex, SlotIndex, OccupiedBinding));
	}

	Actions.Reset();
	TestTrue(TEXT("The stone context menu rebuilds with a full bar"),
		InventoryWidget->BuildContextActionsForSlot(EGridInventoryUiSlotType::Inventory, 0, FacingTarget, Actions));
	AddAction = FindAddToHotbarAction(Actions);
	if (!TestNotNull(TEXT("AddToHotbar remains visible when the bar is full"), AddAction))
	{
		return false;
	}
	TestFalse(TEXT("AddToHotbar is disabled when every configurable slot is occupied"), AddAction->bEnabled);
	TestTrue(TEXT("The full-bar state exposes a reason"), !AddAction->DisabledReason.IsEmpty());

	FGridItemInstance Key;
	Key.RuntimeObjectId = FGuid::NewGuid();
	Key.ItemDefinitionId = KeyId;
	Key.Quantity = 1;
	TestTrue(TEXT("The ordinary key enters inventory"), Inventory->AddItemToCharacterInventory(CharacterIndex, Key));

	int32 KeySlotIndex = INDEX_NONE;
	const FGridCharacterInventoryState& Character = Inventory->PartyInventoryState.ActiveCharacters[CharacterIndex];
	for (int32 SlotIndex = 0; SlotIndex < Character.InventorySlots.Num(); ++SlotIndex)
	{
		if (!Character.InventorySlots[SlotIndex].IsEmpty() && Character.InventorySlots[SlotIndex].Item.RuntimeObjectId == Key.RuntimeObjectId)
		{
			KeySlotIndex = SlotIndex;
			break;
		}
	}
	TestTrue(TEXT("The key inventory slot is located"), KeySlotIndex != INDEX_NONE);

	Actions.Reset();
	TestTrue(TEXT("The ordinary key context menu is built"),
		InventoryWidget->BuildContextActionsForSlot(EGridInventoryUiSlotType::Inventory, KeySlotIndex, FacingTarget, Actions));
	TestNull(TEXT("An item with no quick action and no physical throw support does not expose AddToHotbar"), FindAddToHotbarAction(Actions));

	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridHotbarContext01CanonicalStoneDivergence,
	"Grimrock.Hotbar.HOTBAR_CONTEXT01.CanonicalStoneDivergence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridHotbarContext01CanonicalStoneDivergence::RunTest(const FString&)
{
	FGridCanonicalWeightTestWorld World({}, DefineStone);
	if (!TestNotNull(TEXT("Canonical stone registry"), World.Scope.Registry)) return false;
	auto* Party = World.World->SpawnActor<AGrimrockPartyPawn>();
	if (!TestNotNull(TEXT("Party pawn"), Party)) return false;
	auto* Inventory = Party->PartyInventoryComponent.Get();
	if (!TestNotNull(TEXT("Party inventory"), Inventory)) return false;
	Inventory->InitializeDefaultPartyIfNeeded();
	auto* Runtime = World.World->SpawnActor<AGridLevelRuntimeActor>();
	auto* Controller = World.World->SpawnActor<AGrimrockPlayerController>();
	if (!TestNotNull(TEXT("Grid runtime"), Runtime) ||
		!TestNotNull(TEXT("Player controller"), Controller)) return false;
	auto* Level = NewObject<UGridLevelAsset>(Runtime);
	Level->Width = 4;
	Level->Height = 4;
	Level->CellSize = 200.0f;
	Level->EnsureCellCount();
	for (FGridLevelCellData& Cell : Level->Cells)
	{
		Cell.CellType = EGridCellType::Floor;
		Cell.bBlocksOccupancy = false;
	}
	Runtime->LevelAsset = Level;
	Party->LevelRuntimeActor = Runtime;
	Party->CurrentCellX = 1;
	Party->CurrentCellY = 1;
	Party->SetActorLocation(Runtime->GetCellCenterWorld(1, 1, Party->EyeHeight));
	Controller->Possess(Party);

	auto* Hud = CreateWidget<UGridCombatHudWidget>(World.World, UGridCombatHudWidget::StaticClass());
	if (!TestNotNull(TEXT("Combat HUD"), Hud)) return false;
	Party->CombatHudWidgetInstance = Hud;
	Hud->InitializeCombatHud(Party);

	FGridItemInstance Stone;
	Stone.RuntimeObjectId = FGuid::NewGuid();
	Stone.ItemDefinitionId = StoneId;
	Stone.Quantity = 4;
	if (!TestTrue(TEXT("The canonical stack enters inventory"), Inventory->AddItemToCharacterInventory(0, Stone))) return false;
	const FGridPhysicalThrowPreflightReadView Preflight =
		FGridPhysicalThrowPreflightReadModel::Resolve(World.Scope.Registry, Stone, 10);
	TestTrue(TEXT("The registered non-armament stone passes the generic physical throw preflight"),
		Preflight.bRegisteredIdentity && Preflight.bCanThrow);
	TestEqual(TEXT("The generic launch speed is authored independently"), Preflight.Ballistics.InitialSpeed, 800.0f);

	const UGridItemSubsystem* Registry = World.Scope.Registry;
	TestFalse(TEXT("The generic stone has no specialized projectile"), Preflight.Ballistics.bSpecializedProjectile);
	auto Character = [&]() -> FGridCharacterInventoryState& { return Inventory->PartyInventoryState.ActiveCharacters[0]; };
	constexpr int32 HotbarSlot = 4;
	Character().Attributes.Strength = 1;
	const FGridInventorySlot StackBeforeBinding = Character().InventorySlots[0];
	if (!TestTrue(TEXT("Low Strength can still bind a registered stone"),
		Inventory->SetCharacterCombatHotbarBindingFromItem(0, HotbarSlot, Stone, EGridEquipmentSlot::None)) ||
		!TestTrue(TEXT("HUD contains the configurable slot"), Hud->View.Actions.IsValidIndex(HotbarSlot))) return false;
	TestTrue(TEXT("Binding preserves the complete owned stack"),
		FGridInventorySlot::StaticStruct()->CompareScriptStruct(&StackBeforeBinding, &Character().InventorySlots[0], 0));
	const auto& LowStrength = Hud->View.Actions[HotbarSlot];
	TestTrue(TEXT("Binding remains configured with insufficient Strength"), LowStrength.bHasBinding);
	TestFalse(TEXT("HUD disables activation when Strength is insufficient"), LowStrength.Action.bEnabled);
	TestEqual(TEXT("The HUD shows all four units"), LowStrength.Action.CurrentSourceItemQuantity, 4);
	TestEqual(TEXT("The utility shortcut has no AP cost"), LowStrength.Action.CurrentActionPointCost, 0);
	TestTrue(TEXT("The HUD uses the registered name"),
		LowStrength.Action.Definition.DisplayName.ToString().Contains(TEXT("Pierre HOTBAR-CONTEXT01")));
	TestEqual(TEXT("The HUD uses the registered icon"), LowStrength.Action.Definition.Icon.ToSoftObjectPath(), IconPath);

	FGridCombatActionRequestResult Result;
	TestFalse(TEXT("Insufficient Strength does not arm aiming"), Hud->RequestHotbarSlot(HotbarSlot, Result));
	TestFalse(TEXT("Controller remains out of aiming"), Controller->IsPhysicalThrowAimingActive());
	Character().Attributes.Strength = 10;
	Hud->RefreshFromSources();
	if (!TestTrue(TEXT("A configured hotbar slot opens existing aiming"), Hud->RequestHotbarSlot(HotbarSlot, Result))) return false;
	TestTrue(TEXT("Controller enters physical aiming"), Controller->IsPhysicalThrowAimingActive());
	Controller->CancelPhysicalThrowAiming();
	TestFalse(TEXT("Cancel exits aiming"), Controller->IsPhysicalThrowAimingActive());
	TestEqual(TEXT("Cancel consumes no stones"), Inventory->CountItemDefinitionInCharacterInventory(0, StoneId), 4);

	Party->CurrentCellX = INDEX_NONE;
	TestFalse(TEXT("An invalid launch cell prevents spawn"), Party->TryThrowSelectedCharacterInventoryItem(StoneId, FVector(350, 0, 0)));
	TestEqual(TEXT("Spawn failure consumes no stones"), Inventory->CountItemDefinitionInCharacterInventory(0, StoneId), 4);
	Party->CurrentCellX = 1;

	TSet<FGuid> LaunchedIds;
	for (int32 Remaining = 3; Remaining >= 0; --Remaining)
	{
		if (!TestTrue(TEXT("Each remaining unit can be aimed from the HUD"), Hud->RequestHotbarSlot(HotbarSlot, Result))) return false;
		FHitResult AimHit;
		AimHit.bBlockingHit = true;
		AimHit.ImpactPoint = Party->GetActorLocation() + FVector(300.0, 0.0, -Party->EyeHeight);
		TestTrue(TEXT("Controller handles the simulated world hit"), Controller->HandlePhysicalThrowAimingHit(AimHit));
		TestFalse(TEXT("Successful throw exits aiming"), Controller->IsPhysicalThrowAimingActive());
		TestEqual(TEXT("Exactly one unit was debited"), Inventory->CountItemDefinitionInCharacterInventory(0, StoneId), Remaining);
		TestEqual(TEXT("Binding cleared only on exhaustion"), Hud->View.Actions[HotbarSlot].bHasBinding, Remaining > 0);
		if (Remaining > 0)
		{
			TestEqual(TEXT("HUD quantity updates after debit"), Hud->View.Actions[HotbarSlot].Action.CurrentSourceItemQuantity, Remaining);
			const auto& Stack = Character().InventorySlots[0].Item;
			TestEqual(TEXT("Partially consumed stack retains its identity"), Stack.RuntimeObjectId, Stone.RuntimeObjectId);
			TestEqual(TEXT("Partially consumed stack retains its owner"), Stack.OwnerGuid, Character().CharacterId);
		}
		AGridThrownItemActor* Projectile = nullptr;
		for (TActorIterator<AGridThrownItemActor> It(World.World); It; ++It)
		{
			if (!It->IsActorBeingDestroyed() && !LaunchedIds.Contains(It->ThrownItemInstance.RuntimeObjectId))
			{
				Projectile = *It;
				break;
			}
		}
		if (!TestNotNull(TEXT("Generic projectile spawned"), Projectile)) return false;
		const FGridItemInstance Flight = Projectile->ThrownItemInstance;
		TestEqual(TEXT("The projectile represents one stone at N0"), Flight.Quantity, 1);
		TestEqual(TEXT("The projectile has the registered ID"), Flight.ItemDefinitionId, StoneId);
		TestEqual(TEXT("Physical level preserved"), Flight.ItemLevel, 0);
		TestEqual(TEXT("The projectile unit belongs to the world"), Flight.OwnerType, EGridItemOwnerType::World);
		TestFalse(TEXT("The projectile retains no character owner"), Flight.OwnerGuid.IsValid());
		TestNotEqual(TEXT("The thrown unit has an identity distinct from the source stack"), Flight.RuntimeObjectId, Stone.RuntimeObjectId);
		TestTrue(TEXT("Speed uses the 800 cm/s policy and Strength scaling"),
			FMath::IsNearlyEqual(Projectile->ProjectileMovementComponent->Velocity.Size(), 688.0, 0.1));
		LaunchedIds.Add(Flight.RuntimeObjectId);
		if (Remaining != 0) continue;

		const FVector Impact = Runtime->GetCellCenterWorld(1, 1, 12.0f);
		// Deterministically reach the target so Tick drives the production
		// impact conversion without a physics scene or elapsed frames.
		Projectile->SetActorLocation(Impact);
		Projectile->ConfigureCombatPresentationTarget(true, Impact, 5.0f);
		Projectile->Tick(0.0f);
		AGridItemActor* Pickup = nullptr;
		for (TActorIterator<AGridItemActor> It(World.World); It; ++It)
		{
			if (!It->IsA<AGridThrownItemActor>() && It->GetRuntimeObjectId() == Flight.RuntimeObjectId)
			{
				Pickup = *It;
				break;
			}
		}
		if (!TestNotNull(TEXT("Impact materializes a pickup with the launched GUID"), Pickup)) return false;
		if (!TestTrue(TEXT("The deposited stone is recovered"), Runtime->TryPickupItemActor(Pickup, Party))) return false;
		TestEqual(TEXT("Recovery restores one unit"), Inventory->CountItemDefinitionInCharacterInventory(0, StoneId), 1);
		TestEqual(TEXT("Recovered unit retains its world GUID"), Character().InventorySlots[0].Item.RuntimeObjectId, Flight.RuntimeObjectId);
		TestFalse(TEXT("Recovery does not recreate an exhausted shortcut"), Hud->View.Actions[HotbarSlot].bHasBinding);
		TestTrue(TEXT("Rebinding after recovery is explicit"), Inventory->SetCharacterCombatHotbarBindingFromItem(
			0, HotbarSlot, Character().InventorySlots[0].Item, EGridEquipmentSlot::None));
	}
	TestEqual(TEXT("Four different physical units were launched"), LaunchedIds.Num(), 4);

	auto Capabilities = [Registry, &Stone](FName ItemId, int32 Level, int32 Quantity)
	{
		FGridItemInstance Item = Stone;
		Item.ItemDefinitionId = ItemId;
		Item.ItemLevel = Level;
		Item.Quantity = Quantity;
		return FGridItemInteractionReadModel::Resolve(Registry, Item).bPhysicalThrowHotbar;
	};
	TestTrue(TEXT("An existing specialized ARM remains assignable"), Capabilities(TEXT("Weapon_ThrowingKnife_01"), 1, 1));
	TestTrue(TEXT("A generic one-handed ARM is assignable"), Capabilities(TEXT("Weapon_StraightSword_01"), 1, 1));
	TestFalse(TEXT("A two-handed ARM is not assignable"), Capabilities(TEXT("Weapon_GreatAxe_01"), 1, 1));
	TestFalse(TEXT("An unknown item is not assignable"), Capabilities(TEXT("HOTBAR_Unknown"), 0, 1));
	TestFalse(TEXT("A non-ARM item at N1 is invalid"), Capabilities(StoneId, 1, 1));
	TestFalse(TEXT("A stack above its authored capacity is invalid"), Capabilities(StoneId, 0, 11));
	TestFalse(TEXT("An empty stack is invalid"), Capabilities(StoneId, 0, 0));
	TestFalse(TEXT("An absent registry cannot authorize a stone"),
		FGridItemInteractionReadModel::Resolve(nullptr, Stone).bPhysicalThrowHotbar);
	return !HasAnyErrors();
}

#endif
