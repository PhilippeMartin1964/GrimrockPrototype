param(
    [string]$EngineRoot = $env:UE_ROOT,
    [switch]$SkipAutomation
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$RepoRoot = Split-Path -Parent $PSScriptRoot
$ProjectFile = Join-Path $RepoRoot 'GrimrockPrototype.uproject'

if ([string]::IsNullOrWhiteSpace($EngineRoot))
{
    throw 'Racine Unreal Engine non renseignee. Utilisez -EngineRoot ou definissez UE_ROOT.'
}

$EngineRoot = (Resolve-Path -LiteralPath $EngineRoot).Path
$EditorCmd = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
if (-not (Test-Path -LiteralPath $EditorCmd -PathType Leaf))
{
    throw "UnrealEditor-Cmd.exe introuvable : $EditorCmd"
}

Write-Host '=== RPG03.9.3 Ranger authoring ==='
Write-Host "Repository : $RepoRoot"
Write-Host "Project    : $ProjectFile"
Write-Host "Engine     : $EngineRoot"

$Branch = (& git -C $RepoRoot branch --show-current).Trim()
if ($LASTEXITCODE -ne 0) { throw 'Impossible de lire la branche Git courante.' }
if ($Branch -ne 'master') { throw "RPG03.9.3 doit être authoré sur master. Branche courante : $Branch" }

$GitStatusBefore = @(& git -C $RepoRoot status --short)
if ($LASTEXITCODE -ne 0) { throw 'Impossible de lire git status.' }
if ($GitStatusBefore.Count -gt 0)
{
    Write-Host 'Working tree non propre :'
    $GitStatusBefore | ForEach-Object { Write-Host $_ }
    throw 'Authoring annulé afin de ne pas écraser des changements locaux.'
}

Write-Host '[OK] master / working tree propre.'

& (Join-Path $PSScriptRoot 'ValidateUE.ps1') -EngineRoot $EngineRoot -SkipAutomation

$Arguments = @(
    $ProjectFile,
    '-run=RPGRangerAuthoring',
    '-Unattended',
    '-NoSplash',
    '-NoP4',
    '-NoSound',
    '-NullRHI',
    '-log'
)

Write-Host ''
Write-Host '=== UE5.5.4 RPG03.9.3 authoring commandlet ==='
& $EditorCmd @Arguments
$ExitCode = $LASTEXITCODE
if ($ExitCode -ne 0)
{
    throw "RPGRangerAuthoring a echoue avec le code $ExitCode."
}

Write-Host '[OK] Ranger, marked status, bestiary category presentation and monster category references authored through Unreal Editor.'

if (-not $SkipAutomation)
{
    & (Join-Path $PSScriptRoot 'ValidateUE.ps1') -EngineRoot $EngineRoot -SkipBuild -AutomationFilter 'Grimrock.RPG.RPG03.9.3'
}

Write-Host ''
Write-Host '=== Generated changes ==='
& git -C $RepoRoot status --short --untracked-files=all
if ($LASTEXITCODE -ne 0) { throw 'Impossible de lire le git status final.' }
Write-Host 'Expected D04 first materialization changes:'
Write-Host '  DA_Class_Ranger'
Write-Host '  DA_Status_MarkedByRanger'
Write-Host '  DA_MONCAT_Goblin'
Write-Host '  DA_MONCAT_Vermin'
Write-Host '  DA_MON_GoblinThrower'
Write-Host '  DA_MON_RatGiant'
