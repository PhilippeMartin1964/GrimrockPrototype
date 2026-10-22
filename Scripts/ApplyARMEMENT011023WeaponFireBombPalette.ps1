param(
    [string]$EngineRoot = 'D:\UE_5.5',
    [switch]$Apply
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$RepoRoot = Split-Path -Parent $PSScriptRoot
$Project = Join-Path $RepoRoot 'GrimrockPrototype.uproject'
$Editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$PaletteRelative = 'Content/GrimrockPrototype/Core/DataAssets/DA_ObjectPalette_Default.uasset'
$PaletteFile = Join-Path $RepoRoot ($PaletteRelative.Replace('/', '\'))

if (-not (Test-Path -LiteralPath $Editor -PathType Leaf)) {
    throw "UnrealEditor-Cmd.exe introuvable : $Editor"
}
if (-not (Test-Path -LiteralPath $PaletteFile -PathType Leaf)) {
    throw "Palette introuvable : $PaletteFile"
}
if ((& git -C $RepoRoot branch --show-current).Trim() -ne 'master') {
    throw 'ARMEMENT01.10.2.3 : master uniquement.'
}
if ($Apply -and (Get-Process -Name 'UnrealEditor' -ErrorAction SilentlyContinue)) {
    throw 'Fermer Unreal Editor avant -Apply pour eviter toute sauvegarde concurrente.'
}

$StatusBefore = @(& git -C $RepoRoot status --porcelain=v1 --untracked-files=all)
if ($LASTEXITCODE -ne 0) { throw 'git status a echoue' }
$HashBefore = (Get-FileHash -LiteralPath $PaletteFile -Algorithm SHA256).Hash
Write-Host '=== ARMEMENT01.10.2.3 : Development Editor build ==='
& (Join-Path $PSScriptRoot 'ValidateUE.ps1') -EngineRoot $EngineRoot -SkipAutomation
if ($LASTEXITCODE -ne 0) { throw 'Compilation Unreal echouee' }

$LogDir = Join-Path $RepoRoot 'Saved\ARMEMENT01'
New-Item -ItemType Directory -Path $LogDir -Force | Out-Null
$LogFile = Join-Path $LogDir 'ARMEMENT01_10_2_3_WeaponFireBombPalette.log'
$ArgsEditor = @(
    $Project, '-run=GridWeaponFireBombPalette',
    '-Unattended', '-NoSplash', '-NoP4', '-NoSound', '-NullRHI', '-log'
)
if ($Apply) { $ArgsEditor += '-Apply' }
Write-Host "=== Palette preflight / migration (log : $LogFile) ==="
& $Editor @ArgsEditor *> $LogFile
$Code = $LASTEXITCODE
@((Select-String -LiteralPath $LogFile -Pattern '\[ARMEMENT01\.10\.2\.3\]' -ErrorAction SilentlyContinue)) |
    ForEach-Object { Write-Host $_.Line }

if ($Code -ne 0) {
    Get-Content -LiteralPath $LogFile -Tail 25
    throw "Migration refusee (code $Code). Aucun git add ni commit."
}
$HashAfter = (Get-FileHash -LiteralPath $PaletteFile -Algorithm SHA256).Hash
if (-not $Apply -and $HashBefore -ne $HashAfter) {
    throw 'VIOLATION : palette modifiee durant le preflight read-only.'
}

$StatusAfter = @(& git -C $RepoRoot status --porcelain=v1 --untracked-files=all)
if ($LASTEXITCODE -ne 0) { throw 'git status apres migration a echoue' }
$UnexpectedStatuses = @($StatusAfter | Where-Object {
    ($_ -notin $StatusBefore) -and
    ($_.Length -lt 4 -or $_.Substring(3).Replace('\','/') -ne $PaletteRelative)
})
if ($UnexpectedStatuses.Count -gt 0) {
    $UnexpectedStatuses | ForEach-Object { Write-Host $_ }
    throw 'Fichiers inattendus modifies : ne rien committer avant examen.'
}

if ($Apply) {
    Write-Host '=== Regression ARMEMENT01.10 (inclut les tests palette) ==='
    & (Join-Path $PSScriptRoot 'ValidateUE.ps1') -EngineRoot $EngineRoot -SkipBuild `
        -AutomationFilter 'Grimrock.RPG.ARMEMENT01.10'
    if ($LASTEXITCODE -ne 0) { throw 'Regression ARMEMENT01.10 echouee. Ne rien publier.' }

    Write-Host '=== Compatibilite ARMEMENT01.7.Palette ==='
    & (Join-Path $PSScriptRoot 'ValidateUE.ps1') -EngineRoot $EngineRoot -SkipBuild `
        -AutomationFilter 'Grimrock.RPG.ARMEMENT01.7.Palette'
    if ($LASTEXITCODE -ne 0) { throw 'Regression palette historique echouee. Ne rien publier.' }
}
Write-Host '=== Statut Git : aucun stage, commit ou push automatique ==='
& git -C $RepoRoot status --short --untracked-files=all
if ($Apply) {
    Write-Host '[OK] Palette ecrite uniquement si changements utiles; revue Grid Editor et PIE necessaire.'
    Write-Host 'ATTENTION: la palette etait deja modifiee localement. Examiner avant le commit.'
} else {
    Write-Host '[OK] PREFLIGHT LECTURE SEULE. Appliquer ensuite explicitement avec -Apply.'
}
