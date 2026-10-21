param(
    [switch]$Stage
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$RepoRoot = Split-Path -Parent $PSScriptRoot

# ARMEMENT01.6.3-FINAL: exactly 19 accepted art/data packages.
# In particular, never stage Shuriken or ThrowingKnife modifications.
$Root = 'Content/GrimrockPrototype/'
$Approved = @(
    ($Root + 'Core/DataAssets/Weapons/DA_WeaponVisualCatalogue.uasset'),
    ($Root + 'Meshes/Weapons/SM_Weapon_Dagger_01.uasset'),
    ($Root + 'Meshes/Weapons/SM_Weapon_ShortBow_01.uasset'),
    ($Root + 'Meshes/Weapons/SM_Weapon_RoundShield_01.uasset'),
    ($Root + 'UI/Icons/Weapons/T_Weapon_Dagger_01.uasset'),
    ($Root + 'UI/Icons/Weapons/T_Weapon_ShortBow_01.uasset'),
    ($Root + 'UI/Icons/Weapons/T_Weapon_RoundShield_01.uasset'),
    ($Root + 'Art/Materials/Instances/Weapons/MI_Dagger.uasset'),
    ($Root + 'Art/Materials/Instances/Weapons/MI_Bow.uasset'),
    ($Root + 'Art/Materials/Instances/Weapons/MI_Shield.uasset')
)
foreach ($Weapon in @('Dagger','Bow','Shield')) {
    foreach ($Map in @('BC','N','ORM')) {
        $Approved += ($Root + 'Art/Textures/Final/Weapons/' + $Weapon +
            '/T_' + $Weapon + '_01_' + $Map + '.uasset')
    }
}
$Protected = @(
    ($Root + 'Core/DataAssets/Weapons/DA_Weapon_Shuriken.uasset'),
    ($Root + 'Core/DataAssets/Weapons/DA_Weapon_ThrowingKnife.uasset'),
    ($Root + 'UI/Weapons/T_Weapon_Shuriken.uasset'),
    ($Root + 'UI/Weapons/T_Weapon_ThrowingKnife.uasset'),
    ($Root + 'UI/Icons/Weapons/T_Weapon_Shuriken.uasset'),
    ($Root + 'UI/Icons/Weapons/T_Weapon_ThrowingKnife.uasset')
)
if ($Approved.Count -ne 19 -or (@($Approved | Select-Object -Unique)).Count -ne 19) {
    throw 'Liste blanche invalide : exactement 19 chemins uniques sont requis.'
}
$CurrentBranch = (& git -C $RepoRoot branch --show-current)
if ($LASTEXITCODE -ne 0 -or $CurrentBranch.Trim() -ne 'master') {
    throw 'Le lot ARMEMENT01.6.3 doit etre prepare exclusivement sur master.'
}
& git -C $RepoRoot lfs version
if ($LASTEXITCODE -ne 0) { throw 'Git LFS absent. Ne pas publier de .uasset sans LFS.' }

# Refuse existing staged files: never build a mixed commit accidentally.
$AlreadyStaged = @(& git -C $RepoRoot diff --cached --name-only)
if ($LASTEXITCODE -ne 0) { throw 'Impossible de lire index Git.' }
if ($AlreadyStaged.Count -ne 0) {
    $AlreadyStaged | ForEach-Object { Write-Host "Deja indexe : $_" }
    throw 'Index Git non vide. Examiner les changements avant toute preparation.'
}

$Status = @(& git -C $RepoRoot status --porcelain=v1 --untracked-files=all)
if ($LASTEXITCODE -ne 0) { throw 'Impossible de lire git status.' }
foreach ($Entry in $Status) {
    if ($Entry.Length -lt 4) { throw "Entree git status incorrecte : $Entry" }
    $File = $Entry.Substring(3).Replace('\','/')
    if ($File -notin $Approved -and $File -notin $Protected) {
        throw "Changement non autorise hors du lot pilote : $File"
    }
}

foreach ($File in $Approved) {
    $Absolute = Join-Path $RepoRoot $File
    if (-not (Test-Path -LiteralPath $Absolute -PathType Leaf)) {
        throw "Package Unreal manquant : $File"
    }
    if ((Get-Item -LiteralPath $Absolute).Length -lt 128) {
        throw "Package Unreal anormalement petit : $File"
    }
    $Attribute = @(& git -C $RepoRoot check-attr filter -- $File)
    if ($LASTEXITCODE -ne 0 -or $Attribute.Count -ne 1 -or
        $Attribute[0] -ne ($File + ': filter: lfs')) {
        throw "Git LFS non configure pour : $File. Recu: $($Attribute -join '; ')"
    }
}
Write-Host '=== ARMEMENT01.6.3 — Liste blanche : 19 packages Unreal, tous geres par Git LFS ==='
foreach ($File in $Approved) { Write-Host "  $File" }
Write-Host '=== Les six chemins Shuriken et ThrowingKnife sont exclus et preserves ==='

if (-not $Stage) {
    Write-Host '[OK] Audit en lecture seule termine. Aucun git add ni commit.'
    Write-Host 'Apres validation visuelle dans Unreal : relancer avec -Stage.'
    exit 0
}

# Stage ONLY the allowlisted packages; never the whole Content folder.
& git -C $RepoRoot add -- @Approved
if ($LASTEXITCODE -ne 0) { throw 'git add a echoue. Examiner index Git avant de recommencer.' }
$Staged = @(& git -C $RepoRoot diff --cached --name-only)
if ($LASTEXITCODE -ne 0) { throw 'Impossible de relire index Git.' }
$Diff = @(Compare-Object -ReferenceObject @($Approved | Sort-Object) -DifferenceObject @($Staged | Sort-Object))
if ($Staged.Count -ne 19 -or $Diff.Count -ne 0) {
    Write-Host '=== Index Git apres tentative de stage ==='
    $Staged | ForEach-Object { Write-Host $_ }
    throw 'Contenu de index non conforme : refuser commit et push.'
}
$ProtectedStaged = @($Staged | Where-Object { $_ -in $Protected })
if ($ProtectedStaged.Count -gt 0) { throw 'Protection Shuriken/ThrowingKnife violee.' }

Write-Host '=== Contenu exact de index Git : 19 packages ==='
& git -C $RepoRoot diff --cached --name-only
Write-Host '=== Git LFS status ==='
& git -C $RepoRoot lfs status
if ($LASTEXITCODE -ne 0) { throw 'git lfs status en erreur.' }
Write-Host '[OK] 19 packages indices et rien d autre. Aucun commit, aucun push.'
Write-Host 'Examiner le rendu dans Unreal puis verifier la liste avant le commit artistique final.'
