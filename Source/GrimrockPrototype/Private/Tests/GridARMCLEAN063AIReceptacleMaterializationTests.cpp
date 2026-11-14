#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "../Runtime/GridReceptacleItemMetadataReadModel.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean063AIReceptacleMaterialization,
    "Grimrock.RPG.ARM.CLEAN06_3.CanonicalReceptacleMaterialization",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean063AIReceptacleMaterialization::RunTest(const FString&)
{
    TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem")));
    if (!TestNotNull(TEXT("Real ItemSystem"), Root.Get())) return false;
    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
    TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
    TArray<FString> Errors;
    if (!TestTrue(TEXT("Register canonical ItemSystem"), Registry->RegisterItemSystem(Root.Get(), Errors)))
    {
        for (const FString& Error : Errors) AddError(Error);
        return false;
    }

    TStrongObjectPtr<UGridItemDefinitionAsset> Old(NewObject<UGridItemDefinitionAsset>());
    Old->DefaultWeaponItemLevel = 6;
    Old->DisplayName = FText::FromString(TEXT("WRONG HISTORICAL NAME"));
    Old->Weight = 1000.0f;
    Old->bUseWeaponCatalogue = false; // Contradicts known ARM definition.
    const FName Arms[] = {
        TEXT("Weapon_StraightSword_01"), TEXT("Weapon_GreatAxe_01"),
        TEXT("Weapon_RoundShield_01"), TEXT("Weapon_HolyAegis_01") };
    for (const FName Id : Arms)
    {
        Old->ItemDefinitionId = Id;
        for (const int32 Level : {1, 20})
        {
            FGridResolvedItem Canonical;
            if (!TestTrue(TEXT("Actual canonical N1/N20 exists"),
                Registry->ResolveItem(Id, Level, Canonical))) return false;
            const FGridReceptacleItemMetadataReadView View =
                FGridReceptacleItemMetadataReadModel::Resolve(
                    Registry.Get(), Old.Get(), Id, Level, false);
            TestTrue(TEXT("Registered fixed contained ARM resolves"), View.bCanonicalIdentity && View.bResolved);
            TestEqual(TEXT("Never rewrite a fixed persisted level"), View.FixedItemLevel, Level);
            TestTrue(TEXT("Contained unit weight equals canonical"), FMath::IsNearlyEqual(
                View.UnitWeightKg, Canonical.WeightKg));
            TestEqual(TEXT("Contained display name canonical"),
                View.DisplayName.ToString(), Canonical.DisplayName.ToString());
        }
        FGridResolvedItem Default;
        if (!TestTrue(TEXT("Real default N6 exists"),
            Registry->ResolveItem(Id, 6, Default))) return false;
        const FGridReceptacleItemMetadataReadView Fresh =
            FGridReceptacleItemMetadataReadModel::Resolve(
                Registry.Get(), Old.Get(), Id, 0, true);
        TestTrue(TEXT("New placement inherits authored ARM default"), Fresh.bResolved);
        TestEqual(TEXT("New placement materializes N6 not N1"), Fresh.FixedItemLevel, 6);
        TestTrue(TEXT("Inherited mass still canonical"), FMath::IsNearlyEqual(
            Fresh.UnitWeightKg, Default.WeightKg));
        const FGridReceptacleItemMetadataReadView Persisted =
            FGridReceptacleItemMetadataReadModel::Resolve(
                Registry.Get(), Old.Get(), Id, 0, false);
        TestTrue(TEXT("Persisted invalid ARM N0 remains canonical"), Persisted.bCanonicalIdentity);
        TestFalse(TEXT("Cursor and save restoration never invent N1"), Persisted.bResolved);
        TestTrue(TEXT("Refused saved N0 has no old name"), Persisted.DisplayName.IsEmpty());
        TestEqual(TEXT("Refused saved N0 has no old mass"), Persisted.UnitWeightKg, 0.0f);
        const FGridReceptacleItemMetadataReadView IdOnly =
            FGridReceptacleItemMetadataReadModel::Resolve(
                Registry.Get(), nullptr, Id, 20, false);
        TestTrue(TEXT("Canonical fixed level can resolve without old pointer"), IdOnly.bResolved);
        TestEqual(TEXT("Id-only valid level preserved"), IdOnly.FixedItemLevel, 20);
        TestFalse(TEXT("Id-only N0 cannot inherit an absent default"),
            FGridReceptacleItemMetadataReadModel::Resolve(
                Registry.Get(), nullptr, Id, 0, true).bResolved);
    }

    Old->ItemDefinitionId = TEXT("Weapon_RoundShield_01");
    Old->DefaultWeaponItemLevel = 0;
    TestFalse(TEXT("Invalid authored ARM default fails without legacy fallback"),
        FGridReceptacleItemMetadataReadModel::Resolve(
            Registry.Get(), Old.Get(), Old->ItemDefinitionId, 0, true).bResolved);
    Old->DefaultWeaponItemLevel = 6;
    TestFalse(TEXT("Known ARM N21 rejected"),
        FGridReceptacleItemMetadataReadModel::Resolve(
            Registry.Get(), Old.Get(), Old->ItemDefinitionId, 21, false).bResolved);

    Old->ItemDefinitionId = TEXT("Item_Arrows_01");
    Old->bUseWeaponCatalogue = true; // Contradict ammo family.
    FGridResolvedItem Arrows;
    if (!TestTrue(TEXT("Real arrows N0"), Registry->ResolveItem(Old->ItemDefinitionId, 0, Arrows)))
        return false;
    const FGridReceptacleItemMetadataReadView Ammo =
        FGridReceptacleItemMetadataReadModel::Resolve(
            Registry.Get(), Old.Get(), Old->ItemDefinitionId, 0, false);
    TestTrue(TEXT("Persisted canonical arrows N0 valid"), Ammo.bResolved && Ammo.bCanonicalIdentity);
    TestEqual(TEXT("Ammo stays fixed N0"), Ammo.FixedItemLevel, 0);
    TestTrue(TEXT("Ammo mass from canonical"), FMath::IsNearlyEqual(Ammo.UnitWeightKg, Arrows.WeightKg));
    TestEqual(TEXT("Ammo name from canonical"), Ammo.DisplayName.ToString(),
        Arrows.DisplayName.ToString());
    TestFalse(TEXT("No forged N1 for non-armament initial content"),
        FGridReceptacleItemMetadataReadModel::Resolve(
            Registry.Get(), Old.Get(), Old->ItemDefinitionId, 1, true).bResolved);

    Old->ItemDefinitionId = TEXT("LegacyReceptacle_AI");
    Old->bUseWeaponCatalogue = false;
    Old->Weight = 7.5f;
    const FGridReceptacleItemMetadataReadView Legacy =
        FGridReceptacleItemMetadataReadModel::Resolve(
            Registry.Get(), Old.Get(), Old->ItemDefinitionId, 0, false);
    TestTrue(TEXT("Nonregistered initial item still resolves"), Legacy.bResolved);
    TestFalse(TEXT("Nonregistered stays legacy"), Legacy.bCanonicalIdentity);
    TestEqual(TEXT("Legacy level N0 preserved"), Legacy.FixedItemLevel, 0);
    TestTrue(TEXT("Legacy unit mass preserved"), FMath::IsNearlyEqual(Legacy.UnitWeightKg, 7.5f));
    TestEqual(TEXT("Legacy display text preserved"), Legacy.DisplayName.ToString(),
        Old->DisplayName.ToString());
    const FGridReceptacleItemMetadataReadView IdOnlyLegacy =
        FGridReceptacleItemMetadataReadModel::Resolve(
            Registry.Get(), nullptr, TEXT("LegacyIdOnly_AI"), 12, false);
    TestTrue(TEXT("Old ID-only content still allowed"), IdOnlyLegacy.bResolved);
    TestEqual(TEXT("Old ID-only fixed level preserved"), IdOnlyLegacy.FixedItemLevel, 12);
    TestFalse(TEXT("Mismatched definition identity never borrowed"),
        FGridReceptacleItemMetadataReadModel::Resolve(
            Registry.Get(), Old.Get(), TEXT("Weapon_StraightSword_01"), 6, false).bResolved);
    TestEqual(TEXT("Read model never edits definition default"), Old->DefaultWeaponItemLevel, 6);
    return true;
}

#endif
