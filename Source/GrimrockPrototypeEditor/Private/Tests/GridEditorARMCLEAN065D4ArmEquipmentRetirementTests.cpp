#if WITH_DEV_AUTOMATION_TESTS

#include "AssetRegistry/AssetRegistryModule.h"
#include "ARMEMENT/GridWeaponPipelineManifest.h"
#include "Misc/AutomationTest.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "Tests/GridCanonicalRegistryFixture.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorARMCLEAN065D4ArmExtraStats,
    "Grimrock.RPG.ARM.CLEAN06_5D4.ArmExtraContributionsRetirementEvidence",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorARMCLEAN065D4ArmExtraStats::RunTest(const FString&)
{
    // Both sides of native retirement must retain the canonical 22-ARM model.
    // Before removal, independently compare all 22 historical sources; after
    // removal, do not demand that old .uassets be recreated for this test.
    FGridCanonicalRegistryFixture Fixture;
    if (!TestTrue(TEXT("Actual canonical ItemSystem available"), Fixture.bReady))
        return false;
    UGridItemSystemAsset* Root =
        GetDefault<UGridItemSubsystem>()->LoadConfiguredItemSystemForPreview();
    if (!TestNotNull(TEXT("Native 48-item registry exists"), Root) ||
        !TestEqual(TEXT("All canonical identities present"), Root->ItemDefinitions.Num(), 48))
        return false;
    const bool bRootDirty = Root->GetOutermost()->IsDirty();

    // Never silently flatten legacy extra modifiers: the final ARM item
    // schema has no fields for duplicate weapon-stat or resistance caches.
    const UClass* ArmClass = UGridArmamentDefinitionAsset::StaticClass();
    TestNull(TEXT("Canonical ARM has no duplicate EquipmentStatBonus field"),
        ArmClass->FindPropertyByName(FName(TEXT("EquipmentStatBonus"))));
    TestNull(TEXT("Canonical ARM has no duplicate EquipmentResistanceBonus field"),
        ArmClass->FindPropertyByName(FName(TEXT("EquipmentResistanceBonus"))));

    TArray<FGridWeaponPipelineRow> Rows;
    FString Error;
    if (!TestTrue(TEXT("Read exact 22-ARM production manifest"),
        FGridWeaponPipelineManifest::Load(
            FGridWeaponPipelineManifest::DefaultFile(), Rows, Error)))
    {
        AddError(Error);
        return false;
    }
    if (!TestEqual(TEXT("Exactly 22 canonical armament sources"), Rows.Num(), 22))
        return false;

    IAssetRegistry& Assets = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(
        TEXT("AssetRegistry")).Get();
    Assets.SearchAllAssets(true);
    TArray<FAssetData> OldWeapons;
    Assets.GetAssetsByPath(
        FName(TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons")), OldWeapons, false);
    TSet<FString> OldPaths;
    for (const FAssetData& Asset : OldWeapons)
    {
        if (Asset.AssetClassPath.GetAssetName() ==
            UGridItemDefinitionAsset::StaticClass()->GetFName())
            OldPaths.Add(Asset.GetSoftObjectPath().ToString());
    }
    int32 ManifestSourcesPresent = 0;
    for (const FGridWeaponPipelineRow& Row : Rows)
        ManifestSourcesPresent += OldPaths.Contains(Row.ItemPath) ? 1 : 0;
    const bool bOldSourcesPresent = ManifestSourcesPresent == 22;
    if (!TestTrue(TEXT("All 22 historical ARM sources or none; partial retirement refused"),
        bOldSourcesPresent || ManifestSourcesPresent == 0))
        return false;

    const FGridEquipmentStatBonus NoExtra;
    const FGridDamageResistanceSet NoResistance;
    TSet<FName> Ids;
    for (const FGridWeaponPipelineRow& Row : Rows)
    {
        const FName ItemId(*Row.ItemDefinitionId);
        TestFalse(TEXT("No duplicate ARM ID in the canonical production manifest"),
            Ids.Contains(ItemId));
        Ids.Add(ItemId);
        FGridResolvedItem N1, N20;
        if (!TestTrue(TEXT("Real canonical ARM resolves at both levels"),
            Fixture.Resolve(ItemId, 1, N1) && Fixture.Resolve(ItemId, 20, N20)))
            return false;
        TestTrue(TEXT("Native N1 and N20 armament identities match"),
            N1.bIsArmament && N20.bIsArmament &&
            N1.ItemId == ItemId && N20.ItemId == ItemId &&
            N1.ArchetypeId == FName(*Row.ArchetypeId) &&
            N20.ArchetypeId == N1.ArchetypeId);
        if (!bOldSourcesPresent) continue;

        const UGridItemDefinitionAsset* Old =
            LoadObject<UGridItemDefinitionAsset>(nullptr, *Row.ItemPath);
        if (!TestNotNull(TEXT("Exact historical ARM for pre-retirement evidence"), Old))
            return false;
        if (!TestTrue(TEXT("Historical source path and identity exact"),
            Old->ItemDefinitionId == ItemId && Old->GetPathName() == Row.ItemPath))
            return false;
        const bool bWasDirty = Old->GetOutermost()->IsDirty();
        FGridEquipmentStatBonus Extras = Old->EquipmentStatBonus;
        // N1 armor maxima already have their own canonical growth rules.
        Extras.ArmorBonus = 0;
        Extras.MagicalArmorBonus = 0;
        TestTrue(FString(TEXT("No other ARM source bonus: ")) + Row.ArchetypeId,
            FGridEquipmentStatBonus::StaticStruct()->CompareScriptStruct(
                &Extras, &NoExtra, 0));
        TestTrue(FString(TEXT("No ARM source resistance loss: ")) + Row.ArchetypeId,
            FGridDamageResistanceSet::StaticStruct()->CompareScriptStruct(
                &Old->EquipmentResistanceBonus, &NoResistance, 0));
        TestEqual(TEXT("Historical source remains read-only"),
            Old->GetOutermost()->IsDirty(), bWasDirty);
    }
    TestEqual(TEXT("Every production ARM identity inspected"), Ids.Num(), 22);
    TestEqual(TEXT("Canonical ItemSystem remains read-only"),
        Root->GetOutermost()->IsDirty(), bRootDirty);
    return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS
