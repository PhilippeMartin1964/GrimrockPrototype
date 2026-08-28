param(
    [Parameter(Mandatory = $false)]
    [string]$EngineRoot = "D:\UE_5.5"
)

$ErrorActionPreference = "Stop"

$RepositoryRoot = Split-Path -Parent $PSScriptRoot
$AssetPath = "Content/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_Shuriken.uasset"
$Filter = "Grimrock.TechnicalDebt.TD07_3_5_2.AssetRepair"

Push-Location $RepositoryRoot
try {
    Write-Host "GrimrockPrototype TD07.3.5.2 - Shuriken authoring repair"
    Write-Host "Repository : $RepositoryRoot"
    Write-Host "Asset      : $AssetPath"
    Write-Host ""

    $Branch = (git rev-parse --abbrev-ref HEAD).Trim()
    if ($Branch -ne "master") {
        throw "TD07.3.5.2 repair must run on master. Current branch: $Branch"
    }

    git lfs version | Out-Host
    if ($LASTEXITCODE -ne 0) {
        throw "Git LFS is required to version the repaired .uasset."
    }

    $ExistingAssetChanges = git status --porcelain -- $AssetPath
    if ($ExistingAssetChanges) {
        throw "DA_Weapon_Shuriken already has local changes. Commit/revert them before running this one-shot repair."
    }

    & (Join-Path $PSScriptRoot "ValidateUE.ps1") -EngineRoot $EngineRoot -AutomationFilter $Filter

    if ($LASTEXITCODE -ne 0) {
        throw "TD07.3.5.2 asset repair Automation failed."
    }

    $AssetChanges = git status --porcelain -- $AssetPath
    if (-not $AssetChanges) {
        Write-Host "[OK] Shuriken asset was already current; no LFS commit required."
        exit 0
    }

    Write-Host ""
    Write-Host "=== Version repaired Shuriken asset ==="
    git add -- $AssetPath
    if ($LASTEXITCODE -ne 0) {
        throw "git add failed for $AssetPath"
    }

    git commit --only -m "Repair Shuriken CombatActions authoring" -- $AssetPath
    if ($LASTEXITCODE -ne 0) {
        throw "git commit failed for $AssetPath"
    }

    git push origin master
    if ($LASTEXITCODE -ne 0) {
        throw "git push origin master failed."
    }

    Write-Host "[OK] Repaired Shuriken LFS asset committed and pushed to origin/master."
}
finally {
    Pop-Location
}
