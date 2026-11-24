#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGStartingEquipmentService.h"
#include "Runtime/Items/GridItemCatalogues.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "UObject/StrongObjectPtr.h"

namespace ARMCLEAN064X24
{
	static FRPGStartingEquipmentEntry& Add(
		URPGClassAsset* Class, FName ItemId, int32 Level,
		EGridEquipmentSlot Slot = EGridEquipmentSlot::None, int32 Quantity = 1)
	{
		FRPGStartingEquipmentEntry& Entry =
			Class->StartingEquipmentKits[0].Items.AddDefaulted_GetRef();
		Entry.ItemId = ItemId;
		Entry.ItemLevel = Level;
		Entry.TargetSlot = Slot;
		Entry.Quantity = Quantity;
		return Entry;
	}

	static FGridCharacterInventoryState NewCharacter()
	{
		FGridCharacterInventoryState Character;
		Character.CharacterId = FGuid::NewGuid();
		Character.InventorySlots.SetNum(6);
		return Character;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean064X24PointerlessKits,
	"Grimrock.RPG.ARM.CLEAN06_4X24.PointerlessCanonicalStartingKit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean064X24PointerlessKits::RunTest(const FString&)
{
	TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(
		nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_ItemSystem.DA_ItemSystem")));
	if (!TestNotNull(TEXT("Real ItemSystem"), Root.Get())) { return false; }
	TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
	TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
	TArray<FString> Errors;
	if (!TestTrue(TEXT("Canonical register initialized"),
		Registry->RegisterItemSystem(Root.Get(), Errors)))
	{
		for (const FString& E : Errors) { AddError(E); }
		return false;
	}

	TStrongObjectPtr<URPGClassAsset> Class(NewObject<URPGClassAsset>());
	Class->ClassId = TEXT("Warrior");
	FRPGStartingEquipmentKit& Kit = Class->StartingEquipmentKits.AddDefaulted_GetRef();
	Kit.KitId = TEXT("X24_PointerlessKit");
	ARMCLEAN064X24::Add(Class.Get(), TEXT("Weapon_Dagger_01"), 20,
		EGridEquipmentSlot::MainHand);
	ARMCLEAN064X24::Add(Class.Get(), TEXT("Item_Torch"), 0);
	ARMCLEAN064X24::Add(Class.Get(), TEXT("Item_Arrows_01"), 0,
		EGridEquipmentSlot::None, 3);

	if (!TestTrue(TEXT("Class accepts explicitly canonical pointerless entries"),
			Class->IsValidDefinition()))
	{
		return false;
	}

	FGridCharacterInventoryState Character = ARMCLEAN064X24::NewCharacter();
	FGridCharacterEquipmentState Equipment;

	FString Error;
	if (!TestTrue(TEXT("Mixed N20 ARM / N0 non-ARM kit succeeds"),
		FRPGStartingEquipmentService::TryBuildKit(Class.Get(), Kit.KitId,
			Character, Equipment, Error, Registry.Get())))
	{
		AddError(Error);
		return false;
	}

	FGridResolvedItem Torch;
	if (!TestTrue(TEXT("Real torch resolves"), Registry->ResolveItem(TEXT("Item_Torch"), 0, Torch)) ||
		!TestEqual(TEXT("Chosen kit persisted"), Character.StartingEquipmentKitId, Kit.KitId) ||
		!TestEqual(TEXT("ARM in main hand"), Equipment.MainHand.ItemDefinitionId,
			FName(TEXT("Weapon_Dagger_01"))) ||
		!TestEqual(TEXT("ARM physical N20 kept"), Equipment.MainHand.ItemLevel, 20) ||
		!TestTrue(TEXT("ARM has stable instance GUID"), Equipment.MainHand.RuntimeObjectId.IsValid()) ||
		!TestFalse(TEXT("No off hand invented"), Equipment.OffHand.IsValid()) ||
		!TestEqual(TEXT("Torch in first bag slot"),
			Character.InventorySlots[0].Item.ItemDefinitionId, FName(TEXT("Item_Torch"))) ||
		!TestEqual(TEXT("Torch is N0"), Character.InventorySlots[0].Item.ItemLevel, 0) ||
		!TestEqual(TEXT("Torch light source is canonical"),
			Character.InventorySlots[0].Item.bLightsEnabled, Torch.LightEmitter.bDefaultEnabled) ||
		!TestEqual(TEXT("Arrows in second bag slot"),
			Character.InventorySlots[1].Item.ItemDefinitionId, FName(TEXT("Item_Arrows_01"))) ||
		!TestEqual(TEXT("Arrows quantity keeps authoring"), Character.InventorySlots[1].Item.Quantity, 3) ||
		!TestEqual(TEXT("Arrows remain N0"), Character.InventorySlots[1].Item.ItemLevel, 0) ||
		!TestTrue(TEXT("No duplicate in bag"),
			Character.InventorySlots[2].IsEmpty()))
	{
		return false;
	}

	// Invalid physical level of an explicitly authored ARM cannot become a
	// default N1. The whole transaction fails before granting a valid torch.
	Kit.Items[0].ItemLevel = 0;
	FGridCharacterInventoryState Rejected = ARMCLEAN064X24::NewCharacter();
	FGridCharacterEquipmentState RejectedGear;

	Error.Reset();
	if (!TestFalse(TEXT("Explicit ARM N0 is forbidden"),
		FRPGStartingEquipmentService::TryBuildKit(Class.Get(), Kit.KitId,
			Rejected, RejectedGear, Error, Registry.Get())) ||
		!TestTrue(TEXT("Reject leaves inventory empty"), Rejected.InventorySlots[0].IsEmpty()) ||
		!TestFalse(TEXT("Reject leaves MainHand empty"), RejectedGear.MainHand.IsValid()) ||
		!TestTrue(TEXT("Reject does not commit kit ID"), Rejected.StartingEquipmentKitId.IsNone()))
	{
		return false;
	}
	Kit.Items[0].ItemLevel = 20;

	// Registered non-ARM N1 is invalid. Even a later kit entry cannot
	// commit a partial set of otherwise valid items.
	Kit.Items[2].ItemLevel = 1;
	Rejected = ARMCLEAN064X24::NewCharacter();
	RejectedGear = FGridCharacterEquipmentState();
	Error.Reset();
	if (!TestFalse(TEXT("Explicit non-ARM N1 forbidden"),
		FRPGStartingEquipmentService::TryBuildKit(Class.Get(), Kit.KitId,
			Rejected, RejectedGear, Error, Registry.Get())) ||
		!TestTrue(TEXT("No partial inventory on late invalid item"), Rejected.InventorySlots[0].IsEmpty()) ||
		!TestFalse(TEXT("No partial equipment on late invalid item"), RejectedGear.MainHand.IsValid()))
	{
		return false;
	}
	Kit.Items[2].ItemLevel = 0;

	// Staging valid entries must not allow an unknown identity to mutate the kit owner.
	FRPGStartingEquipmentEntry& UnknownEntry = Kit.Items.AddDefaulted_GetRef();
	UnknownEntry.ItemId = TEXT("X24_UnregisteredFixture");
	UnknownEntry.Quantity = 1;
	Rejected = ARMCLEAN064X24::NewCharacter();
	RejectedGear = FGridCharacterEquipmentState();
	if (!TestFalse(TEXT("Configured registry rejects historical unregistered item"),
		FRPGStartingEquipmentService::TryBuildKit(Class.Get(), Kit.KitId,
			Rejected, RejectedGear, Error, Registry.Get())) ||
		!TestTrue(TEXT("Unknown item refusal is atomic"),
			Rejected.InventorySlots[0].IsEmpty() && !RejectedGear.MainHand.IsValid() &&
			Rejected.StartingEquipmentKitId.IsNone()))
	{
		return false;
	}
	Kit.Items.Pop();
	TStrongObjectPtr<UGridItemSubsystem> Unconfigured(NewObject<UGridItemSubsystem>(GI.Get()));
	if (!TestFalse(TEXT("Present but unconfigured registry cannot build a kit"),
		FRPGStartingEquipmentService::TryBuildKit(Class.Get(), Kit.KitId,
			Rejected, RejectedGear, Error, Unconfigured.Get())) ||
		!TestTrue(TEXT("Unconfigured registry refusal is atomic"),
			Rejected.InventorySlots[0].IsEmpty() && !RejectedGear.MainHand.IsValid() &&
			Rejected.StartingEquipmentKitId.IsNone()))
	{
		return false;
	}


	// WeaponCatalogue hand policy remains authoritative without an old
	// UGridItemDefinitionAsset (offhand-only shield and two-handed bow).
	TStrongObjectPtr<URPGClassAsset> Hands(NewObject<URPGClassAsset>());
	Hands->ClassId = TEXT("Warrior");
	FRPGStartingEquipmentKit& HandKit = Hands->StartingEquipmentKits.AddDefaulted_GetRef();
	HandKit.KitId = TEXT("X24_CanonicalHands");
	ARMCLEAN064X24::Add(Hands.Get(), TEXT("Weapon_ShortBow_01"), 6,
		EGridEquipmentSlot::OffHand);
	FGridCharacterInventoryState BowCharacter = ARMCLEAN064X24::NewCharacter();
	FGridCharacterEquipmentState BowEquipment;

	Error.Reset();
	if (!TestTrue(TEXT("Pointerless two-handed bow equips"),
		FRPGStartingEquipmentService::TryBuildKit(Hands.Get(), HandKit.KitId,
			BowCharacter, BowEquipment, Error, Registry.Get())) ||
		!TestEqual(TEXT("Two-handed bow stored in physical MainHand"),
			BowEquipment.MainHand.ItemDefinitionId, FName(TEXT("Weapon_ShortBow_01"))) ||
		!TestFalse(TEXT("OffHand remains virtual"), BowEquipment.OffHand.IsValid()))
	{
		AddError(Error);
		return false;
	}

	ARMCLEAN064X24::Add(Hands.Get(), TEXT("Weapon_RoundShield_01"), 7,
		EGridEquipmentSlot::OffHand);
	BowCharacter = ARMCLEAN064X24::NewCharacter();
	BowEquipment = FGridCharacterEquipmentState();

	Error.Reset();
	if (!TestFalse(TEXT("Shield cannot occupy bow reserved hand"),
		FRPGStartingEquipmentService::TryBuildKit(Hands.Get(), HandKit.KitId,
			BowCharacter, BowEquipment, Error, Registry.Get())) ||
		!TestFalse(TEXT("Two-handed conflict has no committed MainHand"),
			BowEquipment.MainHand.IsValid()) ||
		!TestTrue(TEXT("Two-handed conflict has no committed kit"),
			BowCharacter.StartingEquipmentKitId.IsNone()))
	{
		return false;
	}

	AddInfo(TEXT("[ARM-CLEAN06.4X.24] pointerless ARM N20, non-ARM N0, "
		"stacking, both-hand policy, invalid levels and transactional failures."));
	return true;
}

// X.43: explicitly canonical starting-kit entries may still carry old
// pointers in authored class data; these must NEVER override MaxStack,
// hand policy, light state or physical N-level of DA_ItemSystem.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean064X43CanonicalStarterAuthority,
    "Grimrock.RPG.ARM.CLEAN06_4X43.CanonicalStarterIgnoresOldItemMetadata",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean064X43CanonicalStarterAuthority::RunTest(const FString&)
{
    TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(
        nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_ItemSystem.DA_ItemSystem")));
    if (!TestNotNull(TEXT("Real item system"), Root.Get())) return false;
    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
    TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
    TArray<FString> Errors;
    if (!TestTrue(TEXT("Canonical registry is configured"),
        Registry->RegisterItemSystem(Root.Get(), Errors)))
    {
        for (const FString& E : Errors) AddError(E);
        return false;
    }

    FGridResolvedItem Sword, Arrows, Torch;
    if (!TestTrue(TEXT("Real sword N20"), Registry->ResolveItem(TEXT("Weapon_StraightSword_01"), 20, Sword)) ||
        !TestTrue(TEXT("Real arrows N0"), Registry->ResolveItem(TEXT("Item_Arrows_01"), 0, Arrows)) ||
        !TestTrue(TEXT("Real torch N0"), Registry->ResolveItem(TEXT("Item_Torch"), 0, Torch)) ||
        !TestTrue(TEXT("Canonical arrows are stackable"), Arrows.MaxStack >= 3) ||
        !TestTrue(TEXT("Canonical torch is equippable offhand"),
            Torch.NonArmEquipmentSlots.Contains(EGridEquipmentSlot::OffHand)))
    {
        return false;
    }

    TStrongObjectPtr<URPGClassAsset> Class(NewObject<URPGClassAsset>());
    Class->ClassId = TEXT("Warrior");
    FRPGStartingEquipmentKit& Kit = Class->StartingEquipmentKits.AddDefaulted_GetRef();
    Kit.KitId = TEXT("X43_MixedExplicitNative");

    ARMCLEAN064X24::Add(Class.Get(), Sword.ItemId, 20);
    ARMCLEAN064X24::Add(Class.Get(), Arrows.ItemId, 0, EGridEquipmentSlot::None, 3);
    ARMCLEAN064X24::Add(Class.Get(), Torch.ItemId, 0, EGridEquipmentSlot::OffHand);

    FGridCharacterInventoryState Character = ARMCLEAN064X24::NewCharacter();
    FGridCharacterEquipmentState Equipment;

    FString Error;
    if (!TestTrue(TEXT("All three explicitly native entries ignore stale source metadata"),
        FRPGStartingEquipmentService::TryBuildKit(Class.Get(), Kit.KitId,
            Character, Equipment, Error, Registry.Get())))
    {
        AddError(Error);
        return false;
    }

    return TestEqual(TEXT("Canonical sword N20 level retained"),
            Character.InventorySlots[0].Item.ItemLevel, 20) &&
        TestEqual(TEXT("Canonical sword light not overwritten by old definition"),
            Character.InventorySlots[0].Item.bLightsEnabled,
            Sword.LightEmitter.bDefaultEnabled) &&
        TestEqual(TEXT("Canonical ammo stack granted as authored"),
            Character.InventorySlots[1].Item.Quantity, 3) &&
        TestEqual(TEXT("Canonical torch occupies actual offhand"),
            Equipment.OffHand.ItemDefinitionId, Torch.ItemId) &&
        TestEqual(TEXT("Canonical torch light remains canonical"),
            Equipment.OffHand.bLightsEnabled, Torch.LightEmitter.bDefaultEnabled) &&
        TestEqual(TEXT("Kit ID committed only once, after full success"),
            Character.StartingEquipmentKitId, Kit.KitId) &&
        !HasAnyErrors();
}

#endif
