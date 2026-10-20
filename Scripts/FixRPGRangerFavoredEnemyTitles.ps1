param(
    [string]$EngineRoot = $env:UE_ROOT
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$RepoRoot = Split-Path -Parent $PSScriptRoot
$ProjectFile = Join-Path $RepoRoot 'GrimrockPrototype.uproject'
$ExpectedChanged = 'Content/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Ranger.uasset'

if ([string]::IsNullOrWhiteSpace($EngineRoot))
{
    throw 'Racine Unreal absente : utilisez -EngineRoot D:\UE_5.5.'
}
$EngineRoot = (Resolve-Path -LiteralPath $EngineRoot).Path
$EditorCmd = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
if (-not (Test-Path -LiteralPath $EditorCmd -PathType Leaf))
{
    throw "UnrealEditor-Cmd.exe introuvable : $EditorCmd"
}

$Branch = (& git -C $RepoRoot branch --show-current).Trim()
if ($LASTEXITCODE -ne 0 -or $Branch -ne 'master')
{
    throw 'Le correctif doit être matérialisé sur master.'
}
$BeforeStatus = @(& git -C $RepoRoot status --short --untracked-files=all)
if ($LASTEXITCODE -ne 0 -or $BeforeStatus.Count -gt 0)
{
    $BeforeStatus | ForEach-Object { Write-Host $_ }
    throw 'Working tree non propre. Matérialisation annulée.'
}

Write-Host '=== RPG03.9.3-FIX — Synchronisation ciblée Ennemi juré ==='
& (Join-Path $PSScriptRoot 'ValidateUE.ps1') -EngineRoot $EngineRoot -SkipAutomation
if (-not $?)
{
    throw 'La compilation UE5 a échoué.'
}

$Arguments = @(
    $ProjectFile,
    '-run=RPGRangerAuthoring',
    '-FavoredEnemyTitlesOnly',
    '-Unattended',
    '-NoSplash',
    '-NoP4',
    '-NoSound',
    '-NullRHI',
    '-log'
)

Write-Host '=== Correction du seul DA_Class_Ranger ==='
& $EditorCmd @Arguments
if ($LASTEXITCODE -ne 0)
{
    throw "Synchronisation du Rôdeur échouée (code $LASTEXITCODE)."
}

$ChangedFiles = @(& git -C $RepoRoot diff --name-only)
if ($LASTEXITCODE -ne 0) { throw 'Impossible de lire le diff Git.' }
$UntrackedFiles = @(& git -C $RepoRoot ls-files --others --exclude-standard)
if ($LASTEXITCODE -ne 0) { throw 'Impossible de lire les fichiers non suivis.' }
$Unexpected = @($ChangedFiles + $UntrackedFiles | Where-Object { $_ -and $_ -ne $ExpectedChanged })
if ($Unexpected.Count -gt 0)
{
    Write-Host 'Fichiers inattendus :'
    $Unexpected | ForEach-Object { Write-Host $_ }
    throw 'La matérialisation a modifié autre chose que DA_Class_Ranger : ne pas committer.'
}
Write-Host '[OK] Aucun fichier hors du DataAsset Ranger n''a été modifié.'

& (Join-Path $PSScriptRoot 'ValidateUE.ps1') -EngineRoot $EngineRoot -SkipBuild -AutomationFilter 'Grimrock.RPG.RPG03.9.3'
if (-not $?)
{
    throw 'Les tests RPG03.9.3 ont échoué.'
}

Write-Host '=== Diff attendu ==='
& git -C $RepoRoot status --short --untracked-files=all
Write-Host '[OK] Vérifier puis committer uniquement DA_Class_Ranger.uasset (Git LFS).'
