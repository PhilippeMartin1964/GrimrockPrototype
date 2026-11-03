<#
.SYNOPSIS
    ITEM-VFX01.4A — inspect legacy item Niagara and normalize the three flasks.
.DESCRIPTION
    Default: read-only preflight of every item definition, plus planned flask edits.
    -Apply: only remove LiquidSparkle from the three whitelisted flask DataAssets.
    -Publish: publish only changed whitelisted .uasset files through Git LFS.
    This tool deliberately does not remove historical UPROPERTY fields.
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
$UnrealCmd = Join-Path $EngineRoot "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$Flasks = @(
    "Content/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Flask_Oil.uasset",
    "Content/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Flask_Acid.uasset",
    "Content/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Flask_CorrosiveCloud.uasset"
)

if ($Apply -and $Publish) {
    throw "Run -Apply, then validate all tests, then -Publish in separate steps."
}

Push-Location $Repo
try {
    $Branch = (git branch --show-current).Trim()
    if ($LASTEXITCODE -ne 0 -or $Branch -ne "master") {
        throw "ITEM-VFX01.4A supports master only."
    }
    if (-not (Test-Path $Project)) {
        throw "Cannot locate Unreal project: $Project"
    }

    if ($Publish) {
        $PreviouslyStaged = @(git diff --cached --name-only)
        if ($LASTEXITCODE -ne 0 -or $PreviouslyStaged.Count -gt 0) {
            throw "Refusing to commit because the Git index is not empty."
        }
        foreach ($Asset in $Flasks) {
            $Filter = (git check-attr filter -- $Asset)
            if ($LASTEXITCODE -ne 0 -or $Filter -notmatch ': filter: lfs$') {
                throw "Expected Git LFS for asset: $Asset"
            }
        }

        git fetch origin master
        if ($LASTEXITCODE -ne 0) { throw "git fetch origin master failed." }
        $LocalHead = (git rev-parse HEAD).Trim()
        $RemoteHead = (git rev-parse origin/master).Trim()
        if ($LocalHead -ne $RemoteHead) {
            throw "master has diverged from origin/master; reconcile before publishing."
        }

        $Changed = @(git diff --name-only -- $Flasks)
        if ($LASTEXITCODE -ne 0) { throw "Cannot inspect modified flasks." }
        if ($Changed.Count -eq 0) {
            Write-Host "[ITEM-VFX01.4A] No changed flask assets to publish."
            exit 0
        }

        git add -- $Flasks
        if ($LASTEXITCODE -ne 0) { throw "git add of flask DataAssets failed." }
        $Staged = @(git diff --cached --name-only)
        $Unexpected = @($Staged | Where-Object { $Flasks -notcontains $_ })
        if ($LASTEXITCODE -ne 0 -or $Unexpected.Count -gt 0) {
            git reset -- $Flasks
            throw "Unexpected files staged: $($Unexpected -join ', ')"
        }

        git commit -m "ITEM-VFX01.4A: reserve LiquidSparkle for the healing potion"
        if ($LASTEXITCODE -ne 0) { throw "git commit failed." }
        git push origin master
        if ($LASTEXITCODE -ne 0) {
            throw "git push failed; local binary commit was retained."
        }
        Write-Host "[ITEM-VFX01.4A] Whitelisted flask LFS assets pushed to master."
        exit 0
    }

    if (-not (Test-Path $UnrealCmd)) {
        throw "UnrealEditor-Cmd.exe not found: $UnrealCmd"
    }

    $Args = @($Project, "-run=GridItemVfx014Audit", "-Unattended", "-nop4", "-NoSplash")
    if ($Apply) { $Args += "-Apply" }
    Write-Host "[ITEM-VFX01.4A] Asset audit and flask normalization. Apply=$Apply"
    & $UnrealCmd @Args
    if ($LASTEXITCODE -ne 0) {
        throw "Asset audit refused legacy/sparkle references or migration failed (exit $LASTEXITCODE). Review BLOCKER lines."
    }

    Write-Host "[ITEM-VFX01.4A] Audit OK. Review tests before any -Publish."
}
finally {
    Pop-Location
}
