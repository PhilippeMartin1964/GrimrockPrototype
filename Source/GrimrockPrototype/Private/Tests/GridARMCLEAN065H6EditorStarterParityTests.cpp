#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGStartingEquipmentPresetCatalog.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean065H6EditorStarterParity,
    "Grimrock.RPG.ARM.CLEAN06_5H.EditorOnlyHistoricalTwoKitParity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean065H6EditorStarterParity::RunTest(const FString&)
{
    // Moving the old source loader behind WITH_EDITOR must not change the
    // production class whitelist or either transient Editor test recipe.
    const TCHAR* Classes[] = {
        TEXT("Warrior"), TEXT("Rogue"), TEXT("Ranger"),
        TEXT("Mage"), TEXT("Priest"), TEXT("Alchemist")
    };
    int32 LegacyKits = 0, ApprovedKits = 0;
    for (const TCHAR* ClassName : Classes)
    {
        const FString Path = FString::Printf(
            TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_%s.DA_Class_%s"),
            ClassName, ClassName);
        auto* Original = LoadObject<URPGClassAsset>(nullptr, *Path);
        if (!TestNotNull(TEXT("Production class exists"), Original) ||
            !TestTrue(TEXT("Production whitelist based on approved recipes"),
                FRPGStartingEquipmentPresetCatalog::IsProductionClassAsset(Original)))
            return false;
        const int32 Before = Original->StartingEquipmentKits.Num();
        const FName OldDefault = Original->DefaultStartingEquipmentKitId;
        TStrongObjectPtr<URPGClassAsset> Old(
            DuplicateObject<URPGClassAsset>(Original, GetTransientPackage()));
        TStrongObjectPtr<URPGClassAsset> Approved(
            DuplicateObject<URPGClassAsset>(Original, GetTransientPackage()));
        FString Error;
        if (!TestTrue(TEXT("Editor-only historical two-kit preview"),
                Old.IsValid() &&
                FRPGStartingEquipmentPresetCatalog::TryPopulateTransientClass(*Old.Get(), Error)))
        {
            AddError(Error);
            return false;
        }
        if (!TestTrue(TEXT("Automation-only approved preview remains available"),
                Approved.IsValid() &&
                FRPGStartingEquipmentPresetCatalog::TryPopulateApprovedTransientClass(*Approved.Get(), Error)))
        {
            AddError(Error);
            return false;
        }
        TestEqual(TEXT("Historical fixture still has two kits"), Old->StartingEquipmentKits.Num(), 2);
        TestEqual(TEXT("Approved fixture still has three kits"), Approved->StartingEquipmentKits.Num(), 3);
        TestEqual(TEXT("Original production class remains untouched"), Original->StartingEquipmentKits.Num(), Before);
        TestEqual(TEXT("Original default stays unchanged"), Original->DefaultStartingEquipmentKitId, OldDefault);
        LegacyKits += Old->StartingEquipmentKits.Num();
        ApprovedKits += Approved->StartingEquipmentKits.Num();
    }
    TestEqual(TEXT("All 12 historical Editor-only kits"), LegacyKits, 12);
    TestEqual(TEXT("All 18 approved kits"), ApprovedKits, 18);
    return !HasAnyErrors();
}

#endif
