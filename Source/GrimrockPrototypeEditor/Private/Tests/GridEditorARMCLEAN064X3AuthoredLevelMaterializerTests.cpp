#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ARMEMENT/GridArmClean064X3AuthoredLevelMaterializer.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridArmClean064X3AuthoredLevelRules,
    "Grimrock.RPG.ARM.CLEAN06_4X3.AuthoredLevelMaterializationRules",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridArmClean064X3AuthoredLevelRules::RunTest(const FString&)
{
    TStrongObjectPtr<UGridArmamentDefinitionAsset> Weapon(
        NewObject<UGridArmamentDefinitionAsset>(GetTransientPackage()));
    TStrongObjectPtr<UGridNonArmamentDefinitionAsset> Generic(
        NewObject<UGridNonArmamentDefinitionAsset>(GetTransientPackage()));
    if (!TestNotNull(TEXT("Transient ARM definition"), Weapon.Get()) ||
        !TestNotNull(TEXT("Transient non-ARM definition"), Generic.Get()))
    {
        return false;
    }
    Weapon->ItemId = TEXT("X3_SourceWeapon");
    Weapon->ArchetypeId = TEXT("X3_ValidArchetype");
    Generic->ItemId = TEXT("X3_SourceGeneric");

    auto Try = [](const UGridItemDefinitionAsset* Item,
        int32 InputLevel, int32 HistoricalDefault,
        int32& OutLevel, bool& bInherited) -> bool
    {
        return FGridArmClean064X3AuthoredLevelMaterializer::TryResolve(
            Item, InputLevel, HistoricalDefault, OutLevel, bInherited);
    };

    int32 Level = INDEX_NONE;
    bool bInherited = false;

    if (!TestTrue(TEXT("Authored ARM N0 uses actual historical N7"),
        Try(Weapon.Get(), 0, 7, Level, bInherited)) ||
        !TestEqual(TEXT("Inherited ARM physical level exactly N7"), Level, 7) ||
        !TestTrue(TEXT("N0 inheritance is explicitly reported"), bInherited))
    {
        return false;
    }

    if (!TestTrue(TEXT("Explicit ARM N1 remains N1"),
        Try(Weapon.Get(), 1, 19, Level, bInherited)) ||
        !TestEqual(TEXT("Explicit N1 value"), Level, 1) ||
        !TestFalse(TEXT("Explicit N1 was not inherited"), bInherited))
    {
        return false;
    }

    if (!TestTrue(TEXT("Explicit ARM N20 remains N20"),
        Try(Weapon.Get(), 20, 3, Level, bInherited)) ||
        !TestEqual(TEXT("Explicit N20 value"), Level, 20) ||
        !TestFalse(TEXT("Explicit N20 was not inherited"), bInherited))
    {
        return false;
    }

    if (!TestTrue(TEXT("Non-ARM authored N0 remains physical N0"),
        Try(Generic.Get(), 0, 17, Level, bInherited)) ||
        !TestEqual(TEXT("Non-ARM level"), Level, 0) ||
        !TestFalse(TEXT("Non-ARM never inherits ARM level"), bInherited))
    {
        return false;
    }

    const auto MustReject = [this, &Try](const TCHAR* Reason,
        const UGridItemDefinitionAsset* Item, int32 Authored,
        int32 DefaultLevel) -> bool
    {
        int32 RejectedLevel = 99;
        bool bRejectedInherited = true;
        const bool bResult =
            Try(Item, Authored, DefaultLevel, RejectedLevel, bRejectedInherited);
        return TestFalse(Reason, bResult) &&
            TestEqual(TEXT("Invalid conversion resets its output"), RejectedLevel, INDEX_NONE) &&
            TestFalse(TEXT("Invalid conversion never signals inheritance"),
                bRejectedInherited);
    };

    if (!MustReject(TEXT("ARM N0 cannot inherit nonexistent N0"), Weapon.Get(), 0, 0) ||
        !MustReject(TEXT("ARM N0 cannot inherit N21"), Weapon.Get(), 0, 21) ||
        !MustReject(TEXT("ARM level N21 rejected"), Weapon.Get(), 21, 1) ||
        !MustReject(TEXT("ARM negative level rejected"), Weapon.Get(), -1, 1) ||
        !MustReject(TEXT("Non-ARM level N1 rejected"), Generic.Get(), 1, 1) ||
        !MustReject(TEXT("Non-ARM level N20 rejected"), Generic.Get(), 20, 1) ||
        !MustReject(TEXT("Null canonical definition rejected"), nullptr, 0, 1))
    {
        return false;
    }

    Weapon->ArchetypeId = NAME_None;
    if (!MustReject(TEXT("ARM missing archetype rejected"), Weapon.Get(), 0, 1))
    {
        return false;
    }

    Weapon->ArchetypeId = TEXT("X3_ValidArchetype");
    Weapon->ItemId = NAME_None;
    if (!MustReject(TEXT("ARM missing ItemId rejected"), Weapon.Get(), 0, 1))
    {
        return false;
    }

    AddInfo(TEXT("[ARM-CLEAN06.4X3] Authored level policy proved; Editor only, no native binary or SaveGame writes."));
    return true;
}

#endif
