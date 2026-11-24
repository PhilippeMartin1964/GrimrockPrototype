#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "../UI/GridInventoryItemMetadataReadModel.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Runtime/Combat/GridCombatTraceTypes.h"
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
			return Registry && Registry->ContainsItem(TEXT("Weapon_StraightSword_01")) &&
				Registry->ContainsItem(TEXT("Item_Arrows_01")) &&
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
	TestTrue(TEXT("Canonical-only party rehydrates from the active registry"),
		Fixture.Inventory->RehydrateOwnedItems(FailedId));
	TestTrue(TEXT("No failure identity"), FailedId.IsNone());
	TestEqual(TEXT("Saved GUID preserved"), Fixture.Inventory->
		PartyInventoryState.ActiveCharacters[0].InventorySlots[0].Item.RuntimeObjectId,
		BeforeSword.RuntimeObjectId);
	TestEqual(TEXT("Saved N20 preserved"), Fixture.Inventory->
		PartyInventoryState.ActiveCharacters[0].InventorySlots[0].Item.ItemLevel, 20);
	const FGridItemInstance& RehydratedSword = Fixture.Inventory->
		PartyInventoryState.ActiveCharacters[0].InventorySlots[0].Item;
	TestTrue(TEXT("All durable item fields preserved on rehydrate"),
		FGridItemInstance::StaticStruct()->CompareScriptStruct(&RehydratedSword, &BeforeSword, 0));
	FGridResolvedItem ExpectedSword;
	if (!TestTrue(TEXT("Canonical N20 sword metadata resolves"),
		Fixture.Registry->ResolveItem(SwordId, 20, ExpectedSword))) return false;
	const auto SwordMetadata = FGridInventoryItemMetadataReadModel::Resolve(Fixture.Registry, RehydratedSword);
	TestTrue(TEXT("Rehydrated name and mass come from canonical definition"), SwordMetadata.bResolved &&
		SwordMetadata.DisplayName.EqualTo(ExpectedSword.DisplayName) &&
		FMath::IsNearlyEqual(SwordMetadata.UnitWeightKg, ExpectedSword.WeightKg));
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

	// A real combat armor trace must preserve the authored localized name,
	// not the internal ID or a historical definition label.
	FGridResolvedItem ShieldMeta;
	if (!TestTrue(TEXT("Canonical shield metadata"), Fixture.Registry->ResolveItem(ShieldId, 1, ShieldMeta))) return false;
	TArray<FGridArmorAbsorptionSourceTrace> Sources;
	Fixture.Inventory->ApplyPhysicalArmorDamage(0, 0, &Sources);
	const auto* ShieldTrace = Sources.FindByPredicate([ShieldId](const FGridArmorAbsorptionSourceTrace& Trace)
	{
		return Trace.bEquipment && Trace.ItemDefinitionId == ShieldId;
	});
	if (!TestNotNull(TEXT("Pointerless shield armor trace exists"), ShieldTrace)) return false;
	TestTrue(TEXT("Trace uses canonical authored shield name"), ShieldTrace->DisplayName.EqualTo(ShieldMeta.DisplayName));

	// A missing definition cannot be recovered from item data. The entire
	// draft roster is stable when validation fails.
	const FName UnknownId(TEXT("ARMCLEAN063AT_UnknownSaveItem"));
	Fixture.Put(2, Fixture.Make(UnknownId, 0, 1));
	const FGridPartyInventoryState BeforeFailure = Fixture.Inventory->PartyInventoryState;
	AddExpectedError(TEXT("Canonical party rehydrate rejected Item=ARMCLEAN063AT_UnknownSaveItem"),
		EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("Unknown saved identity blocks rehydration"),
		Fixture.Inventory->RehydrateOwnedItems(FailedId));
	TestEqual(TEXT("Exact missing unknown ID reported"), FailedId, UnknownId);
	TestTrue(TEXT("Rejection leaves the entire draft roster unchanged"),
		FGridPartyInventoryState::StaticStruct()->CompareScriptStruct(
			&Fixture.Inventory->PartyInventoryState, &BeforeFailure, 0));
	Fixture.Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots[2] = FGridInventorySlot();

	FGridInventorySlot& FirstSlot =
		Fixture.Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots[0];
	for (const int32 InvalidLevel : {0, 21})
	{
		FirstSlot.Item.ItemLevel = InvalidLevel;
		const FGuid OriginalGuid = FirstSlot.Item.RuntimeObjectId;
		TestFalse(TEXT("Saved ARM invalid level is rejected"),
			Fixture.Inventory->RehydrateOwnedItems(FailedId));
		TestEqual(TEXT("Invalid registered item reported"), FailedId, SwordId);
		TestEqual(TEXT("Invalid saved level not rewritten"), FirstSlot.Item.ItemLevel, InvalidLevel);
		TestEqual(TEXT("Rejected item GUID preserved"), FirstSlot.Item.RuntimeObjectId, OriginalGuid);
	}
	FirstSlot.Item = BeforeSword;

	// Stack overflow rejects rather than silently dropping quantity.
	FGridInventorySlot& ArrowSlot =
		Fixture.Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots[1];
	const FGridItemInstance BeforeArrow = ArrowSlot.Item;
	ArrowSlot.Item.Quantity = ArrowMeta.MaxStack + 1;
	TestFalse(TEXT("Impossible saved canonical stack quantity rejects atomically"),
		Fixture.Inventory->RehydrateOwnedItems(FailedId));
	TestEqual(TEXT("Oversized canonical item reported"), FailedId, ArrowId);
	TestEqual(TEXT("Oversized stack is not clamped in-place"),
		ArrowSlot.Item.Quantity, ArrowMeta.MaxStack + 1);
	ArrowSlot.Item = BeforeArrow;
	ArrowSlot.Item.ItemLevel = 1;
	TestFalse(TEXT("Physical non-ARM N1 must not use a family-level alias"),
		Fixture.Inventory->RehydrateOwnedItems(FailedId));
	TestEqual(TEXT("Physical non-ARM N1 reports exact saved identity"), FailedId, ArrowId);
	TestEqual(TEXT("Invalid non-ARM level not rewritten"), ArrowSlot.Item.ItemLevel, 1);
	TestEqual(TEXT("Physical N1 rejection preserves GUID"), ArrowSlot.Item.RuntimeObjectId, BeforeArrow.RuntimeObjectId);
	ArrowSlot.Item = BeforeArrow;

	// Hotbar-only canonical IDs are resolved without fabricated level N1.
	// Normal actions still check their actual runtime source after loading.
	Fixture.Inventory->PartyInventoryState.ActiveCharacters[0].CombatHotbarSlots[1].SlotIndex = 1;
	Fixture.Inventory->PartyInventoryState.ActiveCharacters[0].CombatHotbarSlots[1].ActionId = TEXT("AT_HotbarOnly");
	Fixture.Inventory->PartyInventoryState.ActiveCharacters[0].CombatHotbarSlots[1].SourcePolicy =
		EGridCombatActionSourcePolicy::Equipment;
	Fixture.Inventory->PartyInventoryState.ActiveCharacters[0].CombatHotbarSlots[1].SourceDefinitionId = TEXT("Weapon_Dagger_01");
	Fixture.Inventory->PartyInventoryState.ActiveCharacters[0].CombatHotbarSlots[1].PreferredSourceRuntimeId = FGuid::NewGuid();
	Fixture.Inventory->PartyInventoryState.ActiveCharacters[0].CombatHotbarSlots[1].PreferredEquipmentSlot =
		EGridEquipmentSlot::MainHand;
	TestTrue(TEXT("Hotbar-only registered identity does not require a physical instance"),
		Fixture.Inventory->RehydrateOwnedItems(FailedId));
	TestTrue(TEXT("The shortcut has no missing definition"), FailedId.IsNone());

	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean063ATStrictPartyRehydration,
	"Grimrock.RPG.ARM.CLEAN06_5G32.StrictPartyRehydration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean063ATStrictPartyRehydration::RunTest(const FString&)
{
	ARMCLEAN063AT::FFixture F;
	if (!TestTrue(TEXT("Configured canonical inventory"), F.Ready())) return false;
	const FName Sword(TEXT("Weapon_StraightSword_01"));
	const FName Unknown(TEXT("ARM_CLEAN06_5G32_Unknown"));
	FName Missing = NAME_None;
	F.Put(0, F.Make(Sword, 20, 1));
	TestTrue(TEXT("Valid canonical party rehydrates"),
		F.Inventory->RehydrateOwnedItems(Missing));
	const FGuid StableGuid = F.Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots[0].Item.RuntimeObjectId;
	F.Put(1, F.Make(Unknown, 0, 1));
	const FGridPartyInventoryState BeforeUnknown = F.Inventory->PartyInventoryState;
	TestFalse(TEXT("Unregistered saved identity is refused"),
		F.Inventory->RehydrateOwnedItems(Missing));
	TestEqual(TEXT("Unknown identity reported"), Missing, Unknown);
	TestTrue(TEXT("Failed restore leaves the draft roster unchanged"),
		FGridPartyInventoryState::StaticStruct()->CompareScriptStruct(
			&BeforeUnknown, &F.Inventory->PartyInventoryState, 0));
	F.Put(1, F.Make(Unknown, 0, 0));
	TestFalse(TEXT("Malformed occupied unknown object cannot be skipped"),
		F.Inventory->RehydrateOwnedItems(Missing));
	TestEqual(TEXT("Malformed identity reported"), Missing, Unknown);
	F.Put(1, F.Make(NAME_None, 0, 1));
	TestFalse(TEXT("Occupied slot without identity cannot be skipped"),
		F.Inventory->RehydrateOwnedItems(Missing));
	F.Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots[1] = FGridInventorySlot();
	F.Inventory->PartyInventoryState.ActiveEquipment[0].OffHand = F.Make(Unknown, 1, 0);
	TestFalse(TEXT("Malformed equipped item cannot be skipped"),
		F.Inventory->RehydrateOwnedItems(Missing));
	F.Inventory->PartyInventoryState.ActiveEquipment[0].OffHand = FGridItemInstance();
	F.Inventory->PartyInventoryState.bHasCursorItem = true;
	F.Inventory->PartyInventoryState.CursorItem = F.Make(Unknown, 0, 0, EGridItemOwnerType::Cursor);
	TestFalse(TEXT("Malformed cursor cannot be skipped"),
		F.Inventory->RehydrateOwnedItems(Missing));
	F.Inventory->PartyInventoryState.bHasCursorItem = false;
	F.Inventory->PartyInventoryState.CursorItem = FGridItemInstance();
	auto& Hotbar = F.Inventory->PartyInventoryState.ActiveCharacters[0].CombatHotbarSlots;
	FGridCombatHotbarBinding Shortcut;
	Shortcut.SlotIndex = 1;
	Shortcut.ActionId = TEXT("5G36_HotbarOnly");
	Shortcut.SourcePolicy = EGridCombatActionSourcePolicy::Equipment;
	Shortcut.SourceDefinitionId = Unknown;
	Shortcut.PreferredSourceRuntimeId = FGuid::NewGuid();
	Shortcut.PreferredEquipmentSlot = EGridEquipmentSlot::MainHand;
	Hotbar[1] = Shortcut;
	TestFalse(TEXT("Unregistered shortcut identity is refused"),
		F.Inventory->RehydrateOwnedItems(Missing));
	TestEqual(TEXT("Unknown shortcut reports exact identity"), Missing, Unknown);
	Hotbar[1].SourceDefinitionId = TEXT("Weapon_Dagger_01");
	TestTrue(TEXT("Canonical-only state succeeds once corrected"),
		F.Inventory->RehydrateOwnedItems(Missing));
	TestTrue(TEXT("Shortcut-only identity does not fabricate a physical source"), Missing.IsNone());
	TestEqual(TEXT("Shortcut source GUID preserved"), Hotbar[1].PreferredSourceRuntimeId, Shortcut.PreferredSourceRuntimeId);
	TestEqual(TEXT("Canonical item identity remains stable"),
		F.Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots[0].Item.RuntimeObjectId, StableGuid);
	return !HasAnyErrors();
}

#endif
