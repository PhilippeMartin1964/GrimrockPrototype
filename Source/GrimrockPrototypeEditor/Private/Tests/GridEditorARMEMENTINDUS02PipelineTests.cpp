#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ARMEMENT/GridWeaponPipelineManifest.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Modules/ModuleManager.h"
#include "Runtime/Combat/GridWeaponCatalogueAsset.h"
#include "Runtime/Combat/GridWeaponVisualCatalogueAsset.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "UObject/UObjectGlobals.h"

namespace WeaponPipelineTests
{
    constexpr const TCHAR* RpgPath =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponCatalogue.DA_WeaponCatalogue");
    constexpr const TCHAR* VisualsPath =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponVisualCatalogue.DA_WeaponVisualCatalogue");

    bool Read(TArray<FGridWeaponPipelineRow>& Rows, FAutomationTestBase& Test)
    {
        FString Error;
        return Test.TestTrue(TEXT("Generic production manifest valid"),
            FGridWeaponPipelineManifest::Load(
                FGridWeaponPipelineManifest::DefaultFile(), Rows, Error));
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponPipelineManifestContracts,
    "Grimrock.RPG.ARMEMENT.Industrial.ManifestContracts",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWeaponPipelineManifestContracts::RunTest(const FString&)
{
    TArray<FGridWeaponPipelineRow> Rows;
    if (!WeaponPipelineTests::Read(Rows, *this))
        return false;
    TestTrue(TEXT("At least eight historical production rows"), Rows.Num() >= 8);
    const FGridWeaponPipelineRow* Assassin =
        FGridWeaponPipelineManifest::Find(Rows, TEXT("ARM002"));
    if (!TestNotNull(TEXT("ARM002 is represented"), Assassin))
        return false;
    TestEqual(TEXT("Existing ARM002 item ID"), Assassin->ItemDefinitionId,
        FString(TEXT("Weapon_AssassinDagger_01")));
    TestEqual(TEXT("Existing ARM002 visual ID"), Assassin->VisualId,
        FString(TEXT("VIS_AssassinDagger_01")));
    TestEqual(TEXT("ARM002 is an actual production row"), Assassin->ProductionState,
        FString(TEXT("Active")));

    const FGridWeaponPipelineRow* Axe =
        FGridWeaponPipelineManifest::Find(Rows, TEXT("ARM031"));
    if (!TestNotNull(TEXT("Two-hand archetype included"), Axe))
        return false;
    TestEqual(TEXT("Axe is two-handed"), Axe->BuildKind, FString(TEXT("MeleeTwoHand")));

    const FGridWeaponPipelineRow* Bow =
        FGridWeaponPipelineManifest::Find(Rows, TEXT("ARM045"));
    const FGridWeaponPipelineRow* Shield =
        FGridWeaponPipelineManifest::Find(Rows, TEXT("ARM075"));
    if (!TestNotNull(TEXT("Ranged pilot included"), Bow) ||
        !TestNotNull(TEXT("Passive shield included"), Shield))
        return false;
    TestEqual(TEXT("Bow routed to ranged"), Bow->BuildKind, FString(TEXT("Ranged")));
    TestEqual(TEXT("Shield routed to passive"), Shield->BuildKind,
        FString(TEXT("ShieldPassive")));
    TestTrue(TEXT("Passive shield has no fictitious AttackId"), Shield->AttackId.IsEmpty());

    const FGridWeaponPipelineRow* Stiletto =
        FGridWeaponPipelineManifest::Find(Rows, TEXT("ARM003"));
    if (Stiletto)
    {
        TestEqual(TEXT("Prospective Stiletto not declared produced"), Stiletto->ProductionState,
            FString(TEXT("Planned")));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponPipelineWorkbookContracts,
    "Grimrock.RPG.ARMEMENT.Industrial.WorkbookN1Contracts",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWeaponPipelineWorkbookContracts::RunTest(const FString&)
{
    UGridWeaponCatalogueAsset* Rpg =
        LoadObject<UGridWeaponCatalogueAsset>(nullptr, WeaponPipelineTests::RpgPath);
    if (!TestNotNull(TEXT("Actual weapon catalogue exists"), Rpg))
        return false;
    const bool BeforeDirty = Rpg->GetOutermost()->IsDirty();
    TestTrue(TEXT("1,640 profiles are valid"), Rpg->IsValidCatalogue());
    TestTrue(TEXT("Class affinities from workbook valid"), Rpg->IsValidClassAffinityData());

    TArray<FGridWeaponPipelineRow> Rows;
    if (!WeaponPipelineTests::Read(Rows, *this))
        return false;
    for (const FGridWeaponPipelineRow& Row : Rows)
    {
        const FGridWeaponLevelProfile* N1 =
            Rpg->FindProfile(FName(Row.ArchetypeId), 1);
        if (!TestNotNull(*FString::Printf(TEXT("Real N1 for %s"), *Row.ArchetypeId), N1))
            continue;
        TestTrue(*FString::Printf(TEXT("Valid N1 for %s"), *Row.ArchetypeId),
            N1->IsValidProfile());
        if (Row.IsPassive())
        {
            TestFalse(TEXT("Passive item never acquires dice"), N1->DamageDice.bEnabled);
        }
        else
        {
            FGridAttackDefinition Attack;
            TestTrue(*FString::Printf(TEXT("Real offensive definition %s"),
                *Row.ArchetypeId), N1->TryBuildAttackDefinition(Attack));
            TestEqual(TEXT("One/two hand contract from workbook"), N1->HandUsage,
                Row.BuildKind == TEXT("MeleeOneHand")
                    ? FString(TEXT("1 main")) : FString(TEXT("2 mains")));
        }
    }
    TestEqual(TEXT("Read-only workbook check did not dirty package"),
        Rpg->GetOutermost()->IsDirty(), BeforeDirty);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponPipelineExistingContracts,
    "Grimrock.RPG.ARMEMENT.Industrial.ExistingItemsAndVisuals",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWeaponPipelineExistingContracts::RunTest(const FString&)
{
    UGridWeaponCatalogueAsset* Rpg =
        LoadObject<UGridWeaponCatalogueAsset>(nullptr, WeaponPipelineTests::RpgPath);
    UGridWeaponVisualCatalogueAsset* Visuals =
        LoadObject<UGridWeaponVisualCatalogueAsset>(nullptr, WeaponPipelineTests::VisualsPath);
    if (!TestNotNull(TEXT("RPG catalogue"), Rpg) ||
        !TestNotNull(TEXT("Visual catalogue"), Visuals))
        return false;
    const bool BeforeRpg = Rpg->GetOutermost()->IsDirty();
    const bool BeforeVisuals = Visuals->GetOutermost()->IsDirty();
    TestTrue(TEXT("Existing visual catalogue valid"), Visuals->IsValidCatalogue());

    TArray<FGridWeaponPipelineRow> Rows;
    if (!WeaponPipelineTests::Read(Rows, *this))
        return false;
    IAssetRegistry& Registry =
        FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
    TArray<FString> Scan = { TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons") };
    Registry.ScanPathsSynchronous(Scan, true);
    TArray<FAssetData> Items;
    Registry.GetAssetsByPath(
        FName(TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons")), Items, true);

    for (const FGridWeaponPipelineRow& Row : Rows)
    {
        if (Row.ProductionState != TEXT("Active"))
            continue;
        UGridItemDefinitionAsset* Item =
            LoadObject<UGridItemDefinitionAsset>(nullptr, *Row.ItemPath);
        if (!TestNotNull(*FString::Printf(TEXT("Active authored item %s"),
            *Row.ArchetypeId), Item))
            continue;
        int32 Matches = 0;
        for (const FAssetData& Data : Items)
        {
            const UGridItemDefinitionAsset* Candidate =
                Cast<UGridItemDefinitionAsset>(Data.GetAsset());
            if (Candidate && Candidate->WeaponArchetypeId == FName(Row.ArchetypeId))
                ++Matches;
        }
        TestEqual(TEXT("No item duplicates for active archetype"), Matches, 1);
        TestEqual(TEXT("Same item identity"), Item->ItemDefinitionId, FName(Row.ItemDefinitionId));
        TestEqual(TEXT("Same archetype"), Item->WeaponArchetypeId, FName(Row.ArchetypeId));
        TestTrue(TEXT("Authored item definition remains valid"), Item->IsValidDefinition());
        TestTrue(TEXT("Existing catalogues retained"),
            Item->bUseWeaponCatalogue && Item->bUseWeaponVisualCatalogue &&
            Item->WeaponCatalogue.LoadSynchronous() == Rpg &&
            Item->WeaponVisualCatalogue.LoadSynchronous() == Visuals);
        TestEqual(TEXT("Exact production visual binding"),
            Visuals->FindVisualId(FName(Row.ArchetypeId)), FName(Row.VisualId));
        const FGridWeaponVisualProfile* Look =
            Visuals->FindProfileForArchetype(FName(Row.ArchetypeId));
        if (!TestNotNull(TEXT("Real resolved visual"), Look))
            continue;
        TestTrue(TEXT("Actual icon and world mesh load"),
            Look->BaseIcon.LoadSynchronous() != nullptr &&
            Look->WorldMesh.LoadSynchronous() != nullptr);
        TestTrue(TEXT("Manifest icon follows existing reference"),
            Look->BaseIcon.ToSoftObjectPath() == FSoftObjectPath(Row.IconPath));
        TestTrue(TEXT("Manifest mesh follows existing reference"),
            Look->WorldMesh.ToSoftObjectPath() == FSoftObjectPath(Row.MeshPath));
        const FGridWeaponLevelProfile* N1 =
            Rpg->FindProfile(FName(Row.ArchetypeId), 1);
        if (TestNotNull(TEXT("N1 profile for authored item"), N1))
        {
            TestEqual(TEXT("Item default N1"), Item->DefaultWeaponItemLevel, 1);
            TestTrue(TEXT("Authored N1 weight matches workbook"),
                FMath::IsNearlyEqual(Item->Weight, N1->WeightKg, 0.0001f));
        }
    }
    TestEqual(TEXT("RPG catalogue never mutated"), Rpg->GetOutermost()->IsDirty(), BeforeRpg);
    TestEqual(TEXT("Visual catalogue never mutated"),
        Visuals->GetOutermost()->IsDirty(), BeforeVisuals);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
