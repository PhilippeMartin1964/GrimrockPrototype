#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062HandActionSeparationTest,
	"Grimrock.RPG.ARM.CLEAN06_2.Semantics.HandActionSeparation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean062HandActionSeparationTest::RunTest(const FString&)
{
	TStrongObjectPtr<UGridItemDefinitionAsset> Item(
		NewObject<UGridItemDefinitionAsset>(GetTransientPackage()));
	Item->ItemDefinitionId = TEXT("TestHandActionSeparation");
	Item->HandUsage = EGridItemHandUsage::Auto;
	TestEqual(TEXT("Auto without authored hands is not held"),
		Item->GetEffectiveHandUsage(), EGridItemHandUsage::NotHandHeld);
	TestFalse(TEXT("No implicit main-hand equip"), Item->CanEquipToSlot(EGridEquipmentSlot::MainHand));

	Item->CompatibleEquipmentSlots.Add(EGridEquipmentSlot::MainHand);
	TestEqual(TEXT("Auto with one authored hand becomes one handed"),
		Item->GetEffectiveHandUsage(), EGridItemHandUsage::OneHanded);
	TestTrue(TEXT("One handed equip may widen to offhand"),
		Item->CanEquipToSlot(EGridEquipmentSlot::OffHand));
	TestFalse(TEXT("Widened equip does not grant an offhand attack"),
		Item->CanProvideAttackFromSlot(EGridEquipmentSlot::OffHand));
	TestFalse(TEXT("An equipped item without an attack action cannot attack"),
		Item->CanProvideAttackFromSlot(EGridEquipmentSlot::MainHand));

	Item->HandUsage = EGridItemHandUsage::TwoHanded;
	TestTrue(TEXT("Two handed equip accepts offhand before inventory normalization"),
		Item->CanEquipToSlot(EGridEquipmentSlot::OffHand));
	TestFalse(TEXT("Two handed equip alone does not create an attack"),
		Item->CanProvideAttackFromSlot(EGridEquipmentSlot::OffHand));

	Item->HandUsage = EGridItemHandUsage::NotHandHeld;
	TestFalse(TEXT("NotHandHeld does not implicitly allow offhand"),
		Item->CanEquipToSlot(EGridEquipmentSlot::OffHand));

	Item->bUseWeaponCatalogue = false;
	Item->bCombatThrowWeapon = true;
	TestTrue(TEXT("Legacy combat throw opt-in is independent of ARM catalogue"),
		Item->IsCombatThrowable());
	FGridCombatActionDefinition InventoryAction;
	TestFalse(TEXT("Throw flag without a local attack does not manufacture a QuickItem"),
		Item->BuildInventoryCombatActionDefinition(InventoryAction));
	return true;
}
#endif
