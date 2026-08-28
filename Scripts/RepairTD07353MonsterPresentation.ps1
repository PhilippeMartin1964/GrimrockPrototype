param(
    [Parameter(Mandatory = $false)]
    [string]$EngineRoot = "D:\UE_5.5"
)

$ErrorActionPreference = "Stop"

$RepositoryRoot = Split-Path -Parent $PSScriptRoot
$AssetPaths = @(
    "Content/GrimrockPrototype/Monsters/RatGiant/Data/DA_MON_RatGiant.uasset",
    "Content/GrimrockPrototype/Monsters/GoblinThrower/Data/DA_MON_GoblinThrower.uasset"
)
$Filter = "Grimrock.TechnicalDebt.TD07_3_5_3.AssetRepair"

Push-Location $RepositoryRoot
try {
    Write-Host "GrimrockPrototype TD07.3.5.3 - Monster presentation authoring repair"
    Write-Host "Repository : $RepositoryRoot"
    foreach ($AssetPath in $AssetPaths) {
        Write-Host "Asset      : $AssetPath"
    }
    Write-Host ""

    $Branch = (git rev-parse --abbrev-ref HEAD).Trim()
    if ($Branch -ne "master") {
        throw "TD07.3.5.3 repair must run on master. Current branch: $Branch"
    }

    git lfs version | Out-Host
    if ($LASTEXITCODE -ne 0) {
        throw "Git LFS is required to version the repaired monster DataAssets."
    }

    foreach ($AssetPath in $AssetPaths) {
        $ExistingChanges = git status --porcelain -- $AssetPath
        if ($ExistingChanges) {
            throw "$AssetPath already has local changes. Commit/revert them before running this one-shot repair."
        }
    }

    & (Join-Path $PSScriptRoot "ValidateUE.ps1") -EngineRoot $EngineRoot -AutomationFilter $Filter

    if ($LASTEXITCODE -ne 0) {
        throw "TD07.3.5.3 asset repair Automation failed."
    }

    $ChangedAssets = @()
    foreach ($AssetPath in $AssetPaths) {
        $AssetChanges = git status --porcelain -- $AssetPath
        if ($AssetChanges) {
            $ChangedAssets += $AssetPath
        }
    }

    if ($ChangedAssets.Count -eq 0) {
        Write-Host "[OK] Monster presentation assets were already current; no LFS commit required."
        exit 0
    }

    Write-Host ""
    Write-Host "=== Version repaired monster presentation assets ==="
    git add -- $ChangedAssets
    if ($LASTEXITCODE -ne 0) {
        throw "git add failed for repaired monster DataAssets."
    }

    git commit --only -m "Repair monster presentation authoring" -- $ChangedAssets
    if ($LASTEXITCODE -ne 0) {
        throw "git commit failed for repaired monster DataAssets."
    }

    git push origin master
    if ($LASTEXITCODE -ne 0) {
        throw "git push origin master failed."
    }

    Write-Host "[OK] Repaired monster presentation LFS assets committed and pushed to origin/master."
}
finally {
    Pop-Location
}
