param(
    [string]$EngineRoot = $env:UE_ROOT
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$RepoRoot = Split-Path -Parent $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($EngineRoot)) { throw 'Indiquez -EngineRoot D:\UE_5.5.' }
$EngineRoot = (Resolve-Path -LiteralPath $EngineRoot).Path
$Editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Project = Join-Path $RepoRoot 'GrimrockPrototype.uproject'
$SourceRoot = Join-Path $RepoRoot 'ArtSource\ARMEMENT01_6_3\Meshes'
$IconsRoot = Join-Path $RepoRoot 'Saved\ARMEMENT01\ARMEMENT01_6_3\Icons'
$Asset = Join-Path $RepoRoot 'Content\GrimrockPrototype\Core\DataAssets\Weapons\DA_WeaponVisualCatalogue.uasset'

if (-not (Test-Path -LiteralPath $Editor -PathType Leaf)) { throw "UnrealEditor-Cmd.exe introuvable : $Editor" }
if ((& git -C $RepoRoot branch --show-current).Trim() -ne 'master') {
    throw 'ARMEMENT01.6.3 doit etre materialise depuis master.'
}
$Before = @(& git -C $RepoRoot status --short --untracked-files=all)
if ($LASTEXITCODE -ne 0 -or $Before.Count -gt 0) {
    $Before | ForEach-Object { Write-Host $_ }
    throw 'Working tree non propre : committer ou ranger les changements avant la generation.'
}
if (Test-Path -LiteralPath $Asset) {
    throw "Le catalogue visuel existe deja : $Asset. Ne pas ecraser un asset deja relu."
}

Write-Host '=== ARMEMENT01.6.3 — Generation des trois icones RGBA ==='
& (Join-Path $PSScriptRoot 'GenerateARMEMENT0163Icons.ps1') -Destination $IconsRoot
if (-not $?) { throw 'Generation des icones echouee.' }
$Icons = @('T_Weapon_Dagger_A', 'T_Weapon_ShortBow_A', 'T_Weapon_RoundShield_A')
$Meshes = @('SM_Weapon_Dagger_A', 'SM_Weapon_ShortBow_A', 'SM_Weapon_RoundShield_A')
foreach ($Name in $Icons) {
    $InputFile = Join-Path $IconsRoot ($Name + '.png')
    if (-not (Test-Path -LiteralPath $InputFile -PathType Leaf)) { throw "PNG absent : $InputFile" }
}
foreach ($Name in $Meshes) {
    $Obj = Join-Path $SourceRoot ($Name + '.obj')
    $Mtl = Join-Path $SourceRoot ($Name + '.mtl')
    if (-not (Test-Path -LiteralPath $Obj -PathType Leaf) -or -not (Test-Path -LiteralPath $Mtl -PathType Leaf)) {
        throw "Source OBJ/MTL absente : $Name"
    }
}
Write-Host '=== ARMEMENT01.6.3 — Compilation Development Editor ==='
& (Join-Path $PSScriptRoot 'ValidateUE.ps1') -EngineRoot $EngineRoot -SkipAutomation
if (-not $?) { throw 'Compilation echouee.' }

Write-Host '=== ARMEMENT01.6.3 — Import automatique UE5.5.4 ==='
$EditorArgs = @(
    $Project, '-run=GridWeaponVisualPilotImport',
    "-SourceRoot=$SourceRoot",
    "-IconsRoot=$IconsRoot",
    '-Unattended', '-NoSplash', '-NoP4', '-NoSound', '-NullRHI', '-log'
)
& $Editor @EditorArgs
if ($LASTEXITCODE -ne 0) { throw "Import UE echoue avec le code $LASTEXITCODE." }

$Expected = @('Content/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponVisualCatalogue.uasset')
foreach ($Name in $Icons) {
    $Expected += ('Content/GrimrockPrototype/UI/Icons/Weapons/' + $Name + '.uasset')
}
foreach ($Name in $Meshes) {
    $Expected += ('Content/GrimrockPrototype/Meshes/Weapons/' + $Name + '.uasset')
}
foreach ($RelativePath in $Expected) {
    if (-not (Test-Path -LiteralPath (Join-Path $RepoRoot $RelativePath) -PathType Leaf)) {
        throw "Unreal asset absent apres import : $RelativePath"
    }
}

Write-Host '=== ARMEMENT01.6.3 — Audit des fichiers generes ==='
$Changed = @(& git -C $RepoRoot diff --name-only)
$Untracked = @(& git -C $RepoRoot ls-files --others --exclude-standard)
$Files = @($Changed + $Untracked | Where-Object { $_ })
$Unexpected = @($Files | Where-Object {
    $_ -notmatch '^Content/GrimrockPrototype/(?:Core/DataAssets/Weapons|UI/Icons/Weapons|Meshes/Weapons)/(?:[^/]+/)*[^/]+\.uasset$'
})
if ($Unexpected.Count -gt 0) {
    $Unexpected | ForEach-Object { Write-Host $_ }
    throw 'Import refuse : modifications non prevues en dehors des repertoires pilotes.'
}

Write-Host '=== ARMEMENT01.6.3 — Tests des assets pilotes ==='
& (Join-Path $PSScriptRoot 'ValidateUE.ps1') -EngineRoot $EngineRoot -SkipBuild -AutomationFilter 'Grimrock.RPG.ARMEMENT01.6.Pilot'
if (-not $?) { throw 'Tests du catalogue visuel pilote echoues.' }

Write-Host '=== ARMEMENT01.6.3 — Etat Git (pas de commit automatique) ==='
& git -C $RepoRoot status --short
Write-Host '[OK] Trois icones, trois meshes et le catalogue UE crees et testes.'
Write-Host 'Inspectez les visuels en UE avant de committer les .uasset (Git LFS).'
