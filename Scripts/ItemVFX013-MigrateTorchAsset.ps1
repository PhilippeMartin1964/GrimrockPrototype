<#
.SYNOPSIS
    ITEM-VFX01.3 — migrate the canonical torch's flame to WhenLightEnabled Niagara.
.DESCRIPTION
    Default: dry-run, no Unreal package writes.
    -Apply: save DA_Item_Torch.uasset using UE 5.5.4 Editor commandlet.
    -Publish: stage, commit and push ONLY the whitelisted Git LFS torch asset
              after passing the VFX01_3 and lighting regression tests.
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
$Asset = "Content/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Torch.uasset"

if ($Apply -and $Publish) {
    throw "Use -Apply and -Publish in separate commands, after validating Unreal tests."
}

Push-Location $Repo
try {
    $Branch = (git branch --show-current).Trim()
    if ($LASTEXITCODE -ne 0 -or $Branch -ne "master") {
        throw "ITEM-VFX01.3 only supports master."
    }

    if (-not (Test-Path $Project)) {
        throw "Unreal project file not found: $Project"
    }

    if ($Publish) {
        $PreviouslyStaged = @(git diff --cached --name-only)
        if ($LASTEXITCODE -ne 0 -or $PreviouslyStaged.Count -ne 0) {
            throw "Git index must be empty before publishing the torch LFS asset."
        }

        $LfsFilter = (git check-attr filter -- $Asset)
        if ($LASTEXITCODE -ne 0 -or $LfsFilter -notmatch ': filter: lfs$') {
            throw "Torch asset is not configured as Git LFS; refusing to publish."
        }

        git fetch origin master
        if ($LASTEXITCODE -ne 0) { throw "Unable to fetch origin/master." }

        $LocalHead = (git rev-parse HEAD).Trim()
        $RemoteHead = (git rev-parse origin/master).Trim()
        if ($LocalHead -ne $RemoteHead) {
            throw "master differs from origin/master; reconcile before publishing."
        }

        $Changed = @(git diff --name-only -- $Asset)
        if ($LASTEXITCODE -ne 0) { throw "Unable to check migrated torch changes." }
        if ($Changed.Count -ne 1 -or $Changed[0] -ne $Asset) {
            throw "No modified DA_Item_Torch.uasset to publish. Run -Apply and tests first."
        }

        git add -- $Asset
        if ($LASTEXITCODE -ne 0) { throw "Cannot stage migrated torch asset." }

        $Staged = @(git diff --cached --name-only)
        if ($LASTEXITCODE -ne 0 -or $Staged.Count -ne 1 -or $Staged[0] -ne $Asset) {
            git reset -- $Asset
            throw "Unexpected Git index content after staging; refusing commit."
        }

        git commit -m "ITEM-VFX01.3: materialize conditional Niagara torch flame"
        if ($LASTEXITCODE -ne 0) { throw "Torch LFS commit failed." }

        git push origin master
        if ($LASTEXITCODE -ne 0) { throw "Push failed; committed torch asset remains local." }

        Write-Host "[ITEM-VFX01.3] DA_Item_Torch LFS asset committed and pushed to master."
        exit 0
    }

    if (-not (Test-Path $EditorCmd)) {
        throw "UnrealEditor-Cmd.exe not found: $EditorCmd"
    }

    $Args = @($Project, "-run=GridItemTorchVfxMigration", "-Unattended", "-nop4", "-NoSplash")
    if ($Apply) { $Args += "-Apply" }

    Write-Host "[ITEM-VFX01.3] Running torch migration. Apply=$Apply"
    & $EditorCmd @Args
    if ($LASTEXITCODE -ne 0) {
        throw "Unreal torch migration commandlet failed (code $LASTEXITCODE)."
    }
    Write-Host "[ITEM-VFX01.3] Success. Run VFX01_3 tests before -Publish."
}
finally {
    Pop-Location
}
