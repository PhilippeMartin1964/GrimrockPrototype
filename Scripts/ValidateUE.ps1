param(
    [string]$EngineRoot = $env:UE_ROOT,
    [string]$AutomationFilter,
    [string]$ReportRoot,
    [switch]$SkipBuild,
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

    Invoke-GrimrockNativeStep -Label 'UE5.5.4 Development Editor build' -Executable $BuildBatch -Arguments $BuildArguments
    Write-Host '[OK] Development Editor build.'
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

    $ExecCommands = "Automation RunTest $AutomationFilter;Quit"
    $AutomationConsolePath = Join-Path $SessionReportPath 'Automation.console.log'
    $AutomationSummaryPath = Join-Path $SessionReportPath 'Automation.summary.txt'
    $EditorArguments = @(
        $ProjectFile,
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

    $IndexJsonPath = Join-Path $SessionReportPath 'index.json'
    if (-not (Test-Path -LiteralPath $IndexJsonPath -PathType Leaf))
    {
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

    $Report = Get-Content -LiteralPath $IndexJsonPath -Raw | ConvertFrom-Json
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
        Write-Error "Aucun test Automation n'a ete execute pour le filtre : $AutomationFilter. Resume : $AutomationSummaryPath"
        exit 2
    }

    if ($Failed -gt 0)
    {
        Write-Error "$Failed test(s) Automation en echec. Resume : $AutomationSummaryPath"
        exit 3
    }

    if ($AutomationExitCode -ne 0)
    {
        Write-Error "UE5.5.4 Automation a retourne le code $AutomationExitCode sans test marque en echec. Resume : $AutomationSummaryPath"
        exit 4
    }

    Write-Host '[OK] Automation filter validated.'
}

Write-Host ''
Write-Host 'TD04.2 validation completed successfully.'
exit 0
