#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Runtime/Combat/GridCombatLog.h"
#include "Runtime/Combat/GridEquippedGuardResolver.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/Monsters/GridMonsterTypes.h"

namespace GridGuard193
{
	FGridItemInstance MakeEquipped(const UGridItemDefinitionAsset* Definition, EGridEquipmentSlot Slot)
	{
		FGridItemInstance Item;
		Item.RuntimeObjectId = FGuid::NewGuid();
		Item.ItemDefinitionId = Definition->ItemDefinitionId;
		Item.ItemLevel = 1;
		Item.Quantity = 1;
		Item.OwnerCharacterIndex = 0;
		Item.OwnerType = EGridItemOwnerType::EquipmentSlot;
		Item.EquipmentSlot = Slot;
		return Item;
	}

	UGridPartyInventoryComponent* MakeInventory(UGridItemDefinitionAsset* Definition)
	{
		UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
		FGridCharacterInventoryState Character;
		Character.CharacterId = FGuid::NewGuid();
		Character.DisplayName = FText::FromString(TEXT("Garde"));
		Character.Resources.CurrentHealth = 20;
		Character.Resources.CurrentPhysicalArmor = 0;
		Character.Resources.CurrentMagicalArmor = 0;
		Inventory->PartyInventoryState.ActiveCharacters.Add(Character);
		Inventory->PartyInventoryState.ActiveEquipment.SetNum(1);
		Inventory->RegisterItemDefinition(Definition);
		Inventory->PartyInventoryState.ActiveEquipment[0].OffHand = MakeEquipped(Definition, EGridEquipmentSlot::OffHand);
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
	UGridItemDefinitionAsset* MainGauche = LoadObject<UGridItemDefinitionAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_MainGauche_01.DA_Weapon_MainGauche_01"));
	if (!TestNotNull(TEXT("Real ARM013 Main-gauche exists"), MainGauche))
	{
		return false;
	}
	UGridPartyInventoryComponent* Inventory = GridGuard193::MakeInventory(MainGauche);
	FGridMonsterAttackDefinition Melee;
	Melee.AttackId = TEXT("Test_Monster_Melee");
	Melee.DamageType = EGridDamageType::Physical;
	Melee.MinRangeCells = 1;
	Melee.MaxRangeCells = 1;
	FGridAttackResult Result = GridGuard193::MakePhysicalHit(6);
	if (!TestTrue(TEXT("Actual ARM013 OffHand can parry physical melee"),
		FGridEquippedGuardResolver::CanParry(Inventory, 0, Melee, Result)))
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
	FGridEquippedGuardResolver::ApplyParry(Result);
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
	UGridItemDefinitionAsset* Shield = LoadObject<UGridItemDefinitionAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_RoundShield_01.DA_Weapon_RoundShield_01"));
	if (!TestNotNull(TEXT("Real ARM075 round shield"), Shield))
	{
		return false;
	}
	UGridPartyInventoryComponent* Inventory = GridGuard193::MakeInventory(Shield);
	TestEqual(TEXT("Shield starts at five physical armor"), Inventory->GetEffectiveCurrentPhysicalArmor(0), 5);
	FGridAttackResult First = GridGuard193::MakePhysicalHit(4);
	First.PhysicalArmorDamage = 4;
	First.HealthDamage = 0;
	First.TargetHealthAfter = 20;
	TestEqual(TEXT("Existing armor system consumes four shield points"),
		Inventory->ApplyPhysicalArmorDamage(0, 4, &First.Trace.Damage.PhysicalArmorSources), 4);
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
	TestEqual(TEXT("Depleted shield cannot block"), FGridEquippedGuardResolver::ResolveAbsorbedShieldDamage(Inventory, 0, Third), 0);
	Third.DamageType = EGridDamageType::Fire;
	TestEqual(TEXT("Magic damage does not consume physical shield pool"), FGridEquippedGuardResolver::ResolveAbsorbedShieldDamage(Inventory, 0, Third), 0);

	// An unequipped shield (or even a spoofed trace) cannot claim a block.
	Inventory->PartyInventoryState.ActiveEquipment[0].OffHand = FGridItemInstance();
	TestEqual(TEXT("No equipped shield can label a block"), FGridEquippedGuardResolver::ResolveAbsorbedShieldDamage(Inventory, 0, First), 0);

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

#endif
