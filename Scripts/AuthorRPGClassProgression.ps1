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

Write-Host '=== RPG03.10 Canonical class progression materialization ==='
Write-Host "Repository : $RepoRoot"
Write-Host "Project    : $ProjectFile"
Write-Host "Engine     : $EngineRoot"

$Branch = (& git -C $RepoRoot branch --show-current).Trim()
if ($LASTEXITCODE -ne 0) { throw 'Impossible de lire la branche Git courante.' }
if ($Branch -ne 'master') { throw "RPG03.10 doit etre authore sur master. Branche courante : $Branch" }

$GitStatusBefore = @(& git -C $RepoRoot status --short)
if ($LASTEXITCODE -ne 0) { throw 'Impossible de lire git status.' }
if ($GitStatusBefore.Count -gt 0)
{
    Write-Host 'Working tree non propre :'
    $GitStatusBefore | ForEach-Object { Write-Host $_ }
    throw 'Authoring annule afin de ne pas ecraser des changements locaux.'
}

& (Join-Path $PSScriptRoot 'ValidateUE.ps1') -EngineRoot $EngineRoot -SkipAutomation

$Arguments = @(
    $ProjectFile,
    '-run=RPGClassProgressionAuthoring',
    '-Unattended',
    '-NoSplash',
    '-NoP4',
    '-NoSound',
    '-NullRHI',
    '-log'
)

Write-Host ''
Write-Host '=== UE5.5.4 RPG03.10 progression authoring commandlet ==='
& $EditorCmd @Arguments
$ExitCode = $LASTEXITCODE
if ($ExitCode -ne 0)
{
    throw "RPGClassProgressionAuthoring a echoue avec le code $ExitCode."
}

Write-Host '[OK] Canonical Talent Point grants authored on all six DA_Class_* assets.'

if (-not $SkipAutomation)
{
    & (Join-Path $PSScriptRoot 'ValidateUE.ps1') -EngineRoot $EngineRoot -SkipBuild -AutomationFilter 'Grimrock.RPG.RPG03.10'
}

Write-Host ''
Write-Host '=== Generated changes ==='
& git -C $RepoRoot status --short --untracked-files=all
if ($LASTEXITCODE -ne 0) { throw 'Impossible de lire le git status final.' }
Write-Host 'Expected binaries: exactly six modified DA_Class_* assets.'
