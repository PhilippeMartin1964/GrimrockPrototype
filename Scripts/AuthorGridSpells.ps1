param(
    [string]$EngineRoot = $env:UE_ROOT,
    [switch]$SkipPreValidation,
    [switch]$SkipPostValidation
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

Write-Host '=== UI-RPG-CONTENT-TEXT05.5A Spell DataAsset materialization ==='
Write-Host "Repository : $RepoRoot"
Write-Host "Project    : $ProjectFile"
Write-Host "Engine     : $EngineRoot"

$Branch = (& git -C $RepoRoot branch --show-current).Trim()
if ($LASTEXITCODE -ne 0) { throw 'Impossible de lire la branche Git courante.' }
if ($Branch -ne 'master') { throw "La materialisation Spell doit etre executee sur master. Branche courante : $Branch" }

$GitStatusBefore = @(& git -C $RepoRoot status --short --untracked-files=all)
if ($LASTEXITCODE -ne 0) { throw 'Impossible de lire git status.' }
if ($GitStatusBefore.Count -gt 0)
{
    Write-Host 'Working tree non propre :'
    $GitStatusBefore | ForEach-Object { Write-Host $_ }
    throw 'Materialisation annulee afin de ne pas ecraser des changements locaux.'
}

Write-Host '[OK] master / working tree propre.'

if (-not $SkipPreValidation)
{
    & (Join-Path $PSScriptRoot 'ValidateUE.ps1') -EngineRoot $EngineRoot -AutomationFilter 'Grimrock.UI.RPG.CONTENTTEXT05'
}

$Arguments = @(
    $ProjectFile,
    '-run=GridSpellCatalogAuthoring',
    '-Unattended',
    '-NoSplash',
    '-NoP4',
    '-NoSound',
    '-NullRHI',
    '-log'
)

Write-Host ''
Write-Host '=== UE5.5.4 TEXT05.5A Spell authoring commandlet ==='
& $EditorCmd @Arguments
$ExitCode = $LASTEXITCODE
if ($ExitCode -ne 0) { throw "GridSpellCatalogAuthoring a echoue avec le code $ExitCode." }

$ExpectedRelativePaths = @(
    'Content/GrimrockPrototype/Core/DataAssets/Magic/Spells/DA_Spell_ArcaneBolt.uasset',
    'Content/GrimrockPrototype/Core/DataAssets/Magic/Spells/DA_Spell_LesserHeal.uasset',
    'Content/GrimrockPrototype/Core/DataAssets/Magic/Spells/DA_Spell_Haste.uasset',
    'Content/GrimrockPrototype/Core/DataAssets/Magic/Spells/DA_Spell_CurePoison.uasset'
)

foreach ($RelativePath in $ExpectedRelativePaths)
{
    $FullPath = Join-Path $RepoRoot ($RelativePath -replace '/', '\')
    if (-not (Test-Path -LiteralPath $FullPath -PathType Leaf)) { throw "Asset Spell attendu absent apres authoring : $RelativePath" }
}

Write-Host '[OK] Les 4 DA_Spell_* attendus existent sur disque.'

$GitStatusAfter = @(& git -C $RepoRoot status --porcelain=v1 --untracked-files=all)
if ($LASTEXITCODE -ne 0) { throw 'Impossible de lire git status apres authoring.' }
$ChangedPaths = @(
    $GitStatusAfter | ForEach-Object {
        if ($_.Length -ge 4) { $_.Substring(3).Trim().Replace('\', '/') }
    } | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
)
$UnexpectedPaths = @($ChangedPaths | Where-Object { $_ -notin $ExpectedRelativePaths })
$MissingChanges = @($ExpectedRelativePaths | Where-Object { $_ -notin $ChangedPaths })

if ($UnexpectedPaths.Count -gt 0 -or $MissingChanges.Count -gt 0)
{
    Write-Host ''
    Write-Host '=== git status apres authoring ==='
    $GitStatusAfter | ForEach-Object { Write-Host $_ }
    if ($UnexpectedPaths.Count -gt 0) { Write-Host 'Changements inattendus :'; $UnexpectedPaths | ForEach-Object { Write-Host "  $_" } }
    if ($MissingChanges.Count -gt 0) { Write-Host 'Assets attendus absents de git status :'; $MissingChanges | ForEach-Object { Write-Host "  $_" } }
    throw 'La frontiere binaire TEXT05.5A n est pas exactement celle attendue.'
}

Write-Host '[OK] Frontiere binaire exacte : 4 DA_Spell_* et aucun autre changement.'

if (-not $SkipPostValidation)
{
    & (Join-Path $PSScriptRoot 'ValidateUE.ps1') -EngineRoot $EngineRoot -SkipBuild -AutomationFilter 'Grimrock.UI.RPG.CONTENTTEXT05'
    & (Join-Path $PSScriptRoot 'ValidateUE.ps1') -EngineRoot $EngineRoot -SkipBuild -AutomationFilter 'Grimrock.Magic.MON18.1'
}

Write-Host ''
Write-Host '=== Generated Spell assets ==='
$GitStatusAfter | ForEach-Object { Write-Host $_ }
Write-Host ''
Write-Host '[OK] TEXT05.5A materialisation locale terminee.'
Write-Host 'Ne pas modifier les .uasset hors Unreal. Commit atomique recommande :'
Write-Host '  UI-RPG-CONTENT-TEXT05.5A materialize production spell assets'
