#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridItemContextActionLibrary.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "UI/GridInventoryWidget.h"

namespace
{
	struct FGridHotbarContext01TestWorld
	{
		UWorld* World = nullptr;

		FGridHotbarContext01TestWorld()
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
				FName(*FString::Printf(TEXT("HotbarContext01World_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true,
				ERHIFeatureLevel::Num, &InitializationValues);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FGridHotbarContext01TestWorld()
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

	FGridHotbarContext01TestWorld TestWorld;
	if (!TestNotNull(TEXT("The transient world is created"), TestWorld.World))
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

	UGridItemDefinitionAsset* StoneDefinition = NewObject<UGridItemDefinitionAsset>(Inventory);
	StoneDefinition->ItemDefinitionId = TEXT("Stone_HOTBAR_CONTEXT01");
	StoneDefinition->DisplayName = FText::FromString(TEXT("Pierre HOTBAR-CONTEXT01"));
	StoneDefinition->ItemType = EGridItemType::Misc;
	StoneDefinition->HandUsage = EGridItemHandUsage::OneHanded;
	StoneDefinition->Weight = 1.0f;
	StoneDefinition->ThrowSpeed = 800.0f;
	StoneDefinition->bStackable = true;
	StoneDefinition->MaxStackSize = 10;
	TestTrue(TEXT("The physical stone definition is registered"), Inventory->RegisterItemDefinition(StoneDefinition));

	FGridItemInstance Stone;
	Stone.RuntimeObjectId = FGuid::NewGuid();
	Stone.ItemDefinitionId = StoneDefinition->ItemDefinitionId;
	Stone.DisplayName = StoneDefinition->DisplayName;
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
	for (int32 SlotIndex = 1; SlotIndex < FGridCombatHotbarBinding::SlotCount; ++SlotIndex)
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

	UGridItemDefinitionAsset* KeyDefinition = NewObject<UGridItemDefinitionAsset>(Inventory);
	KeyDefinition->ItemDefinitionId = TEXT("Key_HOTBAR_CONTEXT01");
	KeyDefinition->DisplayName = FText::FromString(TEXT("Clé HOTBAR-CONTEXT01"));
	KeyDefinition->ItemType = EGridItemType::Key;
	KeyDefinition->HandUsage = EGridItemHandUsage::NotHandHeld;
	KeyDefinition->ThrowSpeed = 0.0f;
	KeyDefinition->bProvidesQuickItemCombatAction = false;
	TestTrue(TEXT("The ordinary key definition is registered"), Inventory->RegisterItemDefinition(KeyDefinition));

	FGridItemInstance Key;
	Key.RuntimeObjectId = FGuid::NewGuid();
	Key.ItemDefinitionId = KeyDefinition->ItemDefinitionId;
	Key.DisplayName = KeyDefinition->DisplayName;
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

#endif
