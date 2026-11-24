#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "../Runtime/GridReceptacleItemMetadataReadModel.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean063AIReceptacleMaterialization,
    "Grimrock.RPG.ARM.CLEAN06_3.CanonicalReceptacleMaterialization",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean063AIReceptacleMaterialization::RunTest(const FString&)
{
    TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_ItemSystem.DA_ItemSystem")));
    if (!TestNotNull(TEXT("Real ItemSystem"), Root.Get())) return false;
    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
    TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
    TArray<FString> Errors;
    if (!TestTrue(TEXT("Register canonical ItemSystem"), Registry->RegisterItemSystem(Root.Get(), Errors)))
    {
        for (const FString& Error : Errors) AddError(Error);
        return false;
    }

    FName FixtureId;
    const FName Arms[] = {
        TEXT("Weapon_StraightSword_01"), TEXT("Weapon_GreatAxe_01"),
        TEXT("Weapon_RoundShield_01"), TEXT("Weapon_HolyAegis_01") };
    for (const FName Id : Arms)
    {
        FixtureId = Id;
        for (const int32 Level : {1, 20})
        {
            FGridResolvedItem Canonical;
            if (!TestTrue(TEXT("Actual canonical N1/N20 exists"),
                Registry->ResolveItem(Id, Level, Canonical))) return false;
            const FGridReceptacleItemMetadataReadView View =
                FGridReceptacleItemMetadataReadModel::Resolve(
                    Registry.Get(), Id, Level);
            TestTrue(TEXT("Registered fixed contained ARM resolves"), View.bRegisteredIdentity && View.bResolved);
            TestEqual(TEXT("Never rewrite a fixed persisted level"), View.FixedItemLevel, Level);
            TestTrue(TEXT("Contained unit weight equals canonical"), FMath::IsNearlyEqual(
                View.UnitWeightKg, Canonical.WeightKg));
            TestEqual(TEXT("Contained display name canonical"),
                View.DisplayName.ToString(), Canonical.DisplayName.ToString());
        }
        const FGridReceptacleItemMetadataReadView Fresh =
            FGridReceptacleItemMetadataReadModel::Resolve(
                Registry.Get(), Id, 0);
        TestTrue(TEXT("Malformed authored ARM still classified canonical"), Fresh.bRegisteredIdentity);
        TestFalse(TEXT("Fresh registered ARM N0 cannot inherit obsolete N6"), Fresh.bResolved);
        TestEqual(TEXT("Rejected authored level exposes no fallback mass"), Fresh.UnitWeightKg, 0.0f);
        const FGridReceptacleItemMetadataReadView Persisted =
            FGridReceptacleItemMetadataReadModel::Resolve(
                Registry.Get(), Id, 0);
        TestTrue(TEXT("Persisted invalid ARM N0 remains canonical"), Persisted.bRegisteredIdentity);
        TestFalse(TEXT("Cursor and save restoration never invent N1"), Persisted.bResolved);
        TestTrue(TEXT("Refused saved N0 has no old name"), Persisted.DisplayName.IsEmpty());
        TestEqual(TEXT("Refused saved N0 has no old mass"), Persisted.UnitWeightKg, 0.0f);
        const FGridReceptacleItemMetadataReadView IdOnly =
            FGridReceptacleItemMetadataReadModel::Resolve(
                Registry.Get(), Id, 20);
        TestTrue(TEXT("Canonical fixed level can resolve without old pointer"), IdOnly.bResolved);
        TestEqual(TEXT("Id-only valid level preserved"), IdOnly.FixedItemLevel, 20);
        TestFalse(TEXT("Id-only N0 cannot inherit an absent default"),
            FGridReceptacleItemMetadataReadModel::Resolve(
                Registry.Get(), Id, 0).bResolved);
    }

    FixtureId = TEXT("Weapon_RoundShield_01");
    TestFalse(TEXT("Invalid authored ARM default fails without legacy fallback"),
        FGridReceptacleItemMetadataReadModel::Resolve(
            Registry.Get(), FixtureId, 0).bResolved);
    TestFalse(TEXT("Known ARM N21 rejected"),
        FGridReceptacleItemMetadataReadModel::Resolve(
            Registry.Get(), FixtureId, 21).bResolved);

    FixtureId = TEXT("Item_Arrows_01");
    FGridResolvedItem Arrows;
    if (!TestTrue(TEXT("Real arrows N0"), Registry->ResolveItem(FixtureId, 0, Arrows)))
        return false;
    const FGridReceptacleItemMetadataReadView Ammo =
        FGridReceptacleItemMetadataReadModel::Resolve(
            Registry.Get(), FixtureId, 0);
    TestTrue(TEXT("Persisted canonical arrows N0 valid"), Ammo.bResolved && Ammo.bRegisteredIdentity);
    TestEqual(TEXT("Ammo stays fixed N0"), Ammo.FixedItemLevel, 0);
    TestTrue(TEXT("Ammo mass from canonical"), FMath::IsNearlyEqual(Ammo.UnitWeightKg, Arrows.WeightKg));
    TestEqual(TEXT("Ammo name from canonical"), Ammo.DisplayName.ToString(),
        Arrows.DisplayName.ToString());
    TestFalse(TEXT("No forged N1 for non-armament initial content"),
        FGridReceptacleItemMetadataReadModel::Resolve(
            Registry.Get(), FixtureId, 1).bResolved);

    FixtureId = TEXT("LegacyReceptacle_AI");
    const FGridReceptacleItemMetadataReadView Legacy =
        FGridReceptacleItemMetadataReadModel::Resolve(
            Registry.Get(), FixtureId, 0);
    TestFalse(TEXT("Configured registry rejects unregistered initial item"), Legacy.bResolved);
    TestFalse(TEXT("Unregistered identity not canonical"), Legacy.bRegisteredIdentity);
    TestEqual(TEXT("Rejected source exposes no mass"), Legacy.UnitWeightKg, 0.f);
    TestTrue(TEXT("Rejected source exposes no old display name"), Legacy.DisplayName.IsEmpty());
    const auto FixtureOnly = FGridReceptacleItemMetadataReadModel::Resolve(
        nullptr, FixtureId, 0);
    TestTrue(TEXT("Registry-less source exposes no mass or metadata"),
        !FixtureOnly.bResolved && !FixtureOnly.bRegisteredIdentity && FixtureOnly.UnitWeightKg == 0.f);
    const FGridReceptacleItemMetadataReadView IdOnlyLegacy =
        FGridReceptacleItemMetadataReadModel::Resolve(
            Registry.Get(), TEXT("LegacyIdOnly_AI"), 12);
    TestFalse(TEXT("Configured registry rejects unknown ID-only content"), IdOnlyLegacy.bResolved);
    // Registered physical identity wins over an obsolete definition.
    FGridResolvedItem RealSword;
    if (!TestTrue(TEXT("Real N6 sword is in the canonical registry"),
        Registry->ResolveItem(TEXT("Weapon_StraightSword_01"), 6, RealSword))) return false;
    const auto RegisteredMismatch = FGridReceptacleItemMetadataReadModel::Resolve(
        Registry.Get(), TEXT("Weapon_StraightSword_01"), 6);
    TestTrue(TEXT("Obsolete definition cannot cancel a registered N6 sword"),
        RegisteredMismatch.bRegisteredIdentity && RegisteredMismatch.bResolved &&
        RegisteredMismatch.FixedItemLevel == 6 &&
        FMath::IsNearlyEqual(RegisteredMismatch.UnitWeightKg, RealSword.WeightKg));
    TestEqual(TEXT("Registered mismatch still uses canonical display name"),
        RegisteredMismatch.DisplayName.ToString(), RealSword.DisplayName.ToString());
    TestFalse(TEXT("Unregistered mismatch still rejected"),
        FGridReceptacleItemMetadataReadModel::Resolve(
            Registry.Get(), TEXT("AI_UnregisteredOther"), 0).bResolved);
    return true;
}

#endif
