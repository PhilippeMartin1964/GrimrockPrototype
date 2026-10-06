param(
    [string]$EngineRoot = $env:UE_ROOT,
    [switch]$SkipRPG03Regression
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

Write-Host '=== UI-RPG01.4 production Talent tree materialization ==='
Write-Host "Repository : $RepoRoot"
Write-Host "Project    : $ProjectFile"
Write-Host "Engine     : $EngineRoot"

$Branch = (& git -C $RepoRoot branch --show-current).Trim()
if ($LASTEXITCODE -ne 0)
{
    throw 'Impossible de lire la branche Git courante.'
}
if ($Branch -ne 'master')
{
    throw "UI-RPG01.4 doit etre materialise sur master. Branche courante : $Branch"
}

$GitStatusBefore = @(& git -C $RepoRoot status --short --untracked-files=all)
if ($LASTEXITCODE -ne 0)
{
    throw 'Impossible de lire git status.'
}
if ($GitStatusBefore.Count -gt 0)
{
    Write-Host 'Working tree non propre :'
    $GitStatusBefore | ForEach-Object { Write-Host $_ }
    throw 'Materialisation annulee afin de ne pas ecraser des changements locaux.'
}

Write-Host '[OK] master / working tree propre.'

& (Join-Path $PSScriptRoot 'ValidateUE.ps1') -EngineRoot $EngineRoot -SkipAutomation

$Arguments = @(
    $ProjectFile,
    '-run=UIRPGTalentTreeAuthoring',
    '-Unattended',
    '-NoSplash',
    '-NoP4',
    '-NoSound',
    '-NullRHI',
    '-log'
)

Write-Host ''
Write-Host '=== UE5.5.4 UI-RPG01.4 authoring commandlet ==='
& $EditorCmd @Arguments
$ExitCode = $LASTEXITCODE
if ($ExitCode -ne 0)
{
    throw "UIRPGTalentTreeAuthoring a echoue avec le code $ExitCode."
}

$ExpectedPaths = @(
    'Content/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Warrior.uasset',
    'Content/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Rogue.uasset',
    'Content/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Ranger.uasset',
    'Content/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Mage.uasset',
    'Content/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Priest.uasset',
    'Content/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Alchemist.uasset'
)

$StatusAfterAuthoring = @(& git -C $RepoRoot status --porcelain=v1 --untracked-files=all)
if ($LASTEXITCODE -ne 0)
{
    throw 'Impossible de lire git status apres authoring.'
}

$ChangedPaths = @(
    $StatusAfterAuthoring | ForEach-Object {
        if ($_.Length -lt 4)
        {
            throw "Ligne git status inattendue : $_"
        }
        $_.Substring(3).Trim()
    }
)

$Unexpected = @($ChangedPaths | Where-Object { $_ -notin $ExpectedPaths })
$Missing = @($ExpectedPaths | Where-Object { $_ -notin $ChangedPaths })

if ($Unexpected.Count -gt 0 -or $Missing.Count -gt 0)
{
    Write-Host 'Generated changes:'
    $StatusAfterAuthoring | ForEach-Object { Write-Host $_ }
    if ($Unexpected.Count -gt 0)
    {
        Write-Host 'Unexpected paths:'
        $Unexpected | ForEach-Object { Write-Host "  $_" }
    }
    if ($Missing.Count -gt 0)
    {
        Write-Host 'Missing expected paths:'
        $Missing | ForEach-Object { Write-Host "  $_" }
    }
    throw 'UI-RPG01.4 binary materialization did not produce exactly the six expected DA_Class_* files.'
}

Write-Host '[OK] Exactly six DA_Class_* binaries were modified.'

& (Join-Path $PSScriptRoot 'ValidateUE.ps1') `
    -EngineRoot $EngineRoot `
    -SkipBuild `
    -AutomationFilter 'Grimrock.UI.RPG01.ProductionAssets'

if (-not $SkipRPG03Regression)
{
    & (Join-Path $PSScriptRoot 'ValidateUE.ps1') `
        -EngineRoot $EngineRoot `
        -SkipBuild `
        -AutomationFilter 'Grimrock.RPG.RPG03'
}

Write-Host ''
Write-Host '=== UI-RPG01.4 generated changes ==='
& git -C $RepoRoot status --short --untracked-files=all
if ($LASTEXITCODE -ne 0)
{
    throw 'Impossible de lire le git status final.'
}

Write-Host ''
Write-Host '[OK] UI-RPG01.4 local materialization completed.'
Write-Host 'Do not commit until the validation output has been reviewed.'
