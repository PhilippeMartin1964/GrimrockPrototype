param([string]$EngineRoot = $env:UE_ROOT)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$RepoRoot = Split-Path -Parent $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($EngineRoot)) {
    throw 'Indiquez -EngineRoot D:\UE_5.5.'
}
$EngineRoot = (Resolve-Path -LiteralPath $EngineRoot).Path
$Editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Project = Join-Path $RepoRoot 'GrimrockPrototype.uproject'
$Expected = 'Content/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponCatalogue.uasset'
$Asset = Join-Path $RepoRoot $Expected
if (-not (Test-Path -LiteralPath $Editor -PathType Leaf)) {
    throw "UnrealEditor-Cmd.exe introuvable : $Editor"
}
if ((& git -C $RepoRoot branch --show-current).Trim() -ne 'master') {
    throw 'ARMEMENT01.9.5.2 : master uniquement.'
}
$Before = @(& git -C $RepoRoot status --short --untracked-files=all)
if ($LASTEXITCODE -ne 0 -or $Before.Count -gt 0) {
    $Before | ForEach-Object { Write-Host $_ }
    throw 'Working tree non propre : committer ou sauvegarder avant import.'
}
if (-not (Test-Path -LiteralPath $Asset -PathType Leaf)) {
    throw "Le catalogue de 1640 profils doit déjà exister : $Asset"
}

Write-Host '=== ARMEMENT01.9.5.2 — Projection du XLSX (source unique) ==='
& (Join-Path $PSScriptRoot 'ExportARMEMENT01952Affinities.ps1')
if (-not $?) { throw 'Echec export affinités XLSX.' }

Write-Host '=== ARMEMENT01.9.5.2 — Compilation Development Editor ==='
& (Join-Path $PSScriptRoot 'ValidateUE.ps1') -EngineRoot $EngineRoot -SkipAutomation
if (-not $?) { throw 'Echec compilation.' }

Write-Host '=== ARMEMENT01.9.5.2 — Import non destructif dans DA_WeaponCatalogue ==='
& $Editor @($Project, '-run=GridWeaponClassAffinityImport', '-unattended',
    '-nop4', '-nosplash', '-nullrhi', '-log')
if ($LASTEXITCODE -ne 0) { throw "Echec import: $LASTEXITCODE" }

$Changed = @(& git -C $RepoRoot diff --name-only)
$Untracked = @(& git -C $RepoRoot ls-files --others --exclude-standard)
$Unexpected = @($Changed + $Untracked | Where-Object { $_ -and $_ -ne $Expected })
if ($Unexpected.Count -gt 0) {
    $Unexpected | ForEach-Object { Write-Host $_ }
    throw 'Fichiers inattendus modifiés. Ne rien committer.'
}

Write-Host '=== ARMEMENT01.9.5.2 — Tests projection / affinités ==='
& (Join-Path $PSScriptRoot 'ValidateUE.ps1') -EngineRoot $EngineRoot -SkipBuild -AutomationFilter 'Grimrock.RPG.ARMEMENT01.9.WeaponAffinity'
if (-not $?) { throw 'Echec tests affinité.' }

Write-Host '=== ARMEMENT01.9.5.2 — Non-régression catalogue 1640 profils ==='
& (Join-Path $PSScriptRoot 'ValidateUE.ps1') -EngineRoot $EngineRoot -SkipBuild -AutomationFilter 'Grimrock.RPG.ARMEMENT01.4'
if (-not $?) { throw 'Echec non-régression catalogue.' }

Write-Host '=== Git ==='
& git -C $RepoRoot status --short
Write-Host "[OK] Matrice importée : 82 affinités et six préférences. Les 1640 profils ont été conservés."
Write-Host "Seul asset à versionner séparément via LFS : $Expected"
Write-Host 'Aucun commit automatique.'
