param(
    [string]$EngineRoot = $env:UE_ROOT,
    [switch]$SkipAutomation,
    [switch]$StartingDisarmTool,
    [switch]$Apply
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

Write-Host '=== RPG03.9.2 Rogue authoring ==='
Write-Host "Repository : $RepoRoot"
Write-Host "Project    : $ProjectFile"
Write-Host "Engine     : $EngineRoot"

$Branch = (& git -C $RepoRoot branch --show-current).Trim()
if ($LASTEXITCODE -ne 0) { throw 'Impossible de lire la branche Git courante.' }
if ($Branch -ne 'master') { throw "RPG03.9.2 doit être authoré sur master. Branche courante : $Branch" }

# ARMEMENT01.16.1: isolated production of the Saboteur physical tool.
# No change to Rogue statuses, kits, or other item packages.
if ($StartingDisarmTool)
{
    if (-not $Apply) { throw 'StartingDisarmTool requires explicit -Apply.' }
    Write-Host '=== ARMEMENT01.16.1 - N1 Saboteur physical disarm tool ==='
    & (Join-Path $PSScriptRoot 'ValidateUE.ps1') -EngineRoot $EngineRoot -SkipAutomation
    if ($LASTEXITCODE -ne 0 -or -not $?) { throw 'Editor build failed.' }

    $Shared = @($ProjectFile, '-Unattended', '-NoSplash', '-NoP4', '-NoSound', '-NullRHI', '-log')
    & $EditorCmd @Shared '-run=UIStringTableAuthoring'
    if ($LASTEXITCODE -ne 0) { throw 'ST_UI synchronization failed.' }

    & $EditorCmd @Shared '-run=RPGRogueAuthoring' '-StartingDisarmTool' '-Apply'
    if ($LASTEXITCODE -ne 0) { throw 'N1 disarm tool authoring failed.' }

    if (-not $SkipAutomation)
    {
        & (Join-Path $PSScriptRoot 'ValidateUE.ps1') -EngineRoot $EngineRoot -SkipBuild `
            -AutomationFilter 'Grimrock.RPG.ARMEMENT01.16.DisarmTool'
        if ($LASTEXITCODE -ne 0 -or -not $?) { throw 'Disarm tool contract validation failed.' }
    }
    Write-Host '[OK] Physical tool authored; gameplay disarm and kit grant are intentionally NOT enabled.'
    & git -C $RepoRoot status --short
    return
}
if ($Apply) { throw '-Apply is only valid with -StartingDisarmTool.' }

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
    '-run=RPGRogueAuthoring',
    '-Unattended',
    '-NoSplash',
    '-NoP4',
    '-NoSound',
    '-NullRHI',
    '-log'
)

Write-Host ''
Write-Host '=== UE5.5.4 RPG03.9.2 authoring commandlet ==='
& $EditorCmd @Arguments
$ExitCode = $LASTEXITCODE
if ($ExitCode -ne 0)
{
    throw "RPGRogueAuthoring a echoue avec le code $ExitCode."
}

Write-Host '[OK] DA_Class_Rogue and Rogue status assets authored through Unreal Editor.'

if (-not $SkipAutomation)
{
    & (Join-Path $PSScriptRoot 'ValidateUE.ps1') -EngineRoot $EngineRoot -SkipBuild -AutomationFilter 'Grimrock.RPG.RPG03.9.2'
}

Write-Host ''
Write-Host '=== Generated changes ==='
& git -C $RepoRoot status --short
if ($LASTEXITCODE -ne 0) { throw 'Impossible de lire le git status final.' }
Write-Host 'Expected: DA_Class_Rogue plus nine DA_Status_* assets as the only generated RPG binary changes.'
