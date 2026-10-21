param(
    [string]$EngineRoot = 'D:\UE_5.5',
    [switch]$Apply
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$RepoRoot = Split-Path -Parent $PSScriptRoot
$Project = Join-Path $RepoRoot 'GrimrockPrototype.uproject'
$EditorCmd = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$PalettePath = 'Content/GrimrockPrototype/Core/DataAssets/DA_ObjectPalette_Default.uasset'

if (-not (Test-Path -LiteralPath $Project -PathType Leaf)) {
    throw "Projet Unreal introuvable : $Project"
}
if (-not (Test-Path -LiteralPath $EditorCmd -PathType Leaf)) {
    throw "UnrealEditor-Cmd.exe introuvable : $EditorCmd"
}
if ((& git -C $RepoRoot branch --show-current).Trim() -ne 'master') {
    throw "Reparation autorisee uniquement sur master."
}
& git -C $RepoRoot lfs version
if ($LASTEXITCODE -ne 0) { throw "Git LFS est requis pour modifier la palette." }

$Attr = @(& git -C $RepoRoot check-attr filter -- $PalettePath)
if ($LASTEXITCODE -ne 0 -or $Attr.Count -ne 1 -or $Attr[0] -ne ($PalettePath + ': filter: lfs')) {
    throw "La palette n'est pas suivie via Git LFS : $PalettePath"
}

$Before = @(& git -C $RepoRoot status --porcelain=v1 -- $PalettePath)
if ($LASTEXITCODE -ne 0 -or $Before.Count -ne 0) {
    throw "La palette contient deja des changements locaux. Examiner ces modifications avant une reparation."
}

# The source correction must be compiled before its editor commandlet can run.
& (Join-Path $PSScriptRoot 'ValidateUE.ps1') -EngineRoot $EngineRoot -SkipAutomation
if ($LASTEXITCODE -ne 0 -or -not $?) { throw "Compilation UE5.5.4 echouee." }

$ArgsList = @(
    $Project,
    '-run=GridWeaponPilotPaletteAuthoring',
    '-RepairIcons',
    '-Unattended', '-NoSplash', '-NoP4', '-NoSound', '-NullRHI', '-log'
)
if ($Apply) {
    $ArgsList += '-Apply'
    Write-Host '=== ARMEMENT01.7.1 : REPARATION explicite des icones pilotes ==='
} else {
    Write-Host '=== ARMEMENT01.7.1 : DRY RUN, aucune ecriture ==='
}
& $EditorCmd @ArgsList
if ($LASTEXITCODE -ne 0) {
    throw "La palette reste invalide ou la commande de reparation a echoue. Aucun autre asset ne doit etre modifie."
}

if ($Apply) {
    & (Join-Path $PSScriptRoot 'ValidateUE.ps1') `
        -EngineRoot $EngineRoot `
        -SkipBuild `
        -AutomationFilter 'Grimrock.RPG.ARMEMENT01.7.Palette.ThreeDirectCollectibles'
    if ($LASTEXITCODE -ne 0 -or -not $?) {
        throw "La validation ciblee de la palette a echoue. Examiner le rapport avant de committer."
    }
}

Write-Host '=== Git status (aucun git add ni commit automatique) ==='
& git -C $RepoRoot status --short
if ($LASTEXITCODE -ne 0) { throw "Impossible de lire le statut Git." }
if ($Apply) {
    Write-Host '[OK] Examiner le seul fichier DA_ObjectPalette_Default.uasset avant son commit Git LFS.'
} else {
    Write-Host '[OK] Lecture seule terminee. Relancer avec -Apply seulement si les changements annonces sont acceptes.'
}
