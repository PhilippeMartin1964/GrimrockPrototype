#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/GridCanonicalRegistryFixture.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062HandActionSeparationTest,
	"Grimrock.RPG.ARM.CLEAN06_2.Semantics.HandActionSeparation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean062HandActionSeparationTest::RunTest(const FString&)
{
	FGridCanonicalRegistryFixture Fixture;
	if (!TestTrue(TEXT("Production item system registers"), Fixture.bReady)) return false;
	FGridResolvedItem Sword, Bow, Shield, Stone;
	if (!TestTrue(TEXT("Real hand and action policies resolve"),
		Fixture.Resolve(TEXT("Weapon_StraightSword_01"), 1, Sword) &&
		Fixture.Resolve(TEXT("Weapon_ShortBow_01"), 1, Bow) &&
		Fixture.Resolve(TEXT("Weapon_RoundShield_01"), 1, Shield) &&
		Fixture.Resolve(TEXT("Stone"), 0, Stone))) return false;

	TestTrue(TEXT("Sword can occupy MainHand"), Sword.Weapon.CanEquipInHand(EGridEquipmentSlot::MainHand));
	TestTrue(TEXT("Bow needs two hands"), Bow.Weapon.HandPolicy == EGridHandPolicy::TwoHanded);
	TestFalse(TEXT("Shield does not implicitly occupy MainHand"), Shield.Weapon.CanEquipInHand(EGridEquipmentSlot::MainHand));
	TestTrue(TEXT("Equipping a shield does not create an attack"), Shield.AvailableActions.IsEmpty());
	TestTrue(TEXT("A plain stone has no authored offensive action"), Stone.AvailableActions.IsEmpty());
	FGridResolvedBoundAction Attack;
	if (!TestTrue(TEXT("Sword has an explicit primary action"), !Sword.AvailableActions.IsEmpty())) return false;
	const FName ActionId = Sword.AvailableActions[0].ActionId;
	TestTrue(TEXT("Correct hand grants the sword attack"),
		Fixture.Registry->ResolveBoundAttackForHand(Sword.ItemId, 1, ActionId, EGridEquipmentSlot::MainHand, Attack));
	TestFalse(TEXT("Unequipped slots cannot grant the attack"),
		Fixture.Registry->ResolveBoundAttackForHand(Sword.ItemId, 1, ActionId, EGridEquipmentSlot::Head, Attack));
	TestFalse(TEXT("Physical throwing does not borrow a weapon action"),
		Fixture.Registry->ResolveBoundAction(Stone.ItemId, 0, ActionId, Attack));
	return !HasAnyErrors();
}
#endif
