#if WITH_DEV_AUTOMATION_TESTS

#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/AutomationTest.h"
#include "Runtime/GridItemDefinitionAsset.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062E2WeaponPackageRegistryTest,
	"Grimrock.RPG.ARM.CLEAN06_2.Semantics.WeaponPackageRegistry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean062E2WeaponPackageRegistryTest::RunTest(const FString&)
{
	IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(
		TEXT("AssetRegistry")).Get();
	Registry.SearchAllAssets(true);

	TArray<FAssetData> Assets;
	Registry.GetAssetsByPath(
		FName(TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons")),
		Assets, false);

	const FString Prefix = TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_");
	const FString CombatCatalogue =
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponCatalogue");
	const FString VisualCatalogue =
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponVisualCatalogue");

	TSet<FName> Seen;
	int32 WeaponPackages = 0;
	int32 KnownCatalogues = 0;
	int32 ExpectedClass = 0;
	int32 OtherClass = 0;
	int32 Unloadable = 0;
	for (const FAssetData& Asset : Assets)
	{
		const FString Package = Asset.PackageName.ToString();
		if (Package == CombatCatalogue || Package == VisualCatalogue)
		{
			++KnownCatalogues;
			continue;
		}
		if (!Package.StartsWith(Prefix))
		{
			continue;
		}
		++WeaponPackages;
		TestFalse(TEXT("Weapon asset registry package is unique"),
			Seen.Contains(Asset.PackageName));
		Seen.Add(Asset.PackageName);

		UObject* Loaded = Asset.GetAsset();
		if (!IsValid(Loaded))
		{
			++Unloadable;
		}
		else if (Loaded->IsA<UGridItemDefinitionAsset>())
		{
			++ExpectedClass;
		}
		else
		{
			++OtherClass;
		}
	}

	// Count packages independently of class. This test must not presuppose
	// 24 UGridItemDefinitionAsset instances: that assumption caused the
	// ARM-CLEAN06.2E audit to discard nonmatching real weapon packages.
	TestEqual(TEXT("Exactly 24 original DA_Weapon_* packages"), WeaponPackages, 24);
	TestEqual(TEXT("All weapon package names unique"), Seen.Num(), 24);
	TestEqual(TEXT("Both weapon catalogues preserved separately"), KnownCatalogues, 2);
	TestEqual(TEXT("Expected/other/unloadable classification covers every package"),
		ExpectedClass + OtherClass + Unloadable, WeaponPackages);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
