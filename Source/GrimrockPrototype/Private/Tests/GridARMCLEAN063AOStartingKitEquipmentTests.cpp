#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGStartingEquipmentService.h"
#include "Runtime/GridItemDefinitionAsset.h"
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

	static void Add(URPGClassAsset* Class, UGridItemDefinitionAsset* Definition,
		EGridEquipmentSlot Slot)
	{
		FRPGStartingEquipmentEntry& Entry = Class->StartingEquipmentKits[0].Items.AddDefaulted_GetRef();
		Entry.ItemDefinition = Definition;
		Entry.ItemLevel = 0;
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
		TArray<UGridItemDefinitionAsset*>& Definitions, FString& Error)
	{
		return FRPGStartingEquipmentService::TryBuildKit(
			Class, TEXT("StartingHands_AO"), Character, Equipment,
			Definitions, Error, Registry);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean063AOStartingKitHands,
	"Grimrock.RPG.ARM.CLEAN06_3.CanonicalStartingKitEquipmentHands",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean063AOStartingKitHands::RunTest(const FString&)
{
	TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem")));
	if (!TestNotNull(TEXT("Saved production ItemSystem"), Root.Get())) return false;
	TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
	TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
	TArray<FString> Errors;
	if (!TestTrue(TEXT("Register actual ItemSystem"), Registry->RegisterItemSystem(Root.Get(), Errors)))
	{
		for (const FString& Error : Errors) AddError(Error);
		return false;
	}

	// Intentionally contradictory obsolete assets: a real bow claims it is
	// not handheld, while a real shield claims it needs both hands.
	TStrongObjectPtr<UGridItemDefinitionAsset> Bow(NewObject<UGridItemDefinitionAsset>());
	Bow->ItemDefinitionId = TEXT("Weapon_ShortBow_01");
	Bow->DefaultWeaponItemLevel = 6;
	Bow->HandUsage = EGridItemHandUsage::NotHandHeld;
	Bow->bUseWeaponCatalogue = false;
	TStrongObjectPtr<UGridItemDefinitionAsset> Shield(NewObject<UGridItemDefinitionAsset>());
	Shield->ItemDefinitionId = TEXT("Weapon_RoundShield_01");
	Shield->DefaultWeaponItemLevel = 7;
	Shield->HandUsage = EGridItemHandUsage::TwoHanded;
	Shield->bUseWeaponCatalogue = false;
	TStrongObjectPtr<UGridItemDefinitionAsset> Sword(NewObject<UGridItemDefinitionAsset>());
	Sword->ItemDefinitionId = TEXT("Weapon_StraightSword_01");
	Sword->DefaultWeaponItemLevel = 4;
	Sword->HandUsage = EGridItemHandUsage::TwoHanded;
	Sword->bUseWeaponCatalogue = false;

	FGridResolvedItem RealBow, RealShield, RealSword;
	if (!TestTrue(TEXT("Real ARM045 N6"), Registry->ResolveItem(Bow->ItemDefinitionId, 6, RealBow)) ||
		!TestTrue(TEXT("Real ARM075 N7"), Registry->ResolveItem(Shield->ItemDefinitionId, 7, RealShield)) ||
		!TestTrue(TEXT("Real ARM014 N4"), Registry->ResolveItem(Sword->ItemDefinitionId, 4, RealSword)))
	{
		return false;
	}
	TestEqual(TEXT("Actual bow is two handed"), RealBow.Weapon.HandPolicy, EGridCanonicalHandPolicy::TwoHanded);
	TestEqual(TEXT("Actual shield is offhand only"), RealShield.Weapon.HandPolicy, EGridCanonicalHandPolicy::OffHand);
	TestTrue(TEXT("Actual sword equips MainHand"), RealSword.Weapon.CanEquipInHand(EGridEquipmentSlot::MainHand));
	TestTrue(TEXT("Old bow would incorrectly reject both hands"), !Bow->CanEquipToSlot(EGridEquipmentSlot::OffHand));
	TestTrue(TEXT("Old shield would incorrectly reserve two hands"),
		Shield->GetEffectiveHandUsage() == EGridItemHandUsage::TwoHanded);

	const FGridStartingKitEquipmentPolicyReadView BowPolicy =
		FGridStartingKitEquipmentPolicyReadModel::Resolve(
			Registry.Get(), Bow.Get(), 6, EGridEquipmentSlot::OffHand);
	TestTrue(TEXT("Canonical bow permitted from authored OffHand"), BowPolicy.bCanonicalIdentity && BowPolicy.bResolved);
	TestTrue(TEXT("Canonical bow reserves both hands"), BowPolicy.bTwoHanded);
	TestEqual(TEXT("Two-handed bow physical storage is MainHand"),
		BowPolicy.PhysicalSlot, EGridEquipmentSlot::MainHand);
	const auto ShieldPolicy = FGridStartingKitEquipmentPolicyReadModel::Resolve(
		Registry.Get(), Shield.Get(), 7, EGridEquipmentSlot::OffHand);
	TestTrue(TEXT("Shield OffHand accepted despite historical TwoHanded"),
		ShieldPolicy.bResolved && !ShieldPolicy.bTwoHanded);
	TestEqual(TEXT("Shield keeps its own OffHand slot"), ShieldPolicy.PhysicalSlot, EGridEquipmentSlot::OffHand);
	TestFalse(TEXT("Shield MainHand rejected by actual policy"),
		FGridStartingKitEquipmentPolicyReadModel::Resolve(
			Registry.Get(), Shield.Get(), 7, EGridEquipmentSlot::MainHand).bResolved);
	TestFalse(TEXT("Known ARM level N0 cannot fall back to old hand data"),
		FGridStartingKitEquipmentPolicyReadModel::Resolve(
			Registry.Get(), Bow.Get(), 0, EGridEquipmentSlot::OffHand).bResolved);

	// Build an actual kit, ensuring the physical instance and stored fixed
	// level are exactly what the existing combat/equipment runtime expects.
	TStrongObjectPtr<URPGClassAsset> BowClass(ARMCLEAN063AO::NewClass());
	ARMCLEAN063AO::Add(BowClass.Get(), Bow.Get(), EGridEquipmentSlot::OffHand);
	FGridCharacterInventoryState BowCharacter = ARMCLEAN063AO::NewCharacter();
	FGridCharacterEquipmentState BowEquipment;
	TArray<UGridItemDefinitionAsset*> OutDefinitions;
	FString Error;
	if (!TestTrue(TEXT("Bow kit grants using canonical hands"),
		ARMCLEAN063AO::TryGrant(BowClass.Get(), Registry.Get(), BowCharacter, BowEquipment, OutDefinitions, Error)))
	{
		AddError(Error);
		return false;
	}
	TestTrue(TEXT("Physical bow stored in MainHand"), BowEquipment.MainHand.IsValid());
	TestFalse(TEXT("No duplicate object in OffHand"), BowEquipment.OffHand.IsValid());
	TestEqual(TEXT("Physical bow has real N6 level"), BowEquipment.MainHand.ItemLevel, 6);
	TestEqual(TEXT("No extra definition registered"), OutDefinitions.Num(), 1);

	// The second item cannot steal the virtual OffHand of a two-handed bow.
	ARMCLEAN063AO::Add(BowClass.Get(), Shield.Get(), EGridEquipmentSlot::OffHand);
	FGridCharacterInventoryState FailedCharacter = ARMCLEAN063AO::NewCharacter();
	FGridCharacterEquipmentState FailedEquipment;
	const FGuid OriginalCharacterId = FailedCharacter.CharacterId;
	OutDefinitions.Reset();
	Error.Reset();
	TestFalse(TEXT("Shield plus two handed bow rejects kit atomically"),
		ARMCLEAN063AO::TryGrant(BowClass.Get(), Registry.Get(), FailedCharacter, FailedEquipment, OutDefinitions, Error));
	TestFalse(TEXT("Two-hand conflict reports an error"), Error.IsEmpty());
	TestFalse(TEXT("Failed kit reserves no physical MainHand"), FailedEquipment.MainHand.IsValid());
	TestFalse(TEXT("Failed kit reserves no physical OffHand"), FailedEquipment.OffHand.IsValid());
	TestTrue(TEXT("Failed kit publishes no definitions"), OutDefinitions.IsEmpty());
	TestTrue(TEXT("Failed kit adds no bag items"), FailedCharacter.InventorySlots[0].IsEmpty());
	TestEqual(TEXT("Failed kit leaves character GUID intact"), FailedCharacter.CharacterId, OriginalCharacterId);
	TestTrue(TEXT("Failed kit ID is not recorded"), FailedCharacter.StartingEquipmentKitId.IsNone());

	// Reverse order is also transactional: a shield already in OffHand
	// must prevent an afterwards authored two-handed bow.
	TStrongObjectPtr<URPGClassAsset> Reverse(ARMCLEAN063AO::NewClass());
	ARMCLEAN063AO::Add(Reverse.Get(), Shield.Get(), EGridEquipmentSlot::OffHand);
	ARMCLEAN063AO::Add(Reverse.Get(), Bow.Get(), EGridEquipmentSlot::MainHand);
	OutDefinitions.Reset();
	Error.Reset();
	TestFalse(TEXT("Two-handed bow cannot follow equipped shield"),
		ARMCLEAN063AO::TryGrant(Reverse.Get(), Registry.Get(), FailedCharacter, FailedEquipment, OutDefinitions, Error));
	TestTrue(TEXT("Reverse conflict leaves untouched OffHand"), !FailedEquipment.OffHand.IsValid());

	// The obsolete two-handed flag on a shield must not hide the player's
	// MainHand: the sword and shield remain two separate physical objects.
	TStrongObjectPtr<URPGClassAsset> Normal(ARMCLEAN063AO::NewClass());
	ARMCLEAN063AO::Add(Normal.Get(), Shield.Get(), EGridEquipmentSlot::OffHand);
	ARMCLEAN063AO::Add(Normal.Get(), Sword.Get(), EGridEquipmentSlot::MainHand);
	FGridCharacterInventoryState NormalCharacter = ARMCLEAN063AO::NewCharacter();
	FGridCharacterEquipmentState NormalEquipment;
	OutDefinitions.Reset();
	Error.Reset();
	if (!TestTrue(TEXT("Canonical sword and shield coexist"),
		ARMCLEAN063AO::TryGrant(Normal.Get(), Registry.Get(), NormalCharacter, NormalEquipment, OutDefinitions, Error)))
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

	// Unknown identities retain the temporary original equip policy.
	TStrongObjectPtr<UGridItemDefinitionAsset> Old(NewObject<UGridItemDefinitionAsset>());
	Old->ItemDefinitionId = TEXT("LegacyStarter_AO");
	Old->HandUsage = EGridItemHandUsage::OneHanded;
	const auto OldPolicy = FGridStartingKitEquipmentPolicyReadModel::Resolve(
		Registry.Get(), Old.Get(), 0, EGridEquipmentSlot::MainHand);
	TestFalse(TEXT("Unknown entry remains outside canonical registry"), OldPolicy.bCanonicalIdentity);
	TestTrue(TEXT("Unknown pre-conversion entry may still equip"), OldPolicy.bResolved && !OldPolicy.bTwoHanded);
	return true;
}
#endif
