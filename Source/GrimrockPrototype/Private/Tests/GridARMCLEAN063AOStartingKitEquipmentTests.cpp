#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGStartingEquipmentService.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "../RPG/GridStartingKitEquipmentPolicyReadModel.h"
#include "UObject/StrongObjectPtr.h"

namespace ARMCLEAN063AO
{
	static FGridCharacterInventoryState NewCharacter()
	{
		FGridCharacterInventoryState Character;
		Character.CharacterId = FGuid::NewGuid();
		Character.InventorySlots.SetNum(8);
		return Character;
	}

	static void Add(URPGClassAsset* Class, FName ItemId, int32 ItemLevel,
		EGridEquipmentSlot Slot)
	{
		FRPGStartingEquipmentEntry& Entry = Class->StartingEquipmentKits[0].Items.AddDefaulted_GetRef();
		Entry.ItemId = ItemId;
		Entry.ItemLevel = ItemLevel;
		Entry.Quantity = 1;
		Entry.TargetSlot = Slot;
	}

	static URPGClassAsset* NewClass()
	{
		URPGClassAsset* Class = NewObject<URPGClassAsset>();
		FRPGStartingEquipmentKit& Kit = Class->StartingEquipmentKits.AddDefaulted_GetRef();
		Kit.KitId = TEXT("StartingHands_AO");
		return Class;
	}

	static bool TryGrant(URPGClassAsset* Class, const UGridItemSubsystem* Registry,
		FGridCharacterInventoryState& Character, FGridCharacterEquipmentState& Equipment,
		FString& Error)
	{
		return FRPGStartingEquipmentService::TryBuildKit(
			Class, TEXT("StartingHands_AO"), Character, Equipment,
			Error, Registry);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean063AOStartingKitHands,
	"Grimrock.RPG.ARM.CLEAN06_3.CanonicalStartingKitEquipmentHands",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean063AOStartingKitHands::RunTest(const FString&)
{
	TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_ItemSystem.DA_ItemSystem")));
	if (!TestNotNull(TEXT("Saved production ItemSystem"), Root.Get())) return false;
	TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
	TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
	TArray<FString> Errors;
	if (!TestTrue(TEXT("Register actual ItemSystem"), Registry->RegisterItemSystem(Root.Get(), Errors)))
	{
		for (const FString& Error : Errors) AddError(Error);
		return false;
	}

	const FName Bow(TEXT("Weapon_ShortBow_01"));
	const FName Shield(TEXT("Weapon_RoundShield_01"));
	const FName Sword(TEXT("Weapon_StraightSword_01"));

	FGridResolvedItem RealBow, RealShield, RealSword;
	if (!TestTrue(TEXT("Real ARM045 N6"), Registry->ResolveItem(Bow, 6, RealBow)) ||
		!TestTrue(TEXT("Real ARM075 N7"), Registry->ResolveItem(Shield, 7, RealShield)) ||
		!TestTrue(TEXT("Real ARM014 N4"), Registry->ResolveItem(Sword, 4, RealSword)))
	{
		return false;
	}
	TestEqual(TEXT("Actual bow is two handed"), RealBow.Weapon.HandPolicy, EGridHandPolicy::TwoHanded);
	TestEqual(TEXT("Actual shield is offhand only"), RealShield.Weapon.HandPolicy, EGridHandPolicy::OffHand);
	TestTrue(TEXT("Actual sword equips MainHand"), RealSword.Weapon.CanEquipInHand(EGridEquipmentSlot::MainHand));

	const FGridStartingKitEquipmentPolicyReadView BowPolicy =
		FGridStartingKitEquipmentPolicyReadModel::Resolve(
			Registry.Get(), Bow, 6, EGridEquipmentSlot::OffHand);
	TestTrue(TEXT("Canonical bow permitted from authored OffHand"), BowPolicy.bRegisteredIdentity && BowPolicy.bResolved);
	TestTrue(TEXT("Canonical bow reserves both hands"), BowPolicy.bTwoHanded);
	TestEqual(TEXT("Two-handed bow physical storage is MainHand"),
		BowPolicy.PhysicalSlot, EGridEquipmentSlot::MainHand);
	const auto ShieldPolicy = FGridStartingKitEquipmentPolicyReadModel::Resolve(
		Registry.Get(), Shield, 7, EGridEquipmentSlot::OffHand);
	TestTrue(TEXT("Shield OffHand does not reserve both hands"),
		ShieldPolicy.bResolved && !ShieldPolicy.bTwoHanded);
	TestEqual(TEXT("Shield keeps its own OffHand slot"), ShieldPolicy.PhysicalSlot, EGridEquipmentSlot::OffHand);
	TestFalse(TEXT("Shield MainHand rejected by actual policy"),
		FGridStartingKitEquipmentPolicyReadModel::Resolve(
			Registry.Get(), Shield, 7, EGridEquipmentSlot::MainHand).bResolved);
	TestFalse(TEXT("Known ARM level N0 cannot fall back to old hand data"),
		FGridStartingKitEquipmentPolicyReadModel::Resolve(
			Registry.Get(), Bow, 0, EGridEquipmentSlot::OffHand).bResolved);

	// Build an actual kit, ensuring the physical instance and stored fixed
	// level are exactly what the existing combat/equipment runtime expects.
	TStrongObjectPtr<URPGClassAsset> BowClass(ARMCLEAN063AO::NewClass());
	ARMCLEAN063AO::Add(BowClass.Get(), Bow, 6, EGridEquipmentSlot::OffHand);
	FGridCharacterInventoryState BowCharacter = ARMCLEAN063AO::NewCharacter();
	FGridCharacterEquipmentState BowEquipment;

	FString Error;
	if (!TestTrue(TEXT("Bow kit grants using canonical hands"),
		ARMCLEAN063AO::TryGrant(BowClass.Get(), Registry.Get(), BowCharacter, BowEquipment, Error)))
	{
		AddError(Error);
		return false;
	}
	TestTrue(TEXT("Physical bow stored in MainHand"), BowEquipment.MainHand.IsValid());
	TestFalse(TEXT("No duplicate object in OffHand"), BowEquipment.OffHand.IsValid());
	TestEqual(TEXT("Physical bow has real N6 level"), BowEquipment.MainHand.ItemLevel, 6);

	// The second item cannot steal the virtual OffHand of a two-handed bow.
	ARMCLEAN063AO::Add(BowClass.Get(), Shield, 7, EGridEquipmentSlot::OffHand);
	FGridCharacterInventoryState FailedCharacter = ARMCLEAN063AO::NewCharacter();
	FGridCharacterEquipmentState FailedEquipment;
	const FGuid OriginalCharacterId = FailedCharacter.CharacterId;

	Error.Reset();
	TestFalse(TEXT("Shield plus two handed bow rejects kit atomically"),
		ARMCLEAN063AO::TryGrant(BowClass.Get(), Registry.Get(), FailedCharacter, FailedEquipment, Error));
	TestFalse(TEXT("Two-hand conflict reports an error"), Error.IsEmpty());
	TestFalse(TEXT("Failed kit reserves no physical MainHand"), FailedEquipment.MainHand.IsValid());
	TestFalse(TEXT("Failed kit reserves no physical OffHand"), FailedEquipment.OffHand.IsValid());
	TestTrue(TEXT("Failed kit adds no bag items"), FailedCharacter.InventorySlots[0].IsEmpty());
	TestEqual(TEXT("Failed kit leaves character GUID intact"), FailedCharacter.CharacterId, OriginalCharacterId);
	TestTrue(TEXT("Failed kit ID is not recorded"), FailedCharacter.StartingEquipmentKitId.IsNone());

	// Reverse order is also transactional: a shield already in OffHand
	// must prevent an afterwards authored two-handed bow.
	TStrongObjectPtr<URPGClassAsset> Reverse(ARMCLEAN063AO::NewClass());
	ARMCLEAN063AO::Add(Reverse.Get(), Shield, 7, EGridEquipmentSlot::OffHand);
	ARMCLEAN063AO::Add(Reverse.Get(), Bow, 6, EGridEquipmentSlot::MainHand);

	Error.Reset();
	TestFalse(TEXT("Two-handed bow cannot follow equipped shield"),
		ARMCLEAN063AO::TryGrant(Reverse.Get(), Registry.Get(), FailedCharacter, FailedEquipment, Error));
	TestTrue(TEXT("Reverse conflict leaves untouched OffHand"), !FailedEquipment.OffHand.IsValid());

	// The obsolete two-handed flag on a shield must not hide the player's
	// MainHand: the sword and shield remain two separate physical objects.
	TStrongObjectPtr<URPGClassAsset> Normal(ARMCLEAN063AO::NewClass());
	ARMCLEAN063AO::Add(Normal.Get(), Shield, 7, EGridEquipmentSlot::OffHand);
	ARMCLEAN063AO::Add(Normal.Get(), Sword, 4, EGridEquipmentSlot::MainHand);
	FGridCharacterInventoryState NormalCharacter = ARMCLEAN063AO::NewCharacter();
	FGridCharacterEquipmentState NormalEquipment;

	Error.Reset();
	if (!TestTrue(TEXT("Canonical sword and shield coexist"),
		ARMCLEAN063AO::TryGrant(Normal.Get(), Registry.Get(), NormalCharacter, NormalEquipment, Error)))
	{
		AddError(Error);
		return false;
	}
	TestTrue(TEXT("Sword physically in MainHand"), NormalEquipment.MainHand.IsValid());
	TestTrue(TEXT("Shield physically in OffHand"), NormalEquipment.OffHand.IsValid());
	TestEqual(TEXT("Sword fixed N4"), NormalEquipment.MainHand.ItemLevel, 4);
	TestEqual(TEXT("Shield fixed N7"), NormalEquipment.OffHand.ItemLevel, 7);
	TestTrue(TEXT("Distinct physical GUIDs"),
		NormalEquipment.MainHand.RuntimeObjectId != NormalEquipment.OffHand.RuntimeObjectId);

	const FName Unknown(TEXT("LegacyStarter_AO"));
	const auto OldPolicy = FGridStartingKitEquipmentPolicyReadModel::Resolve(
		Registry.Get(), Unknown, 0, EGridEquipmentSlot::MainHand);
	TestFalse(TEXT("Unknown entry remains outside canonical registry"), OldPolicy.bRegisteredIdentity);
	TestFalse(TEXT("Unknown entry cannot equip in configured world"), OldPolicy.bResolved);
	const auto FixturePolicy = FGridStartingKitEquipmentPolicyReadModel::Resolve(
		nullptr, Unknown, 0, EGridEquipmentSlot::MainHand);
	TestFalse(TEXT("Registry-less entry cannot select an equipment slot"), FixturePolicy.bResolved);
	return true;
}
#endif
