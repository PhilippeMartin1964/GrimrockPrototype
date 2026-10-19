# PERF-PROFILE01: reproducible, local Unreal Insights CPU capture.
# This launcher does not alter assets, save games, or runtime configuration.
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$EngineRoot,

    [ValidateSet('Baseline', 'EventFeed', 'Movement', 'Progression', 'PortraitHUD')]
    [string]$Scenario = 'Baseline',

    [string]$OutputRoot,

    [switch]$PreflightOnly
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$RepoRoot = Split-Path -Parent $PSScriptRoot
$ProjectFile = Join-Path $RepoRoot 'GrimrockPrototype.uproject'
$EditorExecutable = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor.exe'
$InsightsExecutable = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealInsights.exe'

if (-not (Test-Path -LiteralPath $ProjectFile -PathType Leaf)) {
    throw "Unreal project not found: $ProjectFile"
}
if (-not (Test-Path -LiteralPath $EditorExecutable -PathType Leaf)) {
    throw "UE5.5 UnrealEditor.exe not found: $EditorExecutable"
}
if (-not (Test-Path -LiteralPath $InsightsExecutable -PathType Leaf)) {
    Write-Warning "UnrealInsights.exe not found at expected path: $InsightsExecutable. The trace can be inspected later."
}

if ([string]::IsNullOrWhiteSpace($OutputRoot)) {
    $OutputRoot = Join-Path $RepoRoot 'Saved\Profiling\PERF-PROFILE01'
}
$OutputRoot = [System.IO.Path]::GetFullPath($OutputRoot)

$Timestamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$Stem = "PERF-PROFILE01-$Scenario-$Timestamp"
$TraceFile = Join-Path $OutputRoot ($Stem + '.utrace')
$MetadataFile = Join-Path $OutputRoot ($Stem + '.json')
$Channels = 'cpu,frame,bookmark,log'
$EditorArgs = @(
    ('"{0}"' -f $ProjectFile),
    '-game',
    '-nosplash',
    '-log',
    ('-trace={0}' -f $Channels),
    ('-tracefile="{0}"' -f $TraceFile)
)

Write-Host 'GrimrockPrototype PERF-PROFILE01 — Unreal Insights' -ForegroundColor Cyan
Write-Host "Scenario    : $Scenario"
Write-Host "Project     : $ProjectFile"
Write-Host "Editor      : $EditorExecutable"
Write-Host "Channels    : $Channels"
Write-Host "Trace       : $TraceFile"
Write-Host "Insights    : $InsightsExecutable"
Write-Host "Arguments   : $($EditorArgs -join ' ')"

if ($PreflightOnly) {
    Write-Host '[OK] Preflight completed. Unreal Editor was not started.' -ForegroundColor Green
    return
}

if (Test-Path -LiteralPath $TraceFile) {
    throw "A trace with this timestamp already exists: $TraceFile. Wait one second and retry."
}
New-Item -ItemType Directory -Path $OutputRoot -Force | Out-Null

$GitRevision = 'unknown'
if (Get-Command git -ErrorAction SilentlyContinue) {
    $GitOutput = & git -C $RepoRoot rev-parse HEAD 2>$null
    if ($LASTEXITCODE -eq 0 -and $GitOutput) {
        $GitRevision = ($GitOutput | Select-Object -First 1).Trim()
    }
}

Write-Host ''
Write-Host 'Play the scenario, add Trace.Bookmark PERF_PROFILE01_Begin / PERF_PROFILE01_End in the in-game console, then close the game.' -ForegroundColor Yellow
Write-Host 'This PowerShell command will wait until Unreal Editor exits.'
$StartedUtc = (Get-Date).ToUniversalTime().ToString('o')
$Process = Start-Process -FilePath $EditorExecutable -ArgumentList $EditorArgs -PassThru -Wait
$CompletedUtc = (Get-Date).ToUniversalTime().ToString('o')

if (-not (Test-Path -LiteralPath $TraceFile -PathType Leaf)) {
    throw "No trace file found at $TraceFile. Check the Unreal log and the project's Saved/Profiling directory."
}
$TraceBytes = (Get-Item -LiteralPath $TraceFile).Length
if ($TraceBytes -le 0) {
    throw "Trace file was created but is empty: $TraceFile"
}

$Metadata = [ordered]@{
    Milestone = 'PERF-PROFILE01'
    Scenario = $Scenario
    GitRevision = $GitRevision
    EngineRoot = $EngineRoot
    ProjectFile = $ProjectFile
    BuildTarget = 'UnrealEditor Win64 Development -game'
    Channels = $Channels
    StartedUtc = $StartedUtc
    CompletedUtc = $CompletedUtc
    ProcessExitCode = $Process.ExitCode
    TraceFile = $TraceFile
    TraceBytes = $TraceBytes
    Notes = 'Trace covers whole process. Compare only identical bookmark-bounded gameplay intervals; exclude startup/loading.'
}
$Metadata | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $MetadataFile -Encoding UTF8
Write-Host ''
Write-Host "[OK] Trace captured: $TraceFile ($TraceBytes bytes)" -ForegroundColor Green
Write-Host "Metadata : $MetadataFile"
Write-Host ('Open with: "{0}" -OpenTraceFile="{1}"' -f $InsightsExecutable, $TraceFile)
if ($Process.ExitCode -ne 0) {
    Write-Warning "Unreal Editor exited with code $($Process.ExitCode). Inspect the game log and captured trace."
}
