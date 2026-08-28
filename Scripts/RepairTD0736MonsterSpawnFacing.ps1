param(
    [Parameter(Mandatory = $false)]
    [string]$EngineRoot = "D:\UE_5.5"
)

$ErrorActionPreference = "Stop"

$RepositoryRoot = Split-Path -Parent $PSScriptRoot
$Filter = "Grimrock.TechnicalDebt.TD07_3_6.AssetRepair.MonsterSpawnFacing"

Push-Location $RepositoryRoot
try {
    Write-Host "GrimrockPrototype TD07.3.6 - MonsterSpawn facing current-asset repair"
    Write-Host "Repository : $RepositoryRoot"
    Write-Host ""

    $Branch = (git rev-parse --abbrev-ref HEAD).Trim()
    if ($Branch -ne "master") {
        throw "TD07.3.6 repair must run on master. Current branch: $Branch"
    }

    git lfs version | Out-Host
    if ($LASTEXITCODE -ne 0) {
        throw "Git LFS is required to version repaired GridLevelAsset packages."
    }

    $ExistingContentChanges = @(git status --porcelain -- Content)
    if ($ExistingContentChanges.Count -gt 0) {
        throw "Content already has local changes. Commit or revert them before running this one-shot repair."
    }

    & (Join-Path $PSScriptRoot "ValidateUE.ps1") -EngineRoot $EngineRoot -AutomationFilter $Filter
    if ($LASTEXITCODE -ne 0) {
        throw "TD07.3.6 MonsterSpawn facing AssetRepair Automation failed."
    }

    $ChangedAssets = @(git diff --name-only -- Content | Where-Object { $_ -like "*.uasset" })
    if ($ChangedAssets.Count -eq 0) {
        Write-Host "[OK] Current GridLevelAssets were already serialized with current MonsterSpawn facing; no LFS commit required."
        exit 0
    }

    Write-Host ""
    Write-Host "Repaired GridLevelAsset files:"
    $ChangedAssets | ForEach-Object { Write-Host "  $_" }

    git add -- $ChangedAssets
    if ($LASTEXITCODE -ne 0) {
        throw "git add failed for repaired GridLevelAssets."
    }

    git commit --only -m "Repair MonsterSpawn facing authoring" -- $ChangedAssets
    if ($LASTEXITCODE -ne 0) {
        throw "git commit failed for repaired GridLevelAssets."
    }

    git push origin master
    if ($LASTEXITCODE -ne 0) {
        throw "git push origin master failed."
    }

    Write-Host "[OK] Repaired MonsterSpawn facing GridLevelAssets committed and pushed to origin/master."
}
finally {
    Pop-Location
}
