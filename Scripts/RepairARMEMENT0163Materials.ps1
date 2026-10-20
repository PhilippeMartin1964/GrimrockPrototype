param(
    [string]$EngineRoot = 'D:\UE_5.5'
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$RepoRoot = Split-Path -Parent $PSScriptRoot
$Project = Join-Path $RepoRoot 'GrimrockPrototype.uproject'
$Editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Sources = Join-Path $RepoRoot 'ArtSource\ARMEMENT01_6_3\Meshes'

if (-not (Test-Path -LiteralPath $Editor -PathType Leaf)) {
    throw "UnrealEditor-Cmd.exe introuvable : $Editor"
}
if ((& git -C $RepoRoot branch --show-current).Trim() -ne 'master') {
    throw 'Utiliser master pour la reparation des assets pilotes.'
}

# Les sept .uasset pilotes existent deja et sont non suivis : c'est attendu.
# Des changements de code ou de contenu sans rapport ne doivent pas etre touches.
$Tracked = @(& git -C $RepoRoot diff --name-only)
$Staged = @(& git -C $RepoRoot diff --cached --name-only)
$Untracked = @(& git -C $RepoRoot ls-files --others --exclude-standard)
$AllChanged = @($Tracked) + @($Staged) + @($Untracked)
$Unexpected = @($AllChanged | Where-Object {
    $_ -and $_ -notmatch '^Content/GrimrockPrototype/(?:Core/DataAssets/Weapons|UI/Icons/Weapons|Meshes/Weapons)/(?:[^/]+/)*[^/]+\.uasset$'
})
if ($Unexpected.Count -gt 0) {
    $Unexpected | ForEach-Object { Write-Host $_ }
    throw 'Changements locaux hors des assets pilotes. Ne rien supprimer : examiner git status avant la reparation.'
}

$PilotMeshes = @('SM_Weapon_Dagger_A','SM_Weapon_ShortBow_A','SM_Weapon_RoundShield_A')
foreach ($Mesh in $PilotMeshes) {
    $Path = Join-Path $RepoRoot ("Content\GrimrockPrototype\Meshes\Weapons\" + $Mesh + '.uasset')
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "Mesh deja importe introuvable : $Path"
    }
}
$Catalogue = Join-Path $RepoRoot 'Content\GrimrockPrototype\Core\DataAssets\Weapons\DA_WeaponVisualCatalogue.uasset'
if (-not (Test-Path -LiteralPath $Catalogue -PathType Leaf)) {
    throw "Catalogue pilote introuvable : $Catalogue"
}

Write-Host '=== ARMEMENT01.6.3 — Compilation avant reparation ==='
& (Join-Path $PSScriptRoot 'ValidateUE.ps1') -EngineRoot $EngineRoot -SkipAutomation
if (-not $?) { throw 'Compilation UE5.5.4 echouee.' }

Write-Host '=== ARMEMENT01.6.3 — Creation/reparation des 14 materiaux ==='
$EditorArgs = @(
    $Project,
    '-run=GridWeaponPilotMaterialRepair',
    "-SourceRoot=$Sources",
    '-Unattended', '-NoSplash', '-NoP4', '-NoSound', '-NullRHI', '-log'
)
& $Editor @EditorArgs
if ($LASTEXITCODE -ne 0) {
    throw "Reparation des materiaux echouee avec le code $LASTEXITCODE."
}

Write-Host '=== ARMEMENT01.6.3 — Tests des assets pilotes ==='
& (Join-Path $PSScriptRoot 'ValidateUE.ps1') -EngineRoot $EngineRoot -SkipBuild -AutomationFilter 'Grimrock.RPG.ARMEMENT01.6.Pilot'
if (-not $?) { throw 'Tests pilotes echoues.' }

Write-Host '=== ARMEMENT01.6.3 — Etat final sans commit automatique ==='
& git -C $RepoRoot status --short
Write-Host '[OK] La reparation des materiaux a ete executee. Inspecter les assets en UE avant Git LFS.'
