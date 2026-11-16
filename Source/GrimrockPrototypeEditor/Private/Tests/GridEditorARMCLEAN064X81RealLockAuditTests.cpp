#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ARMEMENT/GridArmClean064X81LockRulesCommandlet.h"
#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean064X81RealLockAudit,
    "Grimrock.RPG.ARM.CLEAN06_4X81.RealLockKeyMigrationAuditOnly",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean064X81RealLockAudit::RunTest(const FString&)
{
    UGridArmClean064X81LockRulesCommandlet* Commandlet =
        NewObject<UGridArmClean064X81LockRulesCommandlet>(GetTransientPackage());
    if (!TestNotNull(TEXT("Audit-only lock commandlet"), Commandlet)) return false;
    const FString Output = FPaths::CreateTempFilename(
        *FPaths::ProjectSavedDir(), TEXT("ARM_CLEAN064X81_"), TEXT(".json"));
    const int32 Code = Commandlet->Main(FString::Printf(
        TEXT("-Audit -Output=\"%s\""), *Output));
    FString Data;
    const bool Loaded = FFileHelper::LoadFileToString(Data, *Output);
    IFileManager::Get().Delete(*Output, false, true);
    if (!TestEqual(TEXT("Audit succeeds without native package writes"), Code, 0) ||
        !TestTrue(TEXT("Audit produced JSON report"), Loaded)) return false;

    TSharedPtr<FJsonObject> Report;
    if (!TestTrue(TEXT("Report is valid JSON"),
        FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Data), Report) &&
        Report.IsValid())) return false;
    return TestEqual(TEXT("Contract schema"),
        Report->GetStringField(TEXT("schema")),
        FString(TEXT("ARM-CLEAN06.4X8/real-lock-key-acceptance/v1"))) &&
        TestEqual(TEXT("Mode is only Audit"), Report->GetStringField(TEXT("mode")),
            FString(TEXT("Audit"))) &&
        TestEqual(TEXT("44 native world definitions"), Report->GetIntegerField(TEXT("definitions")), 44) &&
        TestEqual(TEXT("4 real dungeon levels"), Report->GetIntegerField(TEXT("levels")), 4) &&
        TestEqual(TEXT("Two locks"), Report->GetIntegerField(TEXT("lock_lists")), 2) &&
        TestEqual(TEXT("Two pointer sources"), Report->GetIntegerField(TEXT("lock_pointers")), 2) &&
        TestEqual(TEXT("Two already-converted receptacles"), Report->GetIntegerField(TEXT("receptacle_canonical")), 2) &&
        TestEqual(TEXT("Two lock assets only"), Report->GetIntegerField(TEXT("targets")), 2) &&
        TestEqual(TEXT("No instance overrides"), Report->GetIntegerField(TEXT("overrides")), 0) &&
        TestEqual(TEXT("No packages written in Audit"), Report->GetIntegerField(TEXT("packages_saved")), 0) &&
        TestTrue(TEXT("Legacy key pointers preserved"),
            Report->GetBoolField(TEXT("legacy_pointers_retained"))) &&
        TestTrue(TEXT("Lock accepted set preserved"),
            Report->GetBoolField(TEXT("accepted_key_set_unchanged"))) &&
        TestFalse(TEXT("No player SaveGame touched"),
            Report->GetBoolField(TEXT("savegames_opened_or_written")));
}
#endif
