#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridCanonicalItemCatalogues.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "Engine/GameInstance.h"
#include "UObject/StrongObjectPtr.h"
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

	// The current Shuriken combat and physical policy is owned only by the
	// canonical item/weapon catalogues. Old UGridItemDefinitionAsset is a
	// synthetic backwards-characterization fixture above, not an asset source.
	TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
	TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
	UGridItemSystemAsset* Root = LoadObject<UGridItemSystemAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem"));
	if (!TestNotNull(TEXT("Production canonical root loads"), Root)) return false;
	TArray<FString> Errors;
	if (!TestTrue(TEXT("Production canonical registry validates"),
		Registry->RegisterItemSystem(Root, Errors))) return false;
	FGridResolvedItem Shuriken;
	if (!TestTrue(TEXT("Shuriken N1 resolves canonically"),
		Registry->ResolveItem(TEXT("Shuriken"), 1, Shuriken))) return false;
	TestTrue(TEXT("Production Shuriken is an armament"), Shuriken.bIsArmament);
	// A specialized combat projectile (Shuriken) must NOT be mistaken
	// for a normal non-combat hand toss. The pipeline projects three such
	// items with bEnabled=true and both generic flags disabled.
	TestFalse(TEXT("Shuriken does not borrow ordinary physical hand-toss permission"),
		Shuriken.Weapon.Projectile.bGenericThrowOneHanded);
	TestFalse(TEXT("Shuriken does not duplicate a generic throw policy"),
		Shuriken.Weapon.Projectile.bGenericThrowAuthored);
	TestTrue(TEXT("Shuriken physically launches with a positive speed"),
		Shuriken.Weapon.Projectile.InitialSpeed > 0.f);
	TestTrue(TEXT("Shuriken explicitly enables a combat projectile"),
		Shuriken.Weapon.Projectile.bEnabled);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
