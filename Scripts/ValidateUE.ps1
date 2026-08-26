param(
    [string]$EngineRoot = $env:UE_ROOT,
    [string]$AutomationFilter,
    [string]$ReportRoot,
    [switch]$SkipBuild,
    [switch]$SkipAutomation,
    [switch]$UseRHI
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

    Invoke-GrimrockNativeStep -Label 'UE5.5.4 Automation' -Executable $EditorCmd -Arguments $EditorArguments

    $IndexJsonPath = Join-Path $SessionReportPath 'index.json'
    Assert-FileExists -Path $IndexJsonPath -Description 'Rapport Automation index.json'

    $Report = Get-Content -LiteralPath $IndexJsonPath -Raw | ConvertFrom-Json
    $Succeeded = [int]$Report.succeeded
    $SucceededWithWarnings = [int]$Report.succeededWithWarnings
    $Failed = [int]$Report.failed
    $NotRun = [int]$Report.notRun
    $Executed = $Succeeded + $SucceededWithWarnings + $Failed

    Write-Host ''
    Write-Host '=== Automation summary ==='
    Write-Host "Filter                 : $AutomationFilter"
    Write-Host "Succeeded              : $Succeeded"
    Write-Host "Succeeded with warnings: $SucceededWithWarnings"
    Write-Host "Failed                 : $Failed"
    Write-Host "Not run                : $NotRun"
    Write-Host "Report                 : $SessionReportPath"
    Write-Host "Log                    : $AutomationLogPath"

    if ($Executed -le 0)
    {
        Write-Error "Aucun test Automation n'a ete execute pour le filtre : $AutomationFilter"
        exit 2
    }

    if ($Failed -gt 0)
    {
        Write-Error "$Failed test(s) Automation en echec. Consultez : $SessionReportPath"
        exit 3
    }

    Write-Host '[OK] Automation filter validated.'
}

Write-Host ''
Write-Host 'TD04.2 validation completed successfully.'
exit 0
