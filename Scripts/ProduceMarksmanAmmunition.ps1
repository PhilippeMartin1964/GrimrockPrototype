[CmdletBinding()]
param(
    [string]$EngineRoot = 'D:\UE_5.5',
    [switch]$Apply,
    [switch]$Push
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
if ($Push -and -not $Apply) { throw '-Push requires -Apply.' }
$Repo = Split-Path -Parent $PSScriptRoot
$Project = Join-Path $Repo 'GrimrockPrototype.uproject'
$Editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$ReportDir = Join-Path $Repo 'Saved\ARMEMENT-LOT02'
$Bow = 'Content/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_ShortBow_01.uasset'
$Arrow = 'Content/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Arrows_01.uasset'
$Targets = @($Bow, $Arrow)
if (-not (Test-Path -LiteralPath $Editor -PathType Leaf)) { throw 'UnrealEditor-Cmd.exe not found.' }
if ((& git -C $Repo branch --show-current).Trim() -ne 'master') { throw 'Master only.' }
if ($LASTEXITCODE -ne 0) { throw 'Cannot inspect Git branch.' }
$StagedBefore = @(& git -C $Repo diff --cached --name-only)
if ($LASTEXITCODE -ne 0 -or $StagedBefore.Count -ne 0) {
    throw 'Staging area must be empty to avoid unrelated commits.'
}
$DirtyTargets = @(& git -C $Repo status --porcelain=v1 -- $Targets)
if ($LASTEXITCODE -ne 0 -or $DirtyTargets.Count -gt 0) {
    throw 'Bow or arrow target already differs locally: inspect before authoring.'
}
New-Item -ItemType Directory -Force -Path $ReportDir | Out-Null
Write-Host '[LOT02.5] Building Unreal editor without running gameplay automation.'
& (Join-Path $PSScriptRoot 'ValidateUE.ps1') -EngineRoot $EngineRoot -SkipAutomation
if (-not $?) { throw 'Unreal build failed.' }
$Params = @($Project, '-run=GridMarksmanAmmunitionAuthoring',
    '-Unattended','-NoSplash','-NoP4','-NoSound','-NullRHI','-log')
if ($Apply) { $Params += '-Apply' }
$Log = Join-Path $ReportDir 'LOT02_5_ArrowAuthoring.log'
& $Editor @Params *> $Log
$Code = $LASTEXITCODE
$Diagnostics = @(Select-String -LiteralPath $Log -Pattern '\[LOT02\.5\]' | ForEach-Object { $_.Line })
$Diagnostics | ForEach-Object { Write-Host $_ }
if ($Code -ne 0) { throw "Authoring commandlet returned $Code; see $Log" }
if (-not $Apply) {
    if (@($Diagnostics | Where-Object { $_ -match '\[LOT02\.5\] (DRY_RUN|PRESENT_SAME)' }).Count -ne 1) {
        throw 'Expected dry-run confirmation.'
    }
    Write-Host '[LOT02.5] Read-only preflight succeeded.'
    return
}
if (@($Diagnostics | Where-Object { $_ -match '\[LOT02\.5\] (SAVED|PRESENT_SAME)' }).Count -ne 1) {
    throw 'Missing authoritative asset save confirmation.'
}
foreach ($Rel in $Targets) {
    if (-not (Test-Path -LiteralPath (Join-Path $Repo $Rel.Replace('/', '\')) -PathType Leaf)) {
        throw "Expected asset not found: $Rel"
    }
}
$TargetChanges = @(& git -C $Repo status --porcelain=v1 -- $Targets)
if (@($TargetChanges).Count -ne 2) {
    throw 'Expected exactly the new arrow package and modified bow; inspect Git status.'
}
Write-Host '[LOT02.5] Running one grouped local validation (4 filters).'
& (Join-Path $PSScriptRoot 'StarterKitProductionBatch.ps1') -Action Validate -EngineRoot $EngineRoot
if (-not $?) { throw 'Validation failed. No commit or push performed.' }
if (-not $Push) {
    Write-Host '[LOT02.5] Validated locally; two DataAssets left uncommitted intentionally.'
    return
}
& git -C $Repo add -- $Targets
if ($LASTEXITCODE -ne 0) { throw 'Failed staging exact two DataAssets.' }
$Actual = @(& git -C $Repo diff --cached --name-only)
if ($LASTEXITCODE -ne 0 -or ($Actual -join '|') -cne (($Targets | Sort-Object) -join '|')) {
    throw ('Unexpected staged paths: ' + ($Actual -join ', '))
}
& git -C $Repo diff --cached --check
if ($LASTEXITCODE -ne 0) { throw 'Staged whitespace check failed.' }
& git -C $Repo commit -m 'ARMEMENT-LOT02.5: produce arrows and configure short bow ammunition'
if ($LASTEXITCODE -ne 0) { throw 'Commit failed; inspect staged files.' }
& git -C $Repo push origin master
if ($LASTEXITCODE -ne 0) { throw 'Push failed; local commit retained.' }
Write-Host '[LOT02.5] Production, 4 validations, exact two-asset commit and push completed.'
