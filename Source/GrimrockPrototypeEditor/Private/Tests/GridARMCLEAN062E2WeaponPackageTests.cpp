#if WITH_DEV_AUTOMATION_TESTS

#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/AutomationTest.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "Tests/GridCanonicalRegistryFixture.h"

// Retirement transition contract: accept either the intact pre-retirement
// historical set or the fully retired set, never an intermediate partial
// removal. Canonical production data is mandatory in BOTH states.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062E2WeaponPackageRegistryTest,
    "Grimrock.RPG.ARM.CLEAN06_2.Semantics.WeaponPackageRegistry",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean062E2WeaponPackageRegistryTest::RunTest(const FString&)
{
    FGridCanonicalRegistryFixture Fixture;
    if (!TestTrue(TEXT("Real canonical ItemSystem is always available"), Fixture.bReady))
        return false;
    UGridItemSystemAsset* Root =
        GetDefault<UGridItemSubsystem>()->LoadConfiguredItemSystemForPreview();
    if (!TestNotNull(TEXT("Production registry asset exists"), Root) ||
        !TestEqual(TEXT("All 48 native item identities are retained"),
            Root->ItemDefinitions.Num(), 48))
        return false;
    int32 CanonicalArms = 0;
    for (const UGridCanonicalItemDefinitionAsset* Definition : Root->ItemDefinitions)
    {
        if (!TestNotNull(TEXT("Registered native definition is materialized"), Definition))
            return false;
        if (Definition->IsA<UGridArmamentDefinitionAsset>()) ++CanonicalArms;
    }
    TestEqual(TEXT("Exactly 22 canonical ARM owners"), CanonicalArms, 22);

    IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(
        TEXT("AssetRegistry")).Get();
    Registry.SearchAllAssets(true);
    TArray<FAssetData> AllAssets;
    Registry.GetAssetsByPath(FName(TEXT("/Game")), AllAssets, true, true);
    int32 HistoricalDefinitions = 0;
    for (const FAssetData& Asset : AllAssets)
    {
        if (Asset.AssetClassPath.GetAssetName() ==
            UGridItemDefinitionAsset::StaticClass()->GetFName())
            ++HistoricalDefinitions;
    }
    const bool bIntact = HistoricalDefinitions == 48;
    const bool bRetired = HistoricalDefinitions == 0;
    if (!TestTrue(TEXT("Historical item definitions must be ALL 48 or NONE"),
        bIntact || bRetired))
        return false;

    TArray<FAssetData> WeaponAssets;
    Registry.GetAssetsByPath(
        FName(TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons")),
        WeaponAssets, false);

    const FString Prefix =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_");
    const FString CombatCatalogue =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponCatalogue");
    const FString VisualCatalogue =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponVisualCatalogue");
    const FString ThrowingKnifeRedirector =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_ThrowingKnife");

    TSet<FName> Seen;
    int32 WeaponPackages = 0;
    int32 KnownCatalogues = 0;
    int32 OldWeaponDefinitions = 0;
    int32 Redirectors = 0;
    int32 UnexpectedClasses = 0;
    int32 Unloadable = 0;
    for (const FAssetData& Asset : WeaponAssets)
    {
        const FString Package = Asset.PackageName.ToString();
        if (Package == CombatCatalogue || Package == VisualCatalogue)
        {
            ++KnownCatalogues;
            continue; // Historical source catalogues remain protected.
        }
        if (!Package.StartsWith(Prefix)) continue;
        ++WeaponPackages;
        TestFalse(TEXT("Historical weapon package path occurs once"),
            Seen.Contains(Asset.PackageName));
        Seen.Add(Asset.PackageName);

        const FName Class = Asset.AssetClassPath.GetAssetName();
        if (Class == UGridItemDefinitionAsset::StaticClass()->GetFName())
        {
            ++OldWeaponDefinitions;
            if (!IsValid(Asset.GetAsset())) ++Unloadable;
        }
        else if (Class == FName(TEXT("ObjectRedirector")) &&
            Package == ThrowingKnifeRedirector)
        {
            // Preserve until the separate native redirector transaction.
            ++Redirectors;
        }
        else
        {
            ++UnexpectedClasses;
        }
    }
    TestEqual(TEXT("Both historical source catalogues remain protected"),
        KnownCatalogues, 2);
    TestEqual(TEXT("All distinct historical weapon paths classified"),
        Seen.Num(), WeaponPackages);
    TestEqual(TEXT("Every historical weapon package is classified"),
        OldWeaponDefinitions + Redirectors + UnexpectedClasses, WeaponPackages);
    TestEqual(TEXT("No historical weapon definition fails to load while present"),
        Unloadable, 0);

    if (bIntact)
    {
        // Old characterization contract preserved until native retirement.
        TestEqual(TEXT("Intact legacy weapon namespace has 24 packages"),
            WeaponPackages, 24);
        TestTrue(TEXT("Intact legacy set includes the 22 production ARM sources"),
            OldWeaponDefinitions >= 22);
    }
    else
    {
        // The only separately deferred weapon-path object is the known
        // throwing-knife redirector. Never treat newly introduced redirectors
        // or a partially removed legacy collection as a successful retirement.
        TestEqual(TEXT("All old weapon definition packages have gone"),
            OldWeaponDefinitions, 0);
        TestEqual(TEXT("Only the explicit deferred redirector may survive"),
            WeaponPackages, Redirectors);
        TestTrue(TEXT("At most one known throwing-knife redirector remains"),
            Redirectors <= 1);
        TestEqual(TEXT("No unrecognized weapon-path objects survive retirement"),
            UnexpectedClasses, 0);
    }
    return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS
