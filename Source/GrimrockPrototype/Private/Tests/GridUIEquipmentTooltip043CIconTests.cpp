#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "UI/GridItemTooltipWeaponLineWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEquipmentTooltip043CIconAliasTest,
    "Grimrock.UI.EquipmentTooltip043C.IconAliasCompatibility",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEquipmentTooltip043CIconAliasTest::RunTest(const FString&)
{
    using FLine = UGridItemTooltipWeaponLineWidget;

    // Global previews display effective remaining armor, while the historical
    // UI-ITEM01 class-defaults map uses the nominal Armor key.
    TestEqual(TEXT("Global effective armor reuses legacy Armor texture key"),
        FLine::GetFallbackIconStatId(TEXT("CurrentPhysicalArmor")), FName(TEXT("Armor")));
    TestEqual(TEXT("Catalogued physical armor reuses legacy Armor texture key"),
        FLine::GetFallbackIconStatId(TEXT("WeaponPhysicalArmor")), FName(TEXT("Armor")));

    // Existing icon mappings, resistances and weapon rows remain direct keys:
    // no invented texture for numeric damage, AP or unrelated stats.
    const FName DirectIds[] = {
        FName(TEXT("Armor")), FName(TEXT("Strength")), FName(TEXT("Dexterity")),
        FName(TEXT("Constitution")), FName(TEXT("Intelligence")), FName(TEXT("Wisdom")),
        FName(TEXT("Charisma")), FName(TEXT("MaxHealth")), FName(TEXT("MaxMana")),
        FName(TEXT("CarryWeight")), FName(TEXT("PhysicalResistance")),
        FName(TEXT("FireResistance")), FName(TEXT("IceResistance")),
        FName(TEXT("LightningResistance")), FName(TEXT("PoisonResistance")),
        FName(TEXT("HolyResistance")), FName(TEXT("NecroticResistance")),
        FName(TEXT("ArcaneResistance")), FName(TEXT("WeaponAP")),
        FName(TEXT("WeaponMinDamage")), FName(TEXT("WeaponMaxDamage")),
        FName(TEXT("WeaponDamageType")), FName() // Explicit empty name, not EName.
    };
    for (const FName& DirectId : DirectIds)
    {
        TestTrue(TEXT("No fallback for a direct/unmapped stat id"),
            FLine::GetFallbackIconStatId(DirectId).IsNone());
    }
    return true;
}
#endif // WITH_DEV_AUTOMATION_TESTS
