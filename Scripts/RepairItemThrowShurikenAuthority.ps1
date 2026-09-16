param(
    [Parameter(Mandatory = $false)]
    [string]$EngineRoot = "D:\UE_5.5"
)

$ErrorActionPreference = "Stop"

$RepositoryRoot = Split-Path -Parent $PSScriptRoot
$AssetPath = "Content/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_Shuriken.uasset"
$Filter = "Grimrock.Items.ITEM_THROW_MIG01"

Push-Location $RepositoryRoot
try {
    Write-Host "GrimrockPrototype ITEM-THROW-MIG01 - Shuriken throw-authority migration"
    Write-Host "Repository : $RepositoryRoot"
    Write-Host "Asset      : $AssetPath"
    Write-Host ""

    $Branch = (git rev-parse --abbrev-ref HEAD).Trim()
    if ($Branch -ne "master") {
        throw "ITEM-THROW-MIG01 must run on master. Current branch: $Branch"
    }

    git lfs version | Out-Host
    if ($LASTEXITCODE -ne 0) {
        throw "Git LFS is required to version the migrated .uasset."
    }

    $ExistingAssetChanges = git status --porcelain -- $AssetPath
    if ($ExistingAssetChanges) {
        throw "DA_Weapon_Shuriken already has local changes. Commit or restore them before running this one-shot migration."
    }

    & (Join-Path $PSScriptRoot "ValidateUE.ps1") -EngineRoot $EngineRoot -AutomationFilter $Filter
    if ($LASTEXITCODE -ne 0) {
        throw "ITEM-THROW-MIG01 Automation failed."
    }

    $AssetChanges = git status --porcelain -- $AssetPath
    if (-not $AssetChanges) {
        Write-Host "[OK] Shuriken already uses canonical throw authority; no LFS commit required."
        exit 0
    }

    Write-Host ""
    Write-Host "=== Version migrated Shuriken asset ==="
    git add -- $AssetPath
    if ($LASTEXITCODE -ne 0) {
        throw "git add failed for $AssetPath"
    }

    git commit --only -m "ITEM-THROW-MIG01 migrate Shuriken throw authority" -- $AssetPath
    if ($LASTEXITCODE -ne 0) {
        throw "git commit failed for $AssetPath"
    }

    git push origin master
    if ($LASTEXITCODE -ne 0) {
        throw "git push origin master failed."
    }

    Write-Host "[OK] Migrated Shuriken LFS asset committed and pushed to origin/master."
}
finally {
    Pop-Location
}
