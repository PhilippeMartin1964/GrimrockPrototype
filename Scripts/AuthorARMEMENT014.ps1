param(
    [string]$EngineRoot = $env:UE_ROOT
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$RepoRoot = Split-Path -Parent $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($EngineRoot)) {
    throw 'Indiquez -EngineRoot D:\UE_5.5.'
}
$EngineRoot = (Resolve-Path -LiteralPath $EngineRoot).Path
$Editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Project = Join-Path $RepoRoot 'GrimrockPrototype.uproject'
$Csv = Join-Path $RepoRoot 'Saved\ARMEMENT01\ARMEMENT01_4_Profiles.csv'
$Asset = Join-Path $RepoRoot 'Content\GrimrockPrototype\Core\DataAssets\Weapons\DA_WeaponCatalogue.uasset'
if (-not (Test-Path -LiteralPath $Editor -PathType Leaf)) {
    throw "UnrealEditor-Cmd.exe introuvable : $Editor"
}
if ((& git -C $RepoRoot branch --show-current).Trim() -ne 'master') {
    throw 'ARMEMENT01.4 doit être matérialisé depuis master.'
}
$Before = @(& git -C $RepoRoot status --short --untracked-files=all)
if ($LASTEXITCODE -ne 0 -or $Before.Count -gt 0) {
    $Before | ForEach-Object { Write-Host $_ }
    throw 'Working tree non propre : sauvegarder les modifications avant de générer le catalogue.'
}

Write-Host '=== ARMEMENT01.4 — Export contrôlé depuis le classeur complet ==='
& (Join-Path $PSScriptRoot 'ExportARMEMENT014Catalogue.ps1')
if (-not $?) { throw 'Export du catalogue échoué.' }

Write-Host '=== ARMEMENT01.4 — Compilation Development Editor ==='
& (Join-Path $PSScriptRoot 'ValidateUE.ps1') -EngineRoot $EngineRoot -SkipAutomation
if (-not $?) { throw 'Compilation échouée.' }

Write-Host '=== ARMEMENT01.4 — Import UE : asset unique ==='
$EditorArgs = @(
    $Project, '-run=GridWeaponCatalogueImport', "-SourceCSV=$Csv",
    '-Unattended', '-NoSplash', '-NoP4', '-NoSound', '-NullRHI', '-log'
)
& $Editor @EditorArgs
if ($LASTEXITCODE -ne 0) {
    throw "Import échoué, code $LASTEXITCODE."
}
if (-not (Test-Path -LiteralPath $Asset -PathType Leaf)) {
    throw "Catalogue Unreal introuvable après import : $Asset"
}
$Expected = 'Content/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponCatalogue.uasset'
$Changed = @(& git -C $RepoRoot diff --name-only)
$Untracked = @(& git -C $RepoRoot ls-files --others --exclude-standard)
$Unexpected = @($Changed + $Untracked | Where-Object { $_ -and $_ -ne $Expected })
if ($Unexpected.Count -gt 0) {
    $Unexpected | ForEach-Object { Write-Host $_ }
    throw 'Des fichiers autres que le catalogue ont changé. Ne pas committer.'
}

Write-Host '=== ARMEMENT01.4 — Tests de catalogue ==='
& (Join-Path $PSScriptRoot 'ValidateUE.ps1') -EngineRoot $EngineRoot -SkipBuild -AutomationFilter 'Grimrock.RPG.ARMEMENT01.4'
if (-not $?) { throw 'Tests ARMEMENT01.4 échoués.' }

Write-Host '=== ARMEMENT01.4 — Etat Git ==='
& git -C $RepoRoot status --short
Write-Host '[OK] Asset unique généré et tests ciblés réussis.'
Write-Host "Fichier à examiner / versionner (LFS) : $Expected"
Write-Host 'Aucun commit automatique.'
