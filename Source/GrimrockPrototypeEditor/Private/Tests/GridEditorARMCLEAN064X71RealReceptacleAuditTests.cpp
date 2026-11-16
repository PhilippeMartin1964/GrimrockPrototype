#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ARMEMENT/GridArmClean064X71ReceptacleRulesCommandlet.h"
#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean064X71RealReceptacleAudit,
    "Grimrock.RPG.ARM.CLEAN06_4X71.RealReceptacleMigrationAuditOnly",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean064X71RealReceptacleAudit::RunTest(const FString&)
{
    UGridArmClean064X71ReceptacleRulesCommandlet* Commandlet =
        NewObject<UGridArmClean064X71ReceptacleRulesCommandlet>(GetTransientPackage());
    if (!TestNotNull(TEXT("Audit-only commandlet"), Commandlet)) return false;
    const FString Output = FPaths::CreateTempFilename(
        *FPaths::ProjectSavedDir(), TEXT("ARM_CLEAN064X71_"), TEXT(".json"));
    const FString Arguments = FString::Printf(TEXT("-Audit -Output=\"%s\""), *Output);
    const int32 Code = Commandlet->Main(Arguments);
    FString Data;
    const bool bRead = FFileHelper::LoadFileToString(Data, *Output);
    IFileManager::Get().Delete(*Output, false, true);
    if (!TestEqual(TEXT("Audit completes without binary writes"), Code, 0) ||
        !TestTrue(TEXT("Audit JSON report exists"), bRead)) return false;

    TSharedPtr<FJsonObject> Report;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Data);
    if (!TestTrue(TEXT("Report is valid JSON"),
        FJsonSerializer::Deserialize(Reader, Report) && Report.IsValid()))
        return false;
    return TestEqual(TEXT("Audit schema"),
            Report->GetStringField(TEXT("schema")),
            FString(TEXT("ARM-CLEAN06.4X7/real-receptacle-acceptance/v1"))) &&
        TestEqual(TEXT("Only Audit mode used"), Report->GetStringField(TEXT("mode")),
            FString(TEXT("Audit"))) &&
        TestEqual(TEXT("Exactly two objects"), Report->GetIntegerField(TEXT("targets")), 2) &&
        TestEqual(TEXT("Zero native packages written"),
            Report->GetIntegerField(TEXT("packages_saved")), 0) &&
        TestEqual(TEXT("Four real levels"), Report->GetIntegerField(TEXT("levels")), 4) &&
        TestEqual(TEXT("Forty-four definitions"),
            Report->GetIntegerField(TEXT("definitions")), 44) &&
        TestEqual(TEXT("Two rules"), Report->GetIntegerField(TEXT("receptacle_rules")), 2) &&
        TestEqual(TEXT("No overrides"), Report->GetIntegerField(TEXT("overrides")), 0) &&
        TestTrue(TEXT("Historical pointers retained"),
            Report->GetBoolField(TEXT("legacy_pointers_retained"))) &&
        TestFalse(TEXT("No player SaveGame touched"),
            Report->GetBoolField(TEXT("savegames_opened_or_written")));
}
#endif
