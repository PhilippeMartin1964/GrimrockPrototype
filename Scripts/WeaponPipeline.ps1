[CmdletBinding()]
param(
    [string]$EngineRoot = 'D:\UE_5.5',
    [ValidateSet('Audit','Plan','Build','Validate')]
    [string]$Action = 'Audit',
    [string]$Archetype,
    [string[]]$Archetypes = @(),
    [switch]$All,
    [ValidateSet('Visual','Item')]
    [string]$Stage,
    [switch]$Apply
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$Repo = Split-Path -Parent $PSScriptRoot
$Editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Project = Join-Path $Repo 'GrimrockPrototype.uproject'
$Manifest = Join-Path $Repo 'Config\Armement\WeaponProductionManifest.csv'
$ReportDir = Join-Path $Repo 'Saved\ARMEMENT-INDUS'
$VisualCatalogue = 'Content/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponVisualCatalogue.uasset'
$RpgCatalogue = 'Content/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponCatalogue.uasset'
$Palette = 'Content/GrimrockPrototype/Core/DataAssets/DA_ObjectPalette_Default.uasset'

if (-not (Test-Path -LiteralPath $Editor -PathType Leaf)) { throw "Missing UnrealEditor-Cmd: $Editor" }
if (-not (Test-Path -LiteralPath $Manifest -PathType Leaf)) { throw "Missing manifest: $Manifest" }
if ((& git -C $Repo branch --show-current).Trim() -ne 'master') { throw 'Master only.' }
if ($LASTEXITCODE -ne 0) { throw 'Cannot inspect branch.' }
if ($Archetype -and $Archetypes.Count -gt 0) { throw 'Select Archetype or Archetypes, not both.' }
if ($All -and ($Archetype -or $Archetypes.Count -gt 0)) { throw 'All conflicts with explicit IDs.' }
if ($Action -eq 'Build' -and (-not $Archetype -or $All -or $Archetypes.Count -gt 0)) {
    throw 'Build requires one -Archetype.'
}
if ($Apply -and ($Action -ne 'Build' -or -not $Stage)) {
    throw 'Apply requires Build and Stage Visual|Item.'
}
if ($Stage -and $Action -ne 'Build') { throw 'Stage only applies to Build.' }

$Before = @(& git -C $Repo status --porcelain=v1 --untracked-files=all)
if ($LASTEXITCODE -ne 0) { throw 'git status failed' }
if ($Apply -and $Stage -eq 'Visual') {
    $DirtyVisual = @(& git -C $Repo status --porcelain=v1 -- $VisualCatalogue)
    if ($DirtyVisual.Count -gt 0) { throw 'Visual catalogue has local changes; refusing write.' }
}

function Get-Fingerprint([string]$Relative) {
    $Path = Join-Path $Repo ($Relative.Replace('/', '\'))
    if (Test-Path -LiteralPath $Path -PathType Leaf) {
        return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash
    }
    return ''
}
$BeforeHashes = @{}
foreach ($Path in @($RpgCatalogue, $VisualCatalogue, $Palette)) {
    $BeforeHashes[$Path] = Get-Fingerprint $Path
}

Write-Host '=== INDUS02: editor build ==='
& (Join-Path $PSScriptRoot 'ValidateUE.ps1') -EngineRoot $EngineRoot -SkipAutomation
if ($LASTEXITCODE -ne 0) { throw 'UE editor build failed' }
New-Item -ItemType Directory -Force -Path $ReportDir | Out-Null

$Targets = @('ALL')
if ($Archetype) { $Targets = @($Archetype) }
elseif ($Archetypes.Count -gt 0) { $Targets = @($Archetypes | Select-Object -Unique) }

foreach ($Target in $Targets) {
    if ($Target -ne 'ALL' -and $Target -notmatch '^ARM[0-9]{3}$') {
        throw "Invalid archetype: $Target"
    }
    $Suffix = if ($Target -eq 'ALL') { 'All' } else { $Target }
    $Report = Join-Path $ReportDir ('INDUS02_' + $Action + '_' + $Suffix + '.csv')
    $JsonReport = [IO.Path]::ChangeExtension($Report, '.json')
    $Log = Join-Path $ReportDir ('INDUS02_' + $Action + '_' + $Suffix + '.log')
    $ArgsEditor = @(
        $Project, '-run=GridWeaponPipeline', "-Action=$Action",
        "-Manifest=$Manifest", "-Report=$Report",
        '-Unattended', '-NoSplash', '-NoP4', '-NoSound', '-NullRHI', '-log'
    )
    if ($Target -ne 'ALL') { $ArgsEditor += "-Archetype=$Target" }
    if ($Stage) { $ArgsEditor += "-Stage=$Stage" }
    if ($Apply) { $ArgsEditor += '-Apply' }
    Write-Host "=== INDUS02 $Action $Target (Stage=$Stage, Apply=$Apply) ==="
    Write-Host "Log: $Log"
    & $Editor @ArgsEditor *> $Log
    $Code = $LASTEXITCODE
    $Matches = @(Select-String -LiteralPath $Log -Pattern '\[INDUS02\]' -ErrorAction SilentlyContinue)
    $Lines = @($Matches | ForEach-Object { $_.Line })
    $Lines | ForEach-Object { Write-Host $_ }
    if ($Code -ne 0) {
        Get-Content -LiteralPath $Log -Tail 25
        throw "Commandlet failed for $Target (code $Code)"
    }
    $Results = @($Lines | Where-Object { $_ -match '\[INDUS02\] RESULT Archetype=' })
    $Summaries = @($Lines | Where-Object { $_ -match '\[INDUS02\] SUMMARY Action=' })
    if ($Results.Count -eq 0 -or $Summaries.Count -ne 1) {
        throw 'Commandlet diagnostics missing; refusing silent success.'
    }
    if ($Apply) {
        $Applied = @($Lines | Where-Object {
            $_ -match '\[INDUS02\] (SAVED|PRESENT_SAME) (Visual|Item) for '
        })
        if ($Applied.Count -ne 1) { throw 'Apply was not explicitly confirmed.' }
    }
    if (-not (Test-Path -LiteralPath $Report -PathType Leaf)) {
        throw "Expected report missing: $Report"
    }
    @(Import-Csv -LiteralPath $Report) | ConvertTo-Json -Depth 6 |
        Set-Content -LiteralPath $JsonReport -Encoding UTF8
    Write-Host "CSV : $Report"
    Write-Host "JSON: $JsonReport"
}

if ($Action -eq 'Validate' -or $Apply) {
    & (Join-Path $PSScriptRoot 'ValidateUE.ps1') -EngineRoot $EngineRoot -SkipBuild -AutomationFilter 'Grimrock.RPG.ARMEMENT.Industrial'
    if ($LASTEXITCODE -ne 0) { throw 'TD04.2 industrial tests failed; do not publish.' }
}

foreach ($Path in @($RpgCatalogue, $VisualCatalogue, $Palette)) {
    $Allowed = $Apply -and $Stage -eq 'Visual' -and $Path -eq $VisualCatalogue
    if (-not $Allowed -and (Get-Fingerprint $Path) -ne $BeforeHashes[$Path]) {
        throw "Unexpected package change: $Path"
    }
}
$After = @(& git -C $Repo status --porcelain=v1 --untracked-files=all)
if ($LASTEXITCODE -ne 0) { throw 'Post-command git status failed' }
$New = @($After | Where-Object { $_ -notin $Before })
if ($Apply) {
    if ($Stage -eq 'Visual') {
        $Expected = $VisualCatalogue
    } else {
        $Rows = @(Import-Csv -LiteralPath $Manifest | Where-Object { $_.ArchetypeId -eq $Archetype })
        if ($Rows.Count -ne 1) { throw "Manifest ID ambiguous: $Archetype" }
        $LongPath = ($Rows[0].ItemPath -split '\.')[0]
        $Expected = 'Content/' + $LongPath.Substring('/Game/'.Length) + '.uasset'
    }
    $Unexpected = @($New | Where-Object {
        $_.Length -lt 4 -or $_.Substring(3).Replace('\','/') -ne $Expected
    })
    if ($Unexpected.Count -gt 0) {
        $Unexpected | ForEach-Object { Write-Host $_ }
        throw 'Unrelated files changed. Do not stage/commit.'
    }
} elseif ($New.Count -gt 0) {
    $New | ForEach-Object { Write-Host $_ }
    throw 'Read-only operation changed Content or source files.'
}
Write-Host '=== Git status (no auto-stage / commit / push) ==='
& git -C $Repo status --short --untracked-files=all
Write-Host '[OK] INDUS02 completed. PIE acceptance remains separate.'
