param(
    [string]$EngineRoot = 'D:\UE_5.5',
    [switch]$Apply
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$RepoRoot = Split-Path -Parent $PSScriptRoot
$Project = Join-Path $RepoRoot 'GrimrockPrototype.uproject'
$Editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'

if (-not (Test-Path -LiteralPath $Editor -PathType Leaf)) {
    throw "UnrealEditor-Cmd.exe introuvable : $Editor"
}
if ((& git -C $RepoRoot branch --show-current).Trim() -ne 'master') {
    throw 'ARMEMENT01.6.3-FINAL utilise exclusivement master.'
}

# Explicit allowlist: 19 visual assets plus six unrelated paths which
# must not be staged, reverted, modified or included in this ticket.
$Root = 'Content/GrimrockPrototype/'
$Unrelated = @(
    ($Root + 'Core/DataAssets/Weapons/DA_Weapon_Shuriken.uasset'),
    ($Root + 'Core/DataAssets/Weapons/DA_Weapon_ThrowingKnife.uasset'),
    ($Root + 'UI/Weapons/T_Weapon_Shuriken.uasset'),
    ($Root + 'UI/Weapons/T_Weapon_ThrowingKnife.uasset'),
    ($Root + 'UI/Icons/Weapons/T_Weapon_Shuriken.uasset'),
    ($Root + 'UI/Icons/Weapons/T_Weapon_ThrowingKnife.uasset')
)
$Approved = @(
    ($Root + 'Art/Materials/Instances/Weapons/MI_Bow.uasset'),
    ($Root + 'Art/Materials/Instances/Weapons/MI_Dagger.uasset'),
    ($Root + 'Art/Materials/Instances/Weapons/MI_Shield.uasset'),
    ($Root + 'Core/DataAssets/Weapons/DA_WeaponVisualCatalogue.uasset'),
    ($Root + 'Meshes/Weapons/SM_Weapon_Dagger_01.uasset'),
    ($Root + 'Meshes/Weapons/SM_Weapon_RoundShield_01.uasset'),
    ($Root + 'Meshes/Weapons/SM_Weapon_ShortBow_01.uasset'),
    ($Root + 'UI/Icons/Weapons/T_Weapon_Dagger_01.uasset'),
    ($Root + 'UI/Icons/Weapons/T_Weapon_RoundShield_01.uasset'),
    ($Root + 'UI/Icons/Weapons/T_Weapon_ShortBow_01.uasset')
)
foreach ($Name in @('Bow','Dagger','Shield')) {
    foreach ($Suffix in @('BC','N','ORM')) {
        $Approved += ($Root + "Art/Textures/Final/Weapons/$Name/T_" + $Name + "_01_$Suffix.uasset")
    }
}
if ($Approved.Count -ne 19) { throw 'Erreur interne : 19 assets pilotes attendus.' }

$Statuses = @(& git -C $RepoRoot status --porcelain=v1 --untracked-files=all)
if ($LASTEXITCODE -ne 0) { throw 'git status a echoue.' }
foreach ($Line in $Statuses) {
    if ($Line.Length -lt 4) { throw "Statut Git non analysable: $Line" }
    $File = $Line.Substring(3).Replace('\','/')
    if ($File -notin $Approved -and $File -notin $Unrelated) {
        throw "Changement local hors perimetre, aucun fichier modifie : $File"
    }
}
foreach ($File in $Approved) {
    if (-not (Test-Path -LiteralPath (Join-Path $RepoRoot $File) -PathType Leaf)) {
        throw "Asset valide absent du disque : $File"
    }
}
Write-Host '=== ARMEMENT01.6.3-FINAL — 19 assets presents, Shuriken et ThrowingKnife preserves ==='

if (-not $Apply) {
    Write-Host '=== Compilation UE5.5.4 et validation du catalogue (lecture seule) ==='
    & (Join-Path $PSScriptRoot 'ValidateUE.ps1') -EngineRoot $EngineRoot -SkipAutomation
    if (-not $?) { throw 'Compilation echouee.' }
}

$CommandletArgs = @(
    $Project, '-run=GridWeaponVisualFinalize',
    '-Unattended', '-NoSplash', '-NoP4', '-NoSound', '-NullRHI', '-log'
)
if ($Apply) {
    $CommandletArgs += '-Apply'
    Write-Host '=== Mise a jour explicite du SEUL DA_WeaponVisualCatalogue ==='
}
else {
    Write-Host '=== DRY RUN : aucune sauvegarde Unreal ==='
}
& $Editor @CommandletArgs
if ($LASTEXITCODE -ne 0) {
    throw "Commandlet de finalisation echoue : $LASTEXITCODE"
}

if ($Apply) {
    Write-Host '=== Validation ciblee UE5.5.4 ==='
    & (Join-Path $PSScriptRoot 'ValidateUE.ps1') -EngineRoot $EngineRoot -SkipBuild -AutomationFilter 'Grimrock.RPG.ARMEMENT01.6.Pilot'
    if (-not $?) { throw 'Validation ciblee echouee.' }
}

Write-Host '=== Etat Git (aucun commit, aucun add, aucune suppression) ==='
& git -C $RepoRoot status --short --untracked-files=all
if ($Apply) {
    Write-Host '[OK] Catalogue numerique actualise et tests pilotes executes.'
    Write-Host 'Lancer ensuite ValidateUE.ps1 -AutomationFilter Grimrock puis examiner les 3 armes dans Unreal.'
}
else {
    Write-Host '[OK] Preflight en lecture seule termine. Si tout convient : relancer avec -Apply.'
}
