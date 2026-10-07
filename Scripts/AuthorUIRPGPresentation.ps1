param(
    [string]$EngineRoot = $env:UE_ROOT
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

$Branch = (& git -C $RepoRoot branch --show-current).Trim()
if ($LASTEXITCODE -ne 0 -or $Branch -ne 'master')
{
    throw "UI-RPG02.2 doit etre materialise sur master. Branche courante : $Branch"
}

$StatusBefore = @(& git -C $RepoRoot status --short --untracked-files=all)
if ($LASTEXITCODE -ne 0) { throw 'Impossible de lire git status.' }
if ($StatusBefore.Count -gt 0)
{
    $StatusBefore | ForEach-Object { Write-Host $_ }
    throw 'Working tree non propre : materialisation annulee.'
}

Write-Host '=== UI-RPG02.2 minimal production presentation catalog ==='
Write-Host '[OK] master / working tree propre.'

& (Join-Path $PSScriptRoot 'ValidateUE.ps1') -EngineRoot $EngineRoot -SkipAutomation

$Arguments = @($ProjectFile, '-run=UIRPGPresentationAuthoring', '-Unattended', '-NoSplash', '-NoP4', '-NoSound', '-NullRHI', '-log')
& $EditorCmd @Arguments
if ($LASTEXITCODE -ne 0)
{
    throw "UIRPGPresentationAuthoring a echoue avec le code $LASTEXITCODE."
}

$ExpectedPath = 'Content/GrimrockPrototype/Core/DataAssets/UI/RPG/DA_RPGTalentPresentation.uasset'
$StatusAfter = @(& git -C $RepoRoot status --porcelain=v1 --untracked-files=all)
if ($LASTEXITCODE -ne 0) { throw 'Impossible de lire git status apres authoring.' }

$ChangedPaths = @($StatusAfter | ForEach-Object { if ($_.Length -lt 4) { throw "Ligne git status inattendue : $_" }; $_.Substring(3).Trim() })
if ($ChangedPaths.Count -ne 1 -or $ChangedPaths[0] -ne $ExpectedPath)
{
    Write-Host 'Generated changes:'
    $StatusAfter | ForEach-Object { Write-Host $_ }
    throw 'UI-RPG02.2 doit modifier exactement DA_RPGTalentPresentation.uasset.'
}

Write-Host '[OK] Exactly one presentation DataAsset was materialized.'

& (Join-Path $PSScriptRoot 'ValidateUE.ps1') -EngineRoot $EngineRoot -SkipBuild -AutomationFilter 'Grimrock.UI.RPG02.ProductionPresentation'
& (Join-Path $PSScriptRoot 'ValidateUE.ps1') -EngineRoot $EngineRoot -SkipBuild -AutomationFilter 'Grimrock.UI.RPG02.PresentationData'
& (Join-Path $PSScriptRoot 'ValidateUE.ps1') -EngineRoot $EngineRoot -SkipBuild -AutomationFilter 'Grimrock.UI.RPG03.NodeBinding'

Write-Host ''
Write-Host '=== UI-RPG02.2 generated change ==='
& git -C $RepoRoot status --short --untracked-files=all
Write-Host ''
Write-Host '[OK] UI-RPG02.2 local materialization completed.'
Write-Host 'Do not commit until the validation output has been reviewed.'
