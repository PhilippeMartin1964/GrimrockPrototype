<#
.SYNOPSIS
    ITEM-VFX01.2 — legacy helper, now restricted to the healing potion only.
.DESCRIPTION
    The default mode is dry-run (read-only). -Apply writes the healing potion .uasset
    packages. -Publish stages and pushes ONLY those four LFS-tracked assets,
    after the user has validated the editor automation tests.
#>
[CmdletBinding()]
param(
    [string]$EngineRoot = "D:\UE_5.5",
    [switch]$Apply,
    [switch]$Publish
)

$ErrorActionPreference = "Stop"
$Repo = Split-Path -Parent $PSScriptRoot
$Project = Join-Path $Repo "GrimrockPrototype.uproject"
$EditorCmd = Join-Path $EngineRoot "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$Assets = @("Content/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Potion_Healing_01.uasset")

Push-Location $Repo
try {
    if ((git branch --show-current).Trim() -ne "master") {
        throw "Only master is supported; switch to master first."
    }
    if ($LASTEXITCODE -ne 0) { throw "Git branch check failed." }
    if (-not (Test-Path $Project)) { throw "Missing project: $Project" }

    if ($Publish) {
        $ExistingStaged = @(git diff --cached --name-only)
        if ($LASTEXITCODE -ne 0) { throw "Unable to inspect Git index." }
        if ($ExistingStaged.Count -ne 0) {
            throw "The Git index contains staged changes; refusing to combine unrelated work."
        }

        git fetch origin master
        if ($LASTEXITCODE -ne 0) { throw "Cannot fetch origin/master." }
        $LocalHead = (git rev-parse HEAD).Trim()
        $RemoteHead = (git rev-parse origin/master).Trim()
        if ($LocalHead -ne $RemoteHead) {
            throw "master differs from origin/master. Resolve the divergence before publishing assets."
        }

        $Changed = @(git diff --name-only -- $Assets)
        if ($LASTEXITCODE -ne 0) { throw "Cannot inspect pending asset changes." }
        if ($Changed.Count -eq 0) {
            Write-Host "[ITEM-VFX01.2] No migrated assets to publish."
            exit 0
        }
        git add -- $Assets
        if ($LASTEXITCODE -ne 0) { throw "git add failed." }

        $Staged = @(git diff --cached --name-only)
        $Unexpected = @($Staged | Where-Object { $Assets -notcontains $_ })
        if ($Unexpected.Count -ne 0) {
            git reset -- $Assets
            throw "Unexpected files staged: $($Unexpected -join ', ')"
        }

        git commit -m "ITEM-VFX01.2: materialize permanent Niagara on potion and flask DataAssets"
        if ($LASTEXITCODE -ne 0) { throw "git commit failed." }
        git push origin master
        if ($LASTEXITCODE -ne 0) { throw "git push failed; commit remains local." }
        Write-Host "[ITEM-VFX01.2] LFS assets committed and pushed to master."
        exit 0
    }

    if (-not (Test-Path $EditorCmd)) { throw "UE editor commandlet not found: $EditorCmd" }
    $Args = @($Project, "-run=GridItemLiquidVfxMigration", "-Unattended", "-nop4", "-NoSplash")
    if ($Apply) { $Args += "-Apply" }
    Write-Host "[ITEM-VFX01.2] Running UE asset migration. Apply=$Apply"
    & $EditorCmd @Args
    if ($LASTEXITCODE -ne 0) { throw "UE migration commandlet failed (code $LASTEXITCODE)." }
    Write-Host "[ITEM-VFX01.2] Success. Review production assets and run the VFX01_2 tests."
}
finally {
    Pop-Location
}
