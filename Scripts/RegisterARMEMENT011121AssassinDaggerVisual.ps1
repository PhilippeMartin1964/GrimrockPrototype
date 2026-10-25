param(
    [string]$EngineRoot = 'D:\UE_5.5',
    [ValidateSet('ARM002')]
    [string]$Archetype,
    [switch]$Apply
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$RepoRoot = Split-Path -Parent $PSScriptRoot
$Editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Project = Join-Path $RepoRoot 'GrimrockPrototype.uproject'
$CatalogueRelative = 'Content/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponVisualCatalogue.uasset'

if (-not (Test-Path -LiteralPath $Editor -PathType Leaf)) {
    throw "UnrealEditor-Cmd.exe missing: $Editor"
}
if ((& git -C $RepoRoot branch --show-current).Trim() -ne 'master') {
    throw 'ARMEMENT01.11.2.1: only master is supported.'
}
if ($Apply -and -not $Archetype) {
    throw '-Apply requires -Archetype ARM002 (one at a time).'
}

$Before = @(& git -C $RepoRoot status --porcelain=v1 --untracked-files=all)
if ($LASTEXITCODE -ne 0) { throw 'git status failed' }
$BeforeCatalogue = @($Before | Where-Object {
    $_.Length -ge 4 -and $_.Substring(3).Replace('\','/') -eq $CatalogueRelative
})
if ($Apply -and $BeforeCatalogue.Count -gt 0) {
    throw 'Visual catalogue already has local Git changes: review and publish/revert by hand before -Apply.'
}

Write-Host '=== ARMEMENT01.11.2.1: Development Editor build ==='
& (Join-Path $PSScriptRoot 'ValidateUE.ps1') -EngineRoot $EngineRoot -SkipAutomation
if ($LASTEXITCODE -ne 0) { throw 'Unreal Editor build failed' }

$LogDir = Join-Path $RepoRoot 'Saved\ARMEMENT01'
New-Item -ItemType Directory -Path $LogDir -Force | Out-Null
$LogFile = Join-Path $LogDir 'ARMEMENT01_11_2_1_AssassinDaggerVisual.log'

$Arguments = @(
    $Project, '-run=GridAssassinDaggerVisualRegistration',
    '-Unattended', '-NoSplash', '-NoP4', '-NoSound', '-NullRHI', '-log'
)
if ($Archetype) { $Arguments += "-Archetype=$Archetype" }
if ($Apply) { $Arguments += '-Apply' }
Write-Host '=== Visual preflight/registration: concise output ==='
Write-Host "Detailed Unreal log: $LogFile"
& $Editor @Arguments *> $LogFile
$CommandletExit = $LASTEXITCODE

$Messages = @(Select-String -LiteralPath $LogFile -Pattern '\[ARMEMENT01\.11\.2\.1\]' -ErrorAction SilentlyContinue)
$MessageLines = @($Messages | ForEach-Object { $_.Line })
$MessageLines | ForEach-Object { Write-Host $_ }

if ($CommandletExit -ne 0) {
    Get-Content -LiteralPath $LogFile -Tail 25
    throw "Visual registration refused (code $CommandletExit). No silent substitutions."
}

if ($MessageLines.Count -eq 0) {
    throw 'No ARMEMENT01.11.2.1 diagnostics found in the Unreal log; cannot establish READY/BLOCKED/REGISTERED.'
}
if ($Apply) {
    $AppliedOrAlreadyRegistered = @($MessageLines | Where-Object {
        $_ -match '\[ARMEMENT01\.11\.2\.1\] (SAVED|REGISTERED) ARM002'
    })
    $ApplySummary = @($MessageLines | Where-Object {
        $_ -match '\[ARMEMENT01\.11\.2\.1\] APPLY complete:'
    })
    if ($AppliedOrAlreadyRegistered.Count -eq 0 -or $ApplySummary.Count -eq 0) {
        throw 'ARM002 visual registration was not confirmed by commandlet diagnostics.'
    }
} else {
    $ReadOnlySummary = @($MessageLines | Where-Object {
        $_ -match '\[ARMEMENT01\.11\.2\.1\] READ-ONLY PREFLIGHT:'
    })
    $PreflightStatus = @($MessageLines | Where-Object {
        $_ -match '\[ARMEMENT01\.11\.2\.1\] (READY|BLOCKED|REGISTERED) ARM002'
    })
    if ($ReadOnlySummary.Count -eq 0 -or $PreflightStatus.Count -eq 0) {
        throw 'ARM002 read-only preflight did not report a complete readiness verdict.'
    }
}

if ($Apply) {
    Write-Host '=== ARMEMENT01.11.2 targeted regression after visual registration ==='
    & (Join-Path $PSScriptRoot 'ValidateUE.ps1') -EngineRoot $EngineRoot -SkipBuild -AutomationFilter 'Grimrock.RPG.ARMEMENT01.11.2'
    if ($LASTEXITCODE -ne 0) {
        throw 'Targeted tests failed; do not publish changed content.'
    }
}

$After = @(& git -C $RepoRoot status --porcelain=v1 --untracked-files=all)
if ($LASTEXITCODE -ne 0) { throw 'Final git status failed' }
$NewStatuses = @($After | Where-Object { $_ -notin $Before })
if ($Apply) {
    $Unexpected = @($NewStatuses | Where-Object {
        $_.Length -lt 4 -or $_.Substring(3).Replace('\','/') -ne $CatalogueRelative
    })
    if ($Unexpected.Count -gt 0) {
        $Unexpected | ForEach-Object { Write-Host $_ }
        throw 'Unexpected modifications found: do not git add or commit.'
    }
} elseif ($NewStatuses.Count -gt 0) {
    $NewStatuses | ForEach-Object { Write-Host $_ }
    throw 'Read-only preflight changed source-controlled content.'
}
Write-Host '=== Git status (no auto-stage / commit / push) ==='
& git -C $RepoRoot status --short --untracked-files=all
if ($Apply) {
    Write-Host '[OK] Single visual registration applied or already present. Manual art and PIE reviews still required.'
} else {
    if (@($MessageLines | Where-Object {
        $_ -match '\[ARMEMENT01\.11\.2\.1\] BLOCKED ARM002'
    }).Count -gt 0) {
        Write-Host '[BLOCKED] ARM002 is not ready for -Apply. Resolve the reported asset issue.'
    } else {
        Write-Host '[OK] ARM002 preflight completed with READY or REGISTERED; review art before -Apply.'
    }
}
