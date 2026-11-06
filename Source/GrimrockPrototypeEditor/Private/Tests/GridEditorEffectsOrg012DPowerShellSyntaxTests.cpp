#if WITH_DEV_AUTOMATION_TESTS && PLATFORM_WINDOWS

#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformProcess.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorEffectsOrg012DPowerShellSyntaxTest,
    "Grimrock.Editor.EffectsOrg01.TorchMigrationScriptsSyntax",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorEffectsOrg012DPowerShellSyntaxTest::RunTest(const FString&)
{
    // Run only the PowerShell 5.1 parser: the migration itself is never
    // executed, and no files are changed by this regression test.
    const FString SystemRoot = FPlatformMisc::GetEnvironmentVariable(TEXT("SystemRoot"));
    const FString PowerShell = FPaths::Combine(
        SystemRoot, TEXT("System32/WindowsPowerShell/v1.0/powershell.exe"));
    const FString Script = FPaths::Combine(
        FPaths::ProjectDir(), TEXT("Scripts/CheckEffectsOrgMigrationScripts.ps1"));

    if (!TestTrue(TEXT("Windows PowerShell 5.1 exists"), FPaths::FileExists(PowerShell)) ||
        !TestTrue(TEXT("Read-only syntax checker exists"), FPaths::FileExists(Script)))
    {
        return false;
    }

    const FString Arguments = FString::Printf(
        TEXT("-NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File \"%s\""),
        *Script);
    int32 ExitCode = -1;
    FString StdOut;
    FString StdErr;
    const bool bStarted = FPlatformProcess::ExecProcess(
        *PowerShell, *Arguments, &ExitCode, &StdOut, &StdErr);

    if (!TestTrue(TEXT("Windows PowerShell syntax checker started"), bStarted))
    {
        return false;
    }
    return TestEqual(TEXT("Both standalone migration scripts parse under Windows PowerShell 5.1"),
        ExitCode, 0);
}

#endif // WITH_DEV_AUTOMATION_TESTS && PLATFORM_WINDOWS
