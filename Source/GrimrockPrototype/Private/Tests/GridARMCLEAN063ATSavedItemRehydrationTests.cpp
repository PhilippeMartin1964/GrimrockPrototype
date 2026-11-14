#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "UObject/StrongObjectPtr.h"

namespace ARMCLEAN063AT
{
	struct FFixture
	{
		UWorld* World = nullptr;
		UGameInstance* GI = nullptr;
		AGrimrockPartyPawn* Pawn = nullptr;
		UGridPartyInventoryComponent* Inventory = nullptr;
		UGridItemSubsystem* Registry = nullptr;

		FFixture()
		{
			const UWorld::InitializationValues Settings = UWorld::InitializationValues()
				.AllowAudioPlayback(false).RequiresHitProxies(false)
				.CreatePhysicsScene(false).CreateNavigation(false)
				.CreateAISystem(false).ShouldSimulatePhysics(false)
				.SetTransactional(false);
			World = UWorld::CreateWorld(EWorldType::Game, false,
				FName(*FString::Printf(TEXT("ARM063AT_%s"),
					*FGuid::NewGuid().ToString(EGuidFormats::Digits))),
				nullptr, true, ERHIFeatureLevel::Num, &Settings);
			if (!World || !GEngine) return;
			FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
			Context.SetCurrentWorld(World);
			GI = NewObject<UGameInstance>(GetTransientPackage());
			GI->Init();
			World->SetGameInstance(GI);
			Registry = GI->GetSubsystem<UGridItemSubsystem>();
			Pawn = World->SpawnActor<AGrimrockPartyPawn>();
			if (!Pawn) return;
			Inventory = Pawn->PartyInventoryComponent.Get();
			if (Inventory) Inventory->InitializeDefaultPartyIfNeeded();
		}
		~FFixture()
		{
			if (World) World->SetGameInstance(nullptr);
			if (GI) GI->Shutdown();
			if (World)
			{
				World->DestroyWorld(false);
				if (GEngine) GEngine->DestroyWorldContext(World);
			}
		}
		bool Ready() const
		{
			return Registry && Registry->HasCanonicalItem(TEXT("Weapon_StraightSword_01")) &&
				Registry->HasCanonicalItem(TEXT("Item_Arrows_01")) &&
				Inventory && Inventory->PartyInventoryState.ActiveCharacters.IsValidIndex(0) &&
				Inventory->PartyInventoryState.ActiveEquipment.IsValidIndex(0);
		}
		FGridItemInstance Make(FName Id, int32 Level, int32 Quantity,
			EGridItemOwnerType Owner = EGridItemOwnerType::CharacterInventory,
			EGridEquipmentSlot EquipmentSlot = EGridEquipmentSlot::None) const
		{
			FGridItemInstance Item;
			Item.RuntimeObjectId = FGuid::NewGuid();
			Item.ItemDefinitionId = Id;
			Item.ItemLevel = Level;
			Item.Quantity = Quantity;
			Item.Weight = 777.0f; // Old saved cache deliberately remains untouched.
			Item.DisplayName = FText::FromString(TEXT("custom saved label"));
			Item.AbsorbedEquipmentPhysicalArmor = 2;
			Item.AbsorbedEquipmentMagicalArmor = 1;
			Item.bLightsEnabled = true;
			Item.OwnerType = Owner;
			Item.OwnerCharacterIndex =
				Owner == EGridItemOwnerType::Cursor ? INDEX_NONE : 0;
			Item.OwnerGuid = Owner == EGridItemOwnerType::Cursor
				? FGuid() : Inventory->PartyInventoryState.ActiveCharacters[0].CharacterId;
			Item.EquipmentSlot = EquipmentSlot;
			return Item;
		}
		void Put(int32 SlotIndex, const FGridItemInstance& Item) const
		{
			FGridInventorySlot& Slot =
				Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots[SlotIndex];
			Slot.bOccupied = true;
			Slot.Item = Item;
		}
	};

	UGridItemDefinitionAsset* MakeLegacy(UObject* Outer, FName Id)
	{
		UGridItemDefinitionAsset* Legacy = NewObject<UGridItemDefinitionAsset>(Outer);
		Legacy->ItemDefinitionId = Id;
		Legacy->DisplayName = FText::FromName(Id);
		Legacy->Weight = 13.0f;
		Legacy->MaxStackSize = 1;
		return Legacy;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean063ATSavedRehydration,
	"Grimrock.RPG.ARM.CLEAN06_3.CanonicalSavedItemRehydration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean063ATSavedRehydration::RunTest(const FString&)
{
	ARMCLEAN063AT::FFixture Fixture;
	if (!TestTrue(TEXT("Configured GI and inventory are available"), Fixture.Ready())) return false;

	const FName SwordId(TEXT("Weapon_StraightSword_01"));
	const FName ShieldId(TEXT("Weapon_RoundShield_01"));
	const FName ArrowId(TEXT("Item_Arrows_01"));
	FGridResolvedItem ArrowMeta;
	if (!TestTrue(TEXT("Real canonical N0 ammo resolves"),
		Fixture.Registry->ResolveItem(ArrowId, 0, ArrowMeta))) return false;

	const FGridItemInstance Sword =
		Fixture.Make(SwordId, 20, 1);
	const FGridItemInstance Arrows =
		Fixture.Make(ArrowId, 0, FMath::Min(3, ArrowMeta.MaxStack));
	Fixture.Put(0, Sword);
	Fixture.Put(1, Arrows);

	const FGridItemInstance Shield = Fixture.Make(ShieldId, 1, 1,
		EGridItemOwnerType::EquipmentSlot, EGridEquipmentSlot::OffHand);
	Fixture.Inventory->PartyInventoryState.ActiveEquipment[0].OffHand = Shield;

	FGridCharacterInventoryState Pooled;
	Pooled.CharacterId = FGuid::NewGuid();
	Pooled.InventorySlots.SetNum(1);
	Pooled.InventorySlots[0].bOccupied = true;
	Pooled.InventorySlots[0].Item = Fixture.Make(SwordId, 1, 1);
	Pooled.InventorySlots[0].Item.OwnerGuid = Pooled.CharacterId;
	Fixture.Inventory->PartyInventoryState.CharacterPool.Add(Pooled);
	Fixture.Inventory->PartyInventoryState.CursorItem =
		Fixture.Make(ArrowId, 0, 1, EGridItemOwnerType::Cursor);
	Fixture.Inventory->PartyInventoryState.bHasCursorItem = true;

	const FGridItemInstance BeforeSword = Fixture.Inventory->
		PartyInventoryState.ActiveCharacters[0].InventorySlots[0].Item;
	FName FailedId = NAME_None;
	TSet<FName> LookedUpIds;
	TestTrue(TEXT("Canonical-only party rehydrates with no old DA files"),
		Fixture.Inventory->RehydrateOwnedItemDefinitions(
			[&LookedUpIds](FName Id) -> UGridItemDefinitionAsset*
			{
				LookedUpIds.Add(Id);
				return nullptr;
			}, FailedId));
	TestTrue(TEXT("No failure identity"), FailedId.IsNone());
	TestTrue(TEXT("Optional resolver sees every canonical id"), LookedUpIds.Contains(SwordId) &&
		LookedUpIds.Contains(ShieldId) && LookedUpIds.Contains(ArrowId));
	TestNull(TEXT("No old sword definition required"), Fixture.Inventory->FindItemDefinition(SwordId));
	TestNull(TEXT("No old shield definition required"), Fixture.Inventory->FindItemDefinition(ShieldId));
	TestEqual(TEXT("Saved GUID preserved"), Fixture.Inventory->
		PartyInventoryState.ActiveCharacters[0].InventorySlots[0].Item.RuntimeObjectId,
		BeforeSword.RuntimeObjectId);
	TestEqual(TEXT("Saved N20 preserved"), Fixture.Inventory->
		PartyInventoryState.ActiveCharacters[0].InventorySlots[0].Item.ItemLevel, 20);
	TestEqual(TEXT("Cached weight NOT rewritten on rehydrate"), Fixture.Inventory->
		PartyInventoryState.ActiveCharacters[0].InventorySlots[0].Item.Weight, BeforeSword.Weight);
	TestTrue(TEXT("Saved display name preserved"), Fixture.Inventory->
		PartyInventoryState.ActiveCharacters[0].InventorySlots[0].Item.DisplayName.EqualTo(BeforeSword.DisplayName));
	TestEqual(TEXT("Physical armor debt preserved"), Fixture.Inventory->
		PartyInventoryState.ActiveCharacters[0].InventorySlots[0].Item.AbsorbedEquipmentPhysicalArmor, 2);
	TestEqual(TEXT("Magical armor debt preserved"), Fixture.Inventory->
		PartyInventoryState.ActiveCharacters[0].InventorySlots[0].Item.AbsorbedEquipmentMagicalArmor, 1);
	TestTrue(TEXT("Lights preserved"), Fixture.Inventory->
		PartyInventoryState.ActiveCharacters[0].InventorySlots[0].Item.bLightsEnabled);
	TestEqual(TEXT("Arrow stays N0"), Fixture.Inventory->
		PartyInventoryState.ActiveCharacters[0].InventorySlots[1].Item.ItemLevel, 0);
	TestTrue(TEXT("Canonical N20 sword can equip with no old definition"),
		Fixture.Inventory->CanEquipItemToSlot(0, Sword, EGridEquipmentSlot::MainHand));

	UGridItemDefinitionAsset* OldSword = ARMCLEAN063AT::MakeLegacy(Fixture.Inventory, SwordId);
	UGridItemDefinitionAsset* OldShield = ARMCLEAN063AT::MakeLegacy(Fixture.Inventory, ShieldId);
	UGridItemDefinitionAsset* OldArrow = ARMCLEAN063AT::MakeLegacy(Fixture.Inventory, ArrowId);
	const TMap<FName, UGridItemDefinitionAsset*> TransientOld = {
		{SwordId, OldSword}, {ShieldId, OldShield}, {ArrowId, OldArrow}
	};
	TestTrue(TEXT("Matching old DAs remain optional for unconverted readers"),
		Fixture.Inventory->RehydrateOwnedItemDefinitions(
			[&TransientOld](FName Id) -> UGridItemDefinitionAsset*
			{
				return TransientOld.FindRef(Id);
			}, FailedId));
	TestEqual(TEXT("Optional old sword remains registered"),
		Fixture.Inventory->FindItemDefinition(SwordId), OldSword);

	// An UNKNOWN saved object must still have a matching definition. A
	// failure cannot discard a previously populated runtime definition map.
	const FName UnknownId(TEXT("ARMCLEAN063AT_UnknownSaveItem"));
	Fixture.Put(2, Fixture.Make(UnknownId, 0, 1));
	TestFalse(TEXT("Missing legacy definition blocks mixed rehydration"),
		Fixture.Inventory->RehydrateOwnedItemDefinitions(
			[&TransientOld](FName Id) -> UGridItemDefinitionAsset*
			{
				return TransientOld.FindRef(Id);
			}, FailedId));
	TestEqual(TEXT("Exact missing unknown ID reported"), FailedId, UnknownId);
	TestEqual(TEXT("Rollback preserves prior transient registry"),
		Fixture.Inventory->FindItemDefinition(SwordId), OldSword);

	UGridItemDefinitionAsset* OldUnknown = ARMCLEAN063AT::MakeLegacy(Fixture.Inventory, UnknownId);
	const TMap<FName, UGridItemDefinitionAsset*> AllOld = {
		{SwordId, OldSword}, {ShieldId, OldShield},
		{ArrowId, OldArrow}, {UnknownId, OldUnknown}
	};
	TestTrue(TEXT("Mixed legacy/canonical party rehydrates with known old definitions"),
		Fixture.Inventory->RehydrateOwnedItemDefinitions(
			[&AllOld](FName Id) -> UGridItemDefinitionAsset*
			{
				return AllOld.FindRef(Id);
			}, FailedId));
	TestEqual(TEXT("Unknown saved source registered"), Fixture.Inventory->FindItemDefinition(UnknownId), OldUnknown);

	// Invalid canonical levels and impossible N0 ARM must not borrow old
	// definitions or mutate the map. Restore the malformed item only after
	// each check, to avoid contaminating the following scenario.
	FGridInventorySlot& FirstSlot =
		Fixture.Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots[0];
	for (const int32 InvalidLevel : {0, 21})
	{
		FirstSlot.Item.ItemLevel = InvalidLevel;
		const FGuid OriginalGuid = FirstSlot.Item.RuntimeObjectId;
		TestFalse(TEXT("Saved ARM invalid level is rejected before resolver"),
			Fixture.Inventory->RehydrateOwnedItemDefinitions(
				[](FName) -> UGridItemDefinitionAsset* { return nullptr; }, FailedId));
		TestEqual(TEXT("Invalid registered item reported"), FailedId, SwordId);
		TestEqual(TEXT("Invalid saved level not rewritten"), FirstSlot.Item.ItemLevel, InvalidLevel);
		TestEqual(TEXT("Rejected item GUID preserved"), FirstSlot.Item.RuntimeObjectId, OriginalGuid);
		TestEqual(TEXT("Failed read keeps transient registry"), Fixture.Inventory->FindItemDefinition(SwordId), OldSword);
	}
	FirstSlot.Item = BeforeSword;

	// Stack overflow rejects rather than silently dropping quantity.
	FGridInventorySlot& ArrowSlot =
		Fixture.Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots[1];
	const FGridItemInstance BeforeArrow = ArrowSlot.Item;
	ArrowSlot.Item.Quantity = ArrowMeta.MaxStack + 1;
	TestFalse(TEXT("Impossible saved canonical stack quantity rejects atomically"),
		Fixture.Inventory->RehydrateOwnedItemDefinitions(
				[](FName) -> UGridItemDefinitionAsset* { return nullptr; }, FailedId));
	TestEqual(TEXT("Oversized canonical item reported"), FailedId, ArrowId);
	TestEqual(TEXT("Oversized stack is not clamped in-place"),
		ArrowSlot.Item.Quantity, ArrowMeta.MaxStack + 1);
	ArrowSlot.Item = BeforeArrow;

	// The unregistered item used by the mixed-save refusal must not remain
	// in the canonical-only scenario below. Otherwise its required legacy
	// definition correctly blocks the load, obscuring the hotbar check.
	Fixture.Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots[2] =
		FGridInventorySlot();

	// Hotbar-only canonical IDs are resolved without fabricated level N1.
	// Normal actions still check their actual runtime source after loading.
	Fixture.Inventory->PartyInventoryState.ActiveCharacters[0].CombatHotbarSlots[1].SlotIndex = 1;
	Fixture.Inventory->PartyInventoryState.ActiveCharacters[0].CombatHotbarSlots[1].ActionId = TEXT("AT_HotbarOnly");
	Fixture.Inventory->PartyInventoryState.ActiveCharacters[0].CombatHotbarSlots[1].SourcePolicy =
		EGridCombatActionSourcePolicy::Equipment;
	Fixture.Inventory->PartyInventoryState.ActiveCharacters[0].CombatHotbarSlots[1].SourceDefinitionId = SwordId;
	Fixture.Inventory->PartyInventoryState.ActiveCharacters[0].CombatHotbarSlots[1].PreferredSourceRuntimeId = FGuid::NewGuid();
	Fixture.Inventory->PartyInventoryState.ActiveCharacters[0].CombatHotbarSlots[1].PreferredEquipmentSlot =
		EGridEquipmentSlot::MainHand;
	TestTrue(TEXT("Hotbar-only registered identity does not require old DA"),
		Fixture.Inventory->RehydrateOwnedItemDefinitions(
			[](FName) -> UGridItemDefinitionAsset* { return nullptr; }, FailedId));

	return true;
}
#endif
