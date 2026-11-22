#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGStartingEquipmentPresetCatalog.h"
#include "Tests/GridCanonicalRegistryFixture.h"
#include "UObject/StrongObjectPtr.h"

// Preserve the original Automation route while validating the real
// three-per-class production recipes. Historical two-kit DataAssets are no
// longer a required source for this regression test.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean065H6EditorStarterParity,
    "Grimrock.RPG.ARM.CLEAN06_5H.EditorOnlyHistoricalTwoKitParity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean065H6EditorStarterParity::RunTest(const FString&)
{
    FGridCanonicalRegistryFixture Fixture;
    if (!TestTrue(TEXT("Registered canonical ItemSystem"), Fixture.bReady))
        return false;
    const TCHAR* Classes[] = {
        TEXT("Warrior"), TEXT("Rogue"), TEXT("Ranger"),
        TEXT("Mage"), TEXT("Priest"), TEXT("Alchemist")
    };
    int32 ApprovedKits = 0;
    TSet<FName> UniqueKits;
    for (const TCHAR* ClassName : Classes)
    {
        const FString Path = FString::Printf(
            TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_%s.DA_Class_%s"),
            ClassName, ClassName);
        URPGClassAsset* Original = LoadObject<URPGClassAsset>(nullptr, *Path);
        if (!TestNotNull(TEXT("Production class exists"), Original) ||
            !TestTrue(TEXT("Production class whitelist stays valid"),
                FRPGStartingEquipmentPresetCatalog::IsProductionClassAsset(Original)))
            return false;
        const int32 Before = Original->StartingEquipmentKits.Num();
        const FName OldDefault = Original->DefaultStartingEquipmentKitId;
        const bool bOriginalDirty = Original->GetOutermost()->IsDirty();
        TStrongObjectPtr<URPGClassAsset> Preview(
            DuplicateObject<URPGClassAsset>(Original, GetTransientPackage()));
        FString Error;
        if (!TestTrue(TEXT("Canonical approved preview is available without historic DA_Item"),
            Preview.IsValid() &&
            FRPGStartingEquipmentPresetCatalog::TryPopulateApprovedTransientClass(
                *Preview.Get(), Error, Fixture.Registry.Get())))
        {
            AddError(Error);
            return false;
        }
        if (!TestEqual(TEXT("Exactly three canonical kits per class"),
            Preview->StartingEquipmentKits.Num(), 3))
            return false;
        for (const FRPGStartingEquipmentKit& Kit : Preview->StartingEquipmentKits)
        {
            TestFalse(TEXT("One globally unique physical kit identity"),
                UniqueKits.Contains(Kit.KitId));
            UniqueKits.Add(Kit.KitId);
            TestTrue(TEXT("All starter kits contain actual items"), !Kit.Items.IsEmpty());
            for (const FRPGStartingEquipmentEntry& Entry : Kit.Items)
            {
                TestFalse(TEXT("No legacy item authority in an approved recipe"),
                    Entry.ItemDefinition.Get() != nullptr);
                TestFalse(TEXT("Stable canonical physical ItemId"),
                    Entry.ItemId.IsNone());
                FGridResolvedItem Item;
                TestTrue(TEXT("Exact canonical starter identity and level resolve"),
                    Fixture.Resolve(Entry.ItemId, Entry.ItemLevel, Item));
                TestTrue(TEXT("Quantity is bounded by the native stack policy"),
                    Entry.Quantity >= 1 && Entry.Quantity <= Item.MaxStack);
            }
        }
        TestEqual(TEXT("Original class kit data remains unchanged"),
            Original->StartingEquipmentKits.Num(), Before);
        TestEqual(TEXT("Original class default remains unchanged"),
            Original->DefaultStartingEquipmentKitId, OldDefault);
        TestEqual(TEXT("No original class package modification"),
            Original->GetOutermost()->IsDirty(), bOriginalDirty);
        ApprovedKits += Preview->StartingEquipmentKits.Num();
    }
    TestEqual(TEXT("All eighteen approved native starter kits"), ApprovedKits, 18);
    TestEqual(TEXT("No duplicate native kit identifiers"), UniqueKits.Num(), 18);
    return !HasAnyErrors();
}

#endif
