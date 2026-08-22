#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
    bool LoadGameInstanceSource(FString& OutText)
    {
        const FString Path = FPaths::Combine(
            FPaths::ProjectDir(),
            TEXT("Source/GrimrockPrototype/Private/Runtime/GrimrockGameInstance.cpp"));
        return FFileHelper::LoadFileToString(OutText, *Path);
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGridMON1893SaveSlotDiagnosticsTest,
    "Grimrock.Magic.MON18.9.3.SaveSlotDiagnostics",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON1893SaveSlotDiagnosticsTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    FString Source;
    TestTrue(TEXT("GrimrockGameInstance source loads"), LoadGameInstanceSource(Source));
    TestTrue(TEXT("Rejected slot probes carry the MON18.9.3 marker"),
        Source.Contains(TEXT("[MON18.9.3] SlotProbe")));
    TestTrue(TEXT("Rejected slot probes identify slot and user index"),
        Source.Contains(TEXT("Slot=%s UserIndex=%d Result=Rejected")));
    TestTrue(TEXT("Load failure reason is explicit"),
        Source.Contains(TEXT("Reason=LoadFailedOrWrongClass")));
    TestTrue(TEXT("Compatibility failure reason is explicit"),
        Source.Contains(TEXT("Reason=IncompatibleSave")));
    TestTrue(TEXT("Inventory-state failure reason is explicit"),
        Source.Contains(TEXT("Reason=PartyInventoryStateNotLoadable")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGridMON1893CheckpointIsolationTest,
    "Grimrock.Magic.MON18.9.3.CheckpointIsolation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON1893CheckpointIsolationTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    FString Source;
    TestTrue(TEXT("GrimrockGameInstance source loads"), LoadGameInstanceSource(Source));
    TestTrue(TEXT("Primary manual slot remains configured"),
        Source.Contains(TEXT("ConfiguredPartySaveSlotNames.Add(TEXT(\"GrimrockParty\"))")));
    TestTrue(TEXT("Second manual slot remains configured"),
        Source.Contains(TEXT("ConfiguredPartySaveSlotNames.Add(TEXT(\"GrimrockParty_2\"))")));
    TestTrue(TEXT("Third manual slot remains configured"),
        Source.Contains(TEXT("ConfiguredPartySaveSlotNames.Add(TEXT(\"GrimrockParty_3\"))")));
    TestFalse(TEXT("Pre-combat checkpoint is not exposed as a configured manual slot"),
        Source.Contains(TEXT("ConfiguredPartySaveSlotNames.Add(TEXT(\"GrimrockParty_AutoCombat\"))")) ||
        Source.Contains(TEXT("ConfiguredPartySaveSlotNames.Add(TEXT(\"_AutoCombat\"))")));
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
