#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Runtime/Combat/GridCombatLog.h"
#include "Runtime/Combat/GridEquippedGuardResolver.h"
#include "GridCanonicalWeightTestWorld.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/Monsters/GridMonsterTypes.h"
#include "Sound/SoundBase.h"

namespace GridGuard193
{
	FGridItemInstance MakeEquipped(FName ItemId, EGridEquipmentSlot Slot)
	{
		FGridItemInstance Item;
		Item.RuntimeObjectId = FGuid::NewGuid();
		Item.ItemDefinitionId = ItemId;
		Item.ItemLevel = 1;
		Item.Quantity = 1;
		Item.OwnerCharacterIndex = 0;
		Item.OwnerType = EGridItemOwnerType::EquipmentSlot;
		Item.EquipmentSlot = Slot;
		return Item;
	}

	UGridPartyInventoryComponent* MakeInventory(FGridCanonicalWeightTestWorld& Fixture, FName ItemId)
	{
		UGridPartyInventoryComponent* Inventory = Fixture.MakeInventory();
		if (!Inventory) return nullptr;
		FGridCharacterInventoryState Character;
		Character.CharacterId = FGuid::NewGuid();
		Character.DisplayName = FText::FromString(TEXT("Garde"));
		Character.Resources.CurrentHealth = 20;
		Character.Resources.CurrentPhysicalArmor = 0;
		Character.Resources.CurrentMagicalArmor = 0;
		Inventory->PartyInventoryState.ActiveCharacters.Add(Character);
		Inventory->PartyInventoryState.ActiveEquipment.SetNum(1);
		Inventory->PartyInventoryState.ActiveEquipment[0].OffHand =
			MakeEquipped(ItemId, EGridEquipmentSlot::OffHand);
		return Inventory;
	}

	FGridAttackResult MakePhysicalHit(int32 Damage)
	{
		FGridAttackResult Result;
		Result.bHit = true;
		Result.DamageType = EGridDamageType::Physical;
		Result.RawDamage = Damage;
		Result.DamageAfterModifiers = Damage;
		Result.PhysicalArmorDamage = 0;
		Result.HealthDamage = Damage;
		Result.TargetHealthBefore = 20;
		Result.TargetHealthAfter = 20 - Damage;
		Result.AttackRoll = 17;
		Result.DefenseValue = 12;
		return Result;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridGuard193ParryEligibilityTest,
	"Grimrock.RPG.ARMEMENT.LOT02.19.3.MainGaucheParryEligibility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridGuard193ParryEligibilityTest::RunTest(const FString&)
{
	FGridCanonicalWeightTestWorld Fixture({});
	FGridResolvedItem MainGauche;
	if (!TestNotNull(TEXT("Configured production ItemSystem"), Fixture.Scope.Registry) ||
		!TestTrue(TEXT("Real ARM013 Main-gauche N1 resolves canonically"),
			Fixture.Scope.Registry->ResolveItem(TEXT("Weapon_MainGauche_01"), 1, MainGauche)) ||
		!TestTrue(TEXT("ARM013 is an off-hand-capable armament"),
			MainGauche.bIsArmament &&
			MainGauche.Weapon.CanEquipInHand(EGridEquipmentSlot::OffHand)))
		return false;
	UGridPartyInventoryComponent* Inventory =
		GridGuard193::MakeInventory(Fixture, MainGauche.ItemId);
	if (!TestNotNull(TEXT("World-bound inventory without historical Main-gauche"), Inventory))
		return false;
	FGridMonsterAttackDefinition Melee;
	Melee.AttackId = TEXT("Test_Monster_Melee");
	Melee.DamageType = EGridDamageType::Physical;
	Melee.MinRangeCells = 1;
	Melee.MaxRangeCells = 1;
	FGridAttackResult Result = GridGuard193::MakePhysicalHit(6);
	FGridItemInstance ParrySource;
	if (!TestTrue(TEXT("Actual ARM013 OffHand can parry physical melee"), FGridEquippedGuardResolver::CanParry(Inventory, 0, Melee, Result, &ParrySource)))
	{
		return false;
	}

	TestFalse(TEXT("Zero percent never parries"), FGridEquippedGuardResolver::IsSuccessfulParryRoll(1, 0));
	TestTrue(TEXT("First qualifying roll parries"), FGridEquippedGuardResolver::IsSuccessfulParryRoll(1, 15));
	TestTrue(TEXT("Last qualifying roll parries"), FGridEquippedGuardResolver::IsSuccessfulParryRoll(15, 15));
	TestFalse(TEXT("Next roll fails"), FGridEquippedGuardResolver::IsSuccessfulParryRoll(16, 15));
	TestFalse(TEXT("Out of range 0 cannot parry"), FGridEquippedGuardResolver::IsSuccessfulParryRoll(0, 100));
	TestFalse(TEXT("Out of range 101 cannot parry"), FGridEquippedGuardResolver::IsSuccessfulParryRoll(101, 100));
	const int32 InitialArmor = Inventory->GetEffectiveCurrentPhysicalArmor(0);
	const int32 InitialHealth = Inventory->PartyInventoryState.ActiveCharacters[0].Resources.CurrentHealth;
	TestTrue(TEXT("Eligibility alone emits no cue"), Result.EquipmentDefenseEvents.IsEmpty());
	FGridEquippedGuardResolver::ApplyParry(Result, &ParrySource);
	if (TestEqual(TEXT("Effective parry emits one cue"), Result.EquipmentDefenseEvents.Num(), 1))
	{
		const FGridEquipmentDefenseEvent& Cue = Result.EquipmentDefenseEvents[0];
		TestTrue(TEXT("Parry source is the actual OffHand instance"),
			Cue.Cue == EGridEquipmentDefenseCue::Parry && Cue.Slot == EGridEquipmentSlot::OffHand &&
				Cue.RuntimeObjectId == Inventory->PartyInventoryState.ActiveEquipment[0].OffHand.RuntimeObjectId && Cue.ItemLevel == ParrySource.ItemLevel);
		const FGridActionPresentation* Profile = Fixture.Scope.Registry->FindEquipmentDefensePresentation(Cue);
		TestTrue(TEXT("ARM013 resolves native parry audio and VFX"), Profile && !Profile->ParryAudio.Sounds.IsEmpty() && !Profile->ParryVFX.Systems.IsEmpty());
	}
	TestTrue(TEXT("Parry result is explicitly marked"), Result.bEquipmentParried);
	TestFalse(TEXT("Parry is no longer a hit"), Result.bHit);
	TestFalse(TEXT("Parry cannot be a critical"), Result.bCriticalHit);
	TestEqual(TEXT("Parry applies zero physical armor damage"), Result.PhysicalArmorDamage, 0);
	TestEqual(TEXT("Parry applies zero health damage"), Result.HealthDamage, 0);
	TestEqual(TEXT("Parry applies zero post-mitigation damage"), Result.DamageAfterModifiers, 0);
	TestEqual(TEXT("Target HP remains unchanged"), Result.TargetHealthAfter, 20);
	TestEqual(TEXT("Parry never consumes equipment armor"), Inventory->GetEffectiveCurrentPhysicalArmor(0), InitialArmor);
	TestEqual(TEXT("Parry never directly mutates health"), Inventory->PartyInventoryState.ActiveCharacters[0].Resources.CurrentHealth, InitialHealth);

	Result = GridGuard193::MakePhysicalHit(6);
	Result.bCriticalHit = true;
	TestFalse(TEXT("Critical monster hit cannot be parried"), FGridEquippedGuardResolver::CanParry(Inventory, 0, Melee, Result));
	Result.bCriticalHit = false;
	Result.bHit = false;
	TestFalse(TEXT("Miss cannot trigger parry"), FGridEquippedGuardResolver::CanParry(Inventory, 0, Melee, Result));
	Result = GridGuard193::MakePhysicalHit(6);
	Result.DamageType = EGridDamageType::Fire;
	TestFalse(TEXT("Magic attack cannot be parried by physical Main-gauche"), FGridEquippedGuardResolver::CanParry(Inventory, 0, Melee, Result));
	Result.DamageType = EGridDamageType::Physical;
	Melee.MaxRangeCells = 3;
	TestFalse(TEXT("Ranged attack cannot be parried"), FGridEquippedGuardResolver::CanParry(Inventory, 0, Melee, Result));
	Melee.MaxRangeCells = 1;

	// A main-gauche in a bag, or a weapon of another archetype, cannot
	// grant a passive reaction just because the class is Voleur/Ombre.
	const FGridItemInstance OffHand = Inventory->PartyInventoryState.ActiveEquipment[0].OffHand;
	Inventory->PartyInventoryState.ActiveEquipment[0].OffHand = FGridItemInstance();
	TestFalse(TEXT("No OffHand means no parry"), FGridEquippedGuardResolver::CanParry(Inventory, 0, Melee, Result));
	Inventory->PartyInventoryState.ActiveEquipment[0].MainHand = OffHand;
	TestFalse(TEXT("MainHand alone cannot parry"), FGridEquippedGuardResolver::CanParry(Inventory, 0, Melee, Result));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridGuard193ShieldBlockTest,
	"Grimrock.RPG.ARMEMENT.LOT02.19.3.ShieldBlockActualAbsorption",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridGuard193ShieldBlockTest::RunTest(const FString&)
{
	FGridCanonicalWeightTestWorld Fixture({});
	FGridResolvedItem Shield;
	if (!TestNotNull(TEXT("Configured production ItemSystem"), Fixture.Scope.Registry) ||
		!TestTrue(TEXT("Real ARM075 round shield N1 resolves canonically"),
			Fixture.Scope.Registry->ResolveItem(TEXT("Weapon_RoundShield_01"), 1, Shield)) ||
		!TestTrue(TEXT("ARM075 is a passive off-hand shield with five armor"),
			Shield.bIsArmament && Shield.Weapon.PhysicalArmor == 5 &&
			Shield.Weapon.CanEquipInHand(EGridEquipmentSlot::OffHand)))
		return false;
	UGridPartyInventoryComponent* Inventory =
		GridGuard193::MakeInventory(Fixture, Shield.ItemId);
	if (!TestNotNull(TEXT("World-bound inventory without historical shield"), Inventory))
		return false;
	TestEqual(TEXT("Shield starts at five physical armor"), Inventory->GetEffectiveCurrentPhysicalArmor(0), 5);
	FGridAttackResult First = GridGuard193::MakePhysicalHit(4);
	First.PhysicalArmorDamage = 4;
	First.HealthDamage = 0;
	First.TargetHealthAfter = 20;
	TestEqual(TEXT("Existing armor system consumes four shield points"),
		Inventory->ApplyPhysicalArmorDamage(0, 4, &First.Trace.Damage.PhysicalArmorSources, &First.EquipmentDefenseEvents), 4);
	if (TestEqual(TEXT("Shield debit emits one cue"), First.EquipmentDefenseEvents.Num(), 1))
	{
		const FGridEquipmentDefenseEvent& Cue = First.EquipmentDefenseEvents[0];
		TestTrue(TEXT("Shield cue carries only the depleted points and their source"),
			Cue.Cue == EGridEquipmentDefenseCue::PhysicalAbsorption && Cue.Absorbed == 4 && Cue.Slot == EGridEquipmentSlot::OffHand && Cue.ItemLevel == 1 &&
				Cue.RuntimeObjectId == Inventory->PartyInventoryState.ActiveEquipment[0].OffHand.RuntimeObjectId);
		const FGridActionPresentation* Profile = Fixture.Scope.Registry->FindEquipmentDefensePresentation(Cue);
		TestTrue(TEXT("ARM075 resolves native physical block VFX"), Profile && !Profile->BlockVFX.Systems.IsEmpty());
		if (Profile && TestEqual(TEXT("Four wooden shield contacts are authored"), Profile->BlockAudio.Sounds.Num(), 4))
		{
			TestTrue(TEXT("Real round shield debit selects its wood cue"),
				GetNameSafe(Profile->BlockAudio.Sounds[0].LoadSynchronous()).StartsWith(TEXT("SFX_Sword_Impact_Wood_")));
		}
	}
	First.EquipmentBlockedDamage = FGridEquippedGuardResolver::ResolveAbsorbedShieldDamage(Inventory, 0, First);
	First.bEquipmentBlocked = First.EquipmentBlockedDamage > 0;
	TestEqual(TEXT("Block reports only real shield absorption"), First.EquipmentBlockedDamage, 4);
	TestEqual(TEXT("Armor genuinely decreases, not a second invented reserve"), Inventory->GetEffectiveCurrentPhysicalArmor(0), 1);
	TestTrue(TEXT("Shield block is flagged on the original hit"), First.bHit && First.bEquipmentBlocked);

	FGridAttackResult Second = GridGuard193::MakePhysicalHit(4);
	Second.PhysicalArmorDamage = 1;
	Second.HealthDamage = 3;
	Second.TargetHealthAfter = 17;
	TestEqual(TEXT("Second hit exhausts last shield point"), Inventory->ApplyPhysicalArmorDamage(
		0, 1, &Second.Trace.Damage.PhysicalArmorSources), 1);
	TestEqual(TEXT("Second block counts one real shield point"),
		FGridEquippedGuardResolver::ResolveAbsorbedShieldDamage(Inventory, 0, Second), 1);
	TestEqual(TEXT("Shield completely empty after two hits"), Inventory->GetEffectiveCurrentPhysicalArmor(0), 0);
	FGridAttackResult Third = GridGuard193::MakePhysicalHit(2);
	TestEqual(TEXT("Depleted shield has no points to report"), Inventory->ApplyPhysicalArmorDamage(0, 1, nullptr, &Third.EquipmentDefenseEvents), 0);
	TestTrue(TEXT("Depleted shield emits no cue"), Third.EquipmentDefenseEvents.IsEmpty());
	TestEqual(TEXT("Depleted shield cannot block"), FGridEquippedGuardResolver::ResolveAbsorbedShieldDamage(Inventory, 0, Third), 0);
	Third.DamageType = EGridDamageType::Fire;
	TestEqual(TEXT("Magic damage does not consume physical shield pool"), FGridEquippedGuardResolver::ResolveAbsorbedShieldDamage(Inventory, 0, Third), 0);

	// An unequipped shield (or even a spoofed trace) cannot claim a block.
	Inventory->PartyInventoryState.ActiveEquipment[0].OffHand = FGridItemInstance();
	TestEqual(TEXT("No equipped shield can label a block"), FGridEquippedGuardResolver::ResolveAbsorbedShieldDamage(Inventory, 0, First), 0);

	Inventory->PartyInventoryState.ActiveEquipment[0].OffHand = GridGuard193::MakeEquipped(TEXT("Weapon_HolyAegis_01"), EGridEquipmentSlot::OffHand);
	TArray<FGridEquipmentDefenseEvent> AegisEvents;
	TestEqual(TEXT("The Aegis actually absorbs a point of physical damage"), Inventory->ApplyPhysicalArmorDamage(0, 1, nullptr, &AegisEvents), 1);
	if (TestEqual(TEXT("Aegis debit emits one cue"), AegisEvents.Num(), 1))
	{
		const FGridActionPresentation* Profile = Fixture.Scope.Registry->FindEquipmentDefensePresentation(AegisEvents[0]);
		if (TestNotNull(TEXT("Aegis resolves its own defense profile"), Profile) &&
			TestEqual(TEXT("Four metal contacts are authored"), Profile->BlockAudio.Sounds.Num(), 4))
		{
			TestTrue(TEXT("Real Aegis debit selects its metal cue"),
				GetNameSafe(Profile->BlockAudio.Sounds[0].LoadSynchronous()).StartsWith(TEXT("SFX_Sword_Impact_Metal_")));
		}
	}

	const FText Parried = FGridCombatLogFormatter::FormatMonsterAttack(
		FText::FromString(TEXT("Rat")), FText::FromString(TEXT("Elias")), TEXT("Bite"), [&]()
		{
			FGridAttackResult Result = GridGuard193::MakePhysicalHit(3);
			FGridEquippedGuardResolver::ApplyParry(Result);
			return Result;
		}());
	TestFalse(TEXT("Parry generates a distinct readable combat log"), Parried.IsEmpty());
	const FText Blocked = FGridCombatLogFormatter::FormatMonsterAttack(
		FText::FromString(TEXT("Rat")), FText::FromString(TEXT("Elias")), TEXT("Bite"), First);
	TestFalse(TEXT("Shield absorption generates readable combat log"), Blocked.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridEquipmentMagicPresentationTest, "Grimrock.RPG.ARMEMENT.FX01.MagicalDefense", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEquipmentMagicPresentationTest::RunTest(const FString&)
{
	const FName OrbId(TEXT("Weapon_ArcaneOrb_01"));
	{
		FGridCanonicalWeightTestWorld Production({});
		UGridPartyInventoryComponent* Inventory = GridGuard193::MakeInventory(Production, OrbId);
		if (!TestNotNull(TEXT("Production inventory"), Inventory))
		{
			return false;
		}
		Inventory->PartyInventoryState.ActiveEquipment[0].OffHand.ItemLevel = 6;
		Inventory->PartyInventoryState.ActiveCharacters[0].Resources.CurrentMagicalArmor = 3;
		TArray<FGridEquipmentDefenseEvent> Events;
		TestEqual(TEXT("Personal magical armor absorbs damage"), Inventory->ApplyMagicalArmorDamage(0, 3, &Events), 3);
		TestTrue(TEXT("Production N6 orb has no defense cue when only personal armor is spent"), Events.IsEmpty());
	}

	// Exercise the positive path without changing a production numeric profile.
	FGridCanonicalWeightTestWorld Fixture({},
		[](UGridItemSystemAsset& Root)
		{
			Root.WeaponCatalogue = DuplicateObject<UGridWeaponCatalogueAsset>(Root.WeaponCatalogue, &Root);
			FGridWeaponArchetype* Orb = Root.WeaponCatalogue->Archetypes.FindByPredicate(
				[](const FGridWeaponArchetype& Row)
				{
					return Row.ArchetypeId == TEXT("ARM070");
				});
			check(Orb);
			Orb->MagicalArmorN1 = 4;
		});
	UGridPartyInventoryComponent* Inventory = GridGuard193::MakeInventory(Fixture, OrbId);
	if (!TestNotNull(TEXT("Canonical ItemSystem"), Fixture.Scope.Registry) || !TestNotNull(TEXT("World-bound inventory"), Inventory))
	{
		return false;
	}
	FGridItemInstance& Orb = Inventory->PartyInventoryState.ActiveEquipment[0].OffHand;
	Orb.ItemLevel = 6;
	const int32 Capacity = Inventory->GetEffectiveCurrentMagicalArmor(0);
	if (!TestTrue(TEXT("ARM070 fixture supplies magical armor"), Capacity > 0))
	{
		return false;
	}
	TArray<FGridEquipmentDefenseEvent> Events;
	TestEqual(TEXT("Orb loses the exact armor it absorbed"), Inventory->ApplyMagicalArmorDamage(0, Capacity, &Events), Capacity);
	if (TestEqual(TEXT("One effective magical absorption cue"), Events.Num(), 1))
	{
		const FGridEquipmentDefenseEvent& Cue = Events[0];
		TestTrue(TEXT("Cue carries ARM070 identity, level, hand and absorbed points"),
			Cue.Cue == EGridEquipmentDefenseCue::MagicalAbsorption && Cue.ItemDefinitionId == Orb.ItemDefinitionId &&
				Cue.RuntimeObjectId == Orb.RuntimeObjectId && Cue.ItemLevel == 6 && Cue.Slot == EGridEquipmentSlot::OffHand && Cue.Absorbed == Capacity);
		const FGridActionPresentation* Profile = Fixture.Scope.Registry->FindEquipmentDefensePresentation(Cue);
		TestTrue(TEXT("ARM070 resolves a distinct magic absorption effect"),
			Profile && !Profile->MagicalBlockVFX.Systems.IsEmpty() && Profile->BlockVFX.Systems.IsEmpty());
	}
	Events.Reset();
	TestEqual(TEXT("Empty orb does not absorb again"), Inventory->ApplyMagicalArmorDamage(0, 1, &Events), 0);
	TestTrue(TEXT("No cue from an empty orb"), Events.IsEmpty());
	return true;
}

#endif
