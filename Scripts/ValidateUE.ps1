param(
    [string]$EngineRoot = $env:UE_ROOT,
    [string]$AutomationFilter,
    [string]$ReportRoot,
    [switch]$SkipBuild,
    [switch]$BuildShipping,
    [switch]$SkipAutomation,
    [switch]$UseRHI,
    [switch]$ShowAutomationOutput
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$RepoRoot = Split-Path -Parent $PSScriptRoot
$ProjectFile = Join-Path $RepoRoot 'GrimrockPrototype.uproject'
$EditorTarget = 'GrimrockPrototypeEditor'
$Platform = 'Win64'
$Configuration = 'Development'

# VALIDATION-AUTO01.12: a private session stage with a fixed, public-safe
# vocabulary. The stage file contains no paths, errors, or environment values.
function Set-GrimrockValidationStage
{
    param(
        [ValidateSet('PREFLIGHT', 'ROLLBACK_FIXTURE', 'ROLLBACK_RESTORE', 'ROLLBACK_NEW_PACKAGE',
            'ROLLBACK_REFUSALS', 'ROLLBACK_SAVE_REPLACE', 'ROLLBACK_RESULT',
            'BUILD_STARTED', 'BUILD_FAILED', 'BUILD_OK',
            'AUTOMATION_STARTED', 'AUTOMATION_PROCESS_EXITED',
            'AUTOMATION_REPORT_MISSING', 'AUTOMATION_REPORT_READING',
            'AUTOMATION_REPORT_OK', 'AUTOMATION_NO_TESTS',
            'AUTOMATION_TESTS_FAILED', 'AUTOMATION_EXIT_NONZERO', 'SUCCESS')]
        [string]$Stage
    )
    if ([string]::IsNullOrWhiteSpace($ReportRoot)) { return }
    if (-not (Test-Path -LiteralPath $ReportRoot -PathType Container))
    {
        New-Item -ItemType Directory -Force -Path $ReportRoot | Out-Null
    }
    Set-Content -LiteralPath (Join-Path $ReportRoot 'Validation.stage') -Value $Stage -Encoding ASCII
}

# Byte-for-byte observation only; never print slot names or hashes.
function Get-GrimrockLiveSaveInventory
{
    $Directory = Join-Path $RepoRoot 'Saved\SaveGames'
    if (-not (Test-Path -LiteralPath $Directory)) { return }
    $RootItem = Get-Item -LiteralPath $Directory -Force
    if (-not $RootItem.PSIsContainer -or ($RootItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw 'Live SaveGames is not an ordinary directory.'
    }
    foreach ($File in @(Get-ChildItem -LiteralPath $Directory -Force | Sort-Object Name)) {
        if ($File.PSIsContainer -or ($File.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw 'Unexpected directory or reparse point in live SaveGames.'
        }
        if ($File.Extension -ieq '.sav') {
            [pscustomobject]@{ Name=$File.Name; Bytes=$File.Length; Sha256=(Get-FileHash -LiteralPath $File.FullName -Algorithm SHA256).Hash }
        }
    }
}

function Resolve-GrimrockEngineRoot
{
    param([string]$RequestedRoot)

    if ([string]::IsNullOrWhiteSpace($RequestedRoot))
    {
        throw 'Racine Unreal Engine non renseignee. Utilisez -EngineRoot ou definissez la variable d''environnement UE_ROOT.'
    }

    if (-not (Test-Path -LiteralPath $RequestedRoot -PathType Container))
    {
        throw "Racine Unreal Engine introuvable : $RequestedRoot"
    }

    return (Resolve-Path -LiteralPath $RequestedRoot).Path
}

function Assert-FileExists
{
    param(
        [string]$Path,
        [string]$Description
    )

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf))
    {
        throw "$Description introuvable : $Path"
    }
}

function Invoke-GrimrockNativeStep
{
    param(
        [string]$Label,
        [string]$Executable,
        [string[]]$Arguments
    )

    Write-Host ''
    Write-Host "=== $Label ==="
    Write-Host "Executable : $Executable"
    Write-Host ('Arguments  : ' + ($Arguments -join ' '))

    & $Executable @Arguments
    $ExitCode = $LASTEXITCODE
    if ($ExitCode -ne 0)
    {
        Set-GrimrockValidationStage -Stage 'BUILD_FAILED'
        throw "$Label a echoue avec le code $ExitCode."
    }
}

function Get-GrimrockAutomationDiagnosticLines
{
    param(
        [string]$LogPath,
        [int]$FailedCount,
        [int]$WarningCount
    )

    $Lines = @()
    if (-not (Test-Path -LiteralPath $LogPath -PathType Leaf))
    {
        return $Lines
    }

    if ($FailedCount -gt 0)
    {
        $Lines += Get-Content -LiteralPath $LogPath | Where-Object {
            $_ -match 'LogAutomationController: Error:' -or
            $_ -match 'LogAutomationCommandLine: Display: \*\*\*\* TEST COMPLETE\. EXIT CODE:'
        }
    }

    if ($WarningCount -gt 0)
    {
        $Lines += Get-Content -LiteralPath $LogPath | Where-Object {
            $_ -match 'LogAutomationController: Warning:'
        }
    }

    return @($Lines | Select-Object -Unique)
}

function Write-GrimrockAutomationSummary
{
    param(
        [string]$SummaryPath,
        [string]$Filter,
        [int]$Succeeded,
        [int]$SucceededWithWarnings,
        [int]$Failed,
        [int]$NotRun,
        [int]$ProcessExitCode,
        [string]$ReportPath,
        [string]$LogPath,
        [string]$ConsoleLogPath
    )

    $SummaryLines = @(
        '=== Automation summary ===',
        "Filter                 : $Filter",
        "Succeeded              : $Succeeded",
        "Succeeded with warnings: $SucceededWithWarnings",
        "Failed                 : $Failed",
        "Not run                : $NotRun",
        "Process exit code      : $ProcessExitCode",
        "Report                 : $ReportPath",
        "Log                    : $LogPath",
        "Console log            : $ConsoleLogPath",
        "Summary                : $SummaryPath"
    )

    $Diagnostics = @(Get-GrimrockAutomationDiagnosticLines -LogPath $LogPath -FailedCount $Failed -WarningCount $SucceededWithWarnings)
    if ($Diagnostics.Count -gt 0)
    {
        $SummaryLines += ''
        $SummaryLines += '=== Relevant diagnostics ==='
        $SummaryLines += $Diagnostics
    }

    Set-Content -LiteralPath $SummaryPath -Value $SummaryLines -Encoding UTF8

    Write-Host ''
    foreach ($Line in $SummaryLines)
    {
        Write-Host $Line
    }
}

if ($SkipBuild -and $SkipAutomation)
{
    throw 'Rien a valider : -SkipBuild et -SkipAutomation ne peuvent pas etre utilises ensemble.'
}

Set-GrimrockValidationStage -Stage 'PREFLIGHT'
& (Join-Path $PSScriptRoot 'TestARM_CLEAN06_4X66_Rollback.ps1') -Stage { param($Name) Set-GrimrockValidationStage -Stage $Name }
Set-GrimrockValidationStage -Stage 'PREFLIGHT'
$RetirementSelfTestGit=(Get-Command git.exe -CommandType Application -ErrorAction Stop | Select-Object -First 1).Source
$RetirementSelfTestHead=@(& $RetirementSelfTestGit -C $RepoRoot rev-parse HEAD)
if ($LASTEXITCODE -ne 0 -or $RetirementSelfTestHead.Count -ne 1) { throw 'Cannot determine HEAD for retirement audit self-test.' }
& (Join-Path $PSScriptRoot 'AuditARM_CLEAN06_5_Retirement.ps1') -ExpectedHead $RetirementSelfTestHead[0] -SelfTest
& (Join-Path $PSScriptRoot 'PlanARM_CLEAN06_5H12_Retirement.ps1') -SelfTest
& (Join-Path $PSScriptRoot 'PrepareARM_CLEAN06_5H14_IconPaletteSnapshot.ps1') -SelfTest
& (Join-Path $PSScriptRoot 'PreflightARM_CLEAN06_5H15_PaletteContinuity.ps1') -SelfTest
& (Join-Path $PSScriptRoot 'VerifyARM_CLEAN06_5H16_Canary.ps1') -SelfTest
& (Join-Path $PSScriptRoot 'AssertARM_CLEAN06_5H18_ShippingGuards.ps1') -SelfTest
& (Join-Path $PSScriptRoot 'InstallARM_CLEAN06_5B_ReadableSaveCopy.ps1') -SelfTest
& (Join-Path $PSScriptRoot 'TestARM_CLEAN06_5G85_WorldSavePreflight.ps1') -SelfTest
Assert-FileExists -Path $ProjectFile -Description 'Projet Unreal'

$ResolvedEngineRoot = Resolve-GrimrockEngineRoot -RequestedRoot $EngineRoot
$BuildBatch = Join-Path $ResolvedEngineRoot 'Engine\Build\BatchFiles\Build.bat'
$EditorCmd = Join-Path $ResolvedEngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'

Assert-FileExists -Path $BuildBatch -Description 'Unreal Build.bat'
Assert-FileExists -Path $EditorCmd -Description 'UnrealEditor-Cmd.exe'

Write-Host 'GrimrockPrototype TD04.2 - Local UE Validation Harness'
Write-Host "Repository : $RepoRoot"
Write-Host "Project    : $ProjectFile"
Write-Host "Engine     : $ResolvedEngineRoot"

if (-not $SkipBuild)
{
    $BuildArguments = @(
        $EditorTarget,
        $Platform,
        $Configuration,
        "-Project=$ProjectFile",
        '-WaitMutex',
        '-NoHotReloadFromIDE'
    )

    # VALIDATION-AUTO01: the isolated Windows agent selects a complete SDK.
    # Forward it explicitly to UBT; the worktree XML is only a fallback.
    # Interactive builds are unchanged unless this validated env var is set.
    $AgentSdk = [Environment]::GetEnvironmentVariable('GRIMROCK_VALIDATION_WINDOWS_SDK')
    if (-not [string]::IsNullOrWhiteSpace($AgentSdk))
    {
        if ($AgentSdk -notmatch '^10\.0\.\d{4,6}\.\d{1,5}$')
        {
            throw 'VALIDATION-AUTO01: invalid SDK selection passed to build.'
        }
        $BuildArguments += "-WindowsSDKVersion=$AgentSdk"
        Write-Host "[VALIDATION-AUTO01] SDK_SELECTION COMPLETE version=$AgentSdk"
    }

    Set-GrimrockValidationStage -Stage 'BUILD_STARTED'
    Invoke-GrimrockNativeStep -Label 'UE5.5.4 Development Editor build' -Executable $BuildBatch -Arguments $BuildArguments
    Set-GrimrockValidationStage -Stage 'BUILD_OK'
    Write-Host '[OK] Development Editor build.'

    # VALIDATION-AUTO01.SHIPPING01: compile the optimized runtime target in
    # the same isolated worktree; this catches diagnostics masked in Editor.
    # Deliberately no Cook/Stage/Package on every commit.
    if ($BuildShipping)
    {
        $ShippingArguments = @(
            'GrimrockPrototype',
            $Platform,
            'Shipping',
            "-Project=$ProjectFile",
            '-WaitMutex',
            '-NoHotReloadFromIDE'
        )
        if (-not [string]::IsNullOrWhiteSpace($AgentSdk))
        {
            $ShippingArguments += "-WindowsSDKVersion=$AgentSdk"
        }
        Set-GrimrockValidationStage -Stage 'BUILD_STARTED'
        Invoke-GrimrockNativeStep -Label 'UE5.5.4 Shipping runtime build' -Executable $BuildBatch -Arguments $ShippingArguments
        Set-GrimrockValidationStage -Stage 'BUILD_OK'
        Write-Host '[OK] Shipping runtime build (no Cook/Stage/Package).'
    }
}

if (-not $SkipAutomation)
{
    if ([string]::IsNullOrWhiteSpace($AutomationFilter))
    {
        throw 'AutomationFilter est obligatoire sauf avec -SkipAutomation. Exemple : -AutomationFilter "Grimrock.TechnicalDebt.TD03_4.DungeonLevelsDetails.ApplyCurrentLevelContract"'
    }

    if ([string]::IsNullOrWhiteSpace($ReportRoot))
    {
        $ReportRoot = Join-Path $RepoRoot 'Saved\Automation\TD04'
    }

    $Timestamp = Get-Date -Format 'yyyyMMdd-HHmmss'
    $SessionName = 'TD04-' + $Timestamp
    $SessionReportPath = Join-Path $ReportRoot $SessionName
    $AutomationLogPath = Join-Path $SessionReportPath 'Automation.log'

    New-Item -ItemType Directory -Path $SessionReportPath -Force | Out-Null

    # Unique UE user directory per process, not a save filename exception.
    # Keep the isolated output in the private report directory for diagnosis.
    $IsolationRoot = [IO.Path]::GetFullPath((Join-Path $SessionReportPath ('GrimrockAutomation-'+[guid]::NewGuid().ToString('N'))))
    if (Test-Path -LiteralPath $IsolationRoot) { throw 'Automation isolation directory collision.' }
    $LiveSaveRoot = [IO.Path]::GetFullPath((Join-Path $RepoRoot 'Saved\SaveGames')).TrimEnd('\','/')
    if ($IsolationRoot.Equals($LiveSaveRoot,[StringComparison]::OrdinalIgnoreCase) -or
        $IsolationRoot.StartsWith(($LiveSaveRoot+'\'),[StringComparison]::OrdinalIgnoreCase)) {
        throw 'Automation report root must not be in live SaveGames.'
    }
    New-Item -ItemType Directory -Path $IsolationRoot | Out-Null
    $SaveBefore = ConvertTo-Json -InputObject @(Get-GrimrockLiveSaveInventory) -Compress

    $ExecCommands = "Automation RunTest $AutomationFilter;Quit"
    $AutomationConsolePath = Join-Path $SessionReportPath 'Automation.console.log'
    $AutomationSummaryPath = Join-Path $SessionReportPath 'Automation.summary.txt'
    $EditorArguments = @(
        $ProjectFile,
        "-UserDir=$IsolationRoot",
        "-GrimrockAutomationUserDir=$IsolationRoot",
        '-Unattended',
        '-NoSplash',
        '-NoP4',
        '-NoSound',
        "-ExecCmds=$ExecCommands",
        "-ReportExportPath=$SessionReportPath",
        '-log',
        "-abslog=$AutomationLogPath"
    )

    if (-not $UseRHI)
    {
        $EditorArguments += '-NullRHI'
    }

    Write-Host ''
    Set-GrimrockValidationStage -Stage 'AUTOMATION_STARTED'
    Write-Host '=== UE5.5.4 Automation ==='
    Write-Host "Filter      : $AutomationFilter"
    Write-Host "Report      : $SessionReportPath"
    Write-Host "Full log    : $AutomationLogPath"
    if (-not $ShowAutomationOutput)
    {
        Write-Host "Console log : $AutomationConsolePath"
        Write-Host 'Output      : concise (use -ShowAutomationOutput for live Unreal output)'
    }

    if ($ShowAutomationOutput)
    {
        & $EditorCmd @EditorArguments
        $ConsoleLogForSummary = '<live output; not redirected>'
    }
    else
    {
        & $EditorCmd @EditorArguments *> $AutomationConsolePath
        $ConsoleLogForSummary = $AutomationConsolePath
    }
    $AutomationExitCode = $LASTEXITCODE
    # Numerical and bounded Editor-Cmd exit code; no raw exception strings.
    if (-not [string]::IsNullOrWhiteSpace($ReportRoot) -and
        $AutomationExitCode -ge -2147483648 -and $AutomationExitCode -le 2147483647)
    {
        Set-Content -LiteralPath (Join-Path $ReportRoot 'Automation.exitcode') `
            -Value ([string]$AutomationExitCode) -Encoding ASCII
    }
    Set-GrimrockValidationStage -Stage 'AUTOMATION_PROCESS_EXITED'

    $SaveAfter = ConvertTo-Json -InputObject @(Get-GrimrockLiveSaveInventory) -Compress
    if ($SaveBefore -cne $SaveAfter) { throw 'Live SaveGames changed during Automation; refusing validation.' }
    if (-not (Test-Path -LiteralPath $AutomationLogPath -PathType Leaf) -or
        -not (Select-String -LiteralPath $AutomationLogPath -SimpleMatch '[GrimrockAutomation] Isolated SaveGames verified.' -Quiet)) {
        throw 'Automation process did not prove its isolated Saved directory.'
    }
    $ResidualSaves = @(Get-ChildItem -LiteralPath $IsolationRoot -Recurse -Force -File -Filter '*.sav')
    if ($ResidualSaves.Count -ne 0) {
        throw ('Automation leaked isolated test saves: '+$ResidualSaves.Count+'; private evidence retained.')
    }
    Write-Host '[OK] Automation save isolation; live inventory unchanged; no residual test saves.'

    $IndexJsonPath = Join-Path $SessionReportPath 'index.json'
    if (-not (Test-Path -LiteralPath $IndexJsonPath -PathType Leaf))
    {
        Set-GrimrockValidationStage -Stage 'AUTOMATION_REPORT_MISSING'
        $FallbackLines = @(
            '=== Automation summary ===',
            "Filter                 : $AutomationFilter",
            'Result                 : Automation report was not generated',
            "Process exit code      : $AutomationExitCode",
            "Report                 : $SessionReportPath",
            "Log                    : $AutomationLogPath",
            "Console log            : $ConsoleLogForSummary",
            "Summary                : $AutomationSummaryPath"
        )
        Set-Content -LiteralPath $AutomationSummaryPath -Value $FallbackLines -Encoding UTF8
        Write-Host ''
        $FallbackLines | ForEach-Object { Write-Host $_ }
        throw "Rapport Automation index.json introuvable. Consultez : $AutomationSummaryPath"
    }

    Set-GrimrockValidationStage -Stage 'AUTOMATION_REPORT_READING'
    $Report = Get-Content -LiteralPath $IndexJsonPath -Raw | ConvertFrom-Json
    Set-GrimrockValidationStage -Stage 'AUTOMATION_REPORT_OK'
    $Succeeded = [int]$Report.succeeded
    $SucceededWithWarnings = [int]$Report.succeededWithWarnings
    $Failed = [int]$Report.failed
    $NotRun = [int]$Report.notRun
    $Executed = $Succeeded + $SucceededWithWarnings + $Failed

    Write-GrimrockAutomationSummary `
        -SummaryPath $AutomationSummaryPath `
        -Filter $AutomationFilter `
        -Succeeded $Succeeded `
        -SucceededWithWarnings $SucceededWithWarnings `
        -Failed $Failed `
        -NotRun $NotRun `
        -ProcessExitCode $AutomationExitCode `
        -ReportPath $SessionReportPath `
        -LogPath $AutomationLogPath `
        -ConsoleLogPath $ConsoleLogForSummary

    if ($Executed -le 0)
    {
        Set-GrimrockValidationStage -Stage 'AUTOMATION_NO_TESTS'
        Write-Error "Aucun test Automation n'a ete execute pour le filtre : $AutomationFilter. Resume : $AutomationSummaryPath"
        exit 2
    }

    if ($Failed -gt 0)
    {
        Set-GrimrockValidationStage -Stage 'AUTOMATION_TESTS_FAILED'
        Write-Error "$Failed test(s) Automation en echec. Resume : $AutomationSummaryPath"
        exit 3
    }

    if ($AutomationExitCode -ne 0)
    {
        Set-GrimrockValidationStage -Stage 'AUTOMATION_EXIT_NONZERO'
        Write-Error "UE5.5.4 Automation a retourne le code $AutomationExitCode sans test marque en echec. Resume : $AutomationSummaryPath"
        exit 4
    }

    Write-Host '[OK] Automation filter validated.'
}

Write-Host ''
Write-Host 'TD04.2 validation completed successfully.'
Set-GrimrockValidationStage -Stage 'SUCCESS'

exit 0
