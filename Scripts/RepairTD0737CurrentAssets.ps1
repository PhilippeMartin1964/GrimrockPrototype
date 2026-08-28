param(
    [Parameter(Mandatory = $false)]
    [string]$EngineRoot = "D:\UE_5.5"
)

$ErrorActionPreference = "Stop"

$RepositoryRoot = Split-Path -Parent $PSScriptRoot
$Filter = "Grimrock.TechnicalDebt.TD07_3_7.AssetRepair"

Push-Location $RepositoryRoot
try {
    Write-Host "GrimrockPrototype TD07.3.7 - Current Asset Repair / Recreation"
    Write-Host "Repository : $RepositoryRoot"
    Write-Host ""

    $Branch = (git rev-parse --abbrev-ref HEAD).Trim()
    if ($Branch -ne "master") {
        throw "TD07.3.7 asset repair must run on master. Current branch: $Branch"
    }

    git lfs version | Out-Host
    if ($LASTEXITCODE -ne 0) {
        throw "Git LFS is required for TD07.3.7 asset repair."
    }

    $ExistingContentChanges = @(git status --porcelain -- Content)
    if ($ExistingContentChanges.Count -gt 0) {
        throw "Content already has local changes. Commit or revert them before running TD07.3.7 repair."
    }

    & (Join-Path $PSScriptRoot "ValidateUE.ps1") -EngineRoot $EngineRoot -AutomationFilter $Filter
    if ($LASTEXITCODE -ne 0) {
        throw "TD07.3.7 AssetRepair Automation failed."
    }

    $ChangedAssets = @(git status --porcelain -- Content | ForEach-Object {
        if ($_.Length -gt 3) { $_.Substring(3).Trim() }
    } | Where-Object { $_ -like "*.uasset" } | Sort-Object -Unique)

    if ($ChangedAssets.Count -eq 0) {
        Write-Host "[OK] No Content asset changes were required."
        exit 0
    }

    Write-Host ""
    Write-Host "Repaired / created DataAssets:"
    $ChangedAssets | ForEach-Object { Write-Host "  $_" }

    git add -- $ChangedAssets
    if ($LASTEXITCODE -ne 0) {
        throw "git add failed for TD07.3.7 repaired assets."
    }

    git commit --only -m "Repair TD07.3.7 current authoring assets" -- $ChangedAssets
    if ($LASTEXITCODE -ne 0) {
        throw "git commit failed for TD07.3.7 repaired assets."
    }

    git push origin master
    if ($LASTEXITCODE -ne 0) {
        throw "git push origin master failed."
    }

    Write-Host "[OK] TD07.3.7 repaired DataAssets committed and pushed to origin/master."
}
finally {
    Pop-Location
}
