#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "UI/GridInventoryWidget.h"

namespace
{
	struct FGridTD064InventoryStackMergeWorld
	{
		UWorld* World = nullptr;

		FGridTD064InventoryStackMergeWorld()
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
				FName(*FString::Printf(TEXT("TD064InventoryStackMerge_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true,
				ERHIFeatureLevel::Num, &InitializationValues);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FGridTD064InventoryStackMergeWorld()
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

	FGridItemInstance MakeItem(const UGridItemDefinitionAsset* Definition, int32 Quantity)
	{
		FGridItemInstance Item;
		Item.RuntimeObjectId = FGuid::NewGuid();
		Item.ItemDefinitionId = Definition->ItemDefinitionId;
		Item.DisplayName = Definition->DisplayName;
		Item.Quantity = Quantity;
		Item.OwnerType = EGridItemOwnerType::World;
		return Item;
	}

	void SetInventorySlot(UGridPartyInventoryComponent* Inventory, int32 CharacterIndex, int32 SlotIndex,
		const UGridItemDefinitionAsset* Definition, int32 Quantity)
	{
		FGridCharacterInventoryState& Character = Inventory->PartyInventoryState.ActiveCharacters[CharacterIndex];
		FGridItemInstance Item = MakeItem(Definition, Quantity);
		Item.OwnerType = EGridItemOwnerType::CharacterInventory;
		Item.OwnerGuid = Character.CharacterId;
		Item.OwnerCharacterIndex = CharacterIndex;
		Character.InventorySlots[SlotIndex].bOccupied = true;
		Character.InventorySlots[SlotIndex].Item = Item;
	}

	void ResetInventory(UGridPartyInventoryComponent* Inventory, int32 CharacterIndex)
	{
		for (FGridInventorySlot& Slot : Inventory->PartyInventoryState.ActiveCharacters[CharacterIndex].InventorySlots)
		{
			Slot = FGridInventorySlot();
		}
		if (Inventory->HasCursorItem())
		{
			Inventory->ClearCursorItem();
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD064InventoryStackMergeTest, "Grimrock.TechnicalDebt.TD06_4.InventoryStackMerge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD064InventoryStackMergeTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridTD064InventoryStackMergeWorld TestWorld;
	if (!TestNotNull(TEXT("The transient world is created"), TestWorld.World))
	{
		return false;
	}

	AGrimrockPartyPawn* Party = TestWorld.World->SpawnActor<AGrimrockPartyPawn>();
	if (!TestNotNull(TEXT("The party pawn is spawned"), Party) ||
		!TestNotNull(TEXT("The party inventory exists"), Party ? Party->PartyInventoryComponent.Get() : nullptr))
	{
		return false;
	}

	UGridPartyInventoryComponent* Inventory = Party->PartyInventoryComponent;
	Inventory->InitializeDefaultPartyIfNeeded();
	const int32 CharacterIndex = Inventory->GetSelectedCharacterIndex();
	UGridInventoryWidget* Widget = CreateWidget<UGridInventoryWidget>(TestWorld.World, UGridInventoryWidget::StaticClass());
	if (!TestNotNull(TEXT("The inventory widget is created"), Widget))
	{
		return false;
	}
	Widget->InitializeInventoryWidget(Party);

	UGridItemDefinitionAsset* StoneDefinition = NewObject<UGridItemDefinitionAsset>(Inventory);
	StoneDefinition->ItemDefinitionId = TEXT("Stone_TD064_Merge");
	StoneDefinition->DisplayName = FText::FromString(TEXT("Pierre TD06.4 merge"));
	StoneDefinition->ItemType = EGridItemType::Misc;
	StoneDefinition->bStackable = true;
	StoneDefinition->MaxStackSize = 10;
	TestTrue(TEXT("The stackable definition is registered"), Inventory->RegisterItemDefinition(StoneDefinition));

	UGridItemDefinitionAsset* OtherDefinition = NewObject<UGridItemDefinitionAsset>(Inventory);
	OtherDefinition->ItemDefinitionId = TEXT("Other_TD064_Swap");
	OtherDefinition->DisplayName = FText::FromString(TEXT("Autre TD06.4 swap"));
	OtherDefinition->ItemType = EGridItemType::Misc;
	OtherDefinition->bStackable = false;
	OtherDefinition->MaxStackSize = 1;
	TestTrue(TEXT("The non-stackable definition is registered"), Inventory->RegisterItemDefinition(OtherDefinition));

	auto TestOwnership = [this, Inventory](const TCHAR* Context)
	{
		FString OwnershipError;
		return TestTrue(Context, Inventory->ValidateInventoryOwnership(OwnershipError));
	};

	// Scenario A: exercise the real UI routing for Ctrl-split followed by a normal occupied-slot drop.
	SetInventorySlot(Inventory, CharacterIndex, 0, StoneDefinition, 2);
	FGridCharacterInventoryState& Character = Inventory->PartyInventoryState.ActiveCharacters[CharacterIndex];
	const FGuid ScenarioATargetId = Character.InventorySlots[0].Item.RuntimeObjectId;
	TestTrue(TEXT("A Ctrl-drag splits one stone through HandleSlotDrop"),
		Widget->HandleSlotDrop(EGridInventoryUiSlotType::Inventory, 0, EGridInventoryUiSlotType::Inventory, 1, true, 1));
	TestEqual(TEXT("The Ctrl-split leaves one stone in the source"), Character.InventorySlots[0].Item.Quantity, 1);
	TestEqual(TEXT("The Ctrl-split creates one stone in the target"), Character.InventorySlots[1].Item.Quantity, 1);
	TestOwnership(TEXT("Ownership is valid after the UI Ctrl-split"));
	TestTrue(TEXT("A normal occupied-slot drop merges through HandleSlotDrop"),
		Widget->HandleSlotDrop(EGridInventoryUiSlotType::Inventory, 1, EGridInventoryUiSlotType::Inventory, 0));
	TestEqual(TEXT("The UI merge restores one stack of two"), Character.InventorySlots[0].Item.Quantity, 2);
	TestTrue(TEXT("The UI merge clears its source slot"), Character.InventorySlots[1].IsEmpty());
	TestTrue(TEXT("The UI merge preserves the target runtime identity"), Character.InventorySlots[0].Item.RuntimeObjectId == ScenarioATargetId);
	TestOwnership(TEXT("Ownership is valid after the UI merge"));

	// Scenario B: a partial merge fills the target and leaves the source remainder in place.
	ResetInventory(Inventory, CharacterIndex);
	SetInventorySlot(Inventory, CharacterIndex, 0, StoneDefinition, 8);
	SetInventorySlot(Inventory, CharacterIndex, 1, StoneDefinition, 4);
	const FGuid PartialTargetId = Character.InventorySlots[0].Item.RuntimeObjectId;
	const FGuid PartialSourceId = Character.InventorySlots[1].Item.RuntimeObjectId;
	TestTrue(TEXT("An occupied inventory drop performs a partial merge"),
		Widget->HandleSlotDrop(EGridInventoryUiSlotType::Inventory, 1, EGridInventoryUiSlotType::Inventory, 0));
	TestEqual(TEXT("The partial merge caps the target at ten"), Character.InventorySlots[0].Item.Quantity, 10);
	TestEqual(TEXT("The partial merge leaves two in the source"), Character.InventorySlots[1].Item.Quantity, 2);
	TestTrue(TEXT("The partial merge preserves the target runtime identity"), Character.InventorySlots[0].Item.RuntimeObjectId == PartialTargetId);
	TestTrue(TEXT("The partial merge preserves the source remainder identity"), Character.InventorySlots[1].Item.RuntimeObjectId == PartialSourceId);
	TestOwnership(TEXT("Ownership is valid after the partial UI merge"));

	// Scenario C: a full target rejects the drop without mutation or swap.
	ResetInventory(Inventory, CharacterIndex);
	SetInventorySlot(Inventory, CharacterIndex, 0, StoneDefinition, 10);
	SetInventorySlot(Inventory, CharacterIndex, 1, StoneDefinition, 1);
	const FGuid FullTargetId = Character.InventorySlots[0].Item.RuntimeObjectId;
	const FGuid RejectedSourceId = Character.InventorySlots[1].Item.RuntimeObjectId;
	TestFalse(TEXT("A full matching target rejects the occupied inventory drop"),
		Widget->HandleSlotDrop(EGridInventoryUiSlotType::Inventory, 1, EGridInventoryUiSlotType::Inventory, 0));
	TestEqual(TEXT("The rejected target remains at ten"), Character.InventorySlots[0].Item.Quantity, 10);
	TestEqual(TEXT("The rejected source remains at one"), Character.InventorySlots[1].Item.Quantity, 1);
	TestTrue(TEXT("The rejected target does not swap"), Character.InventorySlots[0].Item.RuntimeObjectId == FullTargetId);
	TestTrue(TEXT("The rejected source does not swap"), Character.InventorySlots[1].Item.RuntimeObjectId == RejectedSourceId);
	TestOwnership(TEXT("Ownership is valid after the rejected UI merge"));

	// Scenario D: different items keep the existing swap behavior.
	ResetInventory(Inventory, CharacterIndex);
	SetInventorySlot(Inventory, CharacterIndex, 0, StoneDefinition, 1);
	SetInventorySlot(Inventory, CharacterIndex, 1, OtherDefinition, 1);
	const FGuid SwapTargetId = Character.InventorySlots[0].Item.RuntimeObjectId;
	const FGuid SwapSourceId = Character.InventorySlots[1].Item.RuntimeObjectId;
	TestTrue(TEXT("Different inventory items still swap through HandleSlotDrop"),
		Widget->HandleSlotDrop(EGridInventoryUiSlotType::Inventory, 1, EGridInventoryUiSlotType::Inventory, 0));
	TestTrue(TEXT("The former source item reaches the target"), Character.InventorySlots[0].Item.RuntimeObjectId == SwapSourceId);
	TestTrue(TEXT("The former target item reaches the source"), Character.InventorySlots[1].Item.RuntimeObjectId == SwapTargetId);
	TestOwnership(TEXT("Ownership is valid after the different-item UI swap"));

	// Cursor placement uses the same merge semantics, including partial transfer and full rejection.
	ResetInventory(Inventory, CharacterIndex);
	SetInventorySlot(Inventory, CharacterIndex, 0, StoneDefinition, 8);
	const FGuid CursorTargetId = Character.InventorySlots[0].Item.RuntimeObjectId;
	TestTrue(TEXT("A four-stone stack can be placed on the cursor"), Inventory->SetCursorItem(MakeItem(StoneDefinition, 4)));
	TestTrue(TEXT("A cursor stack partially merges through HandleSlotDrop"),
		Widget->HandleSlotDrop(EGridInventoryUiSlotType::Cursor, INDEX_NONE, EGridInventoryUiSlotType::Inventory, 0));
	TestEqual(TEXT("The cursor merge caps the target at ten"), Character.InventorySlots[0].Item.Quantity, 10);
	TestEqual(TEXT("The cursor keeps the two-stone remainder"), Inventory->GetCursorItem().Quantity, 2);
	TestTrue(TEXT("The cursor merge preserves the target runtime identity"), Character.InventorySlots[0].Item.RuntimeObjectId == CursorTargetId);
	TestOwnership(TEXT("Ownership is valid after the partial cursor merge"));
	TestFalse(TEXT("A full target rejects the cursor remainder without swapping"),
		Widget->HandleSlotDrop(EGridInventoryUiSlotType::Cursor, INDEX_NONE, EGridInventoryUiSlotType::Inventory, 0));
	TestEqual(TEXT("The rejected cursor merge leaves the target at ten"), Character.InventorySlots[0].Item.Quantity, 10);
	TestEqual(TEXT("The rejected cursor merge preserves its remainder"), Inventory->GetCursorItem().Quantity, 2);
	TestTrue(TEXT("The rejected cursor merge preserves the target identity"), Character.InventorySlots[0].Item.RuntimeObjectId == CursorTargetId);
	TestOwnership(TEXT("Ownership is valid after the rejected cursor merge"));

	return true;
}

#endif
