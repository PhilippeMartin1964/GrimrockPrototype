param(
    [string]$EngineRoot = $env:UE_ROOT,
    [string]$RootPath = '/Game/GrimrockPrototype',
    [string]$ReportRoot,
    [switch]$Apply,
    [switch]$SkipBuild
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$RepoRoot = Split-Path -Parent $PSScriptRoot
$ProjectFile = Join-Path $RepoRoot 'GrimrockPrototype.uproject'

if ([string]::IsNullOrWhiteSpace($EngineRoot))
{
    throw 'Racine Unreal Engine non renseignee. Utilisez -EngineRoot ou definissez UE_ROOT.'
}

$EngineRoot = (Resolve-Path -LiteralPath $EngineRoot).Path
$EditorCmd = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
if (-not (Test-Path -LiteralPath $EditorCmd -PathType Leaf))
{
    throw "UnrealEditor-Cmd.exe introuvable : $EditorCmd"
}

if (-not $SkipBuild)
{
    & (Join-Path $PSScriptRoot 'ValidateUE.ps1') -EngineRoot $EngineRoot -SkipAutomation
}

if ([string]::IsNullOrWhiteSpace($ReportRoot))
{
    $ReportRoot = Join-Path $RepoRoot 'Saved\Automation\MIG08'
}

$Timestamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$SessionPath = Join-Path $ReportRoot ('MIG08-' + $Timestamp)
New-Item -ItemType Directory -Path $SessionPath -Force | Out-Null

$ReportPath = Join-Path $SessionPath 'MIG08.report.txt'
$LogPath = Join-Path $SessionPath 'MIG08.log'
$Mode = if ($Apply) { 'APPLY' } else { 'DRY-RUN' }

Write-Host 'GrimrockPrototype WORLDOBJ-MIG08 Asset Migration'
Write-Host "Mode       : $Mode"
Write-Host "Repository : $RepoRoot"
Write-Host "Project    : $ProjectFile"
Write-Host "Engine     : $EngineRoot"
Write-Host "Root       : $RootPath"
Write-Host "Report     : $ReportPath"

$Arguments = @(
    $ProjectFile,
    '-run=GridWorldObjectMIG08',
    "-Root=$RootPath",
    "-Report=$ReportPath",
    '-Unattended',
    '-NoSplash',
    '-NoP4',
    '-NoSound',
    '-NullRHI',
    '-UTF8Output',
    '-log',
    "-abslog=$LogPath"
)

if ($Apply)
{
    $Arguments += '-Apply'
}

& $EditorCmd @Arguments
$ExitCode = $LASTEXITCODE

if (Test-Path -LiteralPath $ReportPath -PathType Leaf)
{
    Write-Host ''
    Get-Content -LiteralPath $ReportPath | ForEach-Object { Write-Host $_ }
}

if ($ExitCode -ne 0)
{
    throw "WORLDOBJ-MIG08 $Mode a echoue avec le code $ExitCode. Voir $ReportPath et $LogPath"
}

Write-Host ''
Write-Host "[OK] WORLDOBJ-MIG08 $Mode termine."
if ($Apply)
{
    Write-Host ''
    Write-Host '=== Git status Content ==='
    git -C $RepoRoot status --short -- Content
}
