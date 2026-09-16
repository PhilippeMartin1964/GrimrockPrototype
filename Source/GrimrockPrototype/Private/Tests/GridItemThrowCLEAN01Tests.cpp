#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Runtime/GridItemDefinitionAsset.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridItemThrowCLEAN01Test, "Grimrock.Items.ITEM_THROW_CLEAN01",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridItemThrowCLEAN01Test::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UClass* ItemClass = UGridItemDefinitionAsset::StaticClass();
	TestNotNull(TEXT("Item definition class exists"), ItemClass);
	if (!ItemClass)
	{
		return false;
	}

	TestNull(TEXT("Legacy bThrowable property is physically removed"), ItemClass->FindPropertyByName(TEXT("bThrowable")));

	UGridItemDefinitionAsset* PhysicalItem = NewObject<UGridItemDefinitionAsset>(GetTransientPackage());
	PhysicalItem->HandUsage = EGridItemHandUsage::OneHanded;
	PhysicalItem->Weight = 1.0f;
	PhysicalItem->ThrowSpeed = 1200.0f;
	PhysicalItem->bCombatThrowWeapon = false;
	TestTrue(TEXT("A one-handed item with valid weight and speed is physically throwable"), PhysicalItem->IsPhysicallyThrowable());
	TestFalse(TEXT("Physical throwability does not grant combat throw authority"), PhysicalItem->IsCombatThrowable());
	PhysicalItem->bCombatThrowWeapon = true;
	TestTrue(TEXT("Explicit combat throw authority enables combat throwing"), PhysicalItem->IsCombatThrowable());

	const TCHAR* ShurikenAssetPath =
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_Shuriken.DA_Weapon_Shuriken");
	UGridItemDefinitionAsset* Shuriken = LoadObject<UGridItemDefinitionAsset>(nullptr, ShurikenAssetPath);
	if (!TestNotNull(TEXT("Production DA_Weapon_Shuriken loads"), Shuriken))
	{
		return false;
	}

	TestEqual(TEXT("Shuriken explicitly authors one-handed physical usage"), Shuriken->HandUsage, EGridItemHandUsage::OneHanded);
	TestTrue(TEXT("Shuriken explicitly authors combat projectile semantics"), Shuriken->bCombatThrowWeapon);
	TestTrue(TEXT("Shuriken is physically throwable"), Shuriken->IsPhysicallyThrowable());
	TestTrue(TEXT("Shuriken is combat throwable"), Shuriken->IsCombatThrowable());
	TestTrue(TEXT("Shuriken remains a valid item definition"), Shuriken->IsValidDefinition());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
