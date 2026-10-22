param(
    [string]$EngineRoot = 'D:\UE_5.5',
    [ValidateSet('ARM014','ARM015','ARM025','ARM031')]
    [string]$Archetype,
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
    throw 'ARMEMENT01.10.2 utilise exclusivement master.'
}
if ($Apply -and -not $Archetype) {
    throw '-Apply exige -Archetype ARM014|ARM015|ARM025|ARM031 : une arme à la fois.'
}

$Allowed = @{
    'ARM014' = 'DA_Weapon_StraightSword_01.uasset'
    'ARM015' = 'DA_Weapon_LongSword_01.uasset'
    'ARM025' = 'DA_Weapon_WarPick_01.uasset'
    'ARM031' = 'DA_Weapon_GreatAxe_01.uasset'
}
$Before = @(& git -C $RepoRoot status --porcelain=v1 --untracked-files=all)
if ($LASTEXITCODE -ne 0) { throw 'Git status a échoué.' }
if ($Apply) {
    $Target = Join-Path $RepoRoot ('Content\GrimrockPrototype\Core\DataAssets\Weapons\' + $Allowed[$Archetype])
    if (Test-Path -LiteralPath $Target -PathType Leaf) {
        throw "Asset déjà existant, aucun écrasement autorisé : $Target"
    }
}

Write-Host '=== ARMEMENT01.10.2 — Compilation Development Editor ==='
& (Join-Path $PSScriptRoot 'ValidateUE.ps1') -EngineRoot $EngineRoot -SkipAutomation
if ($LASTEXITCODE -ne 0) { throw 'Compilation Unreal échouée.' }

$ArgsEditor = @(
    $Project, '-run=GridWarriorN1ItemAuthoring',
    '-unattended', '-nop4', '-nosplash', '-nullrhi', '-log'
)
if ($Archetype) { $ArgsEditor += "-Archetype=$Archetype" }
if ($Apply) {
    $ArgsEditor += '-Apply'
    Write-Host "=== Création contrôlée de $Archetype : un seul asset ==="
} else {
    Write-Host '=== Préflight en lecture seule : absence de ressources autorisée, mais signalée BLOCKED ==='
}

& $Editor @ArgsEditor
$EditorCode = $LASTEXITCODE
if ($EditorCode -ne 0) {
    throw "Authoring refusé (code $EditorCode). Voir les lignes BLOCKED/ERROR. Aucun remplacement automatique."
}

if ($Apply) {
    if (-not (Test-Path -LiteralPath $Target -PathType Leaf)) {
        throw "Le commandlet a réussi mais l'asset attendu est absent : $Target"
    }
    Write-Host '=== Validation ciblée après création ==='
    & (Join-Path $PSScriptRoot 'ValidateUE.ps1') -EngineRoot $EngineRoot -SkipBuild -AutomationFilter 'Grimrock.RPG.ARMEMENT01.10'
    if ($LASTEXITCODE -ne 0) { throw 'Validation ciblée échouée : conserver les logs et ne rien publier.' }
}

$After = @(& git -C $RepoRoot status --porcelain=v1 --untracked-files=all)
if ($LASTEXITCODE -ne 0) { throw 'Impossible de contrôler git status après authoring.' }
$NewStatuses = @($After | Where-Object { $_ -notin $Before })
if ($Apply) {
    $ExpectedRelative = ('Content/GrimrockPrototype/Core/DataAssets/Weapons/' + $Allowed[$Archetype])
    $Unexpected = @($NewStatuses | Where-Object {
        $_.Length -lt 4 -or $_.Substring(3).Replace('\','/') -ne $ExpectedRelative
    })
    if ($Unexpected.Count -gt 0) {
        $Unexpected | ForEach-Object { Write-Host $_ }
        throw 'Changements supplémentaires détectés : NE PAS COMMITTER avant audit.'
    }
} elseif ($NewStatuses.Count -gt 0) {
    $NewStatuses | ForEach-Object { Write-Host $_ }
    throw 'Le préflight a modifié le dépôt de façon inattendue.'
}

Write-Host '=== Etat Git : aucun ajout, commit ni push automatique ==='
& git -C $RepoRoot status --short --untracked-files=all
if ($Apply) {
    Write-Host "[OK] Asset $Archetype créé et testé localement. Revue UE visuelle et tests PIE encore nécessaires."
} else {
    Write-Host '[OK] Préflight terminé. Les ressources manquantes doivent être produites avant -Apply.'
}
