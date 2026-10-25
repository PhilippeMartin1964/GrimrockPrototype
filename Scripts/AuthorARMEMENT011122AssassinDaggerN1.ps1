param(
    [string]$EngineRoot = 'D:\UE_5.5',
    [ValidateSet('ARM002')]
    [string]$Archetype,
    [switch]$Apply
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$RepoRoot = Split-Path -Parent $PSScriptRoot
$Project = Join-Path $RepoRoot 'GrimrockPrototype.uproject'
$Editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$TargetRelative = 'Content/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_AssassinDagger_01.uasset'
$Target = Join-Path $RepoRoot ($TargetRelative.Replace('/', '\'))

if (-not (Test-Path -LiteralPath $Editor -PathType Leaf)) {
    throw "UnrealEditor-Cmd.exe missing: $Editor"
}
if ((& git -C $RepoRoot branch --show-current).Trim() -ne 'master') {
    throw 'ARMEMENT01.11.2.2 requires master.'
}
if ($Apply -and $Archetype -ne 'ARM002') {
    throw '-Apply requires -Archetype ARM002; one item at a time.'
}
$Before = @(& git -C $RepoRoot status --porcelain=v1 --untracked-files=all)
if ($LASTEXITCODE -ne 0) { throw 'git status failed' }
if ($Apply -and (Test-Path -LiteralPath $Target -PathType Leaf)) {
    throw "Refusing overwrite of existing local item: $Target"
}

Write-Host '=== ARMEMENT01.11.2.2: Development Editor build ==='
& (Join-Path $PSScriptRoot 'ValidateUE.ps1') -EngineRoot $EngineRoot -SkipAutomation
if ($LASTEXITCODE -ne 0) { throw 'UE Development Editor build failed' }

$LogDir = Join-Path $RepoRoot 'Saved\ARMEMENT01'
New-Item -ItemType Directory -Force -Path $LogDir | Out-Null
$LogFile = Join-Path $LogDir 'ARMEMENT01_11_2_2_AssassinDaggerN1Authoring.log'
$Arguments = @(
    $Project, '-run=GridAssassinDaggerN1ItemAuthoring',
    '-Unattended', '-NoSplash', '-NoP4', '-NoSound', '-NullRHI', '-log'
)
if ($Archetype) { $Arguments += "-Archetype=$Archetype" }
if ($Apply) { $Arguments += '-Apply' }
Write-Host '=== ARM002: read-only preflight or one-asset authoring ==='
Write-Host "Detailed Unreal log: $LogFile"
& $Editor @Arguments *> $LogFile
$Code = $LASTEXITCODE
$Messages = @(Select-String -LiteralPath $LogFile -Pattern '\[ARMEMENT01\.11\.2\.2\]' -ErrorAction SilentlyContinue)
$Lines = @($Messages | ForEach-Object { $_.Line })
$Lines | ForEach-Object { Write-Host $_ }
if ($Code -ne 0) {
    Get-Content -LiteralPath $LogFile -Tail 25
    throw "ARM002 item authoring refused, exit code $Code"
}
if ($Lines.Count -eq 0) {
    throw 'No ARMEMENT01.11.2.2 diagnostics; refusing silent success.'
}
if ($Apply) {
    $Created = @($Lines | Where-Object { $_ -match '\[ARMEMENT01\.11\.2\.2\] CREATED ARM002' })
    $Applied = @($Lines | Where-Object { $_ -match '\[ARMEMENT01\.11\.2\.2\] APPLY complete:' })
    if ($Created.Count -ne 1 -or $Applied.Count -ne 1) {
        throw 'No explicit confirmation of ARM002 creation.'
    }
    if (-not (Test-Path -LiteralPath $Target -PathType Leaf)) {
        throw "Commandlet returned success but new asset not found: $Target"
    }
    Write-Host '=== ARM002 item tests after single-asset creation ==='
    & (Join-Path $PSScriptRoot 'ValidateUE.ps1') -EngineRoot $EngineRoot -SkipBuild -AutomationFilter 'Grimrock.RPG.ARMEMENT01.11.2.2'
    if ($LASTEXITCODE -ne 0) { throw 'ARM002 item tests failed; do not stage/commit.' }
} else {
    $Summary = @($Lines | Where-Object { $_ -match '\[ARMEMENT01\.11\.2\.2\] READ-ONLY PREFLIGHT:' })
    $Verdict = @($Lines | Where-Object { $_ -match '\[ARMEMENT01\.11\.2\.2\] (READY|BLOCKED|PRESENT) ARM002' })
    if ($Summary.Count -ne 1 -or $Verdict.Count -eq 0) {
        throw 'Read-only preflight has no complete verdict.'
    }
}

$After = @(& git -C $RepoRoot status --porcelain=v1 --untracked-files=all)
if ($LASTEXITCODE -ne 0) { throw 'Final git status failed' }
$New = @($After | Where-Object { $_ -notin $Before })
if ($Apply) {
    $Unexpected = @($New | Where-Object {
        $_.Length -lt 4 -or $_.Substring(3).Replace('\','/') -ne $TargetRelative
    })
    if ($Unexpected.Count -gt 0) {
        $Unexpected | ForEach-Object { Write-Host $_ }
        throw 'Unrelated changes discovered; do not stage or commit.'
    }
} elseif ($New.Count -gt 0) {
    $New | ForEach-Object { Write-Host $_ }
    throw 'Read-only preflight modified local content.'
}
Write-Host '=== Git status (no auto-add/commit/push) ==='
& git -C $RepoRoot status --short --untracked-files=all
if ($Apply) {
    Write-Host '[OK] ARM002 N1 authored; targeted tests passed. PIE gameplay review pending.'
} elseif (@($Lines | Where-Object { $_ -match '\[ARMEMENT01\.11\.2\.2\] READY ARM002' }).Count -gt 0) {
    Write-Host '[OK] READY ARM002; manual approval needed before -Apply.'
} else {
    Write-Host '[BLOCKED/PRESENT] Review the ARM002 preflight verdict before proceeding.'
}
